#pragma once

#include "CoreMinimal.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "Kismet/BlueprintAsyncActionBase.h"

#include "MySimpleComputeShader.generated.h"



struct MYSHADERS_API FMySimpleComputeShaderDispatchParams
{
	FMySimpleComputeShaderDispatchParams(int x, int y, int z) : X(x), Y(y), Z(z) {}

	int X;
	int Y;
	int Z;

	int Input[2];
	int Output;	

	int testArray[100];
};





// This is a public interface that we define so outside code can invoke our compute shader.
// 
// CALLED IN: UMySimpleComputeShaderLibrary_AsyncExecution
class MYSHADERS_API FMySimpleComputeShaderInterface 
{

public:

	// (DEC & DEF): Dispatches this shader. Can be called from any thread
	static void Dispatch(FMySimpleComputeShaderDispatchParams Params,
						 TFunction<void(int OutputVal)> AsyncCallback)
	{
		if (IsInRenderingThread())
		{
			/*UE_LOG(LogTemp, Warning, TEXT("IsInRenderingThread == TRUE"));*/
			DispatchRenderThread(GetImmediateCommandList_ForRenderCommand(), Params, AsyncCallback);
		}
		else
		{
			/*UE_LOG(LogTemp, Warning, TEXT("IsInRenderingThread == FALSE"));*/
			DispatchGameThread(Params, AsyncCallback);
		}
	}



	// (DEC): Executes this shader on the render thread
	// (2/2 in .CPP file) 
	static void DispatchRenderThread(FRHICommandListImmediate& RHICmdList,
									 FMySimpleComputeShaderDispatchParams Params,
									 TFunction<void(int OutputVal)> AsyncCallback);

	// (DEC & DEF): Executes this shader on the render thread from the game thread via EnqueueRenderThreadCommand
	static void DispatchGameThread(FMySimpleComputeShaderDispatchParams Params,
								   TFunction<void(int OutputVal)> AsyncCallback)
	{
		UE_LOG(LogTemp, Warning, TEXT("CALLING ENQUEUE_RENDER_COMMAND"));
		ENQUEUE_RENDER_COMMAND(SceneDrawCompletion)(
			[Params, AsyncCallback](FRHICommandListImmediate& RHICmdList)
			{
				UE_LOG(LogTemp, Warning, TEXT("DispatchRenderThread CALLED"));
				DispatchRenderThread(RHICmdList, Params, AsyncCallback);
			});
	}

};





DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMySimpleComputeShaderLibrary_AsyncExecutionCompleted, const int, Value);  // DEFINES WHAT OUR OUTPUT VALUE WILL BE (I think)



/* 
* 
* 
* This Class is used to create a Blueprint Callable version of our Dispath Function, and call Dispatch:
* 
* s
*/
UCLASS() // Change the _API to match your project
class MYSHADERS_API UMySimpleComputeShaderLibrary_AsyncExecution : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:

	// Instantiate an object that can be used to "broadcast" whatever value(s) we want from our shader to be seen from the editor (I think):
	UPROPERTY(BlueprintAssignable)
		FOnMySimpleComputeShaderLibrary_AsyncExecutionCompleted Completed;


	// SHADER INPUTS:
	int Arg1;
	int Arg2;

	int mapChunkSize;



	virtual void Activate() override 
	{
		// Create a dispatch parameters struct and fill the input array with our args
		FMySimpleComputeShaderDispatchParams Params(1, 1, 1);
		Params.Input[0] = Arg1;
		Params.Input[1] = Arg2;

		// Dispatch the compute shader and wait until it completes
		FMySimpleComputeShaderInterface::Dispatch(Params, [this](int OutputVal)
												  {
												       this->Completed.Broadcast(OutputVal);
												  });
	}



	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", Category = "ComputeShader", WorldContext = "WorldContextObject"))
	static UMySimpleComputeShaderLibrary_AsyncExecution* ExecuteBaseComputeShader(UObject* WorldContextObject, int Arg1, int Arg2, int mapChunkSize) 
	{
		UMySimpleComputeShaderLibrary_AsyncExecution* Action = NewObject<UMySimpleComputeShaderLibrary_AsyncExecution>();
		Action->Arg1 = Arg1;
		Action->Arg2 = Arg2;
		Action->RegisterWithGameInstance(WorldContextObject); // Function inherited from `UBlueprintAsyncActionBase`

		return Action;
	}

};