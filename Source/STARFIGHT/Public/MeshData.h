// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"
#include "MeshData.generated.h"


USTRUCT(BlueprintType)
struct FGeneratedMeshData {
	GENERATED_BODY()

	// Default Constructor DECLARATION:
	FORCEINLINE FGeneratedMeshData();
	// Main Constructor DECLARATION:
	explicit FORCEINLINE FGeneratedMeshData(int32 inputMeshWidth, int32 inputMeshHeight);

	UPROPERTY(BlueprintReadWrite)
		int32 meshWidth = 4;
	UPROPERTY(BlueprintReadWrite)
		int32 meshHeight = 4;

	UPROPERTY(BlueprintReadWrite)
		TArray<FVector> vertices;
	UPROPERTY(BlueprintReadWrite)
		TArray<int32> triangles;
	UPROPERTY(BlueprintReadWrite)
		TArray<FVector2D> uvs;
	UPROPERTY(BlueprintReadWrite)
		TArray<FVector> normals;
	UPROPERTY(BlueprintReadWrite)
		TArray<FProcMeshTangent> tangents;
	UPROPERTY(BlueprintReadWrite)
		TArray<FLinearColor> vertexColors;

	UPROPERTY(BlueprintReadWrite)
		int32 triangleIndex = 0;


	// Add a triangle:
	void AddTriangle(int32 a, int32 b, int32 c) {
		/*UE_LOG(LogTemp, Warning, TEXT("triangle index: %d"), triangleIndex);*/
		triangles[triangleIndex] = a;
		triangles[triangleIndex + 1] = b;
		triangles[triangleIndex + 2] = c;
		triangleIndex += 3;
	}



};

// Default Constructor IMPLEMENTATION (not really needed but here for programmatic reasons...):
FORCEINLINE FGeneratedMeshData::FGeneratedMeshData() {

	vertices.Init(FVector(), meshWidth * meshHeight);
	uvs.Init(FVector2D(), meshWidth * meshHeight);
	triangles.Init(int32(), (meshWidth - 1) * (meshHeight - 1) * 6);

	normals.Init(FVector(), meshWidth * meshHeight);
	tangents.Init(FProcMeshTangent(), meshWidth * meshHeight);
	vertexColors.Init(FLinearColor(), meshWidth * meshHeight);
}

// Main Constructor IMPLEMENTATION:
FORCEINLINE FGeneratedMeshData::FGeneratedMeshData(const int32 inputMeshWidth, const int32 inputMeshHeight) : meshWidth(inputMeshWidth), meshHeight(inputMeshHeight) {

	vertices.Init(FVector(), meshWidth * meshHeight);
	uvs.Init(FVector2D(), meshWidth * meshHeight);
	triangles.Init(int32(), (meshWidth - 1) * (meshHeight - 1) * 6);

	normals.Init(FVector(), meshWidth * meshHeight);
	tangents.Init(FProcMeshTangent(), meshWidth * meshHeight);
	vertexColors.Init(FLinearColor(), meshWidth * meshHeight);
}
