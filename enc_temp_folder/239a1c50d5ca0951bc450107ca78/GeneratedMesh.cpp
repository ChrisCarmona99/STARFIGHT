// Fill out your copyright notice in the Description page of Project Settings.


#include "GeneratedMesh.h"


// Sets default values
AGeneratedMesh::AGeneratedMesh()
{
	ProceduralMesh = CreateDefaultSubobject<UProceduralMeshComponent>("GeneratedMesh");
	RootComponent = ProceduralMesh;

 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false; // ORIGINALY SET TO TRUE

}

// Called when the game starts or when spawned
void AGeneratedMesh::BeginPlay() {
	Super::BeginPlay();
}

// Called every frame
void AGeneratedMesh::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);
}

// Called when 'GeneratedMesh' is spawned (at runtime or when you drop it into the world in editor)
void AGeneratedMesh::PostActorCreated() {
	Super::PostActorCreated();
	GenerateTerrainMesh(TEMP_heightMap);
}

// Called when 'GeneratedMesh' is already in level and map is opened
void AGeneratedMesh::PostLoad() {
	Super::PostLoad();
	GenerateTerrainMesh(TEMP_heightMap);
}

void AGeneratedMesh::GenerateTerrainMesh(float heightMap) {

	int32 width = 10;
	int32 height = 10;

	int32 TEMP = 0;

	for (int32 y = 0; y < height; y++) {
		for (int32 x = 0; x < width; x++) {
			TEMP += 1;
		}
	}

	normals.Add(FVector(1, 0, 0));
	normals.Add(FVector(1, 0, 0));
	normals.Add(FVector(1, 0, 0));

	UV0.Add(FVector2D(0, 0));
	UV0.Add(FVector2D(10, 0));
	UV0.Add(FVector2D(0, 10));

	tangents.Add(FProcMeshTangent(0, 1, 0));
	tangents.Add(FProcMeshTangent(0, 1, 0));
	tangents.Add(FProcMeshTangent(0, 1, 0));

	

	//ProceduralMesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, normals, UV0, vertexColors, tangents, true);

	// Enable collision data:
	ProceduralMesh->ContainsPhysicsTriMeshData(true);

}


