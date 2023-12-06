#pragma once


#include "CoreMinimal.h"

#include "GenericPlatform/GenericPlatformMisc.h"
#include "Kismet/BlueprintAsyncActionBase.h"

#include <thread>
#include <mutex>
#include <chrono>

#include "ErosionMapCS.generated.h"


// Temporary struct to create the .generated.h file:
USTRUCT(BlueprintType)
struct FErosionCSTempStruct
{
	GENERATED_BODY()
};


struct SHADERMODULE_API FErosionMapCSDispatchParams
{
	FErosionMapCSDispatchParams(int x, int y, int z)
		: X(x), Y(y), Z(z) {}

	int X;
	int Y;
	int Z;
	int THREAD_GROUPS_X;

	// Input:
	int* _RandomIndices;
	int* _BrushIndices;
	float* _BrushWeights;

	int _MapChunkSize[1];
	int _BrushLength[1];
	int _BorderSize[1];

	int _MaxDropletLifetime[1];
	float _Inertia[1];
	float _SedimentCapacityFactor[1];
	float _MinSedimentCapacity[1];
	float _DepositSpeed[1];
	float _ErodeSpeed[1];

	float _EvaporateSpeed[1];
	float _Gravity[1];
	float _StartSpeed[1];
	float _StartWater[1];

	int _NumErosionIterations[1];

	// Output:
	float* _NoiseMap;

	float* _DEBUG_1;


};



// This is a public interface that we define so outside code can invoke our compute shader:
class SHADERMODULE_API FErosionMapCSInterface
{

public:

	// Executes this shader on the render thread:
	static void ExecuteErosionMapCS(
		FRHICommandListImmediate& RHICmdList,
		FErosionMapCSDispatchParams Params,
		int BrushIndexOffsetsSize,
		int BrushWeightsSize,
		float*& noiseMap,
		float*& _DEBUG_1,
		FEvent* ErosionMapCompletionEvent);

};

