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
	/*
	* 	All of the subsequent values, while called in our 'GenerateNoiseMap', are defined here BECAUSE we call 'GenerateNoiseMap' ONLY IN 'GenerateMesh.cpp'...
	*	THUS... they are instantiated in 'GeneratedMesh.h' since we only need these variables in 'GeneratedMesh.cpp', thus making it easier to manipulate them
	*		in the editor during runtime!
	*/
	UPROPERTY(EditAnywhere, Category="Generated Mesh Size")
		int32 mapWidth = 50;
	UPROPERTY(EditAnywhere, Category = "Generated Mesh Size")
		int32 mapHeight = 50;
	UPROPERTY(EditAnywhere, Category = "Generated Mesh Properties")
		float noiseScale = 1;
	UPROPERTY(EditAnywhere, Category = "Generated Mesh Properties")
		int octaves = 2;
	UPROPERTY(EditAnywhere, Category = "Generated Mesh Properties")
		float persistance = 0.5;
	UPROPERTY(EditAnywhere, Category = "Generated Mesh Properties")
		float lacunarity = 1; // Should always be GREATER than 1;


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
