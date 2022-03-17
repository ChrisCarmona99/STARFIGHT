// Fill out your copyright notice in the Description page of Project Settings.


#include "GeneratedMesh.h"


// CONSTRUCTOR:
AGeneratedMesh::AGeneratedMesh() {

	ProceduralMesh = CreateDefaultSubobject<UProceduralMeshComponent>("GeneratedMesh");
	RootComponent = ProceduralMesh;

	BaseMaterial = CreateDefaultSubobject<UMaterialInterface>("BaseMaterial");

	noiseMap = UGenerateNoiseMap::GenerateNoiseMap(mapChunkSize, noiseScale, octaves, persistance, lacunarity);
	meshData = FGeneratedMeshData(mapChunkSize, mapChunkSize);

 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false; // ORIGINALY SET TO TRUE

}

// Called when the game starts or when spawned
void AGeneratedMesh::BeginPlay() {
	Super::BeginPlay();
	ProceduralMesh->SetMaterial(0, BaseMaterial);
}

// Called when an instance of this class is placed (in editor) or spawned:
void AGeneratedMesh::OnConstruction(const FTransform& Transform) {
	UpdateTerrainMesh();
}

// Called every frame
void AGeneratedMesh::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);
}

// Called when 'GeneratedMesh' is spawned (at runtime or when you drop it into the world in editor)
void AGeneratedMesh::PostActorCreated() {
	Super::PostActorCreated();
	GenerateTerrainMesh();
}

// Called when 'GeneratedMesh' is already in level and map is opened
void AGeneratedMesh::PostLoad() {
	Super::PostLoad();
	UpdateTerrainMesh();
}

void AGeneratedMesh::GenerateTerrainMesh() {

	float topLeftX = (mapChunkSize - 1) / -2.f;
	float topLeftZ = (mapChunkSize - 1) / 2.f;

	// Calculates the increment for mesh LODs (ensures 'levelOfDetail' is NOT 0:
	int32 meshSimplificationIncrement = levelOfDetail == 0 ? 1 : levelOfDetail * 2;

	int32 vertexIndex = 0;

	for (int32 y = 0; y < mapChunkSize; y += meshSimplificationIncrement) {
		for (int32 x = 0; x < mapChunkSize; x += meshSimplificationIncrement) {

			float currentVertexHeight = calculateWeightCurve(noiseMap[y].secondArray[x], weightCurveExponent) * heightMultiplier;

			// Add each vertex, uv, normal, tangent, & vertexColor:
			meshData.vertices[vertexIndex] = FVector(topLeftX + x, topLeftZ - y, currentVertexHeight);
			meshData.uvs[vertexIndex] = FVector2D(x / (float)mapChunkSize, y / (float)mapChunkSize);
			meshData.normals[vertexIndex] = FVector(1, 0, 0);
			meshData.tangents[vertexIndex] = FProcMeshTangent(0, 1, 0);
			meshData.vertexColors[vertexIndex] = FLinearColor(0.75, 0.75, 0.75, 1.0);
			
			// Check if we are still one vertex away from the bottom and right boundaries:
			if (x < mapChunkSize - 1 && y < mapChunkSize - 1) {
				// Add the two triangles that make up each square tile of mesh:
				meshData.AddTriangle(vertexIndex, vertexIndex + mapChunkSize + 1, vertexIndex + mapChunkSize);
				meshData.AddTriangle(vertexIndex + mapChunkSize + 1, vertexIndex, vertexIndex + 1);
			}
			
			vertexIndex++;
		}
	}

	ProceduralMesh->CreateMeshSection_LinearColor(0, meshData.vertices, meshData.triangles, meshData.normals, meshData.uvs, meshData.vertexColors, meshData.tangents, true);
	// Enable collision data:
	ProceduralMesh->ContainsPhysicsTriMeshData(true);
}



void AGeneratedMesh::UpdateTerrainMesh() {

	/*int32 index = 0;*/
	int32 meshSimplificationIncrement = levelOfDetail == 0 ? 1 : levelOfDetail * 2;

	// The following if-statements 'clamp' our noiseMap values:
	if (noiseScale <= 0) { noiseScale = 0.0001f; }
	if (lacunarity < 1) { lacunarity = 1; }
	if (octaves < 0) { octaves = 0; } else if (octaves > 7) { octaves = 7; }
	if (persistance < 0) { persistance = 0; } else if (persistance > 1) { persistance = 1; }

	if (weightCurveExponent < 1) { weightCurveExponent = 1.0f; }

	TArray<FArray2D> newNoiseMap = UGenerateNoiseMap::GenerateNoiseMap(mapChunkSize, noiseScale, octaves, persistance, lacunarity);

	int32 vertexIndex = 0;

	for (int32 y = 0; y < mapChunkSize; y += meshSimplificationIncrement) {
		for (int32 x = 0; x < mapChunkSize; x += meshSimplificationIncrement) {

			float newVertexHeight = calculateWeightCurve(newNoiseMap[y].secondArray[x], weightCurveExponent) * heightMultiplier;

			meshData.vertices[vertexIndex] = FVector(meshData.vertices[vertexIndex].X, meshData.vertices[vertexIndex].Y, newVertexHeight);

			vertexIndex++;
		}
	}

	ProceduralMesh->UpdateMeshSection_LinearColor(0, meshData.vertices, meshData.normals, meshData.uvs, meshData.vertexColors, meshData.tangents);
	ProceduralMesh->ContainsPhysicsTriMeshData(true);
}



float AGeneratedMesh::calculateWeightCurve(float vertexHeight, float exponent) {
	float output = pow(vertexHeight, exponent);
	return output;
}
