// Fill out your copyright notice in the Description page of Project Settings.


#include "GeneratedMesh.h"


// CONSTRUCTOR:
AGeneratedMesh::AGeneratedMesh() {
	ProceduralMesh = CreateDefaultSubobject<UProceduralMeshComponent>("GeneratedMesh");
	RootComponent = ProceduralMesh;

	BaseMaterial = CreateDefaultSubobject<UMaterialInterface>("BaseMaterial");

	MeshData = FGeneratedMeshData(MapChunkSize, MapChunkSize);
	/*NoiseMap = UGenerateNoiseMap::GenerateNoiseMap(MapChunkSize, NoiseScale, Octaves, Persistance, Lacunarity);*/
	/*FalloffMap = UGenerateNoiseMap::GenerateFalloffMap(MapChunkSize, A, B);*/

 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false; // ORIGINALY SET TO TRUE
}

// Called when the game starts or when spawned
void AGeneratedMesh::BeginPlay() {
	Super::BeginPlay();
}

// Called when an instance of this class is placed (in editor) or spawned:
void AGeneratedMesh::OnConstruction(const FTransform& Transform) {
	/*UE_LOG(LogTemp, Warning, TEXT("\n\nOnConstruction CALLED:\n"));*/
	ProceduralMesh->SetMaterial(0, BaseMaterial);
	GenerateTerrainMesh(MeshData, NoiseMap, FalloffMap, MapChunkSize, Seed, LevelOfDetail, NoiseScale, Octaves, Persistance, Lacunarity, HeightMultiplier, WeightCurveExponent);
}

// Called every frame
void AGeneratedMesh::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);
}

// Called when 'GeneratedMesh' is spawned (at runtime or when you drop it into the world in editor)
void AGeneratedMesh::PostActorCreated() {
	Super::PostActorCreated();
	//GenerateTerrainMesh(MeshData, MapChunkSize, LevelOfDetail, NoiseScale, Octaves, Persistance, Lacunarity, HeightMultiplier, WeightCurveExponent);
}

// Called when 'GeneratedMesh' is already in level and map is opened
void AGeneratedMesh::PostLoad() {
	Super::PostLoad();
}

void AGeneratedMesh::GenerateTerrainMesh(FGeneratedMeshData& meshData, TArray<FArray2D>& noiseMap, TArray<FArray2D>& falloffMap, int32& mapChunkSize, int32& seed, int32& levelOfDetail, float& noiseScale, int& octaves, float& persistance, float& lacunarity, float& heightMultiplier, float& weightCurveExponent) {
	//UE_LOG(LogTemp, Warning, TEXT("\n\nGenerateTerrainMesh CALLED:\n"));

	noiseMap = UGenerateNoiseMap::GenerateNoiseMap(MapChunkSize, seed, NoiseScale, Octaves, Persistance, Lacunarity);
	falloffMap = UGenerateNoiseMap::GenerateFalloffMap(MapChunkSize, A, B, C, D);

	// Clamp our Noise Values:
	noiseScale <= 0 ? noiseScale = 0.0001f : noiseScale = noiseScale;
	lacunarity < 1 ? lacunarity = 1 : lacunarity = lacunarity;
	octaves < 0 ? octaves = 0 : octaves > 7 ? octaves = 7 : octaves = octaves;
	persistance < 0 ? persistance = 0 : persistance > 1 ? persistance > 1 : persistance = persistance;
	weightCurveExponent < 1 ? weightCurveExponent = 1.0f : weightCurveExponent = weightCurveExponent;
	// Clamp our Level of Detail Value:
	levelOfDetail < 0 ? levelOfDetail = 0 : levelOfDetail > 6 ? levelOfDetail = 6 : levelOfDetail = levelOfDetail;


	// Reset all our meshData:
	meshData.ClearData();

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

			//UE_LOG(LogTemp, Warning, TEXT("\n\n   y = %d  :  x = %d"), y, x);
			//UE_LOG(LogTemp, Warning, TEXT("     noiseMap   = %f"), noiseMap[y].secondArray[x]);
			//UE_LOG(LogTemp, Warning, TEXT("     falloffMap = %f"), falloffMap[y].secondArray[x]);

			// Implement our Falloff Map:
			noiseMap[y].secondArray[x] = FMath::Clamp(noiseMap[y].secondArray[x] - falloffMap[y].secondArray[x], 0.0f, 1.0f);
			//noiseMap[x + y] = FMath::Clamp(noiseMap[x + y] - falloffMap[x + y], 0.0f, 1.0f); // UNCOMMENT
		
			//UE_LOG(LogTemp, Warning, TEXT("     NEW noiseMap   = %f"), noiseMap[y].secondArray[x]);

			// Define the height for the current vertex in our iteration:
			float currentHeight = calculateWeightCurve(noiseMap[y].secondArray[x], weightCurveExponent) * heightMultiplier;
			//float currentHeight = calculateWeightCurve(noiseMap[x + y], weightCurveExponent) * heightMultiplier;
			
			//UE_LOG(LogTemp, Warning, TEXT("     FINAL HEIGHT  =  %f"), currentHeight);

			// Add each vertex, uv, normal, tangent, & vertexColor:
			meshData.vertices.Add(FVector(topLeftX + x, topLeftZ - y, currentHeight)); // SETS [X, Y, Z] FOR EACH VERTEX
			meshData.uvs.Add(FVector2D(x / (float)mapChunkSize, y / (float)mapChunkSize));

			//meshData.tangents.Add(FProcMeshTangent(0, 1, 0));
			meshData.vertexColorsNEW.Add(FColor(0.50, 0.75, 1.00, 1.0));
			
			
			// Check if we are still one vertex away from the bottom and right boundaries:
			if (x < mapChunkSize - 1 && y < mapChunkSize - 1) {
				// Add Triangles (two per square tile):
				meshData.AddTriangle(vertexIndex, vertexIndex + verticesPerLine + 1, vertexIndex + verticesPerLine);
				meshData.AddTriangle(vertexIndex + verticesPerLine + 1, vertexIndex, vertexIndex + 1);

				// Add Normals:
				FVector v1 = FVector(1, 0, noiseMap[y].secondArray[x + 1] - noiseMap[y].secondArray[x]);  // (x + 1 - x, y - y, next height - current height)
				FVector v2 = FVector(0, 1, noiseMap[y + 1].secondArray[x] - noiseMap[y].secondArray[x]);  // (x - x, y + 1 - y, next height - current height)
				meshData.normals.Add(FVector::CrossProduct(v2, v1));

				// Add Tangents:
				meshData.tangents.Add(FProcMeshTangent( (v2[0] - v1[0]) / 2, (v2[1] - v1[1]) / 2, (v2[2] - v1[2]) / 2) );
			}
			
			vertexIndex++;
		}
	}
	
	ProceduralMesh->CreateMeshSection(0, meshData.vertices, meshData.triangles, meshData.normals, meshData.uvs, meshData.vertexColorsNEW, meshData.tangents, true);
	// Enable collision data:
	ProceduralMesh->ContainsPhysicsTriMeshData(true);
}



float AGeneratedMesh::calculateWeightCurve(float vertexHeight, float exponent) {
	float output = pow(vertexHeight, exponent);
	return output;
}



// NOT BEING USED ANYMORE!!!
//void AGeneratedMesh::UpdateTerrainMesh() {
//	UE_LOG(LogTemp, Warning, TEXT("\n\nUpdateTerrainMesh CALLED:\n"));
//
//	// The following if-statements 'clamp' our noiseMap values:
//	if (noiseScale <= 0) { noiseScale = 0.0001f; }
//	if (lacunarity < 1) { lacunarity = 1; }
//	if (octaves < 0) { octaves = 0; }
//	else if (octaves > 7) { octaves = 7; }
//	if (persistance < 0) { persistance = 0; }
//	else if (persistance > 1) { persistance = 1; }
//	if (weightCurveExponent < 1) { weightCurveExponent = 1.0f; }
//
//	// Clamp our Level of Detail Value:
//	if (levelOfDetail < 1) { levelOfDetail = 1; }
//	else if (levelOfDetail > 6) { levelOfDetail = 6; }
//
//	// Sets the starting 
//	float topLeftX = (mapChunkSize - 1) / -2.f;
//	float topLeftZ = (mapChunkSize - 1) / 2.f;
//
//	/*int32 index = 0;*/
//	int32 meshSimplificationIncrement = levelOfDetail == 0 ? 1 : levelOfDetail * 2;
//	// Calculates correct number of vertices for our 'vertices' array:
//	int32 verticesPerLine = (mapChunkSize - 1) / meshSimplificationIncrement + 1;
//	
//	TArray<FArray2D> newNoiseMap = UGenerateNoiseMap::GenerateNoiseMap(mapChunkSize, noiseScale, octaves, persistance, lacunarity);
//	
//	int32 vertexIndex = 0;
//
//	// Reset the meshData:
//	UE_LOG(LogTemp, Warning, TEXT("\n\nUPDATE PRE:  meshData.vertices.Num() = : %d\n"), meshData.vertices.Num());
//	for (int32 count = meshData.vertices.Num() - 1; count >= 0; count--) {
//		meshData.vertices.RemoveAtSwap(count, 1, true);
//		meshData.uvs.RemoveAtSwap(count, 1, true);
//		meshData.normals.RemoveAtSwap(count, 1, true);
//		meshData.tangents.RemoveAtSwap(count, 1, true);
//		meshData.vertexColorsNEW.RemoveAtSwap(count, 1, true);
//	}
//	UE_LOG(LogTemp, Warning, TEXT("\n\nUPDATE POST:  meshData.vertices.Num() = : %d\n"), meshData.vertices.Num());
//
//	for (int32 y = 0; y < mapChunkSize; y += meshSimplificationIncrement) {
//		for (int32 x = 0; x < mapChunkSize; x += meshSimplificationIncrement) {
//
//			float newVertexHeight = calculateWeightCurve(newNoiseMap[y].secondArray[x], weightCurveExponent) * heightMultiplier;
//
//			// Add each vertex, uv, normal, tangent, & vertexColor:
//			meshData.vertices.Add(FVector(topLeftX + x, topLeftZ - y, newVertexHeight));
//			meshData.uvs.Add(FVector2D(x / (float)mapChunkSize, y / (float)mapChunkSize));
//			meshData.normals.Add(FVector(1, 0, 0));
//			meshData.tangents.Add(FProcMeshTangent(0, 1, 0));
//			meshData.vertexColorsNEW.Add(FColor(0.75, 0.75, 0.75, 1.0));
//
//			vertexIndex++;
//		}
//	}
//
//	ProceduralMesh->UpdateMeshSection(0, meshData.vertices, meshData.normals, meshData.uvs, meshData.vertexColorsNEW, meshData.tangents);
//	ProceduralMesh->ContainsPhysicsTriMeshData(true);
//}
