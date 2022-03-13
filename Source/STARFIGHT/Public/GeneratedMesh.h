// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Math.h"
#include "ProceduralMeshComponent.h"
#include "GeneratedMesh.generated.h"

UCLASS()
class STARFIGHT_API AGeneratedMesh : public AActor
{
	GENERATED_BODY()
		TArray<FVector> Vertices;
		TArray<int32> Triangles;
		TArray<FVector> normals;
		TArray<FVector2D> UV0;
		TArray<FProcMeshTangent> tangents;
		TArray<FLinearColor> vertexColors;

	UPROPERTY(VisibleAnywhere)
		UProceduralMeshComponent* ProceduralMesh;
	
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
	void CreateTriangle();

};
