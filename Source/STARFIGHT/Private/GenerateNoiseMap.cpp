// Fill out your copyright notice in the Description page of Project Settings.


#include "GenerateNoiseMap.h"



DECLARE_CYCLE_STAT(TEXT("Update Collision"), STAT_ProcMesh_CalcTangents, STATGROUP_ProceduralMesh);



TArray<FArray2D> UGenerateNoiseMap::GenerateNoiseMap(const int32& mapChunkSize, int32& seed, FVector2D& offset, float& noiseScale, int& octaves, float& persistance, float& lacurnarity) {

	TArray<FArray2D> noiseMap;

	// Initialize our noiseMap first with an array of 'FArray2D' structs, then initialize the 'secondArray' of each 'FArray2D' structs with a list of '0.0f'
	noiseMap.Init(FArray2D(), mapChunkSize);
	for (auto& nestedStruct : noiseMap) { nestedStruct.secondArray.Init(0.0f, mapChunkSize); }
	

	TArray<FVector2D> octaveOffsets;

	
	FRandomStream prng = FRandomStream( seed );
	octaveOffsets.Init(FVector2D(), octaves);
	for (int32 i = 0; i < octaves; i++) {
		float offsetX = prng.FRandRange(-100000, 100000) + offset.X;
		float offsetY = prng.FRandRange(-100000, 100000) + offset.Y;
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

			noiseMap[y].secondArray[x] = noiseHeight;
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



TArray<FArray2D> UGenerateNoiseMap::GenerateFalloffMap(const int32& mapChunkSize, float& a, float& b, float& c) {

	const int32 t = 20;
	int32 falloffMap_NEW[t][t];

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



FGeneratedMeshData UGenerateNoiseMap::GenerateProceduralMeshData(const int32 mapChunkSize, int32 seed, FVector2D offset, int32 levelOfDetail, 
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
				//FVector v1 = FVector(1.0f, 0.0f, noiseMap[y].secondArray[x + 1] - noiseMap[y].secondArray[x]);  // (x + 1 - x, y - y, next height - current height)
				//FVector v2 = FVector(0.0f, 1.0f, noiseMap[y + 1].secondArray[x] - noiseMap[y].secondArray[x]);  // (x - x, y + 1 - y, next height - current height)
				//FVector newNormal = FVector::CrossProduct(v2, v1);
				//meshData.normals.Add(newNormal);

				//float magnitude = (FMath::Sqrt((v2[0] + v1[0]) / 2 + (v2[1] + v1[1]) / 2 + (v2[2] + v1[2]) / 2));
				
				//UE_LOG(LogTemp, Warning, TEXT("magnitude = %d   :   NEW TANGENT  =  (%d, %d, %d)"), magnitude, (v2[0] + v1[0]) / (2), (v2[1] + v1[1]) / (2), (v2[2] + v1[2]) / (2 * magnitude) );
				

				// Add Tangents:
				//meshData.tangents.Add(FProcMeshTangent( (v2[0] + v1[0]) / (2), (v2[1] + v1[1]) / (2), (v2[2] + v1[2]) / (2 * magnitude) ));

			}

			vertexIndex++;
		}
	}

	return meshData;
}






// STOLEN FROM "KismetProceduralMeshLibrary.cpp" :

void UGenerateNoiseMap::CalculateNormalsANDTangents(const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector2D>& UVs, TArray<FVector>& Normals, TArray<FProcMeshTangent>& Tangents)
{
	SCOPE_CYCLE_COUNTER(STAT_ProcMesh_CalcTangents);

	if (Vertices.Num() == 0)
	{
		return;
	}

	// Number of triangles
	const int32 NumTris = Triangles.Num() / 3;
	// Number of verts
	const int32 NumVerts = Vertices.Num();

	// Map of vertex to triangles in Triangles array
	TMultiMap<int32, int32> VertToTriMap;
	// Map of vertex to triangles to consider for normal calculation
	TMultiMap<int32, int32> VertToTriSmoothMap;

	// Normal/tangents for each face
	TArray<FVector3f> FaceTangentX, FaceTangentY, FaceTangentZ;
	FaceTangentX.AddUninitialized(NumTris);
	FaceTangentY.AddUninitialized(NumTris);
	FaceTangentZ.AddUninitialized(NumTris);

	// Iterate over triangles
	for (int TriIdx = 0; TriIdx < NumTris; TriIdx++)
	{
		int32 CornerIndex[3];
		FVector3f P[3];

		for (int32 CornerIdx = 0; CornerIdx < 3; CornerIdx++)
		{
			// Find vert index (clamped within range)
			int32 VertIndex = FMath::Min(Triangles[(TriIdx * 3) + CornerIdx], NumVerts - 1);

			CornerIndex[CornerIdx] = VertIndex;
			P[CornerIdx] = (FVector3f)Vertices[VertIndex];

			// Find/add this vert to index buffer
			TArray<int32> VertOverlaps;
			FindVertOverlaps(VertIndex, Vertices, VertOverlaps);

			// Remember which triangles map to this vert
			VertToTriMap.AddUnique(VertIndex, TriIdx);
			VertToTriSmoothMap.AddUnique(VertIndex, TriIdx);

			// Also update map of triangles that 'overlap' this vert (ie don't match UV, but do match smoothing) and should be considered when calculating normal
			for (int32 OverlapIdx = 0; OverlapIdx < VertOverlaps.Num(); OverlapIdx++)
			{
				// For each vert we overlap..
				int32 OverlapVertIdx = VertOverlaps[OverlapIdx];

				// Add this triangle to that vert
				VertToTriSmoothMap.AddUnique(OverlapVertIdx, TriIdx);

				// And add all of its triangles to us
				TArray<int32> OverlapTris;
				VertToTriMap.MultiFind(OverlapVertIdx, OverlapTris);
				for (int32 OverlapTriIdx = 0; OverlapTriIdx < OverlapTris.Num(); OverlapTriIdx++)
				{
					VertToTriSmoothMap.AddUnique(VertIndex, OverlapTris[OverlapTriIdx]);
				}
			}
		}

		// Calculate triangle edge vectors and normal
		const FVector3f Edge21 = P[1] - P[2];
		const FVector3f Edge20 = P[0] - P[2];
		const FVector3f TriNormal = (Edge21 ^ Edge20).GetSafeNormal();

		// If we have UVs, use those to calc 
		if (UVs.Num() == Vertices.Num())
		{
			const FVector2D T1 = UVs[CornerIndex[0]];
			const FVector2D T2 = UVs[CornerIndex[1]];
			const FVector2D T3 = UVs[CornerIndex[2]];

			FMatrix	ParameterToLocal(
				FPlane(P[1].X - P[0].X, P[1].Y - P[0].Y, P[1].Z - P[0].Z, 0),
				FPlane(P[2].X - P[0].X, P[2].Y - P[0].Y, P[2].Z - P[0].Z, 0),
				FPlane(P[0].X, P[0].Y, P[0].Z, 0),
				FPlane(0, 0, 0, 1)
			);

			FMatrix ParameterToTexture(
				FPlane(T2.X - T1.X, T2.Y - T1.Y, 0, 0),
				FPlane(T3.X - T1.X, T3.Y - T1.Y, 0, 0),
				FPlane(T1.X, T1.Y, 1, 0),
				FPlane(0, 0, 0, 1)
			);

			// Use InverseSlow to catch singular matrices.  Inverse can miss this sometimes.
			const FMatrix TextureToLocal = ParameterToTexture.Inverse() * ParameterToLocal;

			FaceTangentX[TriIdx] = FVector4f(TextureToLocal.TransformVector(FVector(1, 0, 0)).GetSafeNormal());
			FaceTangentY[TriIdx] = FVector4f(TextureToLocal.TransformVector(FVector(0, 1, 0)).GetSafeNormal());
		}
		else
		{
			FaceTangentX[TriIdx] = Edge20.GetSafeNormal();
			FaceTangentY[TriIdx] = (FaceTangentX[TriIdx] ^ TriNormal).GetSafeNormal();
		}

		FaceTangentZ[TriIdx] = TriNormal;
	}


	// Arrays to accumulate tangents into
	TArray<FVector3f> VertexTangentXSum, VertexTangentYSum, VertexTangentZSum;
	VertexTangentXSum.AddZeroed(NumVerts);
	VertexTangentYSum.AddZeroed(NumVerts);
	VertexTangentZSum.AddZeroed(NumVerts);

	// For each vertex..
	for (int VertxIdx = 0; VertxIdx < Vertices.Num(); VertxIdx++)
	{
		// Find relevant triangles for normal
		TArray<int32> SmoothTris;
		VertToTriSmoothMap.MultiFind(VertxIdx, SmoothTris);

		for (int i = 0; i < SmoothTris.Num(); i++)
		{
			int32 TriIdx = SmoothTris[i];
			VertexTangentZSum[VertxIdx] += FaceTangentZ[TriIdx];
		}

		// Find relevant triangles for tangents
		TArray<int32> TangentTris;
		VertToTriMap.MultiFind(VertxIdx, TangentTris);

		for (int i = 0; i < TangentTris.Num(); i++)
		{
			int32 TriIdx = TangentTris[i];
			VertexTangentXSum[VertxIdx] += FaceTangentX[TriIdx];
			VertexTangentYSum[VertxIdx] += FaceTangentY[TriIdx];
		}
	}

	// Finally, normalize tangents and build output arrays

	Normals.Reset();
	Normals.AddUninitialized(NumVerts);

	Tangents.Reset();
	Tangents.AddUninitialized(NumVerts);

	for (int VertxIdx = 0; VertxIdx < NumVerts; VertxIdx++)
	{
		FVector3f& TangentX = VertexTangentXSum[VertxIdx];
		FVector3f& TangentY = VertexTangentYSum[VertxIdx];
		FVector3f& TangentZ = VertexTangentZSum[VertxIdx];

		TangentX.Normalize();
		TangentZ.Normalize();

		Normals[VertxIdx] = (FVector)TangentZ;

		// Use Gram-Schmidt orthogonalization to make sure X is orth with Z
		TangentX -= TangentZ * (TangentZ | TangentX);
		TangentX.Normalize();

		// See if we need to flip TangentY when generating from cross product
		const bool bFlipBitangent = ((TangentZ ^ TangentX) | TangentY) < 0.f;

		Tangents[VertxIdx] = FProcMeshTangent((FVector)TangentX, bFlipBitangent);
	}
}



void FindVertOverlaps(int32 TestVertIndex, const TArray<FVector>& Verts, TArray<int32>& VertOverlaps)
{
	// Check if Verts is empty or test is outside range
	if (TestVertIndex < Verts.Num())
	{
		const FVector TestVert = Verts[TestVertIndex];

		for (int32 VertIdx = 0; VertIdx < Verts.Num(); VertIdx++)
		{
			// First see if we overlap, and smoothing groups are the same
			if (TestVert.Equals(Verts[VertIdx]))
			{
				// If it, so we are at least considered an 'overlap' for normal gen
				VertOverlaps.Add(VertIdx);
			}
		}
	}
}





// CURRENLTY NOT IN USE:
FTransform UGenerateNoiseMap::calculateWorldTransform(FVector location, FVector normal, float maxTilt, float maxRotation, FRandomStream seed) {
	
	FTransform result, alignedTransform, nonAlignedTransform;

	// Aligned to Normal Code:
	alignedTransform.SetLocation(location);
	alignedTransform.SetScale3D(FVector(0.0f, 0.0f, 0.0f));


	// Non-Aligned to Normal Code:
	nonAlignedTransform.SetLocation(location);
	nonAlignedTransform.SetScale3D(FVector(0.0f, 0.0f, 0.0f));

	return result;
}



bool UGenerateNoiseMap::calculateSlopeAvailability(float maxSlope, float minSlope, FVector inputNormal) {

	bool result;
	float angle = FMath::Acos(inputNormal.Z) * 180 / PI;
	float angle_minus = angle - 0.001;
	float angle_plus = angle + 0.001;

	((minSlope <= angle_minus && angle_minus <= maxSlope) || (minSlope <= angle_plus && angle_plus <= maxSlope)) ? result = true : result = false;

	return result;
}



bool UGenerateNoiseMap::calculateHeightAvailability(float maxHeight, float minHeight, FVector spawnLocation) {

	bool result;

	(minHeight <= spawnLocation.Z && spawnLocation.Z <= maxHeight) ? result = true : result = false;
	
	return result;
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
	float output = (FMath::Pow(value, a) / (c * FMath::Pow(value, a) + FMath::Pow(b - b * value, a)) );
	return output;
}
