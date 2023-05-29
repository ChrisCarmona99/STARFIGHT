// Fill out your copyright notice in the Description page of Project Settings.


#include "GenerateTerrainBPCallable.h"

// CreateMeshSection  (ProceduralMeshComponent.h)
// CalculateTangentsForMesh  (KismetProceduralMeshLibrary.h)

//FGeneratedMeshData UGenerateTerrainBPCallable::ExecuteProceduralMeshGeneration(FProceduralMeshInputs& Inputs)
//{
//	FGeneratedMeshData meshData;
//
//	if (!IsInRenderingThread())
//	{
//		// Create a completion event to be signaled when the command has completed
//		FEvent* CompletionEvent = FPlatformProcess::GetSynchEventFromPool(true);
//
//		// Enqueue a render command to call MyFunction on the render thread
//		ENQUEUE_RENDER_COMMAND(MyRenderCommand)(
//			[CompletionEvent, Inputs, meshData](FRHICommandListImmediate& RHICmdList) mutable
//			{
//				// Call the function on the render thread
//				int a = 1;
//				UGenerateTerrainBPCallable::GenerateProceduralMeshData(Inputs, meshData, CompletionEvent);
//			}
//		);
//
//		// Wait for the completion event to be signaled on the game thread
//		CompletionEvent->Wait();
//		// Free the completion event
//		FPlatformProcess::ReturnSynchEventToPool(CompletionEvent);
//	}
//	
//	return meshData;
//}
//
//// This is our blueprint callable function that will be used to put the actual generation function on the render thread:
//void UGenerateTerrainBPCallable::GenerateProceduralMeshData(FProceduralMeshInputs& Inputs, FGeneratedMeshData& meshData, FEvent* CompletionEvent)
//{
//	int a = 1;
//}



/*
FGeneratedMeshData UGenerateTerrainBPCallable::GenerateProceduralMeshData_OLD(const int32 mapChunkSize, int32 seed, FVector2D offset, int32 levelOfDetail, 
																		  float noiseScale, int octaves, float persistance, float lacunarity, float heightMultiplier, 
																		  float weightCurveExponent, float a, float b, float c)
{
	std::vector<float> noiseMap;
	FGeneratedMeshData meshData;

	ProceduralGeneration::GenerateNoiseMap_OLD(noiseMap, mapChunkSize, seed, offset, noiseScale, octaves, persistance, lacunarity);
	ProceduralGeneration::ApplyFalloffMap_OLD(noiseMap, mapChunkSize, a, b, c);

	// Sets the starting traversal point for all the vertices
	//		In UE5, the x-axis and y-axis are flipped... +x is 'forward' and +y is 'to the right'
	float topMostX = (mapChunkSize - 1) / 2.f; // +x
	float leftMostY = (mapChunkSize - 1) / -2.f; // -y

	// Calculates the increment for mesh LODs (ensures 'levelOfDetail' is NOT 0):
	int32 LODincrement = levelOfDetail == 0 ? 1 : levelOfDetail * 2;
	// Calculates correct number of vertic  es for our 'vertices' array:
	int32 verticesPerLine = ((mapChunkSize - 1) / LODincrement) + 1;


	// MAKE HLSL:
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
*/
