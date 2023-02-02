
#include "MyFirstComputeShader_TEST.h"

int UMyFirstComputeShader_TEST::CALL_TEST_COMPUTE_SHADER(int a, int b)
{
    // Params struct used to pass args to our compute shader
    FMySimpleComputeShaderDispatchParams Params(1, 1, 1);

    // Fill in your input parameters here
    Params.Input[0] = a;
    Params.Input[1] = b;

    int OutputVal = 0;
    // Executes the compute shader and calls the TFunction when complete.
    FMySimpleComputeShaderInterface::Dispatch(Params, [](int OutputVal) {
        // OutputVal == 10
        // Called when the results are back from the GPU.
        });

    return OutputVal;
}
