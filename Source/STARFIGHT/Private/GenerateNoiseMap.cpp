// Fill out your copyright notice in the Description page of Project Settings.


#include "GenerateNoiseMap.h"


TArray<FArray2D> UGenerateNoiseMap::GenerateNoiseMap(int32& mapChunkSize, int32& seed, FVector2D& offset, float& noiseScale, int& octaves, float& persistance, float& lacurnarity) {

	TArray<FArray2D> noiseMap;

	// Initialize our noiseMap first with an array of 'FArray2D' structs, then initialize the 'secondArray' of each 'FArray2D' structs with a list of '0.0f'
	noiseMap.Init(FArray2D(), mapChunkSize);
	for (auto& nestedStruct : noiseMap) { nestedStruct.secondArray.Init(0.0f, mapChunkSize); }
	

	TArray<FVector2D> octaveOffsets;

	
	FRandomStream prng = FRandomStream( seed );
	octaveOffsets.Init(FVector2D(), octaves);
	for (int32 i = 0; i < octaves; i++) {
		float offsetX = prng.FRandRange(-100000, 10000) + offset.X;
		float offsetY = prng.FRandRange(-100000, 10000) + offset.Y;
		octaveOffsets[i] = FVector2D(offsetX, offsetY);
	}


	if (noiseScale <= 0) {
		noiseScale = 0.0001f;
	}

	float maxNoiseHeight = 0;
	float minNoiseHeight = 1;

	float halfWidth = mapChunkSize / 2.0f;
	float halfHeight = mapChunkSize / 2.0f;

	for (int y = 0; y < mapChunkSize; y++) {
		for (int x = 0; x < mapChunkSize; x++) {

			float amplitude = 1;
			float frequency = 1;
			float noiseHeight = 0;

			for (int i = 0; i < octaves; i++) {
				float sampleX = (x - halfWidth) / noiseScale * frequency + octaveOffsets[i].X;
				float sampleY = (y - halfHeight) / noiseScale * frequency + octaveOffsets[i].Y;

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



TArray<FArray2D> UGenerateNoiseMap::GenerateFalloffMap(int32& mapChunkSize, float& a, float& b, float& c) {

	TArray<FArray2D> falloffMap;
	
	// Initialize our falloffMap first with an array of 'FArray2D' structs, then initialize the 'secondArray' of each 'FArray2D' structs with a list of '0.0f'
	falloffMap.Init(FArray2D(), mapChunkSize);
	for (auto& nestedStruct : falloffMap) { nestedStruct.secondArray.Init(0.0f, mapChunkSize); }

	for (int32 i = 0; i < mapChunkSize; i++) {
		for (int32 j = 0; j < mapChunkSize; j++) {

			// x & y are subdivided into x/chunkSize fractions WITH a midpoint thats halfway through the size of the chunkSize!
			float x = i / (float)mapChunkSize * 2 - 1;
			float y = j / (float)mapChunkSize * 2 - 1;

			// Chooses the greatest coordinate (x OR y) which will denote which one is closer to the center of the terrain grid:
			//float value = FMath::Max(FMath::Abs(x), FMath::Abs(y));
			float value = FMath::Sqrt(FMath::Pow(x, 2) + FMath::Pow(y, 2));

			falloffMap[i].secondArray[j] = calculateFalloff(value, a, b, c);
		}
	}
	//UE_LOG(LogTemp, Warning, TEXT("\n\nWORKING TILL HERE 3\n"));
	return falloffMap;
}



FGeneratedMeshData UGenerateNoiseMap::GenerateProceduralMeshData(int32 mapChunkSize, int32 seed, FVector2D offset, int32 levelOfDetail, 
																float noiseScale, int octaves, float persistance, float lacunarity, float heightMultiplier, 
																float weightCurveExponent, float a, float b, float c) {
	
	FGeneratedMeshData meshData;
	
	TArray<FArray2D> noiseMap = UGenerateNoiseMap::GenerateNoiseMap(mapChunkSize, seed, offset, noiseScale, octaves, persistance, lacunarity);
	TArray<FArray2D> falloffMap = UGenerateNoiseMap::GenerateFalloffMap(mapChunkSize, a, b, c);


	// Sets the starting 
	float topLeftX = (mapChunkSize - 1) / -2.f;
	float topLeftZ = (mapChunkSize - 1) / 2.f;

	// Calculates the increment for mesh LODs (ensures 'levelOfDetail' is NOT 0):
	int32 LODincrement = levelOfDetail == 0 ? 1 : levelOfDetail * 2;
	// Calculates correct number of vertices for our 'vertices' array:
	int32 verticesPerLine = (mapChunkSize - 1) / LODincrement + 1;

	int32 vertexIndex = 0;


	for (int32 y = 0; y < mapChunkSize; y += LODincrement) {
		for (int32 x = 0; x < mapChunkSize; x += LODincrement) {

			// Implement our Falloff Map:
			noiseMap[y].secondArray[x] = FMath::Clamp(noiseMap[y].secondArray[x] - falloffMap[y].secondArray[x], 0.0f, 1.0f);

			// Define the height for the current vertex in our iteration:
			float currentHeight = calculateWeightCurve(noiseMap[y].secondArray[x], weightCurveExponent) * heightMultiplier;
			 
			// Add each vertex, uv, & vertexColor:
			meshData.vertices.Add(FVector(topLeftX + x, topLeftZ - y, currentHeight)); // SETS [X, Y, Z] FOR EACH VERTEX
			//meshData.uvs.Add(FVector2D(x / (float)mapChunkSize, y / (float)mapChunkSize));
			meshData.uvs.Add(FVector2D(x, y));
			meshData.vertexColorsNEW.Add(FColor(0.50, 0.75, 1.00, 1.0));

			// Check if we are still one vertex away from the bottom and right boundaries:
			if (x < mapChunkSize - 1 && y < mapChunkSize - 1) {
				// Add Triangles (two per square tile):
				meshData.AddTriangle(vertexIndex, vertexIndex + verticesPerLine + 1, vertexIndex + verticesPerLine);
				meshData.AddTriangle(vertexIndex + verticesPerLine + 1, vertexIndex, vertexIndex + 1);

				// Add Normals:
				FVector v1 = FVector(1.0f, 0.0f, noiseMap[y].secondArray[x + 1] - noiseMap[y].secondArray[x]);  // (x + 1 - x, y - y, next height - current height)
				FVector v2 = FVector(0.0f, 1.0f, noiseMap[y + 1].secondArray[x] - noiseMap[y].secondArray[x]);  // (x - x, y + 1 - y, next height - current height)
				FVector newNormal = FVector::CrossProduct(v2, v1);
				meshData.normals.Add(newNormal);

				float magnitude = (FMath::Sqrt((v2[0] + v1[0]) / 2 + (v2[1] + v1[1]) / 2 + (v2[2] + v1[2]) / 2));
				//UE_LOG(LogTemp, Warning, TEXT("magnitude = %d   :   NEW TANGENT  =  (%d, %d, %d)"), magnitude, (v2[0] + v1[0]) / (2), (v2[1] + v1[1]) / (2), (v2[2] + v1[2]) / (2 * magnitude) );
				

				// Add Tangents:
				meshData.tangents.Add(FProcMeshTangent( (v2[0] + v1[0]) / (2), (v2[1] + v1[1]) / (2), (v2[2] + v1[2]) / (2 * magnitude) ));

			}

			vertexIndex++;
		}
	}

	return meshData;
}



float UGenerateNoiseMap::calculateWeightCurve(float vertexHeight, float exponent) {
	float output = pow(vertexHeight, exponent);
	return output;
}



float UGenerateNoiseMap::InverseLerp(float min, float max, float value) {
	float output = (value - min) / (max - min);
	return output;
}



float UGenerateNoiseMap::calculateFalloff(float value, float a, float b, float c) {
	/*float a = 3.0f;
	float b = 2.2f;*/
	float output = (FMath::Pow(value, a) / (c * FMath::Pow(value, a) + FMath::Pow(b - b * value, a)) );
	return output;
}
