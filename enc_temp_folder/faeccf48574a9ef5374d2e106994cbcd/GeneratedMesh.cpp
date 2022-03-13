// Fill out your copyright notice in the Description page of Project Settings.


#include "GeneratedMesh.h"


// Sets default values
AGeneratedMesh::AGeneratedMesh()
{
	GeneratedMesh = CreateDefaultSubobject<UProceduralMeshComponent>("GeneratedMesh");
	RootComponent = GeneratedMesh;

 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false; // ORIGINALY SET TO TRUE

}

// Create Mesh:
void AGeneratedMesh::CreateMesh() {

	Vertices.Add(FVector(-50, 0, 50));
	Vertices.Add(FVector(-50, 0, -50));
	Vertices.Add(FVector(50, 0, 50));
	Vertices.Add(FVector(50, 0, -50));

	UVs.Add(FVector2D(0, 0));
	UVs.Add(FVector2D(0, 1));
	UVs.Add(FVector2D(1, 0));
	UVs.Add(FVector2D(1, 1));

	// Triangle 1:
	Triangles.Add(0);
	Triangles.Add(1);
	Triangles.Add(2);

	// Triangle 2:
	Triangles.Add(2);
	Triangles.Add(1);
	Triangles.Add(3);

	// ACTUALLY CREATES OUR MESH WITH OUR DEFINED Vertices, Triangles, AND UVs:
	GeneratedMesh->CreateMeshSection(0, Vertices, Triangles, TArray<FVector>(), UVs, TArray<FColor>(), TArray<FProcMeshTangent>(), true);

	// Sets the Material:
	if (Material) {
		GeneratedMesh->SetMaterial(0, Material);
	}

}

// Called when the game starts or when spawned
void AGeneratedMesh::BeginPlay()
{
	Super::BeginPlay();

	CreateMesh();
	
}

// Called every frame
void AGeneratedMesh::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}
