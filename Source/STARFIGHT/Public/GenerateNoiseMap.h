// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MeshData.h"
#include "Math.h"
#include "Math/RandomStream.h"
#include "Engine/Classes/Materials/Material.h"

#include "GenerateNoiseMap.generated.h"


USTRUCT(BlueprintType)
struct FArray2D {

	GENERATED_BODY()

	// Default Contstructor:
	FORCEINLINE FArray2D();

	UPROPERTY()
		TArray<float> secondArray;

	float operator[] (float i) {
		return secondArray[i];
	}

	void Add(float y) {
		secondArray.Add(y);
	}
};

FArray2D::FArray2D() {}



UCLASS(Blueprintable)
class STARFIGHT_API UGenerateNoiseMap : public UBlueprintFunctionLibrary {

	GENERATED_BODY()


public:
	UFUNCTION()
		static TArray<FArray2D> GenerateNoiseMap(const int32& mapChunkSize, int32& seed, FVector2D& offset, float& noiseScale, int& octaves, float& persistance, float& lacurnarity);

	UFUNCTION()
		static TArray<FArray2D> GenerateFalloffMap(const int32& mapChunkSize, float& a, float& b, float& c);



	UFUNCTION(BlueprintCallable)
		static FGeneratedMeshData GenerateProceduralMeshData(const int32 mapChunkSize, int32 seed, FVector2D offset, int32 levelOfDetail, float noiseScale, int octaves, float persistance, float lacunarity, float heightMultiplier, float weightCurveExponent, float a, float b, float c);
	


	UFUNCTION(BlueprintCallable)
		static FTransform calculateWorldTransform(FVector location, FVector normal, float maxTilt, float maxRotation, FRandomStream seed);

	UFUNCTION(BlueprintCallable)
		static bool calculateSlopeAvailability(float maxSlope, float minSlope, FVector inputNormal);

	UFUNCTION(BlueprintCallable)
		static bool calculateHeightAvailability(float maxHeight, float minHeight, FVector spawnLocation);
	

	UFUNCTION()
		static void CalculateNormalsANDTangents(const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector2D>& UVs, TArray<FVector>& Normals, TArray<FProcMeshTangent>& Tangents);



	UFUNCTION()
		static float calculateWeightCurve(float vertexHeight, float exponent);

	UFUNCTION()
		static float InverseLerp(float min, float max, float value);

	UFUNCTION()
		static float calculateFalloff(float value, float a, float b, float c);
	
};
