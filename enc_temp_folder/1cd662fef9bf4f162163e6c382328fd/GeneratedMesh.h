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

	UPROPERTY(EditAnywhere, Category = "Generated Mesh Material")
		UMaterialInterface* BaseMaterial;

	UPROPERTY()
		int32 mapChunkSize = 121; // NOTE: This value must have factors of 2, 4, 6, 8, 10, & 12
	UPROPERTY(EditAnywhere, Category = "Generated Mesh LODS")
		int32 levelOfDetail = 1;


	/*UPROPERTY()
		TArray<FArray2D> noiseMap;*/
	UPROPERTY()
		FGeneratedMeshData meshData;


	UPROPERTY(EditAnywhere, Category = "Generated Mesh Properties")
		float noiseScale = 20.0f; // 1
	UPROPERTY(EditAnywhere, Category = "Generated Mesh Properties")
		int octaves = 4.0f; // 1
	UPROPERTY(EditAnywhere, Category = "Generated Mesh Properties")
		float persistance = 0.5f; // 0.5
	UPROPERTY(EditAnywhere, Category = "Generated Mesh Properties")
		float lacunarity = 1.0f; // 1

	UPROPERTY(EditAnywhere, Category = "Generated Mesh Properties")
		float heightMultiplier = 70.0f; // 1
	UPROPERTY(EditAnywhere, Category = "Generated Mesh Properties")
		float weightCurveExponent = 6.0f;


	// Sets default values for this actor's properties
	AGeneratedMesh();


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	// Called when any property is updated in the Editor:
	virtual void OnConstruction(const FTransform& Transform) override;


public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void PostActorCreated();
	void PostLoad();

	void GenerateTerrainMesh();
	void UpdateTerrainMesh();

	float calculateWeightCurve(float vertexHeight, float exponent);
};
