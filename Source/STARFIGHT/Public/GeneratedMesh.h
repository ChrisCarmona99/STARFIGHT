// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Math.h"
#include "MeshData.h"
#include "GenerateNoiseMap.h"

#include "GeneratedMesh.generated.h"

UCLASS()
class STARFIGHT_API AGeneratedMesh : public AActor {
	GENERATED_BODY()

public:
	UPROPERTY()
		UProceduralMeshComponent* ProceduralMesh;

	UPROPERTY(EditAnywhere, Category="Generated Mesh Properties")
		int32 mapWidth = 50;
	UPROPERTY(EditAnywhere, Category = "Generated Mesh Properties")
		int32 mapHeight = 50;
	UPROPERTY(EditAnywhere, Category = "Generated Mesh Properties")
		float scale = 1;


	UPROPERTY()
		TArray<FArray2D> noiseMap;
	UPROPERTY()
		FGeneratedMeshData meshData;
	

	// Sets default values for this actor's properties
	AGeneratedMesh();

protected:
	UPROPERTY()
		UMaterialInterface* Material;

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	// Called when any property is updated in the Editor:
	virtual void OnConstruction(const FTransform& Transform) override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void PostActorCreated();
	void PostLoad();

	void GenerateTerrainMesh(int32 inputWidth, int32 inputHeight); // GENERATES OUR TERRAIN MESH
	void UpdateTerrainMesh();

};
