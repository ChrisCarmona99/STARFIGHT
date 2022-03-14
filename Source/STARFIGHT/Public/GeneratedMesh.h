// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Math.h"
#include "ProceduralMeshComponent.h"
#include "MeshData.h"

#include "GeneratedMesh.generated.h"

UCLASS()
class STARFIGHT_API AGeneratedMesh : public AActor
{
	GENERATED_BODY()
		UPROPERTY(VisibleAnywhere)
		TArray<FVector> normals;
		UPROPERTY(VisibleAnywhere)
		TArray<FVector2D> UV0;
		UPROPERTY(VisibleAnywhere)
		TArray<FProcMeshTangent> tangents;
		UPROPERTY(VisibleAnywhere)
		TArray<FLinearColor> vertexColors;

		UPROPERTY(VisibleAnywhere)
		UProceduralMeshComponent* ProceduralMesh;

		UPROPERTY(VisibleAnywhere)
		float TEMP_heightMap;
	
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
	void GenerateTerrainMesh(float heightMap);

};



//USTRUCT()
//struct FGeneratedMeshData {
//	GENERATED_BODY()
//
//	explicit FORCEINLINE FGeneratedMeshData(int32 inputMeshWidth, int32 inputMeshHeight);
//
//	UPROPERTY()
//		int32 meshWidth;
//	UPROPERTY()
//		int32 meshHeight;
//
//	UPROPERTY(EditAnywhere)
//		TArray<FVector> Vertices;
//	UPROPERTY(EditAnywhere)
//		TArray<int32> Triangles;
//
//	UPROPERTY(EditAnywhere)
//		int32 triangleIndex;
//
//	
//
//	// Add a triangle:
//	void addTriangle(int32 a, int32 b, int32 c) {
//
//		Triangles[triangleIndex] = a;
//		Triangles[triangleIndex + 1] = b;
//		Triangles[triangleIndex + 2] = c;
//
//		triangleIndex += 3;
//	}
//};
//
////FORCEINLINE FGeneratedMeshData::FGeneratedMeshData(const int32 inputMeshWidth, const int32 inputMeshHeight) : meshWidth(inputMeshWidth), meshHeight(inputMeshHeight) {
////	Vertices.Init(FVector(), meshWidth * meshHeight);
////	Triangles.Init(int32(), (meshWidth - 1) * (meshHeight - 1) * 6);
////	triangleIndex = 0;
////}
//
//FGeneratedMeshData::FGeneratedMeshData(int32 inputMeshWidth, int32 inputMeshHeight) : meshWidth(inputMeshWidth), meshHeight(inputMeshHeight)
//{
//}