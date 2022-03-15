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
		scale = 0.0001f; // append the 'f' to the end to ensure this value is of type 'float' instead of 'double'
	}
	
	for (int y = 0; y < mapHeight; y++) {
		for (int x = 0; x < mapWidth; x++) {

			float sampleX = x / scale;
			float sampleY = y / scale;

			//UE_LOG(LogTemp, Warning, TEXT("sampleX: %f"), sampleX);  // DELETE
			//UE_LOG(LogTemp, Warning, TEXT("sampleY: %f"), sampleY);  // DELETE

			float perlinValue = FMath::PerlinNoise2D(FVector2D(sampleX * 0.3, sampleY * 0.3));
			//UE_LOG(LogTemp, Warning, TEXT("perlinValue: %f"), perlinValue);  // DELETE

			noiseMap[y].secondArray[x] = perlinValue; // appends our perlineValue to the 'secondArray' TArray in the FArray2D struct at index 'y'
		}
	}

	return noiseMap;
}
