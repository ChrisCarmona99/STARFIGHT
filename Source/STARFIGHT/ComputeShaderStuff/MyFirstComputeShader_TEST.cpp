
#include "MyFirstComputeShader_TEST.h"

TArray<float> UMyFirstComputeShader_TEST::CALL_TEST_COMPUTE_SHADER(int mapChunkSize)
{

    /*
    * THE FOLLOWING CODE CAN BE CALLED FROM C++
    */


    // Params struct used to pass args to our compute shader
    FMySimpleComputeShaderDispatchParams Params(1, 1, 1);

    InputParameterReferences InputParamRefs;

  
    // Executes the compute shader and calls the TFunction (the lambda) when complete.
    FMySimpleComputeShaderInterface::Dispatch(Params, InputParamRefs, [this](TArray<float> noiseMap) {
        	// Called when the results are back from the GPU.
        output = noiseMap;
        });

    return output; // returns the first element of the noiseMap
}
