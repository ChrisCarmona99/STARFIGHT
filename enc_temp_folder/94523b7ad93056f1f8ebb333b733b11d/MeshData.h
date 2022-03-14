// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MeshData.generated.h"


USTRUCT(BlueprintType)
struct FGeneratedMeshData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	int32 X = 0;

	UPROPERTY(BlueprintReadWrite)
	int32 Y = 0;


};