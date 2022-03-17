// Fill out your copyright notice in the Description page of Project Settings.


#include "GenerateNoiseMap.h"


TArray<FArray2D> UGenerateNoiseMap::GenerateNoiseMap(int32 mapWidth, int32 mapHeight, float noiseScale, int octaves, float persistance, float lacurnarity) {

	TArray<FArray2D> noiseMap;

	float perlinValue;

	// Initialize our noiseMap first with an array of 'FArray2D' structs, then initialize the 'secondArray' of each 'FArray2D' structs with a list of '0.0f'
	noiseMap.Init(FArray2D(), mapWidth);
	for (auto& nestedStruct : noiseMap) { nestedStruct.secondArray.Init(0.0f, mapHeight); }
	
	if (noiseScale <= 0) {
		noiseScale = 0.0001f;
	}

	float maxNoiseHeight = 0;
	float minNoiseHeight = 1;

	
	for (int y = 0; y < mapHeight; y++) {
		for (int x = 0; x < mapWidth; x++) {

			float amplitude = 1;
			float frequency = 1;
			float noiseHeight = 0;

			for (int i = 0; i < octaves; i++) {
				float sampleX = x / noiseScale * frequency;
				float sampleY = y / noiseScale * frequency;

				perlinValue = FMath::PerlinNoise2D(FVector2D(sampleX, sampleY)) * 2 - 1; // we multiply by '2' then subtract '1' so that our values can be negative!!
				noiseHeight += perlinValue * amplitude;

				amplitude *= persistance;  // Decreases with each octave
				frequency *= lacurnarity; // multiplied by 'lacurnarity' so the frequency increases each octave (since lacurnarity is always greater than 1)
			}

			if (noiseHeight > maxNoiseHeight) {
				maxNoiseHeight = noiseHeight;
			} else if (noiseHeight < minNoiseHeight) {
				minNoiseHeight = noiseHeight;
			}

			noiseMap[y].secondArray[x] = perlinValue;
		}
	}

	for (int y = 0; y < mapHeight; y++) {
		for (int x = 0; x < mapWidth; x++) {
			noiseMap[y].secondArray[x] = InverseLerp(minNoiseHeight, maxNoiseHeight, noiseMap[y].secondArray[x]);
		}
	}

	return noiseMap;
}


float UGenerateNoiseMap::InverseLerp(float min, float max, float value) {
	float output = (value - min) / (max - min);
	return output;
}


// CURRENTLY NOT BEING USED:
TArray<FArray2D> UGenerateNoiseMap::UpdateNoiseMap(int32 mapWidth, int32 mapHeight, float noiseScale) {
	return TArray<FArray2D>();
}
