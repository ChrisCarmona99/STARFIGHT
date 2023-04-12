#pragma once

#include "CoreMinimal.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "Kismet/BlueprintAsyncActionBase.h"

#include "MySimpleComputeShader.generated.h"



struct MYSHADERS_API FMySimpleComputeShaderDispatchParams
{
	FMySimpleComputeShaderDispatchParams(int x, int y, int z)
	: X(x), Y(y), Z(z) {}

	int X;
	int Y;
	int Z;


	int32 mapChunkSize[1];
	std::shared_ptr<float> noiseMap;
	std::shared_ptr<FIntVector> indexArray;
};





// This is a public interface that we define so outside code can invoke our compute shader.
// 
// CALLED IN: UMySimpleComputeShaderLibrary_AsyncExecution
class MYSHADERS_API FMySimpleComputeShaderInterface 
{

public:

	// (DEC & DEF): Dispatches this shader. Can be called from any thread
	static void Dispatch(FMySimpleComputeShaderDispatchParams Params,
						 TFunction<void(float TEMP)> AsyncCallback)
	{
		if (IsInRenderingThread())
		{
			DispatchRenderThread(GetImmediateCommandList_ForRenderCommand(), Params, AsyncCallback);
		}
		else
		{
			DispatchGameThread(Params, AsyncCallback);
		}
	}

	// Executes this shader on the render thread:
	static void DispatchRenderThread(FRHICommandListImmediate& RHICmdList,
									 FMySimpleComputeShaderDispatchParams Params,
									 TFunction<void(float TEMP)> AsyncCallback);

	// Executes this shader on the render thread from the game thread via EnqueueRenderThreadCommand:
	static void DispatchGameThread(FMySimpleComputeShaderDispatchParams Params,
								   TFunction<void(float TEMP)> AsyncCallback)
	{
		ENQUEUE_RENDER_COMMAND(SceneDrawCompletion)(
			[Params, AsyncCallback](FRHICommandListImmediate& RHICmdList)
			{
				DispatchRenderThread(RHICmdList, Params, AsyncCallback);
			});
	}

};


//DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMySimpleComputeShaderLibrary_AsyncExecutionCompleted, const TArray<float>&, noiseMap);  // DEFINES OUR BLUEPRINT FUNCTIONS OUTPUT VALUES
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMySimpleComputeShaderLibrary_AsyncExecutionCompleted, const float, TEMP);  // DEFINES OUR BLUEPRINT FUNCTIONS OUTPUT VALUES



/* 
* 
* This Class is used to create a Blueprint Callable version of our Dispath Function, and call Dispatch:
* 
*/
UCLASS() // Change the _API to match your project
class MYSHADERS_API UMySimpleComputeShaderLibrary_AsyncExecution : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:

	// Instantiate an object that can be used to "broadcast" whatever value(s) we want from our shader to be seen from the editor (I think):
	UPROPERTY(BlueprintAssignable)
		FOnMySimpleComputeShaderLibrary_AsyncExecutionCompleted Completed;

	
	// SHADER INPUTS from our Blueprint function:
	int32 _mapChunkSize;
	std::shared_ptr<float> _noiseMap;
	std::shared_ptr<FIntVector> _indexArray;


	virtual void Activate() override
	{
		UE_LOG(LogTemp, Warning, TEXT("    2: 'Activate' Called"));

		// Create a dispatch parameters struct
		FMySimpleComputeShaderDispatchParams Params(1024, 1, 1); // Input Parameters define our groups


		// Assign our inputs from our Blueprint function to our shader values. Make any necessary conversions here:
		Params.mapChunkSize[0] = _mapChunkSize;
		Params.noiseMap = _noiseMap;
		Params.indexArray = _indexArray;


		// Dispatch the compute shader and wait until it completes
		FMySimpleComputeShaderInterface::Dispatch(Params, [this](float TEMP)
												  {   
												      this->Completed.Broadcast(TEMP);
												  });
		UE_LOG(LogTemp, Warning, TEXT("    3: 'Activate' FINISHED"));
	}



	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", Category = "ComputeShader", WorldContext = "WorldContextObject"))
		static UMySimpleComputeShaderLibrary_AsyncExecution* ExecuteBaseComputeShader(UObject* WorldContextObject, int32 mapChunkSize)
	{
		UMySimpleComputeShaderLibrary_AsyncExecution* Action = NewObject<UMySimpleComputeShaderLibrary_AsyncExecution>();

		// Set our UMySimpleComputeShaderLibrary_AsyncExecution Class private variables that will then be called in the Activate() method: 
		Action->_mapChunkSize = mapChunkSize;
		std::shared_ptr<float> noiseMap(new float[mapChunkSize * mapChunkSize]);
		/*std::shared_ptr<FIntVector> indexArray(new FIntVector[mapChunkSize * mapChunkSize]);*/
		std::shared_ptr<FIntVector> indexArray(new FIntVector[5]);
		Action->_noiseMap = noiseMap;
		Action->_indexArray = indexArray;

		
		// Implicitly calls `Dispatch`:
		Action->RegisterWithGameInstance(WorldContextObject);
		UE_LOG(LogTemp, Warning, TEXT("    1: Returning 'Action'"));
		return Action; /* CALLS Activate() */
	}

};
