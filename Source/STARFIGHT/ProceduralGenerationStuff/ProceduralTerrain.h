// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "RHIGPUReadback.h"

#include "ShaderModule/Public/NormalsAndTangentsCS.h"
#include "ProceduralGeneration.h"
#include "MeshData.h"

#include "ProceduralTerrain.generated.h"

UCLASS()
class STARFIGHT_API AProceduralTerrain : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AProceduralTerrain();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void OnConstruction(const FTransform& Transform) override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;


	// Components:
	UPROPERTY(VisibleAnywhere, Category = "Mesh Components")
		UProceduralMeshComponent* _ProceduralTerrainMesh;

	UPROPERTY(EditAnywhere, Category = "Mesh Components")
		UMaterialInterface* _TerrainMaterial;


	// Mesh Parameters:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "1. Mesh Parameters")
		int32 MapChunkSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "1. Mesh Parameters")
		int32 Seed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "1. Mesh Parameters")
		FVector2D Offset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "1. Mesh Parameters")
		int32 LevelOfDetail;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "1. Mesh Parameters")
		FVector SpawnLocation;
 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "1. Mesh Parameters")
		FVector MeshScale;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "1. Mesh Parameters")
		bool UseRandomSeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "1. Mesh Parameters")
		int32 SeedRangeMAX;
	

	// Noise Parameters:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise Parameters")
		float NoiseScale;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise Parameters")
		int32 Octaves;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise Parameters")
		float Persistence;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise Parameters")
		float Lacurnarity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise Parameters")
		float HeighMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise Parameters")
		float WeightCurveExponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise Parameters")
		float A;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise Parameters")
		float B;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise Parameters")
		float C;


	// Erosion Parameters:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Erosion Parameters")
		int32 NumErosionIterations;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Erosion Parameters")
		int32 ErosionBrushRadius;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Erosion Parameters")
		int32 MaxDropletLifetime; // maxLifetime

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Erosion Parameters")
		float SedimentCapacityFactor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Erosion Parameters")
		float MinSedimentCapacity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Erosion Parameters")
		float DepositSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Erosion Parameters")
		float ErodeSpeed;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Erosion Parameters")
		float EvaporateSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Erosion Parameters")
		float Gravity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Erosion Parameters")
		float StartSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Erosion Parameters")
		float StartWater;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Erosion Parameters")
		float Inertia;


	// Execute:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Execute")
		bool GENERATE_IN_EDITOR;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Execute")
		bool REGENERATE_MAP;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Execute")
		bool APPLY_FALLOFF_MAP;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Execute")
		bool APPLY_EROSION_MAP;


	static void BuildTerrainMesh(UProceduralMeshComponent* proceduralTerrainMesh, std::shared_ptr<FGeneratedMeshData> meshData);


	static void ExecuteProceduralMeshGeneration(FProceduralMeshInputs& Inputs);

	static void GenerateProceduralMeshData(std::shared_ptr<FGeneratedMeshData> MeshData, FProceduralMeshInputs& Inputs);

	static void AddNormalsAndTangents(float*& normals, float*& tangents, float*& noiseMap, const int32& mapChunkSize);

};
