#include "ShaderModule/Public/ComputeShaderExternals.h"
#include "ShaderModule/Public/ErosionMapCS.h"
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

DECLARE_STATS_GROUP(TEXT("ErosionMapCS"), STATGROUP_ErosionMapCS, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("ErosionMapCS Execute"), STAT_ErosionMapCS_Execute, STATGROUP_ErosionMapCS);


/*
* This class carries our parameter declarations and acts as the bridge between cpp and HLSL.
*
* DEC & DEF in the .cpp file because this class is only used in 'DispatchRenderThread'
*/
class SHADERMODULE_API FErosionMapCS : public FGlobalShader
{
public:

	DECLARE_GLOBAL_SHADER(FErosionMapCS);
	SHADER_USE_PARAMETER_STRUCT(FErosionMapCS, FGlobalShader);


	class FErosionMapCS_Perm_TEST : SHADER_PERMUTATION_INT("ErosionGenerator", 1);
	using FPermutationDomain = TShaderPermutationDomain<FErosionMapCS_Perm_TEST>;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		// Shader Parameter Definitions:
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<float>, _NoiseMap)
		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<int>, _RandomIndices)
		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<int>, _BrushIndices)
		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<float>, _BrushWeights)

		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<int>, _MapChunkSize)
		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<int>, _BrushLength)
		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<int>, _BorderSize)

		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<int>, _MaxDropletLifetime)
		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<float>, _Inertia)
		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<float>, _SedimentCapacityFactor)
		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<float>, _MinSedimentCapacity)
		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<float>, _DepositSpeed)
		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<float>, _ErodeSpeed)

		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<float>, _EvaporateSpeed)
		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<float>, _Gravity)
		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<float>, _StartSpeed)
		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<float>, _StartWater)

		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<int>, _NumErosionIterations)


		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<float>, _DEBUG_1)



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
IMPLEMENT_GLOBAL_SHADER(FErosionMapCS, "/Plugins/ProceduralTerrainGeneration/ErosionMapCS.usf", "ErosionMapCS", SF_Compute);





void FErosionMapCSInterface::ExecuteErosionMapCS(
	FRHICommandListImmediate& RHICmdList,
	FErosionMapCSDispatchParams Params,
	int BrushIndexOffsetsSize,
	int BrushWeightsSize,
	float*& noiseMap,
	float*& _DEBUG_1,
	FEvent* ErosionMapCompletionEvent)
{
	uint32 ThreadId = FPlatformTLS::GetCurrentThreadId();
	const FString ThreadName = FThreadManager::Get().GetThreadName(ThreadId);

	UE_LOG(LogTemp, Warning, TEXT(""));
	UE_LOG(LogTemp, Warning, TEXT("| %s | 9: DispatchRenderThread CALLED"), *ThreadName);

	// Create the GraphBuilder Instance:
	FRDGBuilder GraphBuilder(RHICmdList);

	// Define variables that will be used outside of the code block that sets all of our queues on the Graph Builder:
	int elementCount = 0;
	uint32 ByteCount = 0;

	{
		SCOPE_CYCLE_COUNTER(STAT_ErosionMapCS_Execute);
		DECLARE_GPU_STAT(ErosionMapCS)
			RDG_EVENT_SCOPE(GraphBuilder, "ErosionMapCS");
		RDG_GPU_STAT_SCOPE(GraphBuilder, ErosionMapCS);

		// Initialize a Permutation Vector:
		typename FErosionMapCS::FPermutationDomain PermutationVector;

		// Initialize the noiseMap Compute Shader instance:
		TShaderMapRef<FErosionMapCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel), PermutationVector);

		bool bIsShaderValid = ComputeShader.IsValid();

		if (bIsShaderValid)
		{
			// Allocate a parameter struct 'PassParameters' with a lifetime tied to graph execution
			FErosionMapCS::FParameters* PassParameters = GraphBuilder.AllocParameters<FErosionMapCS::FParameters>();

			// Create all the Buffer parameters:
			uint32 BytesPerElement;
			uint32 NumElements;
			void* InitialData;
			uint64 InitialDataSize;

			FRDGBufferDesc Desc;



			// _RandomIndices:
			BytesPerElement = sizeof(int);
			NumElements = Params._NumErosionIterations[0];
			InitialData = (void*)Params._RandomIndices;
			InitialDataSize = BytesPerElement * NumElements;

			FRDGBufferRef _RandomIndicesBuffer = CreateUploadBuffer(GraphBuilder, TEXT("_RandomIndicesBuffer"), BytesPerElement, NumElements, InitialData, InitialDataSize);
			PassParameters->_RandomIndices = GraphBuilder.CreateSRV(FRDGBufferSRVDesc(_RandomIndicesBuffer, PF_R32_SINT));


			// _BrushIndices:
			BytesPerElement = sizeof(int);
			NumElements = BrushIndexOffsetsSize;
			InitialData = (void*)Params._BrushIndices;
			InitialDataSize = BytesPerElement * NumElements;

			FRDGBufferRef _BrushIndicesBuffer = CreateUploadBuffer(GraphBuilder, TEXT("_BrushIndicesBuffer"), BytesPerElement, NumElements, InitialData, InitialDataSize);
			PassParameters->_BrushIndices = GraphBuilder.CreateSRV(FRDGBufferSRVDesc(_BrushIndicesBuffer, PF_R32_SINT));


			// _BrushWeights:
			BytesPerElement = sizeof(float);
			NumElements = BrushIndexOffsetsSize;
			InitialData = (void*)Params._BrushWeights;
			InitialDataSize = BytesPerElement * NumElements;

			FRDGBufferRef _BrushWeightsBuffer = CreateUploadBuffer(GraphBuilder, TEXT("_BrushWeightsBuffer"), BytesPerElement, NumElements, InitialData, InitialDataSize);
			PassParameters->_BrushWeights = GraphBuilder.CreateSRV(FRDGBufferSRVDesc(_BrushWeightsBuffer, PF_R32_SINT));



			// _MapChunkSize:
			BytesPerElement = sizeof(int);
			NumElements = 1;
			InitialData = (void*)Params._MapChunkSize;
			InitialDataSize = BytesPerElement * NumElements;

			FRDGBufferRef _MapChunkSizeBuffer = CreateUploadBuffer(GraphBuilder, TEXT("_MapChunkSizeBuffer"), BytesPerElement, NumElements, InitialData, InitialDataSize);
			PassParameters->_MapChunkSize = GraphBuilder.CreateSRV(FRDGBufferSRVDesc(_MapChunkSizeBuffer, PF_R32_SINT));


			// _BrushLength:
			BytesPerElement = sizeof(int);
			NumElements = 1;
			InitialData = (void*)Params._BrushLength;
			InitialDataSize = BytesPerElement * NumElements;

			FRDGBufferRef _BrushLengthBuffer = CreateUploadBuffer(GraphBuilder, TEXT("_BrushLengthBuffer"), BytesPerElement, NumElements, InitialData, InitialDataSize);
			PassParameters->_BrushLength = GraphBuilder.CreateSRV(FRDGBufferSRVDesc(_BrushLengthBuffer, PF_R32_SINT));


			// _BorderSize:
			BytesPerElement = sizeof(int);
			NumElements = 1;
			InitialData = (void*)Params._BorderSize;
			InitialDataSize = BytesPerElement * NumElements;

			FRDGBufferRef _BorderSizeBuffer = CreateUploadBuffer(GraphBuilder, TEXT("_BorderSizeBuffer"), BytesPerElement, NumElements, InitialData, InitialDataSize);
			PassParameters->_BorderSize = GraphBuilder.CreateSRV(FRDGBufferSRVDesc(_BorderSizeBuffer, PF_R32_SINT));



			// _MaxDropletLifetime:
			BytesPerElement = sizeof(int);
			NumElements = 1;
			InitialData = (void*)Params._MaxDropletLifetime;
			InitialDataSize = BytesPerElement * NumElements;

			FRDGBufferRef _MaxDropletLifetimeBuffer = CreateUploadBuffer(GraphBuilder, TEXT("_MaxDropletLifetimeBuffer"), BytesPerElement, NumElements, InitialData, InitialDataSize);
			PassParameters->_MaxDropletLifetime = GraphBuilder.CreateSRV(FRDGBufferSRVDesc(_MaxDropletLifetimeBuffer, PF_R32_SINT));


			// _Inertia:
			BytesPerElement = sizeof(float);
			NumElements = 1;
			InitialData = (void*)Params._Inertia;
			InitialDataSize = BytesPerElement * NumElements;

			FRDGBufferRef _InertiaBuffer = CreateUploadBuffer(GraphBuilder, TEXT("_InertiaBuffer"), BytesPerElement, NumElements, InitialData, InitialDataSize);
			PassParameters->_Inertia = GraphBuilder.CreateSRV(FRDGBufferSRVDesc(_InertiaBuffer, PF_R32_SINT));


			// _SedimentCapacityFactor:
			BytesPerElement = sizeof(float);
			NumElements = 1;
			InitialData = (void*)Params._SedimentCapacityFactor;
			InitialDataSize = BytesPerElement * NumElements;

			FRDGBufferRef _SedimentCapacityFactorBuffer = CreateUploadBuffer(GraphBuilder, TEXT("_SedimentCapacityFactorBuffer"), BytesPerElement, NumElements, InitialData, InitialDataSize);
			PassParameters->_SedimentCapacityFactor = GraphBuilder.CreateSRV(FRDGBufferSRVDesc(_SedimentCapacityFactorBuffer, PF_R32_SINT));


			// _MinSedimentCapacity:
			BytesPerElement = sizeof(float);
			NumElements = 1;
			InitialData = (void*)Params._MinSedimentCapacity;
			InitialDataSize = BytesPerElement * NumElements;

			FRDGBufferRef _MinSedimentCapacityBuffer = CreateUploadBuffer(GraphBuilder, TEXT("_MinSedimentCapacityBuffer"), BytesPerElement, NumElements, InitialData, InitialDataSize);
			PassParameters->_MinSedimentCapacity = GraphBuilder.CreateSRV(FRDGBufferSRVDesc(_MinSedimentCapacityBuffer, PF_R32_SINT));


			// _DepositSpeed:
			BytesPerElement = sizeof(float);
			NumElements = 1;
			InitialData = (void*)Params._DepositSpeed;
			InitialDataSize = BytesPerElement * NumElements;

			FRDGBufferRef _DepositSpeedBuffer = CreateUploadBuffer(GraphBuilder, TEXT("_DepositSpeedBuffer"), BytesPerElement, NumElements, InitialData, InitialDataSize);
			PassParameters->_DepositSpeed = GraphBuilder.CreateSRV(FRDGBufferSRVDesc(_DepositSpeedBuffer, PF_R32_SINT));


			// _ErodeSpeed:
			BytesPerElement = sizeof(float);
			NumElements = 1;
			InitialData = (void*)Params._ErodeSpeed;
			InitialDataSize = BytesPerElement * NumElements;

			FRDGBufferRef _ErodeSpeedBuffer = CreateUploadBuffer(GraphBuilder, TEXT("_ErodeSpeedBuffer"), BytesPerElement, NumElements, InitialData, InitialDataSize);
			PassParameters->_ErodeSpeed = GraphBuilder.CreateSRV(FRDGBufferSRVDesc(_ErodeSpeedBuffer, PF_R32_SINT));



			// _EvaporateSpeed:
			BytesPerElement = sizeof(float);
			NumElements = 1;
			InitialData = (void*)Params._EvaporateSpeed;
			InitialDataSize = BytesPerElement * NumElements;

			FRDGBufferRef _EvaporateSpeedBuffer = CreateUploadBuffer(GraphBuilder, TEXT("_EvaporateSpeedBuffer"), BytesPerElement, NumElements, InitialData, InitialDataSize);
			PassParameters->_EvaporateSpeed = GraphBuilder.CreateSRV(FRDGBufferSRVDesc(_EvaporateSpeedBuffer, PF_R32_SINT));


			// _Gravity:
			BytesPerElement = sizeof(float);
			NumElements = 1;
			InitialData = (void*)Params._Gravity;
			InitialDataSize = BytesPerElement * NumElements;

			FRDGBufferRef _GravityBuffer = CreateUploadBuffer(GraphBuilder, TEXT("_GravityBuffer"), BytesPerElement, NumElements, InitialData, InitialDataSize);
			PassParameters->_Gravity = GraphBuilder.CreateSRV(FRDGBufferSRVDesc(_GravityBuffer, PF_R32_SINT));


			// _StartSpeed:
			BytesPerElement = sizeof(float);
			NumElements = 1;
			InitialData = (void*)Params._StartSpeed;
			InitialDataSize = BytesPerElement * NumElements;

			FRDGBufferRef _StartSpeedBuffer = CreateUploadBuffer(GraphBuilder, TEXT("_StartSpeedBuffer"), BytesPerElement, NumElements, InitialData, InitialDataSize);
			PassParameters->_StartSpeed = GraphBuilder.CreateSRV(FRDGBufferSRVDesc(_StartSpeedBuffer, PF_R32_SINT));


			// _StartWater:
			BytesPerElement = sizeof(float);
			NumElements = 1;
			InitialData = (void*)Params._StartWater;
			InitialDataSize = BytesPerElement * NumElements;

			FRDGBufferRef _StartWaterBuffer = CreateUploadBuffer(GraphBuilder, TEXT("_StartWaterBuffer"), BytesPerElement, NumElements, InitialData, InitialDataSize);
			PassParameters->_StartWater = GraphBuilder.CreateSRV(FRDGBufferSRVDesc(_StartWaterBuffer, PF_R32_SINT));


			// _NumErosionIterations:
			BytesPerElement = sizeof(int);
			NumElements = 1;
			InitialData = (void*)Params._NumErosionIterations;
			InitialDataSize = BytesPerElement * NumElements;

			FRDGBufferRef _NumErosionIterationsBuffer = CreateUploadBuffer(GraphBuilder, TEXT("_NumErosionIterationsBuffer"), BytesPerElement, NumElements, InitialData, InitialDataSize);
			PassParameters->_NumErosionIterations = GraphBuilder.CreateSRV(FRDGBufferSRVDesc(_NumErosionIterationsBuffer, PF_R32_SINT));


			


			// _NoiseMap:
			BytesPerElement = sizeof(float);
			NumElements = Params._MapChunkSize[0] * Params._MapChunkSize[0];
			FRDGBufferRef _NoiseMapBuffer = GraphBuilder.CreateBuffer(
				FRDGBufferDesc::CreateBufferDesc(
					BytesPerElement,
					NumElements),
				TEXT("_NoiseMapBuffer"));
			PassParameters->_NoiseMap = GraphBuilder.CreateUAV(FRDGBufferUAVDesc(_NoiseMapBuffer, PF_R32_SINT));


			// _DEBUG_1:
			BytesPerElement = sizeof(float);
			NumElements = Params._NumErosionIterations[0];
			FRDGBufferRef _DEBUG_1Buffer = GraphBuilder.CreateBuffer(
				FRDGBufferDesc::CreateBufferDesc(
					BytesPerElement,
					NumElements),
				TEXT("_DEBUG_1Buffer"));
			PassParameters->_DEBUG_1 = GraphBuilder.CreateUAV(FRDGBufferUAVDesc(_DEBUG_1Buffer, PF_R32_SINT));
			



			// Get the number of groups to dispatch to our compute shader:
			FIntVector GroupCount = FIntVector(Params.THREAD_GROUPS_X, 1, 1);

			// Adds a lambda pass to the graph with a runtime-generated parameter struct:
			GraphBuilder.AddPass(
				RDG_EVENT_NAME("ExecuteErosionMapCS"),
				PassParameters,
				ERDGPassFlags::AsyncCompute,
				[&PassParameters, ComputeShader, GroupCount](FRHIComputeCommandList& RHICmdList)
				{
					FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, *PassParameters, GroupCount);
				});


			// Adds a "Copy" pass for the noise map buffer:
			elementCount = Params._MapChunkSize[0] * Params._MapChunkSize[0];
			ByteCount = sizeof(float) * (elementCount);
			FRHIGPUBufferReadback* NoiseMapBufferReadback = new FRHIGPUBufferReadback(TEXT("ExecuteNoiseMapComputeShaderOutput"));
			AddEnqueueCopyPass(GraphBuilder, NoiseMapBufferReadback, _NoiseMapBuffer, 0u);  // NOTE: Passing '0u' for the 'uint32 NumBytes' parameter just means that the entire buffer will be copied over

			// Adds a "Copy" pass for the _DEBUG_1 buffer:
			elementCount = Params._NumErosionIterations[0];
			ByteCount = sizeof(float) * (elementCount);
			FRHIGPUBufferReadback* _DEBUG_1BufferReadback = new FRHIGPUBufferReadback(TEXT("Execute_DEBUG_1ComputeShaderOutput"));
			AddEnqueueCopyPass(GraphBuilder, _DEBUG_1BufferReadback, _DEBUG_1Buffer, 0u);  // NOTE: Passing '0u' for the 'uint32 NumBytes' parameter just means that the entire buffer will be copied over


			auto RunnerFunc = [NoiseMapBufferReadback, _DEBUG_1BufferReadback, ErosionMapCompletionEvent, &noiseMap, &_DEBUG_1, Params, elementCount, ByteCount](auto&& RunnerFunc) mutable -> void
			{
				uint32 ThreadId = FPlatformTLS::GetCurrentThreadId();
				const FString ThreadName = FThreadManager::Get().GetThreadName(ThreadId);
				FPlatformProcess::Sleep(0.05f);

				if (NoiseMapBufferReadback->IsReady() && _DEBUG_1BufferReadback->IsReady())
				{
					UE_LOG(LogTemp, Warning, TEXT("| %s |	    THREAD: NoiseMapBufferReadback->IsReady() == '%s'  :  _DEBUG_1BufferReadback->IsReady() == '%s'"), 
						*ThreadName, NoiseMapBufferReadback->IsReady() ? TEXT("true") : TEXT("false"), _DEBUG_1BufferReadback->IsReady() ? TEXT("true") : TEXT("false"));

					// Wait a small amount of time for synchonization purposes:
					FPlatformProcess::Sleep(0.05f);

					ByteCount = Params._MapChunkSize[0] * Params._MapChunkSize[0] * sizeof(float);
					float* NOISEMAP_BUFFER = (float*)NoiseMapBufferReadback->Lock(ByteCount);
					std::memcpy(noiseMap, NOISEMAP_BUFFER, elementCount * sizeof(float));
					NoiseMapBufferReadback->Unlock();
					delete NoiseMapBufferReadback;

					ByteCount = Params._NumErosionIterations[0] * sizeof(float);
					float* _DEBUG_1_BUFFER = (float*)_DEBUG_1BufferReadback->Lock(ByteCount);
					std::memcpy(_DEBUG_1, _DEBUG_1_BUFFER, elementCount * sizeof(float));
					_DEBUG_1BufferReadback->Unlock();
					delete _DEBUG_1BufferReadback;

					if (ErosionMapCompletionEvent != nullptr)
					{
						ErosionMapCompletionEvent->Trigger();
					}
					else
					{
						UE_LOG(LogTemp, Warning, TEXT("|    |	    ERROR: ErosionMapCompletionEvent == nullptr"), *ThreadName);
					}
				}
				// If our GPU readback is NOT complete, then just execute our 'RunnerFunc' AGAIN on our Rendering thread (non-game thread) again and see if it will be ready by the next execution:
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("|    |	    THREAD: Buffer NOT READY"), *ThreadName);
					AsyncTask(ENamedThreads::ActualRenderingThread,
						[RunnerFunc]() mutable
						{
							RunnerFunc(RunnerFunc);
						});
				}
			};

			// Call our 'RunnerFunc' asynchronously for the FIRST TIME on our Rendering thread ('RunnerFunc' will keep calling this Async func again and again until the GPU has outputed its contents into our 'NoiseMapBufferReadback':
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

	UE_LOG(LogTemp, Warning, TEXT("| %s |	10: GraphBuilder.Execute() EXECUTED"), *ThreadName);
	GraphBuilder.Execute();

	UE_LOG(LogTemp, Warning, TEXT("| %s |	11: ExecuteErosionMapCS EXITING....."), *ThreadName);
}

