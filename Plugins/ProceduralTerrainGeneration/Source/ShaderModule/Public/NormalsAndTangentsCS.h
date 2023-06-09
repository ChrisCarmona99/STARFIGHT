#pragma once


#include "CoreMinimal.h"

#include "GenericPlatform/GenericPlatformMisc.h"
#include "Kismet/BlueprintAsyncActionBase.h"

#include <thread>
#include <mutex>
#include <chrono>

#include "NormalsAndTangentsCS.generated.h"


// Temporary struct to create the .generated.h file:
USTRUCT(BlueprintType)
struct FMyNewStruct
{
	GENERATED_BODY()
};


struct SHADERMODULE_API FNormalsAndTangentsCSDispatchParams
{
	FNormalsAndTangentsCSDispatchParams(int x, int y, int z)
		: X(x), Y(y), Z(z) {}

	int X;
	int Y;
	int Z;
	int THREAD_GROUPS_X;


	int32 mapChunkSize[1];
	float* noiseMap;

	float* normals;
	float* tangents;

};



// This is a public interface that we define so outside code can invoke our compute shader:
class SHADERMODULE_API FNormalsAndTangentsCSInterface
{

public:

	// Executes this shader on the render thread:
	static void ExecuteNormalsAndTangentsCS(
		FRHICommandListImmediate& RHICmdList,
		FNormalsAndTangentsCSDispatchParams Params,
		float*& normals,
		float*& tangents,
		FEvent* NormalsAndTangentsCompletionEvent);

};

