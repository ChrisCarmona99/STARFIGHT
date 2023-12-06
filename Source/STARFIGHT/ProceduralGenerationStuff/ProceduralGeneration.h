// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ShaderModule/Public/NoiseMapComputeShader.h"
#include "ShaderModule/Public/ErosionMapCS.h"
#include "MeshData.h"

#include "CoreMinimal.h"


/**
 * 
 */
class STARFIGHT_API ProceduralGeneration
{
public:
	ProceduralGeneration();
	~ProceduralGeneration();

	static void GenerateNoiseMap(float*& noiseMap, const int32& mapChunkSize, int32& seed, FVector2D& offset, float& noiseScale, int& octaves, float& persistance, float& lacurnarity, float& heightMultiplier, float& weightCurveExponent);
	static void ApplyFalloffMap(float*& noiseMap, const int32& mapChunkSize, float& a, float& b, float& c);
	static void ApplyErosionMap(float*& noiseMap, const int32& seed, const int32& mapChunkSizeWithBorder, const int32& mapChunkSize, const int32& numErosionIterations, int32& erosionBrushRadius, int32& maxDropletLifetime, float& sedimentCapacityFactor, float& minSedimentCapacity,
		float& depositSpeed, float& erodeSpeed, float& evaporateSpeed, float& gravity, float& startSpeed, float& startWater, float& inertia);


	static float calculateWeightCurve(float vertexHeight, float exponent);
	static float InverseLerp(float min, float max, float value);
	static float calculateFalloff(float value, float a, float b, float c);
};
