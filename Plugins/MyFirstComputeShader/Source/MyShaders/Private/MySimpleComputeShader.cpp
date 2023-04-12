#include "MyShaders/Public/MySimpleComputeShaderExternals.h"
#include "MyShaders/Public/MySimpleComputeShader.h"
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

DECLARE_STATS_GROUP(TEXT("MySimpleComputeShader"), STATGROUP_MySimpleComputeShader, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("MySimpleComputeShader Execute"), STAT_MySimpleComputeShader_Execute, STATGROUP_MySimpleComputeShader);

#include <chrono>


/* 
* 
* 
* This class carries our parameter declarations and acts as the bridge between cpp and HLSL.
* 
* 
* DEC & DEF in the .cpp file because this class is only used in 'DispatchRenderThread'
*/
class MYSHADERS_API FMySimpleComputeShader : public FGlobalShader
{
public:

	DECLARE_GLOBAL_SHADER(FMySimpleComputeShader);
	SHADER_USE_PARAMETER_STRUCT(FMySimpleComputeShader, FGlobalShader);

	
	class FMySimpleComputeShader_Perm_TEST : SHADER_PERMUTATION_INT("TEST", 1);
	using FPermutationDomain = TShaderPermutationDomain<FMySimpleComputeShader_Perm_TEST>;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		/*
		* Here's where you define one or more of the input parameters for your shader.
		* Some examples:
		*/
		// SHADER_PARAMETER(uint32, MyUint32) // On the shader side: uint32 MyUint32;
		// SHADER_PARAMETER(FVector3f, MyVector) // On the shader side: float3 MyVector;

		// SHADER_PARAMETER_TEXTURE(Texture2D, MyTexture) // On the shader side: Texture2D<float4> MyTexture; (float4 should be whatever you expect each pixel in the texture to be, in this case float4(R,G,B,A) for 4 channels)
		// SHADER_PARAMETER_SAMPLER(SamplerState, MyTextureSampler) // On the shader side: SamplerState MySampler; // CPP side: TStaticSamplerState<ESamplerFilter::SF_Bilinear>::GetRHI();

		// SHADER_PARAMETER_ARRAY(float, MyFloatArray, [3]) // On the shader side: float MyFloatArray[3];

		// SHADER_PARAMETER_UAV(RWTexture2D<FVector4f>, MyTextureUAV) // On the shader side: RWTexture2D<float4> MyTextureUAV;
		// SHADER_PARAMETER_UAV(RWStructuredBuffer<FMyCustomStruct>, MyCustomStructs) // On the shader side: RWStructuredBuffer<FMyCustomStruct> MyCustomStructs;
		// SHADER_PARAMETER_UAV(RWBuffer<FMyCustomStruct>, MyCustomStructs) // On the shader side: RWBuffer<FMyCustomStruct> MyCustomStructs;

		// SHADER_PARAMETER_SRV(StructuredBuffer<FMyCustomStruct>, MyCustomStructs) // On the shader side: StructuredBuffer<FMyCustomStruct> MyCustomStructs;
		// SHADER_PARAMETER_SRV(Buffer<FMyCustomStruct>, MyCustomStructs) // On the shader side: Buffer<FMyCustomStruct> MyCustomStructs;
		// SHADER_PARAMETER_SRV(Texture2D<FVector4f>, MyReadOnlyTexture) // On the shader side: Texture2D<float4> MyReadOnlyTexture;

		// SHADER_PARAMETER_STRUCT_REF(FMyCustomStruct, MyCustomStruct)


		//SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<int>, Input)
		//SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<int>, Output)

		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<int>, mapChunkSize)
		//SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<float>, noiseMap)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<float>, noiseMap)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<FIntVector>, indexArray)


	END_SHADER_PARAMETER_STRUCT()

public:

	// NOTE: NOTE BEING CALLED AT THE MOMENT
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		UE_LOG(LogTemp, Warning, TEXT("ShouldCompilePermutation CALLED"));
		const FPermutationDomain PermutationVector(Parameters.PermutationId);

		return true;
	}

	// NOTE: NOTE BEING CALLED AT THE MOMENT
	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		UE_LOG(LogTemp, Warning, TEXT("ModifyCompilationEnvironment CALLED"));
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

		const FPermutationDomain PermutationVector(Parameters.PermutationId);

		/*
		* Here you define constants that can be used statically in the shader code.
		* Example:
		*/
		// OutEnvironment.SetDefine(TEXT("MY_CUSTOM_CONST"), TEXT("1"));

		/*
		* These defines are used in the thread count section of our shader
		*/
		OutEnvironment.SetDefine(TEXT("THREADS_X"), NUM_THREADS_MySimpleComputeShader_X);
		OutEnvironment.SetDefine(TEXT("THREADS_Y"), NUM_THREADS_MySimpleComputeShader_Y);
		OutEnvironment.SetDefine(TEXT("THREADS_Z"), NUM_THREADS_MySimpleComputeShader_Z);

		// This shader must support typed UAV load and we are testing if it is supported at runtime using RHIIsTypedUAVLoadSupported
		//OutEnvironment.CompilerFlags.Add(CFLAG_AllowTypedUAVLoads);

		// FForwardLightingParameters::ModifyCompilationEnvironment(Parameters.Platform, OutEnvironment);
	}
private:
};





// This will tell the engine to create the shader and where the shader entry point is:
// 
//                            ShaderType                            ShaderPath								Shader function name    Type
IMPLEMENT_GLOBAL_SHADER(FMySimpleComputeShader, "/Plugins/MyFirstComputeShader/MySimpleComputeShader.usf", "MySimpleComputeShader", SF_Compute);





void FMySimpleComputeShaderInterface::DispatchRenderThread(FRHICommandListImmediate& RHICmdList, 
														   FMySimpleComputeShaderDispatchParams Params,
														   TFunction<void(float TEMP)> AsyncCallback) 
{
	UE_LOG(LogTemp, Warning, TEXT("DispatchRenderThread CALLED"));

	FRDGBuilder GraphBuilder(RHICmdList);

	// Not too sure why this pair of brackets starting on the next line are here, but I think there may be a mutex lock within one of these 4 macros
	{
		SCOPE_CYCLE_COUNTER(STAT_MySimpleComputeShader_Execute);
		DECLARE_GPU_STAT(MySimpleComputeShader)
			RDG_EVENT_SCOPE(GraphBuilder, "MySimpleComputeShader");
		RDG_GPU_STAT_SCOPE(GraphBuilder, MySimpleComputeShader);

		typename FMySimpleComputeShader::FPermutationDomain PermutationVector;

		// Add any static permutation options here
		// PermutationVector.Set<FMySimpleComputeShader::FMyPermutationName>(12345);

		TShaderMapRef<FMySimpleComputeShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel), PermutationVector);


		bool bIsShaderValid = ComputeShader.IsValid();

		if (bIsShaderValid) 
		{
			// Allocate a parameter struct 'PassParameters' with a lifetime tied to graph execution
			FMySimpleComputeShader::FParameters* PassParameters = GraphBuilder.AllocParameters<FMySimpleComputeShader::FParameters>();

			/*
			* From here, use our FRDGBuilder instance 'GraphBuilder' to create the various buffer objects:
			*/
			uint32 BytesPerElement;
			uint32 NumElements;
			const void* InitialData;
			uint64 InitialDataSize;

			FRDGBufferDesc Desc;

			/*
			// Input:
			BytesPerElement = sizeof(int);
			NumElements = 2;
			InitialData = (void*)Params.Input;
			InitialDataSize = BytesPerElement * NumElements;

			FRDGBufferRef InputBuffer = CreateUploadBuffer(GraphBuilder, TEXT("InputBuffer"), BytesPerElement, NumElements, InitialData, InitialDataSize);
			PassParameters->Input = GraphBuilder.CreateSRV(FRDGBufferSRVDesc(InputBuffer, PF_R32_SINT));

			
			// Output:
			const FRDGBufferDesc& Desc = FRDGBufferDesc::CreateBufferDesc(sizeof(int32), 1);

			FRDGBufferRef OutputBuffer = GraphBuilder.CreateBuffer(Desc, TEXT("OutputBuffer"));
			PassParameters->Output = GraphBuilder.CreateUAV(FRDGBufferUAVDesc(OutputBuffer, PF_R32_SINT));


			// TestFloat:
			BytesPerElement = sizeof(float);
			NumElements = 1;
			InitialData = (void*)Params.TestFloat;
			InitialDataSize = BytesPerElement * NumElements;

			FRDGBufferRef TestFloatBuffer = CreateStructuredBuffer(GraphBuilder, TEXT("TestFloatBuffer"), BytesPerElement, NumElements, InitialData, InitialDataSize);
			PassParameters->TestFloat = GraphBuilder.CreateUAV(FRDGBufferUAVDesc(TestFloatBuffer, PF_R32_SINT));
			*/

			// mapChunkSize:
			BytesPerElement = sizeof(int32);
			NumElements = 1;
			InitialData = (void*)Params.mapChunkSize;
			InitialDataSize = BytesPerElement * NumElements;

			FRDGBufferRef mapChunkSizeBuffer = CreateUploadBuffer(GraphBuilder, TEXT("mapChunkSizeBuffer"), BytesPerElement, NumElements, InitialData, InitialDataSize);
			PassParameters->mapChunkSize = GraphBuilder.CreateSRV(FRDGBufferSRVDesc(mapChunkSizeBuffer, PF_R32_SINT));

			// noiseMap:
			/*BytesPerElement = sizeof(float);
			NumElements = Params.mapChunkSize[0] * Params.mapChunkSize[0];;
			InitialData = (void*)Params.noiseMap;
			InitialDataSize = BytesPerElement * NumElements;

			FRDGBufferRef noiseMapBuffer = CreateStructuredBuffer(GraphBuilder, TEXT("noiseMapBuffer"), BytesPerElement, NumElements, InitialData, InitialDataSize);
			PassParameters->noiseMap = GraphBuilder.CreateUAV(FRDGBufferUAVDesc(noiseMapBuffer, PF_R32_SINT));*/

			NumElements = Params.mapChunkSize[0] * Params.mapChunkSize[0];
			UE_LOG(LogTemp, Warning, TEXT("noiseMapBuffer NumElements == %d"), 1);
			Desc = FRDGBufferDesc::CreateBufferDesc(sizeof(float), NumElements);

			FRDGBufferRef noiseMapBuffer = GraphBuilder.CreateBuffer(Desc, TEXT("noiseMapBuffer"));
			PassParameters->noiseMap = GraphBuilder.CreateUAV(FRDGBufferUAVDesc(noiseMapBuffer, PF_R32_SINT));

			// indexArray:
			NumElements = Params.mapChunkSize[0] * Params.mapChunkSize[0];
			UE_LOG(LogTemp, Warning, TEXT("NumElements == %d"), 1);
			Desc = FRDGBufferDesc::CreateBufferDesc(sizeof(FIntVector), NumElements);

			FRDGBufferRef indexArrayBuffer = GraphBuilder.CreateBuffer(Desc, TEXT("indexArrayBuffer"));
			PassParameters->indexArray = GraphBuilder.CreateUAV(FRDGBufferUAVDesc(indexArrayBuffer, PF_R32_SINT));





			// Get the number of groups to dispatch to our compute shader:
			int32 groupSize = FComputeShaderUtils::kGolden2DGroupSize; // This parameter is set to 8, apparently this is the "Ideal size of group size 8x8 to occupy at least an entire wave on GCN, two warp on Nvidia"
			FIntVector GroupCount = FComputeShaderUtils::GetGroupCount(FIntVector(Params.X, Params.Y, Params.Z), 1); // Resulting group count for X, Y, Z == Params.<>/groupSize rounded up for each
			UE_LOG(LogTemp, Warning, TEXT("    GroupCount == %s"), *GroupCount.ToString());

			// Adds a lambda pass to the graph with a runtime-generated parameter struct:
			GraphBuilder.AddPass(RDG_EVENT_NAME("ExecuteMySimpleComputeShader"),
								 PassParameters,
								 ERDGPassFlags::AsyncCompute,
								 [&PassParameters, ComputeShader, GroupCount](FRHIComputeCommandList& RHICmdList)
								 {
								     FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, *PassParameters, GroupCount);
								 });


			FRHIGPUBufferReadback* GPUBufferReadback = new FRHIGPUBufferReadback(TEXT("ExecuteMySimpleComputeShaderOutput"));

			// Adds a pass to readback contents of an RDG buffer:
			AddEnqueueCopyPass(GraphBuilder, GPUBufferReadback, noiseMapBuffer, 0u);
			//AddEnqueueCopyPass(GraphBuilder, GPUBufferReadback, indexArrayBuffer, 0u);


			// Define's a lambda for our Compute Shader that will grab the output from our 'GPUBufferReadback' and populate our output variables with it, or call our async func again to wait for our GPU to be done:
			auto RunnerFunc = [GPUBufferReadback, Params, AsyncCallback](auto&& RunnerFunc) -> void
			{
				// If our GPU readback is complete, then unlock the buffer, read from it, set our output variables with the contents in the buffer, and execute this all on the main thread...
				if (GPUBufferReadback->IsReady()) 
				{
					int vertexCount = Params.mapChunkSize[0] * Params.mapChunkSize[0];
					uint32 ByteCount = sizeof(float) * vertexCount;

					float* BUFFER = (float*)GPUBufferReadback->Lock(ByteCount); // This returns a pointer to the first index in our Buffer. If we've returned an array, just increment over the memory to access the rest of the elements.
					//FIntVector* BUFFER = (FIntVector*)GPUBufferReadback->Lock(1);

					// Just used to debug our BUFFER:
					for (int i = 0; i < vertexCount; i += FMath::DivideAndRoundUp(vertexCount, 50))
					{
						float value = *(BUFFER + i);
						UE_LOG(LogTemp, Warning, TEXT("        index: %d   |   value: %f"), i, value);
						/*FIntVector value = *(BUFFER + i);
						UE_LOG(LogTemp, Warning, TEXT("        index: %d   |   value: %s"), i, *value.ToString());*/
					}


					GPUBufferReadback->Unlock();

					//float TEMP = *(BUFFER);
					float TEMP = 1.0f;
					AsyncTask(ENamedThreads::GameThread, 
							  [AsyncCallback, TEMP]()
							  {
							      AsyncCallback(TEMP);
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
