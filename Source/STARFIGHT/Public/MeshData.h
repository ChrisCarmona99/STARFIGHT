// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MeshData.generated.h"


USTRUCT(BlueprintType)
struct FGeneratedMeshData
{
	GENERATED_BODY()

	// Default Constructor DECLARATION:
	FORCEINLINE FGeneratedMeshData();
	// Main Constructor DECLARATION:
	explicit FORCEINLINE FGeneratedMeshData(int32 InX, int32 InY);

	UPROPERTY(BlueprintReadWrite)
	int32 meshWidth = 4;
	UPROPERTY(BlueprintReadWrite)
	int32 meshHeight = 4;

	UPROPERTY(BlueprintReadWrite)
	TArray<FVector> Vertices;
	UPROPERTY(BlueprintReadWrite)
	TArray<int32> Triangles;

	UPROPERTY(BlueprintReadWrite)
	int32 triangleIndex;


	// Add a triangle:
	void addTriangle(int32 a, int32 b, int32 c) {
		Triangles[triangleIndex] = a;
		Triangles[triangleIndex + 1] = b;
		Triangles[triangleIndex + 2] = c;
		triangleIndex += 3;
	}

};

// Default Constructor IMPLEMENTATION:
FORCEINLINE FGeneratedMeshData::FGeneratedMeshData() {
	Vertices.Init(FVector(), meshWidth * meshHeight);
	Triangles.Init(int32(), (meshWidth - 1) * (meshHeight - 1) * 6);
	triangleIndex = 0;
}

// Main Constructor IMPLEMENTATION:
FORCEINLINE FGeneratedMeshData::FGeneratedMeshData(const int32 inputMeshWidth, const int32 inputMeshHeight) : meshWidth(inputMeshWidth), meshHeight(inputMeshHeight) {
	Vertices.Init(FVector(), meshWidth * meshHeight);
	Triangles.Init(int32(), (meshWidth - 1) * (meshHeight - 1) * 6);
	triangleIndex = 0;
}