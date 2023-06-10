// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ShaderModule/Public/NoiseMapComputeShader.h"
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
	static void GenerateNoiseMap_OLD(float*& noiseMap, const int32& mapChunkSize, int32& seed, FVector2D& offset, float& noiseScale, int& octaveCount, float& persistance, float& lacurnarity);
	static void ApplyFalloffMap(float*& noiseMap, const int32& mapChunkSize, float& a, float& b, float& c);
	static void ApplyErosionMap(float*& noiseMap, const int32& mapChunkSize, const int32& seed, int32& dropletLifetime, const int32& numIterations);

	/*
	static void GenerateNoiseMap_OLD(std::vector<float>& noiseMap, const int32& mapChunkSize, int32& seed, FVector2D& offset, float& noiseScale, int& octaves, float& persistance, float& lacurnarity);
	static void ApplyFalloffMap_OLD(std::vector<float>& noiseMap, const int32& mapChunkSize, float& a, float& b, float& c);
	static void ApplyErosionMap_OLD(std::vector<float>& noiseMap, const int32& mapChunkSize, const int32& seed, int32& dropletLifetime, const int32& numIterations);
	*/

	static float calculateWeightCurve(float vertexHeight, float exponent);
	static float InverseLerp(float min, float max, float value);
	static float calculateFalloff(float value, float a, float b, float c);
};
