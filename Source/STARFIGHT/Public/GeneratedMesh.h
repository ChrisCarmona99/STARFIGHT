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

	UPROPERTY(VisibleAnywhere)
		UProceduralMeshComponent* ProceduralMesh;

	UPROPERTY()
		int32 mapWidth = 10;
	UPROPERTY()
		int32 mapHeight = 10;
	UPROPERTY()
		float scale = 0.2;


	UPROPERTY(VisibleAnywhere)
		TArray<FArray2D> heightMap = UGenerateNoiseMap::GenerateNoiseMap(mapWidth, mapHeight, scale);
	
public:	
	// Sets default values for this actor's properties
	AGeneratedMesh();

protected:
	UPROPERTY(EditAnywhere, Category = "Procedural Mesh Material")
		UMaterialInterface* Material;

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void PostActorCreated();
	void PostLoad();
	void GenerateTerrainMesh(int32 inputWidth, int32 inputHeight, TArray<FArray2D> inputHeightMap);

};
