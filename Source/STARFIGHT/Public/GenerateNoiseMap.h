// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Landscape.h"
#include "Math.h"
#include "Engine/Classes/Materials/Material.h"

#include "GenerateNoiseMap.generated.h"

/**
 * 
 */
UCLASS()
class STARFIGHT_API UGenerateNoiseMap : public UBlueprintFunctionLibrary {

	GENERATED_BODY()
		UFUNCTION(BlueprintCallable, Category="Procedural Generation")
		static FVector2D GenerateNoiseMap(int mapWidth, int mapHeight, float scale, ALandscape *GeneratedLandscape);

//public:
//	UPROPERTY(VisibleAnywhere)
//		UStaticMeshComponent* MeshComponent;
//
//	UPROPERTY(VisibleAnywhere)
//		UMaterial* StoredMaterial;
//
//	UPROPERTY(VisibleAnywhere)
//		UMaterialInstanceDynamic* DynamicMaterialInst;
};

USTRUCT()
struct FArray_2D {
	GENERATED_USTRUCT_BODY()

public:
		UPROPERTY()
		TArray<float> secondArray;

		float operator[] (float i) {
			return secondArray[i];
		}

		void Add(float y) {
			secondArray.Add(y);
		}

		// Constructor:
		/*FArray_2D() {
			constructor secondArray = TArray;
		}*/
};