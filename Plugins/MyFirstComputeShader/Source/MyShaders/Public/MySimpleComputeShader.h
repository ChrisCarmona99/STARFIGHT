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

	int Input[2];
	int Output;

	int32 mapChunkSize[1];
	float* noiseMap;
	float TestFloat[1];
};


// A struct used to organize and pass the original inputs from our blueprint/cpp call to be used within our 'Dispatch' function. This may or may not be necessary, but it helps to organize the parameters.
struct InputParameterReferences
{
	InputParameterReferences() {}

	int32 mapChunkSize_REF;
	std::vector<float> noiseMap_REF;
};


struct TestStruct
{
	TestStruct() {}

	int32 A;
	float B;
	float* C;
};



// This is a public interface that we define so outside code can invoke our compute shader.
// 
// CALLED IN: UMySimpleComputeShaderLibrary_AsyncExecution
class MYSHADERS_API FMySimpleComputeShaderInterface 
{

public:

	// (DEC & DEF): Dispatches this shader. Can be called from any thread
	static void Dispatch(FMySimpleComputeShaderDispatchParams Params,
						 InputParameterReferences InputParamRefs,
						 TFunction<void(int OutputVal, float noiseMap, float TestFloat)> AsyncCallback)
	{
		if (IsInRenderingThread())
		{
			DispatchRenderThread(GetImmediateCommandList_ForRenderCommand(), Params, InputParamRefs, AsyncCallback);
		}
		else
		{
			DispatchGameThread(Params, InputParamRefs, AsyncCallback);
		}
	}

	// Executes this shader on the render thread:
	static void DispatchRenderThread(FRHICommandListImmediate& RHICmdList,
									 FMySimpleComputeShaderDispatchParams Params,
									 InputParameterReferences InputParamRefs,
									 TFunction<void(int OutputVal, float noiseMap, float TestFloat)> AsyncCallback);

	// Executes this shader on the render thread from the game thread via EnqueueRenderThreadCommand:
	static void DispatchGameThread(FMySimpleComputeShaderDispatchParams Params,
								   InputParameterReferences InputParamRefs,
								   TFunction<void(int OutputVal, float noiseMap, float TestFloat)> AsyncCallback)
	{
		ENQUEUE_RENDER_COMMAND(SceneDrawCompletion)(
			[Params, InputParamRefs, AsyncCallback](FRHICommandListImmediate& RHICmdList)
			{
				DispatchRenderThread(RHICmdList, Params, InputParamRefs, AsyncCallback);
			});
	}

};



DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnMySimpleComputeShaderLibrary_AsyncExecutionCompleted, const int, Value, const float, noiseMap, const float, TestFloat);  // DEFINES OUR BLUEPRINT FUNCTIONS OUTPUT VALUES



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
	int _Arg1;
	int _Arg2;

	int32 _mapChunkSize;
	std::vector<float> _noiseMap;


	virtual void Activate() override 
	{
		UE_LOG(LogTemp, Warning, TEXT("    2: 'Activate' Called"));

		// Create a dispatch parameters struct
		FMySimpleComputeShaderDispatchParams Params(1, 1, 1);

		// Pass our original input parameters to a struct we can then pass and use in our Dispatch function:
		InputParameterReferences InputParamRefs;
		InputParamRefs.mapChunkSize_REF = _mapChunkSize;
		InputParamRefs.noiseMap_REF = _noiseMap;

		// Assign our inputs from our Blueprint function to our shader values. Make any necessary conversions here:
		Params.Input[0] = _Arg1;
		Params.Input[1] = _Arg2;

		Params.mapChunkSize[0] = _mapChunkSize;
		Params.noiseMap = &_noiseMap[0];
		Params.TestFloat[0] = 2.465;


		// Dispatch the compute shader and wait until it completes
		FMySimpleComputeShaderInterface::Dispatch(Params, InputParamRefs, [this](int OutputVal, float noiseMap, float TestFloat)
												  {
												       this->Completed.Broadcast(OutputVal, noiseMap, TestFloat);
												  });
		UE_LOG(LogTemp, Warning, TEXT("    3: 'Activate' FINISHED"));
	}



	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", Category = "ComputeShader", WorldContext = "WorldContextObject"))
		static UMySimpleComputeShaderLibrary_AsyncExecution* ExecuteBaseComputeShader(UObject* WorldContextObject, int Arg1, int Arg2, int32 mapChunkSize)
	{
		UMySimpleComputeShaderLibrary_AsyncExecution* Action = NewObject<UMySimpleComputeShaderLibrary_AsyncExecution>();

		// Set our UMySimpleComputeShaderLibrary_AsyncExecution Class private variables that will then be called in the Activate() method: 
		Action->_Arg1 = Arg1;
		Action->_Arg2 = Arg2;
		Action->_mapChunkSize = mapChunkSize;
		std::vector<float> Test_noiseMap(mapChunkSize * mapChunkSize, 1);
		Action->_noiseMap = Test_noiseMap;

		// Implicitly calls `Dispatch`:
		Action->RegisterWithGameInstance(WorldContextObject);
		UE_LOG(LogTemp, Warning, TEXT("    1: Returning 'Action'"));
		return Action; /* CALLS Activate() */
	}

};
