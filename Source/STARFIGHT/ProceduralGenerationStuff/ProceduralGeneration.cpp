// Fill out your copyright notice in the Description page of Project Settings.


#include "ProceduralGeneration.h"

ProceduralGeneration::ProceduralGeneration()
{
}

ProceduralGeneration::~ProceduralGeneration()
{
}

struct vector2D
{
	vector2D(int32 x, int32 y) : X(x), Y(y) {}
	int32 X;
	int32 Y;
};

struct vector3D
{
	vector3D(int32 x, int32 y, int32 z) : X(x), Y(y), Z(z) {}
	int32 X;
	int32 Y;
	int32 Z;
};
 
void ProceduralGeneration::GenerateNoiseMap(std::vector<float>& noiseMap, const int32& mapChunkSize, int32& seed, FVector2D& offset, float& noiseScale, int& octaves, float& persistance, float& lacurnarity)
{
	std::vector<vector2D> octaveOffsets;

	FRandomStream prng = FRandomStream(seed);
	for (int32 i = 0; i < octaves; i++)
	{
		float offsetX = prng.FRandRange(-1000000, 1000000) + offset.X;
		float offsetY = prng.FRandRange(-100000, 100000) + offset.Y;
		octaveOffsets.push_back(vector2D(offsetX, offsetY));
	}

	if (noiseScale <= 0) {
		noiseScale = 0.0001f;
	}

	float maxNoiseHeight = 0;
	float minNoiseHeight = 1;

	float halfWidth = mapChunkSize / 2.0f;
	float halfHeight = mapChunkSize / 2.0f;

	// MAKE HLSL:
	for (int i1 = 0; i1 < mapChunkSize * mapChunkSize; i1++)
	{
		int x = i1 % mapChunkSize;
		int y = std::floor(i1 / mapChunkSize);
		
		float amplitude = 1;
		float frequency = 1;
		float noiseHeight = 0;

		// Apply Octaves
		for (int i2 = 0; i2 < octaves; i2++)
		{
			float sampleX = (x - halfWidth) / noiseScale * frequency + octaveOffsets[i2].X;
			float sampleY = (y - halfHeight) / noiseScale * frequency + octaveOffsets[i2].Y;

			float perlinValue = FMath::PerlinNoise2D(FVector2D(sampleX, sampleY)) * 2 - 1; // we multiply by '2' then subtract '1' so that our values can be negative!!
			noiseHeight += perlinValue * amplitude;

			amplitude *= persistance;  // Decreases with each octave
			frequency *= lacurnarity; // multiplied by 'lacurnarity' so the frequency increases each octave (since lacurnarity is always greater than 1)
		}

		// BIG flaw with this code; for the inverse lerp to take full effect, we need to be doing this check after we've filled the map with it's unfiltered perlin values...
		if (noiseHeight > maxNoiseHeight)
		{
			maxNoiseHeight = noiseHeight;
		}
		else if (noiseHeight < minNoiseHeight)
		{
			minNoiseHeight = noiseHeight;
		}

		noiseMap.push_back(noiseHeight);

		// Normalize all of our height values between the high and low height values encountered
		noiseMap[i1] = InverseLerp(minNoiseHeight, maxNoiseHeight, noiseMap[i1]);
		//UE_LOG(LogTemp, Warning, TEXT("		noiseMap[%d] == %d"), i1, noiseMap[i1]);
	}
}


void ProceduralGeneration::ApplyFalloffMap(std::vector<float>& noiseMap, const int32& mapChunkSize, float& a, float& b, float& c)
{

	for (int i = 0; i < mapChunkSize * mapChunkSize; i++)
	{
		int xi = i % mapChunkSize;
		int yi = std::floor(i / mapChunkSize);

		// NOTE: revisit... don't understand:
		float x = xi / (float)mapChunkSize * 2 - 1;
		float y = yi / (float)mapChunkSize * 2 - 1;

		// Chooses the greatest coordinate (x OR y) which will denote which one is closer to the center of the terrain grid:
		//float value = FMath::Max(FMath::Abs(x), FMath::Abs(y));
		float value = FMath::Sqrt(FMath::Pow(x, 2) + FMath::Pow(y, 2));

		noiseMap[i] = FMath::Clamp(noiseMap[i] - calculateFalloff(value, a, b, c), 0.0f, 1.0f);
	}
}

void ProceduralGeneration::ApplyErosionMap(std::vector<float>& noiseMap, const int32& mapChunkSize, const int32& seed, int32& dropletLifetime, const int32& numIterations)
{
	FRandomStream prng = FRandomStream(seed);

	for (int iteration = 0; iteration < numIterations; iteration++)
	{
		// Create Water Droplet at a random point on the map:
		float x = prng.FRandRange(0, mapChunkSize - 1);
		float y = prng.FRandRange(0, mapChunkSize - 1);
		//TArray<FArray2D> direction; // two dimensional normalized float vector 
		float velocity = 0;
		float water = 0;
		float sediment = 0;

		for (int lifetime = 0; lifetime < dropletLifetime; lifetime++)
		{
			// Convert the generated 'x' and 'y' coordinates to the corresponding index
			//int x = i % mapChunkSize;
			//int y = std::floor(i / mapChunkSize);

			// Calculate droplet's height and the direction of the flow with bilinear interpolation of surrounding heights
			//float height = GeneratedMap[y].secondArray[x];



			// Update the droplet's position (move 1 unit regardless of speed so as not to skip over sections fo the map)

			// Find the droplet's new height and calculate the 'deltaHeight'

			// Calculate the droplet's sediment capacity (higher when moving fast down a slop and contains lots of water)

			// - If carrying more sediment than capacity, or if flowing up a slope:
			// deposite a fraction of the sediment to the surrounding nodes (with bilinear interpolation)

			// - Otherwise:
			// Erode a fraction of the droplet's remaining capacity from teh soil, distributed over the radius of the droplets
			// NOTE: don't erode more than deltaHeight to avoid digging holes behind the droplet and creating spikes

			// Update droplet's speed based on deltaHeight
			// Evaporate a fraction of the droplet's water

		}
	}
}



void ProceduralGeneration::CalculateNormalsANDTangents(FGeneratedMeshData meshData)
{
	TArray<FVector> vertices = meshData.vertices;
	TArray<int32> triangles = meshData.triangles;

	std::vector<vector3D> normals;
}



float ProceduralGeneration::calculateWeightCurve(float vertexHeight, float exponent) {
	float output = pow(vertexHeight, exponent);
	return output;
}

float ProceduralGeneration::InverseLerp(float min, float max, float value) {
	float output = (value - min) / (max - min);
	return output;
}

float ProceduralGeneration::calculateFalloff(float value, float a, float b, float c) {
	float output = (FMath::Pow(value, a) / (c * FMath::Pow(value, a) + FMath::Pow((b - b * value), a)));
	return output;
}


auto printVector = [](std::vector<int32> vect) {std::string out = ""; for (int32 elem : vect) { out += "'"; out += elem; out += "'"; out += ", "; } };