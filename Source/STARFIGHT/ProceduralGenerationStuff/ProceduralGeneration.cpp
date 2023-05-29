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

struct vector2D
{
	vector2D(int32 x, int32 y) : X(x), Y(y) {}
	int32 X;
	int32 Y;
};

struct vector3D
{
	vector3D(int32 x, int32 y, int32 z) : X(x), Y(y), Z(z) {}
	int32 X;
	int32 Y;
	int32 Z;
};

void ProceduralGeneration::GenerateNoiseMap(float*& noiseMap, const int32& mapChunkSize, int32& seed, FVector2D& offset, float& noiseScale, int& octaveCount, float& persistance, float& lacurnarity)
{
	uint32 ThreadId = FPlatformTLS::GetCurrentThreadId();
	const FString ThreadName = FThreadManager::Get().GetThreadName(ThreadId);

	UE_LOG(LogTemp, Warning, TEXT(""));
	UE_LOG(LogTemp, Warning, TEXT("| %s | 6: GenerateNoiseMap CALLED"), *ThreadName);


	//std::shared_ptr<FVector2f[]> octaveOffsets(new FVector2f[octaveCount]);
	//FVector2f* octaveOffsets_ptr = octaveOffsets.get();
	FVector2f* octaveOffsets = new FVector2f[octaveCount];

	FRandomStream prng = FRandomStream(seed);
	for (int32 i = 0; i < octaveCount; i++)
	{
		float offsetX = prng.FRandRange(-1000000, 1000000) + offset.X;
		float offsetY = prng.FRandRange(-1000000, 1000000) + offset.Y;
		octaveOffsets[i] = FVector2f(offsetX, offsetY);
	}

	/*if (noiseScale <= 0) {
		noiseScale = 0.0001f;
	}*/

	int THREADS_X = 1024; // This is hardcoded for now, but just the number of threads set in the compute shader in the x dimension
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
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("|    |	 : IN RENDERING THREAD WHEN WE SHOULDN'T BE..."), *ThreadName);
	}

	UE_LOG(LogTemp, Warning, TEXT("| %s | 7.5: NoiseMapCompletionEvent->Wait() CALLED"), *ThreadName);
	NoiseMapCompletionEvent->Wait();
	UE_LOG(LogTemp, Warning, TEXT("| %s | 7.6: NoiseMapCompletionEvent->Wait() COMPLETED"), *ThreadName);

	delete[] octaveOffsets;
	UE_LOG(LogTemp, Warning, TEXT("| %s | 14: ** Dispatch DONE **  | NoiseMapComputeShader DONE"), *ThreadName);
}


void ProceduralGeneration::ApplyFalloffMap(float*& noiseMap, const int32& mapChunkSize, float& a, float& b, float& c)
{
	uint32 ThreadId = FPlatformTLS::GetCurrentThreadId();
	const FString ThreadName = FThreadManager::Get().GetThreadName(ThreadId);

	UE_LOG(LogTemp, Warning, TEXT(""));
	UE_LOG(LogTemp, Warning, TEXT("| %s | 15: ApplyFalloffMap CALLED"), *ThreadName);

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
	}
}


void ProceduralGeneration::ApplyErosionMap(float*& noiseMap, const int32& mapChunkSize, const int32& seed, int32& dropletLifetime, const int32& numIterations)
{
	uint32 ThreadId = FPlatformTLS::GetCurrentThreadId();
	const FString ThreadName = FThreadManager::Get().GetThreadName(ThreadId);

	UE_LOG(LogTemp, Warning, TEXT(""));
	UE_LOG(LogTemp, Warning, TEXT("| %s | 16: ApplyErosionMap CALLED"), *ThreadName);

	FRandomStream prng = FRandomStream(seed);

	for (int iteration = 0; iteration < numIterations; iteration++)
	{
		// Create Water Droplet at a random point on the map:
		float x = prng.FRandRange(0, mapChunkSize - 1);
		float y = prng.FRandRange(0, mapChunkSize - 1);
		//TArray<FArray2D> direction; // two dimensional normalized float vector 
		float velocity = 0;
		float water = 0;
		float sediment = 0;

		for (int lifetime = 0; lifetime < dropletLifetime; lifetime++)
		{
			// Convert the generated 'x' and 'y' coordinates to the corresponding index
			//int x = i % mapChunkSize;
			//int y = std::floor(i / mapChunkSize);

			// Calculate droplet's height and the direction of the flow with bilinear interpolation of surrounding heights
			//float height = GeneratedMap[y].secondArray[x];



			// Update the droplet's position (move 1 unit regardless of speed so as not to skip over sections fo the map)

			// Find the droplet's new height and calculate the 'deltaHeight'

			// Calculate the droplet's sediment capacity (higher when moving fast down a slop and contains lots of water)

			// - If carrying more sediment than capacity, or if flowing up a slope:
			// deposite a fraction of the sediment to the surrounding nodes (with bilinear interpolation)

			// - Otherwise:
			// Erode a fraction of the droplet's remaining capacity from teh soil, distributed over the radius of the droplets
			// NOTE: don't erode more than deltaHeight to avoid digging holes behind the droplet and creating spikes

			// Update droplet's speed based on deltaHeight
			// Evaporate a fraction of the droplet's water

		}
	}
}

/*
*

void ProceduralGeneration::GenerateNoiseMap_OLD(std::vector<float>& noiseMap, const int32& mapChunkSize, int32& seed, FVector2D& offset, float& noiseScale, int& octaveCount, float& persistance, float& lacurnarity)
{
	UE_LOG(LogTemp, Warning, TEXT("GenerateNoiseMap CALLED"));


	std::vector<vector2D> octaveOffsets_OLD;
	std::shared_ptr<FVector2f[]> octaveOffsets(new FVector2f[octaveCount]);
	FVector2f* octaveOffsets_ptr = octaveOffsets.get();

	FRandomStream prng = FRandomStream(seed);
	for (int32 i = 0; i < octaveCount; i++)
	{
		float offsetX = prng.FRandRange(-1000000, 1000000) + offset.X;
		float offsetY = prng.FRandRange(-1000000, 1000000) + offset.Y;
		octaveOffsets_OLD.push_back(vector2D(offsetX, offsetY));
		octaveOffsets_ptr[i] = FVector2f(offsetX, offsetY);
	}

	if (noiseScale <= 0) {
		noiseScale = 0.0001f;
	}



	int THREADS_X = 1024; // This is hardcoded for now, but just the number of threads set in the compute shader in the x dimension
	int THREAD_GROUPS_X = FMath::DivideAndRoundUp(mapChunkSize * mapChunkSize, THREADS_X);

	FNoiseMapComputeShaderDispatchParams Params(THREAD_GROUPS_X, 1, 1);
	Params.mapChunkSize[0] = mapChunkSize;
	Params.noiseScale[0] = noiseScale;
	Params.octaveCount[0] = octaveCount;
	Params.persistance[0] = persistance;
	Params.lacurnarity[0] = lacurnarity;
	Params.octaveOffsets = octaveOffsets;

	std::shared_ptr<float[]> _noiseMap(new float[mapChunkSize * mapChunkSize]);
	//std::shared_ptr<float[]> _noiseMap;
	Params.noiseMap = _noiseMap;

	std::shared_ptr<float[]> outputMap;
	//FNoiseMapComputeShaderInterface::Dispatch(Params, [_noiseMap, outputMap](std::shared_ptr<float[]> OUTPUT) mutable {  // NOTE: Making the input mutable could fuck things up... double check this
	//	//_noiseMap = OUTPUT;
	//	//std::shared_ptr<float[]> TEST(new float[10]);
	//	//_noiseMap = TEST;
	//	outputMap = OUTPUT;
	//	});

	//std::this_thread::sleep_for(std::chrono::seconds(1));



	// MAKE HLSL:
	float maxNoiseHeight = 0;
	float minNoiseHeight = 1;

	float halfWidth = mapChunkSize / 2.0f;
	float halfHeight = mapChunkSize / 2.0f;

	for (int i1 = 0; i1 < mapChunkSize * mapChunkSize; i1++)
	{
		int x = i1 % mapChunkSize;
		int y = std::floor(i1 / mapChunkSize);

		float amplitude = 1;
		float frequency = 1;
		float noiseHeight = 0;

		// Apply Octaves
		for (int i2 = 0; i2 < octaveCount; i2++)
		{
			float sampleX = (x - halfWidth) / noiseScale * frequency + octaveOffsets_OLD[i2].X;
			float sampleY = (y - halfHeight) / noiseScale * frequency + octaveOffsets_OLD[i2].Y;

			float perlinValue = FMath::PerlinNoise2D(FVector2D(sampleX, sampleY)) * 2 - 1; // we multiply by '2' then subtract '1' so that our values can be negative!!
			noiseHeight += perlinValue * amplitude;

			amplitude *= persistance;  // Decreases with each octave
			frequency *= lacurnarity; // multiplied by 'lacurnarity' so the frequency increases each octave (since lacurnarity is always greater than 1)
		}

		// BIG flaw with this code; for the inverse lerp to take full effect, we need to be doing this check after we've filled the map with it's unfiltered perlin values...
		if (noiseHeight > maxNoiseHeight)
		{
			maxNoiseHeight = noiseHeight;
		}
		else if (noiseHeight < minNoiseHeight)
		{
			minNoiseHeight = noiseHeight;
		}

		noiseMap.push_back(noiseHeight);

		// Normalize all of our height values between the high and low height values encountered
		noiseMap[i1] = InverseLerp(minNoiseHeight, maxNoiseHeight, noiseMap[i1]);
		//UE_LOG(LogTemp, Warning, TEXT("		noiseMap[%d] == %d"), i1, noiseMap[i1]);
	}
	int _T_ = 1;
}

void ProceduralGeneration::ApplyFalloffMap_OLD(std::vector<float>& noiseMap, const int32& mapChunkSize, float& a, float& b, float& c)
{

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

		noiseMap[i] = FMath::Clamp(noiseMap[i] - calculateFalloff(value, a, b, c), 0.0f, 1.0f);
	}
}

void ProceduralGeneration::ApplyErosionMap_OLD(std::vector<float>& noiseMap, const int32& mapChunkSize, const int32& seed, int32& dropletLifetime, const int32& numIterations)
{
	FRandomStream prng = FRandomStream(seed);

	for (int iteration = 0; iteration < numIterations; iteration++)
	{
		// Create Water Droplet at a random point on the map:
		float x = prng.FRandRange(0, mapChunkSize - 1);
		float y = prng.FRandRange(0, mapChunkSize - 1);
		//TArray<FArray2D> direction; // two dimensional normalized float vector 
		float velocity = 0;
		float water = 0;
		float sediment = 0;

		for (int lifetime = 0; lifetime < dropletLifetime; lifetime++)
		{
			// Convert the generated 'x' and 'y' coordinates to the corresponding index
			//int x = i % mapChunkSize;
			//int y = std::floor(i / mapChunkSize);

			// Calculate droplet's height and the direction of the flow with bilinear interpolation of surrounding heights
			//float height = GeneratedMap[y].secondArray[x];



			// Update the droplet's position (move 1 unit regardless of speed so as not to skip over sections fo the map)

			// Find the droplet's new height and calculate the 'deltaHeight'

			// Calculate the droplet's sediment capacity (higher when moving fast down a slop and contains lots of water)

			// - If carrying more sediment than capacity, or if flowing up a slope:
			// deposite a fraction of the sediment to the surrounding nodes (with bilinear interpolation)

			// - Otherwise:
			// Erode a fraction of the droplet's remaining capacity from teh soil, distributed over the radius of the droplets
			// NOTE: don't erode more than deltaHeight to avoid digging holes behind the droplet and creating spikes

			// Update droplet's speed based on deltaHeight
			// Evaporate a fraction of the droplet's water

		}
	}
}

*
*/

float ProceduralGeneration::calculateWeightCurve(float vertexHeight, float exponent) {
	float output = pow(vertexHeight, exponent);
	return output;
}

float ProceduralGeneration::InverseLerp(float min, float max, float value) {
	float output = (value - min) / (max - min);
	return output;
}

float ProceduralGeneration::calculateFalloff(float value, float a, float b, float c) {
	float output = (FMath::Pow(value, a) / (c * FMath::Pow(value, a) + FMath::Pow((b - b * value), a)));
	return output;
}


auto printVector = [](std::vector<int32> vect) {std::string out = ""; for (int32 elem : vect) { out += "'"; out += elem; out += "'"; out += ", "; } };