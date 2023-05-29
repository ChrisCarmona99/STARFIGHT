#pragma once


#include "CoreMinimal.h"

#include "GenericPlatform/GenericPlatformMisc.h"
#include "Kismet/BlueprintAsyncActionBase.h"

#include <thread>
#include <mutex>
#include <chrono>

#include "NoiseMapComputeShader.generated.h"



struct SHADERMODULE_API FNoiseMapComputeShaderDispatchParams
{
	FNoiseMapComputeShaderDispatchParams(int x, int y, int z)
		: X(x), Y(y), Z(z) {}

	int X;
	int Y;
	int Z;
	int THREAD_GROUPS_X;

	
	int32 mapChunkSize[1];
	float noiseScale[1];
	int32 octaveCount[1];
	float persistance[1];
	float lacurnarity[1];
	FVector2f* octaveOffsets;

	float* noiseMap;

};



// This is a public interface that we define so outside code can invoke our compute shader:
class SHADERMODULE_API FNoiseMapComputeShaderInterface
{

public:
	/*
	//// (DEC & DEF): Dispatches this shader. Can be called from any thread
	//static void Dispatch(FNoiseMapComputeShaderDispatchParams Params,
	//					 TFunction<void(std::shared_ptr<float[]> OUTPUT)> AsyncCallback)
	//{
	//	if (IsInRenderingThread())
	//	{
	//		DispatchRenderThread(GetImmediateCommandList_ForRenderCommand(), Params, AsyncCallback);
	//	}
	//	else
	//	{
	//		DispatchGameThread(Params, AsyncCallback);
	//	}
	//}


	//// Executes this shader on the render thread from the game thread via EnqueueRenderThreadCommand:
	//static void DispatchGameThread(FNoiseMapComputeShaderDispatchParams Params,
	//							   TFunction<void(std::shared_ptr<float[]> OUTPUT)> AsyncCallback)
	//{
	//	ENQUEUE_RENDER_COMMAND(SceneDrawCompletion)(
	//		[Params, AsyncCallback](FRHICommandListImmediate& RHICmdList)
	//		{
	//			DispatchRenderThread(RHICmdList, Params, AsyncCallback);
	//		});
	//}
	*/
	

	// Executes this shader on the render thread:
	static void ExecuteNoiseMapComputeShader(
		FRHICommandListImmediate& RHICmdList,
		FNoiseMapComputeShaderDispatchParams Params,
		float*& noiseMap,
		std::mutex& ComputeShaderMutex,
		FEvent* NoiseMapCompletionEvent);

	/*
	// Executes this shader on the render thread:
	static void DispatchRenderThread_OLD(FRHICommandListImmediate& RHICmdList,
		FNoiseMapComputeShaderDispatchParams Params,
		TFunction<void(std::shared_ptr<float[]> OUTPUT)> AsyncCallback);
	*/

};


/*

//DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNoiseMapComputeShaderLibrary_AsyncExecutionCompleted, const TArray<float>&, noiseMap);  // DEFINES OUR BLUEPRINT FUNCTIONS OUTPUT VALUES
//DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNoiseMapComputeShaderLibrary_AsyncExecutionCompleted, const float, TEMP);  // DEFINES OUR BLUEPRINT FUNCTIONS OUTPUT VALUES




// This Class is used to create a Blueprint Callable version of our Dispath Function, and call Dispatch:
UCLASS()
class SHADERMODULE_API UNoiseMapComputeShaderLibrary_AsyncExecution : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:

	// Instantiate an object that can be used to "broadcast" whatever value(s) we want from our shader to be seen from the editor (I think):
	UPROPERTY(BlueprintAssignable)
		FOnNoiseMapComputeShaderLibrary_AsyncExecutionCompleted Completed;


	// SHADER INPUTS from our Blueprint function:
	int32 _mapChunkSize;
	int32 _seed;
	FVector2D _offset;
	float _noiseScale;
	int32 _octaves;
	float _persistance;
	float _lacurnarity;

	std::shared_ptr<float> _noiseMap;


	
	virtual void Activate() override
	{
		UE_LOG(LogTemp, Warning, TEXT("    2: 'Activate' Called"));

		// Create a dispatch parameters struct
		FNoiseMapComputeShaderDispatchParams Params(1024, 1, 1); // Input Parameters define our groups


		// Assign our inputs from our Blueprint function to our shader values. Make any necessary conversions here:
		Params.mapChunkSize[0] = _mapChunkSize;
		Params.seed[0] = _seed;
		Params.offset[0] = _offset;
		Params.noiseScale[0] = _noiseScale;
		Params.octaves[0] = _octaves;
		Params.persistance[0] = _persistance;
		Params.lacurnarity[0] = _lacurnarity;

		Params.noiseMap = _noiseMap;


		// Dispatch the compute shader and wait until it completes
		FNoiseMapComputeShaderInterface::Dispatch(Params, [this](float TEMP)
			{
				this->Completed.Broadcast(TEMP);
			});
		UE_LOG(LogTemp, Warning, TEXT("    3: 'Activate' FINISHED"));
	}



	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", Category = "ComputeShader", WorldContext = "WorldContextObject"))
		static UNoiseMapComputeShaderLibrary_AsyncExecution* ExecuteBaseComputeShader(UObject* WorldContextObject, int32 mapChunkSize)
	{
		UNoiseMapComputeShaderLibrary_AsyncExecution* Action = NewObject<UNoiseMapComputeShaderLibrary_AsyncExecution>();

		// Set our UNoiseMapComputeShaderLibrary_AsyncExecution Class private variables that will then be called in the Activate() method: 
		Action->_mapChunkSize = mapChunkSize;


		std::shared_ptr<float> noiseMap(new float[mapChunkSize * mapChunkSize]);
		Action->_noiseMap = noiseMap;
		Action->_seed = 1;
		Action->_offset = FVector2D(1, 2);
		Action->_noiseScale = 1;
		Action->_octaves = 1;
		Action->_persistance = 1;
		Action->_lacurnarity = 1;

		// Implicitly calls `Dispatch`:
		Action->RegisterWithGameInstance(WorldContextObject);
		UE_LOG(LogTemp, Warning, TEXT("    1: Returning 'Action'"));
		return Action; // CALLS Activate()
	}
};

*/
