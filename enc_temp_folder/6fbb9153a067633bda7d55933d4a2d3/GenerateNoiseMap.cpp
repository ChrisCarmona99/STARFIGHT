// Fill out your copyright notice in the Description page of Project Settings.


#include "GenerateNoiseMap.h"


TArray<FArray2D> UGenerateNoiseMap::GenerateNoiseMap(int32& mapChunkSize, int32& seed, float& noiseScale, int& octaves, float& persistance, float& lacurnarity) {

	TArray<FArray2D> noiseMap;
	//TArray<float> noiseMap;

	// Initialize our noiseMap first with an array of 'FArray2D' structs, then initialize the 'secondArray' of each 'FArray2D' structs with a list of '0.0f'
	noiseMap.Init(FArray2D(), mapChunkSize);
	for (auto& nestedStruct : noiseMap) { nestedStruct.secondArray.Init(0.0f, mapChunkSize); }
	

	int prng = FMath::RandRange(3, 8);

	if (noiseScale <= 0) {
		noiseScale = 0.0001f;
	}

	float maxNoiseHeight = 0;
	float minNoiseHeight = 1;

	for (int y = 0; y < mapChunkSize; y++) {
		for (int x = 0; x < mapChunkSize; x++) {

			float amplitude = 1;
			float frequency = 1;
			float noiseHeight = 0;

			for (int i = 0; i < octaves; i++) {
				float sampleX = x / noiseScale * frequency;
				float sampleY = y / noiseScale * frequency;

				float perlinValue = FMath::PerlinNoise2D(FVector2D(sampleX, sampleY)) * 2 - 1; // we multiply by '2' then subtract '1' so that our values can be negative!!
				noiseHeight += perlinValue * amplitude;

				amplitude *= persistance;  // Decreases with each octave
				frequency *= lacurnarity; // multiplied by 'lacurnarity' so the frequency increases each octave (since lacurnarity is always greater than 1)
			}
			
			if (noiseHeight > maxNoiseHeight) {
				maxNoiseHeight = noiseHeight;
			} else if (noiseHeight < minNoiseHeight) {
				minNoiseHeight = noiseHeight;
			}

			noiseMap[y].secondArray[x] = noiseHeight; // BRUH NO WAY
			//noiseMap.Add(perlinValue);
		}
	}

	for (int y = 0; y < mapChunkSize; y++) {
		for (int x = 0; x < mapChunkSize; x++) {
			noiseMap[y].secondArray[x] = InverseLerp(minNoiseHeight, maxNoiseHeight, noiseMap[y].secondArray[x]);
			//noiseMap[x + y] = InverseLerp(minNoiseHeight, maxNoiseHeight, noiseMap[x + y]);
		}
	}
	//UE_LOG(LogTemp, Warning, TEXT("\n\nWORKING TILL HERE 2\n"));
	return noiseMap;
}



TArray<FArray2D> UGenerateNoiseMap::GenerateFalloffMap(int32& mapChunkSize, float& a, float& b, float& c, float& d) {

	TArray<FArray2D> falloffMap;
	//TArray<float> falloffMap;

	// Initialize our falloffMap first with an array of 'FArray2D' structs, then initialize the 'secondArray' of each 'FArray2D' structs with a list of '0.0f'
	falloffMap.Init(FArray2D(), mapChunkSize);
	for (auto& nestedStruct : falloffMap) { nestedStruct.secondArray.Init(0.0f, mapChunkSize); }

	for (int32 i = 0; i < mapChunkSize; i++) {
		for (int32 j = 0; j < mapChunkSize; j++) {

			// x & y are subdivided into x/chunkSize fractions WITH a midpoint thats halfway through the size of the chunkSize!
			float x = i / (float)mapChunkSize * 2 - 1;
			float y = j / (float)mapChunkSize * 2 - 1;

			/*UE_LOG(LogTemp, Warning, TEXT("%s"), " ");
			UE_LOG(LogTemp, Warning, TEXT(" i = %d  :  j = %d"), i, j);
			UE_LOG(LogTemp, Warning, TEXT(" X = %f  :  Y = %f"), x, y);*/

			// Chooses the greatest coordinate (x OR y) which will denote which one is closer to the center of the terrain grid:
			float value = FMath::Max(FMath::Abs(x), FMath::Abs(y));

			falloffMap[i].secondArray[j] = calculateFalloff(value, a, b, c, d);
			//falloffMap.Add(calculateFalloff(value));
		}
	}
	//UE_LOG(LogTemp, Warning, TEXT("\n\nWORKING TILL HERE 3\n"));
	return falloffMap;
}



float UGenerateNoiseMap::InverseLerp(float min, float max, float value) {
	float output = (value - min) / (max - min);
	return output;
}



float UGenerateNoiseMap::calculateFalloff(float value, float a, float b, float c, float d) {
	/*float a = 3.0f;
	float b = 2.2f;*/
	float output = (FMath::Pow(value, a) / (c * FMath::Pow(value, a) + FMath::Pow(b - b * value, a))) + d;
	return output;
}



// CURRENTLY NOT BEING USED:
TArray<FArray2D> UGenerateNoiseMap::UpdateNoiseMap(int32 mapChunkSize, float noiseScale) {
	return TArray<FArray2D>();
}
