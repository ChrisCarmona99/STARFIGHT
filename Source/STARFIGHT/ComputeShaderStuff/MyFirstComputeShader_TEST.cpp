//#include "MyShaders/Public/MySimpleComputeShader.h"
//
//// Params struct used to pass args to our compute shader
//FMySimpleComputeShaderDispatchParams Params(1, 1, 1);
//
//// Fill in your input parameters here
//Params.Input[0] = 2;
//Params.Input[1] = 5;
//
//// Executes the compute shader and calls the TFunction when complete.
//FMySimpleComputeShaderInterface::Dispatch(Params, [](int OutputVal) {
//    OutputVal == 10;
//    // Called when the results are back from the GPU.
//    });
//
//
//
//
//int OutputTmp = 0;
//// Executes the compute shader and calls the TFunction when complete.
//FMySimpleComputeShaderInterface::Dispatch(Params, [&OutputTmp](int OutputVal) {
//	//	// Called when the results are back from the GPU.
//	OutputTmp = OutputVal;
//	GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, FString::Printf(TEXT("OutputVal= %d"), OutputTmp)); // Returns 10
//	});
//
//GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, FString::Printf(TEXT("OutputTmp= %d"), OutputTmp)); // Returns 0