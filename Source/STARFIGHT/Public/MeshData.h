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
		int32 meshWidth;
	UPROPERTY(BlueprintReadWrite)
		int32 meshHeight;

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
		TArray<FColor> vertexColorsNEW;

	UPROPERTY(BlueprintReadWrite)
		int32 triangleIndex = 0;


	// Add a triangle:
	void AddTriangle(int32 a, int32 b, int32 c) {
		triangles.Add(a);
		triangles.Add(b);
		triangles.Add(c);
	}

};

// Default Constructor IMPLEMENTATION (not really needed but here for programmatic reasons...):
FORCEINLINE FGeneratedMeshData::FGeneratedMeshData() {
}

// Main Constructor IMPLEMENTATION:
FORCEINLINE FGeneratedMeshData::FGeneratedMeshData(const int32 inputMeshWidth, const int32 inputMeshHeight) : meshWidth(inputMeshWidth), meshHeight(inputMeshHeight) {
}
