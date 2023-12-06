#include "ShaderModule/Public/ComputeShaderExternals.h"
#include "ShaderModule/Public/NormalsAndTangentsCS.h"
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

DECLARE_STATS_GROUP(TEXT("NormalsAndTangentsCS"), STATGROUP_NormalsAndTangentsCS, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("NormalsAndTangentsCS Execute"), STAT_NormalsAndTangentsCS_Execute, STATGROUP_NormalsAndTangentsCS);


/*
* This class carries our parameter declarations and acts as the bridge between cpp and HLSL.
*
* DEC & DEF in the .cpp file because this class is only used in 'DispatchRenderThread'
*/
class SHADERMODULE_API FNormalsAndTangentsCS : public FGlobalShader
{
public:

	DECLARE_GLOBAL_SHADER(FNormalsAndTangentsCS);
	SHADER_USE_PARAMETER_STRUCT(FNormalsAndTangentsCS, FGlobalShader);


	class FNormalsAndTangentsCS_Perm_TEST : SHADER_PERMUTATION_INT("NormalsAndTangentsGenerator", 1);
	using FPermutationDomain = TShaderPermutationDomain<FNormalsAndTangentsCS_Perm_TEST>;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		// Shader Parameter Definitions:
		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<int>, mapChunkSize)
		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<float>, noiseMap)

		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<float>, normals)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<float>, tangents)

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
IMPLEMENT_GLOBAL_SHADER(FNormalsAndTangentsCS, "/Plugins/ProceduralTerrainGeneration/NormalsAndTangentsCS.usf", "NormalsAndTangentsCS", SF_Compute);





void FNormalsAndTangentsCSInterface::ExecuteNormalsAndTangentsCS(
	FRHICommandListImmediate& RHICmdList,
	FNormalsAndTangentsCSDispatchParams Params,
	float*& normals,
	float*& tangents,
	FEvent* NormalsAndTangentsCompletionEvent)
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
		SCOPE_CYCLE_COUNTER(STAT_NormalsAndTangentsCS_Execute);
		DECLARE_GPU_STAT(NormalsAndTangentsCS)
			RDG_EVENT_SCOPE(GraphBuilder, "NormalsAndTangentsCS");
		RDG_GPU_STAT_SCOPE(GraphBuilder, NormalsAndTangentsCS);

		// Initialize a Permutation Vector:
		typename FNormalsAndTangentsCS::FPermutationDomain PermutationVector;

		// Initialize the noiseMap Compute Shader instance:
		TShaderMapRef<FNormalsAndTangentsCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel), PermutationVector);

		bool bIsShaderValid = ComputeShader.IsValid();

		if (bIsShaderValid)
		{
			// Allocate a parameter struct 'PassParameters' with a lifetime tied to graph execution
			FNormalsAndTangentsCS::FParameters* PassParameters = GraphBuilder.AllocParameters<FNormalsAndTangentsCS::FParameters>();

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


			// noiseMap:
			BytesPerElement = sizeof(float);
			NumElements = Params.mapChunkSize[0] * Params.mapChunkSize[0];
			InitialData = (void*)Params.noiseMap;
			InitialDataSize = BytesPerElement * NumElements;

			FRDGBufferRef noiseMapBuffer = CreateUploadBuffer(GraphBuilder, TEXT("noiseMapBuffer"), BytesPerElement, NumElements, InitialData, InitialDataSize);
			PassParameters->noiseMap = GraphBuilder.CreateSRV(FRDGBufferSRVDesc(noiseMapBuffer, PF_R32_SINT));


			// normals:
			BytesPerElement = sizeof(float);
			NumElements = Params.mapChunkSize[0] * Params.mapChunkSize[0] * 3;
			FRDGBufferRef normalsBuffer = GraphBuilder.CreateBuffer(
				FRDGBufferDesc::CreateBufferDesc(
					BytesPerElement,
					NumElements),
				TEXT("normalsBuffer"));
			PassParameters->normals = GraphBuilder.CreateUAV(FRDGBufferUAVDesc(normalsBuffer, PF_R32_SINT));


			// tangents:
			BytesPerElement = sizeof(float);
			NumElements = Params.mapChunkSize[0] * Params.mapChunkSize[0] * 3;
			FRDGBufferRef tangentsBuffer = GraphBuilder.CreateBuffer(
				FRDGBufferDesc::CreateBufferDesc(
					BytesPerElement,
					NumElements),
				TEXT("tangentsBuffer"));
			PassParameters->tangents = GraphBuilder.CreateUAV(FRDGBufferUAVDesc(tangentsBuffer, PF_R32_SINT));



			// Get the number of groups to dispatch to our compute shader:
			FIntVector GroupCount = FIntVector(Params.THREAD_GROUPS_X, 1, 1);

			// Adds a lambda pass to the graph with a runtime-generated parameter struct:
			GraphBuilder.AddPass(
				RDG_EVENT_NAME("ExecuteNormalsAndTangentsCS"),
				PassParameters,
				ERDGPassFlags::AsyncCompute,
				[&PassParameters, ComputeShader, GroupCount](FRHIComputeCommandList& RHICmdList)
				{
					FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, *PassParameters, GroupCount);
				});


			// Adds a "Copy" pass for the normals buffer:
			elementCount = Params.mapChunkSize[0] * Params.mapChunkSize[0] * 3;
			ByteCount = sizeof(float) * (elementCount);
			FRHIGPUBufferReadback* NormalsBufferReadback = new FRHIGPUBufferReadback(TEXT("ExecuteNormalsAndTangentsCSNormalsOutput"));
			AddEnqueueCopyPass(GraphBuilder, NormalsBufferReadback, normalsBuffer, 0u);  // NOTE: Passing '0u' for the 'uint32 NumBytes' parameter just means that the entire buffer will be copied over

			// Adds a "Copy" pass for the tangents buffer:
			elementCount = Params.mapChunkSize[0] * Params.mapChunkSize[0] * 3;
			ByteCount = sizeof(float) * (elementCount);
			FRHIGPUBufferReadback* TangentsBufferReadback = new FRHIGPUBufferReadback(TEXT("ExecuteNormalsAndTangentsCSTangentsOutput"));
			AddEnqueueCopyPass(GraphBuilder, TangentsBufferReadback, tangentsBuffer, 0u);  // NOTE: Passing '0u' for the 'uint32 NumBytes' parameter just means that the entire buffer will be copied over


			auto RunnerFunc = [NormalsBufferReadback, TangentsBufferReadback, NormalsAndTangentsCompletionEvent, &normals, &tangents, elementCount, ByteCount](auto&& RunnerFunc) mutable -> void
			{
				uint32 ThreadId = FPlatformTLS::GetCurrentThreadId();
				const FString ThreadName = FThreadManager::Get().GetThreadName(ThreadId);
				FPlatformProcess::Sleep(0.05f);

				if (NormalsBufferReadback->IsReady() && TangentsBufferReadback->IsReady())
				//if (NormalsBufferReadback->IsReady())
				{
					UE_LOG(LogTemp, Warning, TEXT("| %s |	    THREAD: NormalsBufferReadback->IsReady() == '%s'    |    TangentsBufferReadback->IsReady() == '%s'"), 
												  *ThreadName, NormalsBufferReadback->IsReady() ? TEXT("true") : TEXT("false"), TangentsBufferReadback->IsReady() ? TEXT("true") : TEXT("false"));

					// Wait a small amount of time for synchonization purposes:
					FPlatformProcess::Sleep(0.05f);

					float* NORMALS_BUFFER = (float*)NormalsBufferReadback->Lock(ByteCount);
					std::memcpy(normals, NORMALS_BUFFER, elementCount * sizeof(float));
					NormalsBufferReadback->Unlock();
					delete NormalsBufferReadback;

					float* TANGENTS_BUFFER = (float*)TangentsBufferReadback->Lock(ByteCount);
					std::memcpy(tangents, TANGENTS_BUFFER, elementCount * sizeof(float));
					TangentsBufferReadback->Unlock();
					delete TangentsBufferReadback;

					NormalsAndTangentsCompletionEvent->Trigger();
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

	UE_LOG(LogTemp, Warning, TEXT("| %s |	10: GraphBuilder.Execute() EXECUTED"), *ThreadName);
	GraphBuilder.Execute();

	UE_LOG(LogTemp, Warning, TEXT("| %s |	11: ExecuteNormalsAndTangentsCS EXITING....."), *ThreadName);
}

