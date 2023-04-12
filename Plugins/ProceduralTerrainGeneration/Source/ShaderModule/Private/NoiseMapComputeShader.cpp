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

#include <thread>

DECLARE_STATS_GROUP(TEXT("NoiseMapComputeShader"), STATGROUP_NoiseMapComputeShader, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("NoiseMapComputeShader Execute"), STAT_NoiseMapComputeShader_Execute, STATGROUP_NoiseMapComputeShader);


/*
*
*
* This class carries our parameter declarations and acts as the bridge between cpp and HLSL.
*
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
		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<FVector2f>, octaveOffsets) // On the shader side: float3 MyVector;

		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<float>, noiseMap)
		//SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<float>, noiseMap)

		END_SHADER_PARAMETER_STRUCT()

};



// This will tell the engine to create the shader and where the shader entry point is:
// 
//                            ShaderType                            ShaderPath										Shader function name    Type
IMPLEMENT_GLOBAL_SHADER(FNoiseMapComputeShader, "/Plugins/ProceduralTerrainGeneration/NoiseMapComputeShader.usf", "NoiseMapComputeShader", SF_Compute);



void FNoiseMapComputeShaderInterface::DispatchRenderThread(FRHICommandListImmediate& RHICmdList,
	FNoiseMapComputeShaderDispatchParams Params,
	TFunction<void(std::shared_ptr<float[]> OUTPUT)> AsyncCallback)
{
	UE_LOG(LogTemp, Warning, TEXT("DispatchRenderThread CALLED"));

	FRDGBuilder GraphBuilder(RHICmdList);
	FRHIGPUFence* GPUFence = RHICreateGPUFence(TEXT("NoiseMapComputeShaderFence"));

	{
		SCOPE_CYCLE_COUNTER(STAT_NoiseMapComputeShader_Execute);
		DECLARE_GPU_STAT(NoiseMapComputeShader)
			RDG_EVENT_SCOPE(GraphBuilder, "NoiseMapComputeShader");
		RDG_GPU_STAT_SCOPE(GraphBuilder, NoiseMapComputeShader);

		typename FNoiseMapComputeShader::FPermutationDomain PermutationVector;

		TShaderMapRef<FNoiseMapComputeShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel), PermutationVector);


		bool bIsShaderValid = ComputeShader.IsValid();

		if (bIsShaderValid)
		{
			// Allocate a parameter struct 'PassParameters' with a lifetime tied to graph execution
			FNoiseMapComputeShader::FParameters* PassParameters = GraphBuilder.AllocParameters<FNoiseMapComputeShader::FParameters>();


			/*
			* From here, use our FRDGBuilder instance 'GraphBuilder' to create the various buffer objects:
			*/
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

			// octaveOffsets:
			BytesPerElement = sizeof(FVector2f);
			NumElements = Params.octaveCount[0];
			InitialData = reinterpret_cast<void*>(Params.octaveOffsets.get()); // NOTE: This is how you can cast to a 'void*' when using shared pointers
			InitialDataSize = BytesPerElement * NumElements;

			FRDGBufferRef octaveOffsetsBuffer = CreateUploadBuffer(GraphBuilder, TEXT("octaveOffsetsBuffer"), BytesPerElement, NumElements, InitialData, InitialDataSize);
			PassParameters->octaveOffsets = GraphBuilder.CreateSRV(FRDGBufferSRVDesc(octaveOffsetsBuffer, PF_R32_SINT));


			// noiseMap:
			/*NumElements = Params.mapChunkSize[0] * Params.mapChunkSize[0] + 1;
			UE_LOG(LogTemp, Warning, TEXT("noiseMapBuffer NumElements + 1 == %d"), NumElements);
			Desc = FRDGBufferDesc::CreateBufferDesc(sizeof(float), NumElements);

			FRDGBufferRef noiseMapBuffer = GraphBuilder.CreateBuffer(Desc, TEXT("noiseMapBuffer"));
			PassParameters->noiseMap = GraphBuilder.CreateUAV(FRDGBufferUAVDesc(noiseMapBuffer, PF_R32_SINT));*/

			BytesPerElement = sizeof(float);
			NumElements = Params.mapChunkSize[0] * Params.mapChunkSize[0];
			InitialData = reinterpret_cast<void*>(Params.noiseMap.get());
			//InitialDataSize = BytesPerElement * NumElements;
			InitialDataSize = 0; // Because our noiseMap is just a nullptr, are initial data size is just 0, making it any greater would cause 
			
			FRDGBufferRef noiseMapBuffer = CreateStructuredBuffer(GraphBuilder, TEXT("noiseMapBuffer"), BytesPerElement, NumElements, InitialData, InitialDataSize, ERDGInitialDataFlags::None);
			PassParameters->noiseMap = GraphBuilder.CreateUAV(FRDGBufferUAVDesc(noiseMapBuffer, PF_R32_SINT));

			TRefCountPtr<FRDGPooledBuffer> pooled_noiseMap;

			GraphBuilder.QueueBufferExtraction(
				noiseMapBuffer,
				&pooled_noiseMap,
				ERHIAccess::CPURead);










			// Get the number of groups to dispatch to our compute shader:
			int32 groupSize = FComputeShaderUtils::kGolden2DGroupSize; // This parameter is set to 8, apparently this is the "Ideal size of group size 8x8 to occupy at least an entire wave on GCN, two warp on Nvidia"
			FIntVector GroupCount = FComputeShaderUtils::GetGroupCount(FIntVector(Params.X, Params.Y, Params.Z), 1); // Resulting group count for X, Y, Z == Params.<>/groupSize rounded up for each
			UE_LOG(LogTemp, Warning, TEXT("    GroupCount == %s"), *GroupCount.ToString());

			// Adds a lambda pass to the graph with a runtime-generated parameter struct:
			GraphBuilder.AddPass(RDG_EVENT_NAME("ExecuteNoiseMapComputeShader"),
								 PassParameters,
								 ERDGPassFlags::AsyncCompute,
								 [&PassParameters, ComputeShader, GroupCount](FRHIComputeCommandList& RHICmdList)
								 {
								 	FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, *PassParameters, GroupCount);
								 });


			FRHIGPUBufferReadback* GPUBufferReadback = new FRHIGPUBufferReadback(TEXT("ExecuteNoiseMapComputeShaderOutput"));

			int vertexCount = Params.mapChunkSize[0] * Params.mapChunkSize[0];
			uint32 ByteCount = sizeof(float) * (vertexCount);

			// Adds a pass to readback contents of an RDG buffer:
			/*
			* PARAMS:
			*	SourceBuffer: specifies the GPU buffer whose contents you want to copy
			*	Readback: specifies the CPU buffer into which you want to copy the contents of SourceBuffer
			*	NumBytes: specifies the number of bytes to copy
			*/
			
			// AddEnqueueCopyPass(GraphBuilder, GPUBufferReadback, noiseMapBuffer, 0u); // Passing '0u' for the 'uint32 NumBytes' parameter just means that the entire buffer will be copied over
			AddEnqueueCopyPass(GraphBuilder, GPUBufferReadback, noiseMapBuffer, ByteCount);

			//// Enqueue a copy command to copy the output data from the GPU to the CPU
			//FRDGBufferRef OutputBuffer = GraphBuilder.RegisterExternalBuffer();
			//CopyBuffer(GraphBuilder, OutputBuffer, PassParameters->OutputBuffer);










			// Define's a lambda for our Compute Shader that will grab the output from our 'GPUBufferReadback' and populate our output variables with it, or call our async func again to wait for our GPU to be done:
			auto RunnerFunc = [GPUBufferReadback, ByteCount, Params, AsyncCallback](auto&& RunnerFunc) -> void
			{
				// If our GPU readback is complete, then unlock the buffer, read from it, set our output variables with the contents in the buffer, and execute this all on the main thread...
				if (GPUBufferReadback->IsReady())
				{
					std::shared_ptr<float[]> BUFFER(reinterpret_cast<float*>(GPUBufferReadback->Lock(ByteCount)));

					// Just used to debug our BUFFER:
					//float* BUFFER_ptr = BUFFER.get();
					//for (int index = 0; index < vertexCount; index += FMath::DivideAndRoundUp(vertexCount, 50))
					//for (int index = 0; index < vertexCount; index++)
					//{
					//	  float value = BUFFER_ptr[index];
					//	  UE_LOG(LogTemp, Warning, TEXT("        index: %d   |   value: %f"), index, value);
					//}

					GPUBufferReadback->Unlock();

					// Set output variable:
					std::shared_ptr<float[]> OUTPUT = BUFFER;

					// FINAL ASYNC (Propogates compute shader output):
					AsyncTask(ENamedThreads::GameThread,
						[AsyncCallback, OUTPUT]()
						{
							AsyncCallback(OUTPUT);
						});

					delete GPUBufferReadback;
				}
				// If our GPU readback is NOT complete, then just execute our 'RunnerFunc' AGAIN on our Rendering thread (non-game thread) again and see if it will be ready by the next execution:
				else
				{
					AsyncTask(ENamedThreads::ActualRenderingThread,
						[RunnerFunc]()
						{
							RunnerFunc(RunnerFunc);
						});
				}

				
			};

			// Call our 'RunnerFunc' asynchronously for the FIRST TIME on our Rendering thread ('RunnerFunc' will keep calling this Async func again and again until the GPU has outputed its contents into our 'GPUBufferReadback':
			AsyncTask(ENamedThreads::ActualRenderingThread,
				[RunnerFunc]()
				{
					RunnerFunc(RunnerFunc); //Call RunnerFunc and pass an instance of itself so that it can recursively call itself within another 'AsyncTask' call if the GPU is not done yet
				});

		}
		else {
			// We silently exit here as we don't want to crash the game if the shader is not found or has an error.
			UE_LOG(LogTemp, Warning, TEXT("COMPUTE SHADER SILENTLY EXITED... This is most likely due to an error within the GPU during execution."));
		}
	}

	GraphBuilder.Execute();
	UE_LOG(LogTemp, Warning, TEXT("DispatchRenderThread FINISHED"));
}
