
#include "MyFirstComputeShader_TEST.h"

int UMyFirstComputeShader_TEST::CALL_TEST_COMPUTE_SHADER(int a, int b)
{
    // Params struct used to pass args to our compute shader
    FMySimpleComputeShaderDispatchParams Params(1, 1, 1);

    // Fill in your input parameters here
    Params.Input[0] = a;
    Params.Input[1] = b;

    //int OutputVal = 0;


    UE_LOG(LogTemp, Warning, TEXT("CALLING Dispatch"));
    
    //// Executes the compute shader and calls the TFunction when complete.
    //FMySimpleComputeShaderInterface::Dispatch(Params, [this](int OutputVal) {
    //    // OutputVal == 10
    //    // Called when the results are back from the GPU.
    //    OutputTmp = OutputVal;
    //    });

    //UE_LOG(LogTemp, Warning, TEXT("OutputTmp WITHIN Dispatch == %d"), OutputTmp);
    ////return OutputVal;
    //return OutputTmp;




    // Executes the compute shader and calls the TFunction when complete.
    FMySimpleComputeShaderInterface::Dispatch(Params, [this](int OutputVal) {
        //	// Called when the results are back from the GPU.
            OutputTmp = OutputVal;
            GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, FString::Printf(TEXT("**TEST_1**   OutputVal= %d"), OutputTmp)); // Returns 10
        });

    GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, FString::Printf(TEXT("**TEST_2**   OutputTmp= %d"), OutputTmp)); // Returns 10, maybe not on the first try

    return OutputTmp;
}
