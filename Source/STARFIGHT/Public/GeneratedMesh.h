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

	UPROPERTY()
		int32 MapChunkSize = 121; // 241NOTE: This value - 1 must have factors of 2, 4, 6, 8, 10, & 12
	UPROPERTY()
		FGeneratedMeshData MeshData;
	UPROPERTY()
		TArray<FArray2D> NoiseMap;
	UPROPERTY()
		TArray<FArray2D> FalloffMap;

	UPROPERTY(EditAnywhere, Category = "Generated Mesh Material")
		UMaterialInterface* BaseMaterial;
	
	UPROPERTY(EditAnywhere, Category = "Generated Mesh LODS")
		int32 LevelOfDetail = 0;

	UPROPERTY(EditAnywhere, Category = "Generated Mesh World Position")
		int32 Seed = 1;
	UPROPERTY(EditAnywhere, Category = "Generated Mesh World Position")
		FVector2D Offset;

	UPROPERTY(EditAnywhere, Category = "Generated Mesh Properties")
		float NoiseScale = 27.6f; // 1
	UPROPERTY(EditAnywhere, Category = "Generated Mesh Properties")
		int Octaves = 4.0f; // 1
	UPROPERTY(EditAnywhere, Category = "Generated Mesh Properties")
		float Persistance = 0.5f; // 0.5
	UPROPERTY(EditAnywhere, Category = "Generated Mesh Properties")
		float Lacunarity = 2.0f; // 12

	UPROPERTY(EditAnywhere, Category = "Generated Mesh Properties")
		float HeightMultiplier = 30.0f; // 70.0
	UPROPERTY(EditAnywhere, Category = "Generated Mesh Properties")
		float WeightCurveExponent = 2.6f;

	UPROPERTY(EditAnywhere, Category = "Falloff Map Properties")
		float A = 1.0f;
	UPROPERTY(EditAnywhere, Category = "Falloff Map Properties")
		float B = 100.0f;
	UPROPERTY(EditAnywhere, Category = "Falloff Map Properties")
		float C = 1.7f;


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

	
	void GenerateTerrainMesh(FGeneratedMeshData& meshData, TArray<FArray2D>& noiseMap, TArray<FArray2D>& falloffMap, int32& mapChunkSize, int32& seed, FVector2D& offset, int32& levelOfDetail, float& noiseScale, int& octaves, float& persistance, float& lacunarity, float& heightMultiplier, float& weightCurveExponent, float& a, float& b, float& c);
	
	UFUNCTION()
		float calculateWeightCurve(float vertexHeight, float exponent);


	/*void UpdateTerrainMesh();*/
};
