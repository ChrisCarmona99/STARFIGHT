// Fill out your copyright notice in the Description page of Project Settings.


#include "ProceduralGeneration.h"
#include <thread>
#include <mutex>
#include "HAL/ThreadManager.h"
#include "../../../../../../../../Program Files/Epic Games/UE_5.1/Engine/Source/Runtime/RHI/Public/RHIGPUReadback.h"
//#include "RHIGPUReadback.h"



ProceduralGeneration::ProceduralGeneration()
{
}

ProceduralGeneration::~ProceduralGeneration()
{
}

//struct vector2D
//{
//	vector2D(int32 x, int32 y) : X(x), Y(y) {}
//	int32 X;
//	int32 Y;
//};
//
//struct vector3D
//{
//	vector3D(int32 x, int32 y, int32 z) : X(x), Y(y), Z(z) {}
//	int32 X;
//	int32 Y;
//	int32 Z;
//};

void ProceduralGeneration::GenerateNoiseMap(float*& noiseMap, const int32& mapChunkSize, int32& seed, FVector2D& offset, float& noiseScale, int& octaveCount, float& persistance, float& lacurnarity, float& heightMultiplier, float& weightCurveExponent)
{
	uint32 ThreadId = FPlatformTLS::GetCurrentThreadId();
	const FString ThreadName = FThreadManager::Get().GetThreadName(ThreadId);

	UE_LOG(LogTemp, Warning, TEXT(""));
	UE_LOG(LogTemp, Warning, TEXT("| %s | 6: GenerateNoiseMap CALLED"), *ThreadName);


	FVector2f* octaveOffsets = new FVector2f[octaveCount];

	FRandomStream prng = FRandomStream(seed);
	for (int32 i = 0; i < octaveCount; i++)
	{
		// NOTE: Be careful how large the lower and upper bounds are set here... making the number too large causes floating point rounding issues when calculating `sampleX` and `sampleY` later on
		float offsetX = prng.FRandRange(-10000, 10000) + offset.X;
		float offsetY = prng.FRandRange(-10000, 10000) + offset.Y;
		octaveOffsets[i] = FVector2f(offsetX, offsetY);
	}

	// Define our Compute Shader's input parameters:
	int THREADS_X = 1024;
	int THREADS_Y = 1;
	int THREADS_Z = 1;
	int THREAD_GROUPS_X = FMath::DivideAndRoundUp(mapChunkSize * mapChunkSize, THREADS_X);

	FNoiseMapComputeShaderDispatchParams Params(THREADS_X, THREADS_Y, THREADS_Z);
	Params.THREAD_GROUPS_X = THREAD_GROUPS_X;

	Params.mapChunkSize[0] = mapChunkSize;
	Params.noiseScale[0] = noiseScale;
	Params.octaveCount[0] = octaveCount;
	Params.persistance[0] = persistance;
	Params.lacurnarity[0] = lacurnarity;
	Params.heightMultiplier[0] = heightMultiplier;
	Params.weightCurveExponent[0] = weightCurveExponent;
	Params.octaveOffsets = octaveOffsets;

	Params.noiseMap = noiseMap;

	
	std::mutex ComputeShaderMutex;

	// Create a completion event to be signaled when the Compute Shader has completed:
	FEvent* NoiseMapCompletionEvent = FPlatformProcess::GetSynchEventFromPool(true);

	UE_LOG(LogTemp, Warning, TEXT("| %s | 7: Launching ENQUEUE_RENDER_COMMAND thread..."), *ThreadName);
	if (!IsInRenderingThread())
	{
		UE_LOG(LogTemp, Warning, TEXT("|    |	 : NOT IN RENDERING THREAD... WE GOOD!"), *ThreadName);
		// Enqueue a render command to call MyFunction on the render thread
		ENQUEUE_RENDER_COMMAND(MyRenderCommand)(
			[&Params, noiseMap, &ComputeShaderMutex, NoiseMapCompletionEvent](FRHICommandListImmediate& RHICmdList) mutable
			{
				FNoiseMapComputeShaderInterface::ExecuteNoiseMapComputeShader(
					GetImmediateCommandList_ForRenderCommand(),
					Params,
					noiseMap,
					ComputeShaderMutex,
					NoiseMapCompletionEvent);
			});

		UE_LOG(LogTemp, Warning, TEXT("| %s | 7.5: NoiseMapCompletionEvent->Wait() CALLED"), *ThreadName);
		NoiseMapCompletionEvent->Wait();
		UE_LOG(LogTemp, Warning, TEXT("| %s | 7.6: NoiseMapCompletionEvent->Wait() COMPLETED"), *ThreadName);
		FGenericPlatformProcess::FlushPoolSyncEvents();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("|    |	 : IN RENDERING THREAD WHEN WE SHOULDN'T BE..."), *ThreadName);
	}

	delete[] octaveOffsets;
	UE_LOG(LogTemp, Warning, TEXT("| %s | 14: ** Dispatch DONE **  | NoiseMapComputeShader DONE"), *ThreadName);
}


void ProceduralGeneration::ApplyFalloffMap(float*& noiseMap, const int32& mapChunkSize, float& a, float& b, float& c)
{
	uint32 ThreadId = FPlatformTLS::GetCurrentThreadId();
	const FString ThreadName = FThreadManager::Get().GetThreadName(ThreadId);

	UE_LOG(LogTemp, Warning, TEXT(""));
	UE_LOG(LogTemp, Warning, TEXT("| %s | 15: ApplyFalloffMap CALLED"), *ThreadName);

	int printIncrement = FMath::DivideAndRoundUp(mapChunkSize * mapChunkSize, 50);
	int printIndex = printIncrement;
	for (int i = 0; i < mapChunkSize * mapChunkSize; i++)
	{
		int xi = i % mapChunkSize;
		int yi = std::floor(i / mapChunkSize);

		// NOTE: revisit... don't understand:
		float x = xi / (float)mapChunkSize * 2 - 1;
		float y = yi / (float)mapChunkSize * 2 - 1;

		// Chooses the greatest coordinate (x OR y) which will denote which one is closer to the center of the terrain grid:
		//float value = FMath::Max(FMath::Abs(x), FMath::Abs(y));
		float value = FMath::Sqrt(FMath::Pow(x, 2) + FMath::Pow(y, 2));

		*(noiseMap + i) = FMath::Clamp(*(noiseMap + i) - calculateFalloff(value, a, b, c), 0.0f, 1.0f);

		if (i == printIndex) {
			UE_LOG(LogTemp, Warning, TEXT("        index: %d   |   "), i);
			printIndex += printIncrement;
		}
	}
}


void ProceduralGeneration::ApplyErosionMap(
	float*& noiseMap, const int32& seed, const int32& mapChunkSizeWithBorder, const int32& mapChunkSize, const int32& numErosionIterations, int32& erosionBrushRadius, int32& maxDropletLifetime, 
	float& sedimentCapacityFactor, float& minSedimentCapacity, float& depositSpeed, float& erodeSpeed, float& evaporateSpeed, float& gravity, 
	float& startSpeed, float& startWater, float& inertia)
{
	uint32 ThreadId = FPlatformTLS::GetCurrentThreadId();
	const FString ThreadName = FThreadManager::Get().GetThreadName(ThreadId);

	UE_LOG(LogTemp, Warning, TEXT(""));
	UE_LOG(LogTemp, Warning, TEXT("| %s | 16: ApplyErosionMap CALLED"), *ThreadName);


	float* _DEBUG_1 = new float[numErosionIterations];


	// Create brush:
	std::vector<int> brushIndexOffsets;
	std::vector<float> brushWeights;

	float weightSum = 0;
	for (int brushY = -erosionBrushRadius; brushY <= erosionBrushRadius; brushY++) {
		for (int brushX = -erosionBrushRadius; brushX <= erosionBrushRadius; brushX++) {
			float sqrDst = brushX * brushX + brushY * brushY;
			if (sqrDst < erosionBrushRadius * erosionBrushRadius) {
				brushIndexOffsets.emplace_back(brushY * mapChunkSize + brushX);
				float brushWeight = 1 - std::sqrt(sqrDst) / erosionBrushRadius;
				weightSum += brushWeight;
				brushWeights.emplace_back(brushWeight);
			}
		}
	}
	for (int i = 0; i < brushWeights.size(); i++) {
		brushWeights[i] /= weightSum;
	}

	// Generate random indices for droplet placement:
	int* randomIndices = new int[numErosionIterations];
	FRandomStream prng = FRandomStream(seed);
	for (int i = 0; i < numErosionIterations; i++) {
		int randomX = prng.FRandRange(erosionBrushRadius, mapChunkSize - erosionBrushRadius);  // I think this is the problem... should be [erosionBrushRadius, mapChunkSize - erosionBrushRadius
		int randomY = prng.FRandRange(erosionBrushRadius, mapChunkSize - erosionBrushRadius);
		randomIndices[i] = randomY * mapChunkSize + randomX;
	}

	const int BrushIndexOffsetsSize = brushIndexOffsets.size();
	const int BrushWeightsSize = brushWeights.size();

	int* BrushIndexOffsets = new int[BrushIndexOffsetsSize];
	std::memcpy(BrushIndexOffsets, brushIndexOffsets.data(), BrushIndexOffsetsSize * sizeof(int));
	float* BrushWeights = new float[BrushWeightsSize];
	std::memcpy(BrushWeights, brushWeights.data(), BrushWeightsSize * sizeof(float));


	// Define our Compute Shader's input parameters:
	int THREADS_X = 1024;
	int THREADS_Y = 1;
	int THREADS_Z = 1;
	int THREAD_GROUPS_X = FMath::DivideAndRoundUp(numErosionIterations, THREADS_X);

	FErosionMapCSDispatchParams Params(THREADS_X, THREADS_Y, THREADS_Z);
	Params.THREAD_GROUPS_X = THREAD_GROUPS_X;

	Params._RandomIndices = randomIndices;
	Params._BrushIndices = BrushIndexOffsets;
	Params._BrushWeights = BrushWeights;

	Params._MapChunkSize[0] = mapChunkSizeWithBorder;  // NOTE: NOT the original mapChunkSize... mapChunksize + (erosionBrushRadius * 2)
	Params._BrushLength[0] = brushIndexOffsets.size();
	Params._BorderSize[0] = erosionBrushRadius;
	
	Params._MaxDropletLifetime[0] = maxDropletLifetime;
	Params._SedimentCapacityFactor[0] = sedimentCapacityFactor;
	Params._MinSedimentCapacity[0] = minSedimentCapacity;
	Params._DepositSpeed[0] = depositSpeed;
	Params._ErodeSpeed[0] = erodeSpeed;

	Params._EvaporateSpeed[0] = evaporateSpeed;
	Params._Gravity[0] = gravity;
	Params._StartSpeed[0] = startSpeed;
	Params._StartWater[0] = startWater;
	Params._Inertia[0] = inertia;

	Params._NumErosionIterations[0] = numErosionIterations;

	Params._NoiseMap = noiseMap;

	Params._DEBUG_1 = _DEBUG_1;

	// Create a completion event to be signaled when the Compute Shader has completed:
	FEvent* ErosionMapCompletionEvent = FPlatformProcess::GetSynchEventFromPool(false);

	UE_LOG(LogTemp, Warning, TEXT("| %s | 7: Launching ENQUEUE_RENDER_COMMAND thread..."), *ThreadName);
	if (!IsInRenderingThread())
	{
		UE_LOG(LogTemp, Warning, TEXT("|    |	 : NOT IN RENDERING THREAD... WE GOOD!"), *ThreadName);
		// Enqueue a render command to call MyFunction on the render thread
		ENQUEUE_RENDER_COMMAND(MyRenderCommand)(
			[&Params, noiseMap, _DEBUG_1, BrushIndexOffsetsSize, BrushWeightsSize, ErosionMapCompletionEvent](FRHICommandListImmediate& RHICmdList) mutable
			{
				FErosionMapCSInterface::ExecuteErosionMapCS(
					GetImmediateCommandList_ForRenderCommand(),
					Params,
					BrushIndexOffsetsSize,
					BrushWeightsSize,
					noiseMap,
					_DEBUG_1,
					ErosionMapCompletionEvent);
			});

		UE_LOG(LogTemp, Warning, TEXT("| %s | 7.5: ErosionMapCompletionEvent->Wait() CALLED"), *ThreadName);
		ErosionMapCompletionEvent->Wait();
		UE_LOG(LogTemp, Warning, TEXT("| %s | 7.6: ErosionMapCompletionEvent->Wait() COMPLETED"), *ThreadName);
		FGenericPlatformProcess::FlushPoolSyncEvents();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("|    |	 : IN RENDERING THREAD WHEN WE SHOULDN'T BE..."), *ThreadName);
	}

	int upperLimit = numErosionIterations < 500 ? numErosionIterations : 500;
	for (int index = 0; index < upperLimit; index++)
	{
		UE_LOG(LogTemp, Warning, TEXT("| %s | #:#		DEBUG[%d] == %f   |   (x,y) == (%d,%d)"), *ThreadName, index, _DEBUG_1[index], (int32) _DEBUG_1[index] % mapChunkSize, (int32) _DEBUG_1[index] / mapChunkSize)
	}

	// Cleanup pointers:
	delete[] randomIndices;
	delete[] BrushIndexOffsets;
	delete[] BrushWeights;

	delete ErosionMapCompletionEvent;

	UE_LOG(LogTemp, Warning, TEXT("| %s | 14: ** Dispatch DONE **  | NormalsAndTangentsCS DONE"), *ThreadName);
}





//float ProceduralGeneration::calculateWeightCurve(float vertexHeight, float exponent) {
//	float output = pow(vertexHeight, exponent);
//	return output;
//}
//
//float ProceduralGeneration::InverseLerp(float min, float max, float value) {
//	float output = (value - min) / (max - min);
//	return output;
//}

float ProceduralGeneration::calculateFalloff(float value, float a, float b, float c) {
	float output = (FMath::Pow(value, a) / (c * FMath::Pow(value, a) + FMath::Pow((b - b * value), a)));
	return output;
}


auto printVector = [](std::vector<int32> vect) {std::string out = ""; for (int32 elem : vect) { out += "'"; out += elem; out += "'"; out += ", "; } };