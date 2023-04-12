
#include "MyFirstComputeShader_TEST.h"

float UMyFirstComputeShader_TEST::CALL_TEST_COMPUTE_SHADER(int mapChunkSize)
{

    /*
    * THE FOLLOWING CODE CAN BE CALLED FROM C++
    */


    // Params struct used to pass args to our compute shader
    FMySimpleComputeShaderDispatchParams Params(1, 1, 1);

  
    // Executes the compute shader and calls the TFunction (the lambda) when complete.
    FMySimpleComputeShaderInterface::Dispatch(Params, [this](float TEMP) {
        	// Called when the results are back from the GPU.
        output = 1.0f;
        });

    return output; // returns the first element of the noiseMap
}
