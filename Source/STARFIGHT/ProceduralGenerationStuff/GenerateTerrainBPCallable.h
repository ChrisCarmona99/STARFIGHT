// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "ProceduralGeneration.h"
#include "MeshData.h"

#include "GenerateTerrainBPCallable.generated.h"

/**
 * 
 */
UCLASS()
class STARFIGHT_API UGenerateTerrainBPCallable : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable)
		static FGeneratedMeshData GenerateProceduralMeshDataNEW(const int32 mapChunkSize, int32 seed, FVector2D offset, int32 levelOfDetail, 
															 float noiseScale, int octaves, float persistance, float lacunarity, float heightMultiplier,
															 float weightCurveExponent, float a, float b, float c);
};
