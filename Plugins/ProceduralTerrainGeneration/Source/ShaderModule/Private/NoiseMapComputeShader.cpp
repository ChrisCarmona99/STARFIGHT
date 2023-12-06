#include "ShaderModule/Public/NoiseMapComputeShaderExternals.h"
#include "ShaderModule/Public/NoiseMapComputeShader.h"
#include "PixelShaderUtils.h"
#include "RenderCore/Public/RenderGraphUtils.h"
#include "MeshPassProcessor.inl"
#include "StaticMeshResources.h"
#include "DynamicMeshBuilder.h"
#include "RenderGraphResources.h"
#include "GlobalShader.h"
#include "UnifiedBuffer.h"
#include "CanvasTypes.h"
#include "MaterialShader.h"

#include "HAL/ThreadManager.h"
#include "RenderGraph.h" // Included this for `FRDGSyncEventRef`, but couldn't get the dependency import to work (RenderCoreTypes)
#include "RHIGPUReadback.h"

DECLARE_STATS_GROUP(TEXT("NoiseMapComputeShader"), STATGROUP_NoiseMapComputeShader, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("NoiseMapComputeShader Execute"), STAT_NoiseMapComputeShader_Execute, STATGROUP_NoiseMapComputeShader);


/*
* This class carries our parameter declarations and acts as the bridge between cpp and HLSL.
*
* DEC & DEF in the .cpp file because this class is only used in 'DispatchRenderThread'
*/
class SHADERMODULE_API FNoiseMapComputeShader : public FGlobalShader
{
public:

	DECLARE_GLOBAL_SHADER(FNoiseMapComputeShader);
	SHADER_USE_PARAMETER_STRUCT(FNoiseMapComputeShader, FGlobalShader);


	class FNoiseMapComputeShader_Perm_TEST : SHADER_PERMUTATION_INT("NoiseMapGenerator", 1);
	using FPermutationDomain = TShaderPermutationDomain<FNoiseMapComputeShader_Perm_TEST>;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		// Shader Parameter Definitions:
		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<int>, mapChunkSize)
		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<float>, noiseScale)
		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<int>, octaveCount)
		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<float>, persistance)
		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<float>, lacurnarity)
		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<float>, heightMultiplier)
		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<float>, weightCurveExponent)
		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<FVector2f>, octaveOffsets) // On the shader side: float3 MyVector;

		//SHADER_PARAMETER_UAV(RW_STRUCTURED_BUFFER<float>, someParam)

		//SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<float>, noiseMap)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<float>, noiseMap)

	END_SHADER_PARAMETER_STRUCT()

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		const FPermutationDomain PermutationVector(Parameters.PermutationId);

		return true;
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

		const FPermutationDomain PermutationVector(Parameters.PermutationId);

		/*
		* Here you define constants that can be used statically in the shader code.
		* Example:
		*/
		// OutEnvironment.SetDefine(TEXT("MY_CUSTOM_CONST"), TEXT("1"));

		/*
		* These defines are used in the thread count section of our shader
		* 
		* NOTE: These values will NOT update after modification UNLESS the shader is re-compiled... this only happens when something changes in the shader...
		*/
		int32 NUM_THREADS_X = 1024;
		int32 NUM_THREADS_Y = 1;
		int32 NUM_THREADS_Z = 1;

		OutEnvironment.SetDefine(TEXT("THREAD_COUNT_X"), NUM_THREADS_X);
		OutEnvironment.SetDefine(TEXT("THREAD_COUNT_Y"), NUM_THREADS_Y);
		OutEnvironment.SetDefine(TEXT("THREAD_COUNT_Z"), NUM_THREADS_Z);

		// This shader must support typed UAV load and we are testing if it is supported at runtime using RHIIsTypedUAVLoadSupported
		//OutEnvironment.CompilerFlags.Add(CFLAG_AllowTypedUAVLoads);

		// FForwardLightingParameters::ModifyCompilationEnvironment(Parameters.Platform, OutEnvironment);
	}
private:
};





// This will tell the engine to create the shader and where the shader entry point is:
// 
//                            ShaderType                            ShaderPath										Shader function name    Type
IMPLEMENT_GLOBAL_SHADER(FNoiseMapComputeShader, "/Plugins/ProceduralTerrainGeneration/NoiseMapComputeShader.usf", "NoiseMapComputeShader", SF_Compute);





void FNoiseMapComputeShaderInterface::ExecuteNoiseMapComputeShader(
	FRHICommandListImmediate& RHICmdList,
	FNoiseMapComputeShaderDispatchParams Params,
	float*& noiseMap,
	std::mutex& ComputeShaderMutex,
	FEvent* NoiseMapCompletionEvent)
{
	//ComputeShaderMutex.lock();
	//std::lock_guard<std::mutex> lock(ComputeShaderMutex);

	uint32 ThreadId = FPlatformTLS::GetCurrentThreadId();
	const FString ThreadName = FThreadManager::Get().GetThreadName(ThreadId);

	UE_LOG(LogTemp, Warning, TEXT(""));
	UE_LOG(LogTemp, Warning, TEXT("| %s | 9: DispatchRenderThread CALLED"), *ThreadName);
	
	// Create the GraphBuilder Instance:
	FRDGBuilder GraphBuilder(RHICmdList);

	// Define variables that will be used outside of the code block that sets all of our queues on the Graph Builder:
	int vertexCount = 0;
	uint32 ByteCount = 0;

	{
		SCOPE_CYCLE_COUNTER(STAT_NoiseMapComputeShader_Execute);
		DECLARE_GPU_STAT(NoiseMapComputeShader)
			RDG_EVENT_SCOPE(GraphBuilder, "NoiseMapComputeShader");
		RDG_GPU_STAT_SCOPE(GraphBuilder, NoiseMapComputeShader);

		// Initialize a Permutation Vector:
		typename FNoiseMapComputeShader::FPermutationDomain PermutationVector;

		// Initialize the noiseMap Compute Shader instance:
		TShaderMapRef<FNoiseMapComputeShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel), PermutationVector);

		bool bIsShaderValid = ComputeShader.IsValid();

		if (bIsShaderValid)
		{
			// Allocate a parameter struct 'PassParameters' with a lifetime tied to graph execution
			FNoiseMapComputeShader::FParameters* PassParameters = GraphBuilder.AllocParameters<FNoiseMapComputeShader::FParameters>();

			// Create all the Buffer parameters:
			uint32 BytesPerElement;
			uint32 NumElements;
			void* InitialData;
			uint64 InitialDataSize;

			FRDGBufferDesc Desc;


			// mapChunkSize:
			BytesPerElement = sizeof(int32);
			NumElements = 1;
			InitialData = (void*)Params.mapChunkSize;
			InitialDataSize = BytesPerElement * NumElements;

			FRDGBufferRef mapChunkSizeBuffer = CreateUploadBuffer(GraphBuilder, TEXT("mapChunkSizeBuffer"), BytesPerElement, NumElements, InitialData, InitialDataSize);
			PassParameters->mapChunkSize = GraphBuilder.CreateSRV(FRDGBufferSRVDesc(mapChunkSizeBuffer, PF_R32_SINT));

			// noiseScale:
			BytesPerElement = sizeof(float);
			NumElements = 1;
			InitialData = (void*)Params.noiseScale;
			InitialDataSize = BytesPerElement * NumElements;

			FRDGBufferRef noiseScaleBuffer = CreateUploadBuffer(GraphBuilder, TEXT("noiseScaleBuffer"), BytesPerElement, NumElements, InitialData, InitialDataSize);
			PassParameters->noiseScale = GraphBuilder.CreateSRV(FRDGBufferSRVDesc(noiseScaleBuffer, PF_R32_SINT));

			// octaveCount:
			BytesPerElement = sizeof(int32);
			NumElements = 1;
			InitialData = (void*)Params.octaveCount;
			InitialDataSize = BytesPerElement * NumElements;

			FRDGBufferRef octaveCountBuffer = CreateUploadBuffer(GraphBuilder, TEXT("octaveCountBuffer"), BytesPerElement, NumElements, InitialData, InitialDataSize);
			PassParameters->octaveCount = GraphBuilder.CreateSRV(FRDGBufferSRVDesc(octaveCountBuffer, PF_R32_SINT));

			// persistance:
			BytesPerElement = sizeof(float);
			NumElements = 1;
			InitialData = (void*)Params.persistance;
			InitialDataSize = BytesPerElement * NumElements;

			FRDGBufferRef persistanceBuffer = CreateUploadBuffer(GraphBuilder, TEXT("persistanceBuffer"), BytesPerElement, NumElements, InitialData, InitialDataSize);
			PassParameters->persistance = GraphBuilder.CreateSRV(FRDGBufferSRVDesc(persistanceBuffer, PF_R32_SINT));

			// lacurnarity:
			BytesPerElement = sizeof(float);
			NumElements = 1;
			InitialData = (void*)Params.lacurnarity;
			InitialDataSize = BytesPerElement * NumElements;

			FRDGBufferRef lacurnarityBuffer = CreateUploadBuffer(GraphBuilder, TEXT("lacurnarityBuffer"), BytesPerElement, NumElements, InitialData, InitialDataSize);
			PassParameters->lacurnarity = GraphBuilder.CreateSRV(FRDGBufferSRVDesc(lacurnarityBuffer, PF_R32_SINT));

			// heightMultiplier:
			BytesPerElement = sizeof(float);
			NumElements = 1;
			InitialData = (void*)Params.heightMultiplier;
			InitialDataSize = BytesPerElement * NumElements;

			FRDGBufferRef heightMultiplierBuffer = CreateUploadBuffer(GraphBuilder, TEXT("heightMultiplierBuffer"), BytesPerElement, NumElements, InitialData, InitialDataSize);
			PassParameters->heightMultiplier = GraphBuilder.CreateSRV(FRDGBufferSRVDesc(heightMultiplierBuffer, PF_R32_SINT));

			// weightCurveExponent:
			BytesPerElement = sizeof(float);
			NumElements = 1;
			InitialData = (void*)Params.weightCurveExponent;
			InitialDataSize = BytesPerElement * NumElements;

			FRDGBufferRef weightCurveExponentBuffer = CreateUploadBuffer(GraphBuilder, TEXT("weightCurveExponentBuffer"), BytesPerElement, NumElements, InitialData, InitialDataSize);
			PassParameters->weightCurveExponent = GraphBuilder.CreateSRV(FRDGBufferSRVDesc(weightCurveExponentBuffer, PF_R32_SINT));

			// octaveOffsets:
			BytesPerElement = sizeof(FVector2f);
			NumElements = Params.octaveCount[0];
			//InitialData = reinterpret_cast<void*>(Params.octaveOffsets.get()); // NOTE: This is how you can cast to a 'void*' when using shared pointers
			InitialData = (void*)Params.octaveOffsets;
			InitialDataSize = BytesPerElement * NumElements;

			FRDGBufferRef octaveOffsetsBuffer = CreateUploadBuffer(GraphBuilder, TEXT("octaveOffsetsBuffer"), BytesPerElement, NumElements, InitialData, InitialDataSize);
			PassParameters->octaveOffsets = GraphBuilder.CreateSRV(FRDGBufferSRVDesc(octaveOffsetsBuffer, PF_R32_SINT));



			// noiseMap:
			// 
			//BytesPerElement = sizeof(float);
			//NumElements = Params.mapChunkSize[0] * Params.mapChunkSize[0];
			////InitialData = reinterpret_cast<void*>(Params.noiseMap.get());
			//InitialData = (void*)Params.noiseMap;
			//InitialDataSize = BytesPerElement * NumElements;
			////InitialDataSize = 0; // Because our noiseMap is just a nullptr, are initial data size is just 0, making it any greater would cause 
			//FRDGBufferRef noiseMapBuffer = CreateStructuredBuffer(
			//	GraphBuilder, 										  
			//	TEXT("noiseMapBuffer"), 								  
			//	BytesPerElement, 						  
			//	NumElements, 								      
			//	InitialData, 									  
			//	InitialDataSize, 									  
			//	ERDGInitialDataFlags::None);
			//PassParameters->noiseMap = GraphBuilder.CreateUAV(FRDGBufferUAVDesc(noiseMapBuffer, PF_R32_SINT));

			BytesPerElement = sizeof(float);
			NumElements = Params.mapChunkSize[0] * Params.mapChunkSize[0];
			FRDGBufferRef noiseMapBuffer = GraphBuilder.CreateBuffer(
				FRDGBufferDesc::CreateBufferDesc(
					BytesPerElement,
					NumElements),
				TEXT("noiseMapBuffer"));
			PassParameters->noiseMap = GraphBuilder.CreateUAV(FRDGBufferUAVDesc(noiseMapBuffer, PF_R32_SINT));

			

			// Get the number of groups to dispatch to our compute shader:
			int32 groupSize = FComputeShaderUtils::kGolden2DGroupSize; // This parameter is set to 8, apparently this is the "Ideal size of group size 8x8 to occupy at least an entire wave on GCN, two warp on Nvidia"
			UE_LOG(LogTemp, Warning, TEXT("| %s |	 #: Params.THREAD_GROUPS_X == %d"), *ThreadName, Params.THREAD_GROUPS_X);
			//FIntVector GroupCount = FComputeShaderUtils::GetGroupCount(FIntVector(Params.X, Params.Y, Params.Z), Params.THREAD_GROUPS_X); // Resulting group count for X, Y, Z == Params.<>/groupSize rounded up for each
			FIntVector GroupCount = FIntVector(Params.THREAD_GROUPS_X, 1, 1);
			UE_LOG(LogTemp, Warning, TEXT("| %s |	 #: FIntVector GroupCount == (%d, %d, %d)"), *ThreadName, GroupCount.X, GroupCount.Y, GroupCount.Z);



			// Adds a lambda pass to the graph with a runtime-generated parameter struct:
			GraphBuilder.AddPass(
				RDG_EVENT_NAME("ExecuteNoiseMapComputeShader"),
				PassParameters,
				ERDGPassFlags::AsyncCompute,
				[&PassParameters, ComputeShader, GroupCount](FRHIComputeCommandList& RHICmdList)
				{
					FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, *PassParameters, GroupCount);
				});
			

			/*
			*	In summary, CopyBuffer is a simpler, synchronous function that can be used for small buffer copies 
			*	that do not require asynchronous execution, while AddEnqueueCopyPass is an asynchronous function that 
			*	can be used for larger buffer copies where performance gains can be achieved by allowing the CPU to 
			*	continue executing other commands while the GPU performs the copy operation.
			*/
			// Adds a "Copy" pass that, when called, will copy the data from a buffer that exists on the GPU (noiseMapBuffer) do a variable on the local thread the graph was executed on (GPUBufferReadback):
			vertexCount = Params.mapChunkSize[0] * Params.mapChunkSize[0];
			ByteCount = sizeof(float) * (vertexCount);
			FRHIGPUBufferReadback* GPUBufferReadback = new FRHIGPUBufferReadback(TEXT("ExecuteNoiseMapComputeShaderOutput"));
			AddEnqueueCopyPass(GraphBuilder, GPUBufferReadback, noiseMapBuffer, 0u);  // NOTE: Passing '0u' for the 'uint32 NumBytes' parameter just means that the entire buffer will be copied over


			
			// Define Runner Lambda to wait recursively check if GPUBufferReadback is ready by enqueing itself on the `ActualRenderingThread` thread:
			auto RunnerFunc = [GPUBufferReadback, NoiseMapCompletionEvent, &noiseMap, vertexCount, ByteCount](auto&& RunnerFunc) mutable -> void
			{
				uint32 ThreadId = FPlatformTLS::GetCurrentThreadId();
				const FString ThreadName = FThreadManager::Get().GetThreadName(ThreadId);
				FPlatformProcess::Sleep(0.05f);

				// If our GPU readback is complete, then unlock the buffer, read from it, set our output variables with the contents in the buffer, and execute this all on the main thread...
				if (GPUBufferReadback->IsReady())
				{
					UE_LOG(LogTemp, Warning, TEXT("| %s |	    THREAD: Buffer READY"), *ThreadName);

					// Wait a small amount of time for synchonization purposes:
					FPlatformProcess::Sleep(0.05f);

					//std::shared_ptr<float[]> BUFFER(reinterpret_cast<float*>(GPUBufferReadback->Lock(ByteCount)));
					float* BUFFER = (float*)GPUBufferReadback->Lock(ByteCount);
					std::memcpy(noiseMap, BUFFER, vertexCount * sizeof(float));
					GPUBufferReadback->Unlock();
					delete GPUBufferReadback;

					NoiseMapCompletionEvent->Trigger();
				}
				// If our GPU readback is NOT complete, then just execute our 'RunnerFunc' AGAIN on our Rendering thread (non-game thread) again and see if it will be ready by the next execution:
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("| %s |	    THREAD: Buffer NOT READY"), *ThreadName);
					AsyncTask(ENamedThreads::ActualRenderingThread,
						[RunnerFunc]() mutable
						{
							RunnerFunc(RunnerFunc);
						});
				}
			};

			// Call our 'RunnerFunc' asynchronously for the FIRST TIME on our Rendering thread ('RunnerFunc' will keep calling this Async func again and again until the GPU has outputed its contents into our 'GPUBufferReadback':
			UE_LOG(LogTemp, Warning, TEXT("| %s |	10: AsyncTask CALLED FIRST TIME"), *ThreadName);
			AsyncTask(ENamedThreads::ActualRenderingThread,
				[RunnerFunc]() mutable
				{
					uint32 ThreadId = FPlatformTLS::GetCurrentThreadId();
					const FString ThreadName = FThreadManager::Get().GetThreadName(ThreadId);
					UE_LOG(LogTemp, Warning, TEXT("| %s |	    GPU Waiting Started..."), *ThreadName);

					RunnerFunc(RunnerFunc); //Call RunnerFunc and pass an instance of itself so that it can recursively call itself within another 'AsyncTask' call if the GPU is not done yet
				});

		}
		else {
			// We silently exit here as we don't want to crash the game if the shader is not found or has an error.
			UE_LOG(LogTemp, Warning, TEXT("| %s | COMPUTE SHADER SILENTLY EXITED... This is most likely due to an error within the GPU during execution."), *ThreadName);
		}
	}


	// OLD ATTEMPTS:
	/*
	* 
	* 
	//UE_LOG(LogTemp, Warning, TEXT("| %s |	8: GraphBuilder.Execute() EXECUTED"), *ThreadName);

	//GraphBuilder.Execute();

	//UE_LOG(LogTemp, Warning, TEXT("| %s |	9: GraphBuilder.Execute() FINISHED"), *ThreadName);


	//UE_LOG(LogTemp, Warning, TEXT("| %s |	10: AllPassesCompleted->Wait() EXECUTED"), *ThreadName);

	//AllPassesCompleted->Wait();
	//UE_LOG(LogTemp, Warning, TEXT("| %s |	10.1: GPUWaitThread DISPATCHED"), *ThreadName);
	//std::thread GPUWaitThread(WaitForGPUBuffer, PassCompletionEvent, GPUBufferReadback);





	//UE_LOG(LogTemp, Warning, TEXT("| %s |	10.4: GPUWaitThread.join() EXECUTED"), *ThreadName);
	//GPUWaitThread.join();
	//UE_LOG(LogTemp, Warning, TEXT("| %s |	10.5: GPUWaitThread.join() JOINED"), *ThreadName);
	
	//RHICmdList.WaitForRHIThreadTasks();



	//FGraphEventArray Prerequisites;

	//TGraphTask<ComputeShader>* ComputeTask = new (TGraphTask<ComputeShader>::CreateTask()) ComputeShader(RHICmdList, GPUBufferReadback);

	//FGraphEventRef TaskToAdd_ComputeGraphEvent = ComputeShader->GetCompletionEvent();
	//Prerequisites.Add(RHICmdList.CreateGPUFence(TEXT("NoiseMapGPUFence")));

	//// Get a reference to the compute graph event
	//FGraphEventRef ComputeGraphEvent = MyComputeShaderTask->GetCompletionEvent();


	//RHICmdList.CreateComputeFence(TEXT("NoiseMapComputeShader"));
	//FRHIComputeFence* ComputeFence = RHICreateComputeFence(TEXT("NoiseMapComputeShader"));
	//RHICmdList.WaitComputeFence(ComputeFence);

	//UE_LOG(LogTemp, Warning, TEXT("| %s |	11: AllPassesCompleted->Wait() FINISHED"), *ThreadName);

	//CommandList.EnqueueSetGPUFence(GPUFence, CommandList);
	//GPUFence->Wait();

	//FGPUFenceRHIRef GPUFence = RHICmdList.CreateGPUFence(TEXT("GPUFence"));
	//RHICmdList.WriteGPUFence(GPUFence);
	*
	*
	*/


	UE_LOG(LogTemp, Warning, TEXT("| %s |	10: GraphBuilder.Execute() EXECUTED"), *ThreadName);
	GraphBuilder.Execute();

	UE_LOG(LogTemp, Warning, TEXT("| %s |	11: ExecuteNoiseMapComputeShader EXITING....."), *ThreadName);
}

