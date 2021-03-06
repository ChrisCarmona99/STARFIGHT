// Fill out your copyright notice in the Description page of Project Settings.


#include "GeneratedMesh.h"


// CONSTRUCTOR:
AGeneratedMesh::AGeneratedMesh() {
	ProceduralMesh = CreateDefaultSubobject<UProceduralMeshComponent>("GeneratedMesh");
	RootComponent = ProceduralMesh;

	BaseMaterial = CreateDefaultSubobject<UMaterialInterface>("BaseMaterial");

	MeshData_0 = FGeneratedMeshData(MapChunkSize_0, MapChunkSize_0);

 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false; // ORIGINALY SET TO TRUE
}

// Called when the game starts or when spawned
void AGeneratedMesh::BeginPlay() {
	Super::BeginPlay();
}

// Called when an instance of this class is placed (in editor) or spawned:
void AGeneratedMesh::OnConstruction(const FTransform& Transform) {
	ProceduralMesh->SetMaterial(0, BaseMaterial);

	// Clamp our Noise Values:
	NoiseScale_0 <= 0 ? NoiseScale_0 = 0.0001f : NoiseScale_0 = NoiseScale_0;
	Lacunarity_0 < 1 ? Lacunarity_0 = 1 : Lacunarity_0 = Lacunarity_0;
	Octaves_0 < 0 ? Octaves_0 = 0 : Octaves_0 > 7 ? Octaves_0 = 7 : Octaves_0 = Octaves_0;
	Persistance_0 < 0 ? Persistance_0 = 0 : Persistance_0 > 1 ? Persistance_0 > 1 : Persistance_0 = Persistance_0;
	WeightCurveExponent_0 < 1 ? WeightCurveExponent_0 = 1.0f : WeightCurveExponent_0 = WeightCurveExponent_0;
	// Clamp our Level of Detail Value:
	LevelOfDetail_0 < 0 ? LevelOfDetail_0 = 0 : LevelOfDetail_0 > 6 ? LevelOfDetail_0 = 6 : LevelOfDetail_0 = LevelOfDetail_0;

	//GenerateTerrainMesh(MeshData, NoiseMap, FalloffMap, MapChunkSize, Seed, Offset, LevelOfDetail, NoiseScale, Octaves, Persistance, Lacunarity, HeightMultiplier, WeightCurveExponent, A, B, C);
	ConstructProceduralTerrainMesh(MeshData_0, NoiseMap_0, FalloffMap_0, MapChunkSize_0, Seed_0, Offset_0, LevelOfDetail_0, NoiseScale_0, Octaves_0, Persistance_0, Lacunarity_0, HeightMultiplier_0, WeightCurveExponent_0, A_0, B_0, C_0);

}

// Called every frame
void AGeneratedMesh::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);
}

// Called when 'GeneratedMesh' is spawned (at runtime or when you drop it into the world in editor)
void AGeneratedMesh::PostActorCreated() {
	Super::PostActorCreated();
}

// Called when 'GeneratedMesh' is already in level and map is opened
void AGeneratedMesh::PostLoad() {
	Super::PostLoad();
}

void AGeneratedMesh::ConstructProceduralTerrainMesh(FGeneratedMeshData& meshData, TArray<FArray2D>& noiseMap, TArray<FArray2D>& falloffMap, int32& mapChunkSize, int32& seed, FVector2D& offset, int32& levelOfDetail, float& noiseScale, int& octaves, float& persistance, float& lacunarity, float& heightMultiplier, float& weightCurveExponent, float& a, float&b, float&c) {

	noiseMap = UGenerateNoiseMap::GenerateNoiseMap(mapChunkSize, seed, offset, noiseScale, octaves, persistance, lacunarity);
	falloffMap = UGenerateNoiseMap::GenerateFalloffMap(mapChunkSize, a, b, c);

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
				FVector v1 = FVector(1, 0, noiseMap[y].secondArray[x + 1] - noiseMap[y].secondArray[x]);  // (x + 1 - x, y - y, next height - current height)
				FVector v2 = FVector(0, 1, noiseMap[y + 1].secondArray[x] - noiseMap[y].secondArray[x]);  // (x - x, y + 1 - y, next height - current height)
				meshData.normals.Add(FVector::CrossProduct(v2, v1));

				// Add Tangents:
				meshData.tangents.Add(FProcMeshTangent( (v2[0] - v1[0]) / 2, (v2[1] - v1[1]) / 2, (v2[2] - v1[2]) / 2) );
				
			}
			
			vertexIndex++;
		}
	}
	// Generate our mesh with given mesh data:
	ProceduralMesh->CreateMeshSection(0, meshData.vertices, meshData.triangles, meshData.normals, meshData.uvs, meshData.vertexColorsNEW, meshData.tangents, true);
	// Enable collision data:
	ProceduralMesh->ContainsPhysicsTriMeshData(true);
}



float AGeneratedMesh::calculateWeightCurve(float vertexHeight, float exponent) {
	float output = pow(vertexHeight, exponent);
	return output;
}
