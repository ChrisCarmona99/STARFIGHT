// Fill out your copyright notice in the Description page of Project Settings.


#include "GeneratedMesh.h"


// CONSTRUCTOR:
AGeneratedMesh::AGeneratedMesh()
{
	ProceduralMesh = CreateDefaultSubobject<UProceduralMeshComponent>("GeneratedMesh");
	RootComponent = ProceduralMesh;

	noiseMap = UGenerateNoiseMap::GenerateNoiseMap(mapWidth, mapHeight, noiseScale, octaves, persistance, lacunarity);
	meshData = FGeneratedMeshData(mapWidth, mapHeight);

 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false; // ORIGINALY SET TO TRUE

}

// Called when the game starts or when spawned
void AGeneratedMesh::BeginPlay() {
	Super::BeginPlay();
}

void AGeneratedMesh::OnConstruction(const FTransform& Transform) {
	/*
	If you get the same 'array index' error again... its becuase you are calling this function, which fucks up any spawned mesh in the world...
	*/
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


// CURRENT ERROR IS CAUSE A 50 by 50 GENERATED MESH IS IN THE WORLD... AND I'M CALLING ONE OF OUR MESH GENERATION FUNCTIONS ON SAID MESH..... bruh come on...

void AGeneratedMesh::UpdateTerrainMesh() {

	int32 index = 0;

	if (mapWidth < 1) { mapWidth = 1; }
	if (mapHeight < 1) { mapHeight = 1; }
	if (noiseScale <= 0) { noiseScale = 0.0001f; }
	if (lacunarity < 1) { lacunarity = 1; }
	if (octaves < 0) { octaves = 0; } else if (octaves > 7) { octaves = 7; }
	if (persistance < 0) { persistance = 0; } else if (persistance > 1) { persistance = 1; }

	TArray<FArray2D> newNoiseMap = UGenerateNoiseMap::GenerateNoiseMap(mapWidth, mapHeight, noiseScale, octaves, persistance, lacunarity);

	int32 vertexIndex = 0;

	for (int32 y = 0; y < mapWidth; y++) {
		for (int32 x = 0; x < mapHeight; x++) {

			float newVertexHeight = newNoiseMap[y].secondArray[x];

			meshData.vertices[vertexIndex] = FVector(meshData.vertices[vertexIndex].X, meshData.vertices[vertexIndex].Y, newVertexHeight);

			vertexIndex++;
		}
	}

	ProceduralMesh->UpdateMeshSection_LinearColor(0, meshData.vertices, meshData.normals, meshData.uvs, meshData.vertexColors, meshData.tangents);

}
