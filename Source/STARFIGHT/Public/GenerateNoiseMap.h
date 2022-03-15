// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Math.h"
#include "Engine/Classes/Materials/Material.h"

#include "GenerateNoiseMap.generated.h"


USTRUCT()
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



UCLASS()
class STARFIGHT_API UGenerateNoiseMap : public UBlueprintFunctionLibrary {

	GENERATED_BODY()


public:
	UFUNCTION()
		static TArray<FArray2D> GenerateNoiseMap(int32 mapWidth, int32 mapHeight, float scale); // GENERATES OUR NOISE MAP
	
	UFUNCTION()
		static TArray<FArray2D> UpdateNoiseMap(int32 mapWidth, int32 mapHeight, float scale); // UPDATES OUR NOISE MAP (in editor)
};
