// Fill out your copyright notice in the Description page of Project Settings.


#include "GeneratedMesh.h"


// CONSTRUCTOR:
AGeneratedMesh::AGeneratedMesh()
{
	ProceduralMesh = CreateDefaultSubobject<UProceduralMeshComponent>("GeneratedMesh");
	RootComponent = ProceduralMesh;

	noiseMap = UGenerateNoiseMap::GenerateNoiseMap(mapWidth, mapHeight, scale);
	meshData = FGeneratedMeshData(mapWidth, mapHeight);

 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false; // ORIGINALY SET TO TRUE

}

// Called when the game starts or when spawned
void AGeneratedMesh::BeginPlay() {
	Super::BeginPlay();
}

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
	GenerateTerrainMesh(mapWidth, mapHeight);
}

// Called when 'GeneratedMesh' is already in level and map is opened
void AGeneratedMesh::PostLoad() {
	Super::PostLoad();
	/*
	NOTE: the following 'GenerateTerrainMesh' function call was breaking the game...
		Because I left an instance of the generated mesh in the level after closing, 'GenerateTerrainMesh' was called once
		I opened the editor... however because the instance already have a filled triangle array, called the function again
		tried to fill the triangles array AGAIN... causing an index error

	SOLUTION: COMMENT OUT THIS FUNCTION IF YOU ACCIDENTALY LEAVE A MESH INSTANCE IN THE LEVEL AGAIN!!!!
	*/

	//GenerateTerrainMesh(mapWidth, mapHeight);
}

void AGeneratedMesh::GenerateTerrainMesh(int32 inputWidth, int32 inputHeight) {

	int32 width = inputWidth;
	int32 height = inputHeight;
	float topLeftX = (width - 1) / -2.f;
	float topLeftZ = (height - 1) / 2.f;

	int32 vertexIndex = 0;

	for (int32 y = 0; y < height; y++) {
		for (int32 x = 0; x < width; x++) {

			float currentVertexHeight = noiseMap[y].secondArray[x];
			//UE_LOG(LogTemp, Warning, TEXT("Output: %f"), currentVertexHeight);  // DELETE

			// Add each vertex, uv, normal, tangent, & vertexColor:
			meshData.vertices[vertexIndex] = FVector(topLeftX + x, topLeftZ - y, currentVertexHeight);
			meshData.uvs[vertexIndex] = FVector2D(x / (float)width, y / (float)height);
			meshData.normals[vertexIndex] = FVector(1, 0, 0);
			meshData.tangents[vertexIndex] = FProcMeshTangent(0, 1, 0);
			meshData.vertexColors[vertexIndex] = FLinearColor(0.75, 0.75, 0.75, 1.0);
			
			// Check if we are still one vertex away from the bottom and right boundaries:
			if (x < width - 1 && y < height - 1) {
				// Add the two triangles that make up each square tile of mesh:
				meshData.AddTriangle(vertexIndex, vertexIndex + width + 1, vertexIndex + width);
				meshData.AddTriangle(vertexIndex + width + 1, vertexIndex, vertexIndex + 1);
			}
			
			vertexIndex++;
		}
	}

	ProceduralMesh->CreateMeshSection_LinearColor(0, meshData.vertices, meshData.triangles, meshData.normals, meshData.uvs, meshData.vertexColors, meshData.tangents, true);

	// Enable collision data:
	ProceduralMesh->ContainsPhysicsTriMeshData(true);

}


void AGeneratedMesh::UpdateTerrainMesh() {
	int32 index = 0;

	for (auto& vertex : meshData.vertices) {

		float newSampleX = vertex.X / scale;
		float newSampleY = vertex.Y / scale;

		float newPerlinValue = FMath::PerlinNoise2D(FVector2D(newSampleX, newSampleY));

		vertex = FVector(vertex.X, vertex.Y, newPerlinValue);
		index++;
	}

	ProceduralMesh->UpdateMeshSection_LinearColor(0, meshData.vertices, meshData.normals, meshData.uvs, meshData.vertexColors, meshData.tangents);

}
