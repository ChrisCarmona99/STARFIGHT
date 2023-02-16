
#include "MyFirstComputeShader_TEST.h"

int UMyFirstComputeShader_TEST::CALL_TEST_COMPUTE_SHADER(int a, int b, int mapChunkSize)
{

    /*
    * THE FOLLOWING CODE CAN BE CALLED FROM C++
    */


    // Params struct used to pass args to our compute shader
    FMySimpleComputeShaderDispatchParams Params(1, 1, 1);

    Params.Input[0] = a;
    Params.Input[1] = b;

  
    // Executes the compute shader and calls the TFunction (the lambda) when complete.
    FMySimpleComputeShaderInterface::Dispatch(Params, [this](int OutputVal) {
        	// Called when the results are back from the GPU.
            OutputTmp = OutputVal;
        });

    return OutputTmp;
}
