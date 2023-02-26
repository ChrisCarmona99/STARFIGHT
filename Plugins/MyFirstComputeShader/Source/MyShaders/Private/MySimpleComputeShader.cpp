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
														   TFunction<void(int OutputVal)> AsyncCallback) 
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





			// Get the number of groups to dispatch to our compute shader:
			auto GroupCount = FComputeShaderUtils::GetGroupCount(FIntVector(Params.X, Params.Y, Params.Z), FComputeShaderUtils::kGolden2DGroupSize);
			
			/** Adds a lambda pass to the graph with an accompanied pass parameter struct.
			*
			*  RDG resources declared in the struct (via _RDG parameter macros) are safe to access in the lambda. The pass parameter struct
			*  should be allocated by AllocParameters(), and once passed in, should not be mutated. It is safe to provide the same parameter
			*  struct to multiple passes, so long as it is kept immutable. The lambda is deferred until execution unless the immediate debug
			*  mode is enabled. All lambda captures should assume deferral of execution.
			*
			*  The lambda must include a single RHI command list as its parameter. The exact type of command list depends on the workload.
			*  For example, use FRHIComputeCommandList& for Compute / AsyncCompute workloads. Raster passes should use FRHICommandList&.
			*  Prefer not to use FRHICommandListImmediate& unless actually required.
			*
			*  Declare the type of GPU workload (i.e. Copy, Compute / AsyncCompute, Graphics) to the pass via the Flags argument. This is
			*  used to determine async compute regions, render pass setup / merging, RHI transition accesses, etc. Other flags exist for
			*  specialized purposes, like forcing a pass to never be culled (NeverCull). See ERDGPassFlags for more info.
			*
			*  The pass name is used by debugging / profiling tools.
			*/
			GraphBuilder.AddPass(RDG_EVENT_NAME("ExecuteMySimpleComputeShader"),
								 PassParameters,
								 ERDGPassFlags::AsyncCompute,
								 [&PassParameters, ComputeShader, GroupCount](FRHIComputeCommandList& RHICmdList)
								 {
								     FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, *PassParameters, GroupCount);
								 });


			FRHIGPUBufferReadback* GPUBufferReadback = new FRHIGPUBufferReadback(TEXT("ExecuteMySimpleComputeShaderOutput"));
			// Adds a pass to readback contents of an RDG buffer:
			AddEnqueueCopyPass(GraphBuilder, GPUBufferReadback, OutputBuffer, 0u);


			auto RunnerFunc = [GPUBufferReadback, AsyncCallback](auto&& RunnerFunc) -> void 
			{
				if (GPUBufferReadback->IsReady()) {

					int32* Buffer = (int32*)GPUBufferReadback->Lock(1);
					int OutVal = Buffer[0];

					GPUBufferReadback->Unlock();

					AsyncTask(ENamedThreads::GameThread, 
							  [AsyncCallback, OutVal]() 
							  {
							      AsyncCallback(OutVal);
							  });

					delete GPUBufferReadback;
				}
				else {
					AsyncTask(ENamedThreads::ActualRenderingThread, 
							  [RunnerFunc]() 
							  {
							      RunnerFunc(RunnerFunc);
							  });
				}
			};
			


			AsyncTask(ENamedThreads::ActualRenderingThread, 
					  [RunnerFunc]() 
					  {
					      RunnerFunc(RunnerFunc);
					  });

		}
		else {
			// We silently exit here as we don't want to crash the game if the shader is not found or has an error.

		}
	}

	GraphBuilder.Execute();
	UE_LOG(LogTemp, Warning, TEXT("DispatchRenderThread FINISHED"));
}
