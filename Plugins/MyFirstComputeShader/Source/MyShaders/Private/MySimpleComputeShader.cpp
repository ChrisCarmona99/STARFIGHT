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


		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<int>, Input)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<int>, Output)

		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<int>, mapChunkSize)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<float>, noiseMap)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<float>, TestFloat)


	END_SHADER_PARAMETER_STRUCT()

public:

	// (DEC & DEF): 
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		const FPermutationDomain PermutationVector(Parameters.PermutationId);

		return true;
	}

	// (DEC & DEF): 
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
														   InputParameterReferences InputParamRefs,
														   TFunction<void(int OutputVal, float noiseMap, float TestFloat)> AsyncCallback) 
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


			// Input:
			BytesPerElement = sizeof(int);
			NumElements = 2;
			InitialData = (void*)Params.Input;
			InitialDataSize = BytesPerElement * NumElements;

			FRDGBufferRef InputBuffer = CreateUploadBuffer(GraphBuilder, TEXT("InputBuffer"), BytesPerElement, NumElements, InitialData, InitialDataSize);
			PassParameters->Input = GraphBuilder.CreateSRV(FRDGBufferSRVDesc(InputBuffer, PF_R32_SINT));

			// mapChunkSize:
			BytesPerElement = sizeof(int32);
			NumElements = 1;
			InitialData = (void*)Params.mapChunkSize;
			InitialDataSize = BytesPerElement * NumElements;

			FRDGBufferRef mapChunkSizeBuffer = CreateUploadBuffer(GraphBuilder, TEXT("mapChunkSizeBuffer"), BytesPerElement, NumElements, InitialData, InitialDataSize);
			PassParameters->mapChunkSize= GraphBuilder.CreateSRV(FRDGBufferSRVDesc(mapChunkSizeBuffer, PF_R32_SINT));
			
			// Output:
			const FRDGBufferDesc& Desc = FRDGBufferDesc::CreateBufferDesc(sizeof(int32), 1);

			FRDGBufferRef OutputBuffer = GraphBuilder.CreateBuffer(Desc, TEXT("OutputBuffer"));
			PassParameters->Output = GraphBuilder.CreateUAV(FRDGBufferUAVDesc(OutputBuffer, PF_R32_SINT));


			// noiseMap:
			BytesPerElement = sizeof(float);
			NumElements = InputParamRefs.noiseMap_REF.size();
			InitialData = (void*)Params.noiseMap;
			InitialDataSize = BytesPerElement * NumElements;

			FRDGBufferRef noiseMapBuffer = CreateStructuredBuffer(GraphBuilder, TEXT("noiseMapBuffer"), BytesPerElement, NumElements, InitialData, InitialDataSize);
			PassParameters->noiseMap = GraphBuilder.CreateUAV(FRDGBufferUAVDesc(noiseMapBuffer, PF_R32_SINT));


			// TestFloat:
			BytesPerElement = sizeof(float);
			NumElements = 1;
			InitialData = (void*)Params.TestFloat;
			InitialDataSize = BytesPerElement * NumElements;

			FRDGBufferRef TestFloatBuffer = CreateStructuredBuffer(GraphBuilder, TEXT("TestFloatBuffer"), BytesPerElement, NumElements, InitialData, InitialDataSize);
			PassParameters->TestFloat = GraphBuilder.CreateUAV(FRDGBufferUAVDesc(TestFloatBuffer, PF_R32_SINT));



			// Get the number of groups to dispatch to our compute shader:
			auto GroupCount = FComputeShaderUtils::GetGroupCount(FIntVector(Params.X, Params.Y, Params.Z), FComputeShaderUtils::kGolden2DGroupSize);
			
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
			AddEnqueueCopyPass(GraphBuilder, GPUBufferReadback, TestFloatBuffer, 0u);
			AddEnqueueCopyPass(GraphBuilder, GPUBufferReadback, OutputBuffer, 0u);
			AddEnqueueCopyPass(GraphBuilder, GPUBufferReadback, noiseMapBuffer, 0u);


			// Define's a lambda for our Compute Shader that will grab the output from our 'GPUBufferReadback' and populate our output variables with it, or call our async func again to wait for our GPU to be done:
			auto RunnerFunc = [GPUBufferReadback, AsyncCallback](auto&& RunnerFunc) -> void 
			{
				// If our GPU readback is complete, then unlock the buffer, read from it, set our output variables with the contents in the buffer, and execute this all on the main thread...
				if (GPUBufferReadback->IsReady()) 
				{
					float* BUFFER = (float*)GPUBufferReadback->Lock(10); // This returns a pointer to the first index in our Buffer. If we've returned an array, just increment over the memory to access the rest of the elements.

					int OutVal = -1.0;
					float* noiseMap = BUFFER;
					float TestFloat = -1.0;

					// Just used to debug our BUFFER:
					for (int i = 0; i < 15; i++)
					{
						int index = i;
						float V = *(BUFFER + i);
						UE_LOG(LogTemp, Warning, TEXT("        index == %d   |   V == %f"), index, V);
					}

					GPUBufferReadback->Unlock();

					AsyncTask(ENamedThreads::GameThread, 
							  [AsyncCallback, OutVal, noiseMap, TestFloat]()
							  {
							      AsyncCallback(OutVal, *noiseMap, TestFloat);
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
