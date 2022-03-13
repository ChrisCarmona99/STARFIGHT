// Fill out your copyright notice in the Description page of Project Settings.


#include "GenerateNoiseMap.h"


FVector2D UGenerateNoiseMap::GenerateNoiseMap(int mapWidth, int mapHeight, float scale, ALandscape *GeneratedLandscape) {
	//UTexture2D* heightMap = GeneratedLandscape->LandscapeComponents[0]->HeightmapTexture;

	
	FVector2D perlinNoiseInput;

	TArray<FVector2D> noiseMap;
	noiseMap.Init(FVector2D(), mapWidth * mapHeight);

	TArray<FArray_2D> noiseMap2;
	

	if (scale <= 0) {
		scale = 0.0001f; // append the 'f' to the end to ensure this value is of type 'float' instead of 'double'
	}
	
	for (int y = 0; y < mapHeight; y++) {
		noiseMap2.Add(FArray_2D());
		for (int x = 0; x < mapWidth; x++) {

			float sampleX = x / scale;
			float sampleY = y / scale;

			perlinNoiseInput.Set(sampleX, sampleY);
			float perlinValue = FMath::PerlinNoise2D(perlinNoiseInput);

			/*noiseMap2[x].
				secondArray[y] = perlinValue;*/

			noiseMap2[x].Add(perlinValue);
		}
	}

	return FVector2D();
}
