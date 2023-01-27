// Fill out your copyright notice in the Description page of Project Settings.


#include "GenerateTerrainBPCallable.h"

FGeneratedMeshData UGenerateTerrainBPCallable::GenerateProceduralMeshDataNEW(const int32 mapChunkSize, int32 seed, FVector2D offset, int32 levelOfDetail, 
																		  float noiseScale, int octaves, float persistance, float lacunarity, float heightMultiplier, 
																		  float weightCurveExponent, float a, float b, float c)
{
	std::vector<float> noiseMap;
	FGeneratedMeshData meshData;

	ProceduralGeneration::GenerateNoiseMap(noiseMap, mapChunkSize, seed, offset, noiseScale, octaves, persistance, lacunarity);
	ProceduralGeneration::ApplyFalloffMap(noiseMap, mapChunkSize, a, b, c);

	// Sets the starting traversal point for all the vertices
	//		In UE5, the x-axis and y-axis are flipped... +x is 'forward' and +y is 'to the right'
	float topMostX = (mapChunkSize - 1) / 2.f; // +x
	float leftMostY = (mapChunkSize - 1) / -2.f; // -y

	// Calculates the increment for mesh LODs (ensures 'levelOfDetail' is NOT 0):
	int32 LODincrement = levelOfDetail == 0 ? 1 : levelOfDetail * 2;
	// Calculates correct number of vertic  es for our 'vertices' array:
	int32 verticesPerLine = ((mapChunkSize - 1) / LODincrement) + 1;

	for (int i = 0; i < mapChunkSize * mapChunkSize; i++)
	{
		// Define our proportionate 'x' and 'y' indices so that we can map our 1D vector too an 'x' and 'y' coordinate for our vertex
		int y = i % mapChunkSize; // inner vector, traverses the y-axis (left to right)
		int x = std::floor(i / mapChunkSize); // outer vector, traveres the x-axis (top to bottom)

		// Define the height for the current vertex in our iteration
		float currentHeight = ProceduralGeneration::calculateWeightCurve(noiseMap[i], weightCurveExponent) * heightMultiplier;

		// ** ADD MESH DATA (Vertices, UVs, VertexColors, Triangles) **
		meshData.vertices.Add(FVector(topMostX - x, leftMostY + y, currentHeight));
		meshData.uvs.Add(FVector2D(x, y));
		meshData.vertexColorsNEW.Add(FColor(0.50, 0.75, 1.00, 1.0));
		// Add Triangles:
		if (y < mapChunkSize - 1 && x < mapChunkSize - 1)
		{
			// NOTE: UE5 render mode is counter-clockwise when looking from +z -> =z... the order in which we pass these vertices in is based off that 
			meshData.AddTriangle(i, i + verticesPerLine, i + verticesPerLine + 1);
			meshData.AddTriangle(i + verticesPerLine + 1, i + 1, i);
		}
	}

	return meshData;
}
