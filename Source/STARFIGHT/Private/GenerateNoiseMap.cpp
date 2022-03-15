// Fill out your copyright notice in the Description page of Project Settings.


#include "GenerateNoiseMap.h"


TArray<FArray2D> UGenerateNoiseMap::GenerateNoiseMap(int32 mapWidth, int32 mapHeight, float scale) {

	TArray<FArray2D> noiseMap;

	// Initialize our noiseMap first with an array of 'FArray2D' structs, then initialize the 'secondArray' of each 'FArray2D' structs with a list of '0.0f'
	noiseMap.Init(FArray2D(), mapWidth);
	for (auto& nestedStruct : noiseMap) { 
		nestedStruct.secondArray.Init(0.0f, mapHeight); 
	}
	
	if (scale <= 0) { 
		scale = 0.0001f; 
	}
	
	for (int y = 0; y < mapHeight; y++) {
		for (int x = 0; x < mapWidth; x++) {

			float sampleX = (x * 0.9) / scale;
			float sampleY = (y * 0.9) / scale;

			float perlinValue = FMath::PerlinNoise2D(FVector2D(sampleX, sampleY));

			noiseMap[y].secondArray[x] = perlinValue; 
		}
	}

	return noiseMap;
}


TArray<FArray2D> UGenerateNoiseMap::UpdateNoiseMap(int32 mapWidth, int32 mapHeight, float scale) {
	return TArray<FArray2D>();
}
