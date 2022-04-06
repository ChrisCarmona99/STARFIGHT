// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Math.h"
//#include "MeshData.h"
#include "GenerateNoiseMap.h"

#include "GeneratedMesh.generated.h"

UCLASS()
class STARFIGHT_API AGeneratedMesh : public AActor {
	GENERATED_BODY()


public:
	UPROPERTY()
		UProceduralMeshComponent* ProceduralMesh;

	UPROPERTY()
		int32 MapChunkSize_0 = 241; // 241NOTE: This value - 1 must have factors of 2, 4, 6, 8, 10, & 12
	UPROPERTY()
		FGeneratedMeshData MeshData_0;
	UPROPERTY()
		TArray<FArray2D> NoiseMap_0;
	UPROPERTY()
		TArray<FArray2D> FalloffMap_0;

	UPROPERTY(EditAnywhere, Category = "Generated Mesh Material")
		UMaterialInterface* BaseMaterial;
	
	UPROPERTY(EditAnywhere, Category = "Generated Mesh LODS")
		int32 LevelOfDetail_0 = 0;

	UPROPERTY(EditAnywhere, Category = "Generated Mesh World Position")
		int32 Seed_0 = 1; // 4234545
	UPROPERTY(EditAnywhere, Category = "Generated Mesh World Position")
		FVector2D Offset_0;

	UPROPERTY(EditAnywhere, Category = "Generated Mesh Properties")
		float NoiseScale_0 = 27.6f; // 1
	UPROPERTY(EditAnywhere, Category = "Generated Mesh Properties")
		int Octaves_0 = 4.0f; // 1
	UPROPERTY(EditAnywhere, Category = "Generated Mesh Properties")
		float Persistance_0 = 0.45f; // 0.5
	UPROPERTY(EditAnywhere, Category = "Generated Mesh Properties")
		float Lacunarity_0 = 1.9f; // 12

	UPROPERTY(EditAnywhere, Category = "Generated Mesh Properties")
		float HeightMultiplier_0 = 28.0f; // 70.0
	UPROPERTY(EditAnywhere, Category = "Generated Mesh Properties")
		float WeightCurveExponent_0 = 3.0f;

	UPROPERTY(EditAnywhere, Category = "Falloff Map Properties")
		float A_0 = 1.8f;
	UPROPERTY(EditAnywhere, Category = "Falloff Map Properties")
		float B_0 = 4.0f;
	UPROPERTY(EditAnywhere, Category = "Falloff Map Properties")
		float C_0 = 0.6f;


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

	UFUNCTION(BlueprintCallable)
		void ConstructProceduralTerrainMesh(FGeneratedMeshData& meshData, TArray<FArray2D>& noiseMap, TArray<FArray2D>& falloffMap, int32& mapChunkSize, int32& seed, FVector2D& offset, int32& levelOfDetail, float& noiseScale, int& octaves, float& persistance, float& lacunarity, float& heightMultiplier, float& weightCurveExponent, float& a, float& b, float& c);

	
	UFUNCTION()
		float calculateWeightCurve(float vertexHeight, float exponent);

};
