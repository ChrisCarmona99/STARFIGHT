// Fill out your copyright notice in the Description page of Project Settings.


#include "GeneratedMesh.h"


// CONSTRUCTOR:
AGeneratedMesh::AGeneratedMesh() {

	UE_LOG(LogTemp, Warning, TEXT("\n\nMesh Created\n\n"));

	ProceduralMesh = CreateDefaultSubobject<UProceduralMeshComponent>("GeneratedMesh");
	RootComponent = ProceduralMesh;

	BaseMaterial = CreateDefaultSubobject<UMaterialInterface>("BaseMaterial");

	//noiseMap = UGenerateNoiseMap::GenerateNoiseMap(mapChunkSize, noiseScale, octaves, persistance, lacunarity);
	meshData = FGeneratedMeshData(mapChunkSize, mapChunkSize);

 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false; // ORIGINALY SET TO TRUE

}

// Called when the game starts or when spawned
void AGeneratedMesh::BeginPlay() {
	Super::BeginPlay();
}

// Called when an instance of this class is placed (in editor) or spawned:
void AGeneratedMesh::OnConstruction(const FTransform& Transform) {

	UE_LOG(LogTemp, Warning, TEXT("\n\nOnConstruction CALLED:\n"));

	ProceduralMesh->SetMaterial(0, BaseMaterial);
	GenerateTerrainMesh();
	//UpdateTerrainMesh();
}

// Called every frame
void AGeneratedMesh::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);
}

// Called when 'GeneratedMesh' is spawned (at runtime or when you drop it into the world in editor)
void AGeneratedMesh::PostActorCreated() {

	UE_LOG(LogTemp, Warning, TEXT("\n\nPostActorCreated CALLED:\n"));

	Super::PostActorCreated();
	GenerateTerrainMesh();
}

// Called when 'GeneratedMesh' is already in level and map is opened
void AGeneratedMesh::PostLoad() {
	Super::PostLoad();
	//UpdateTerrainMesh();
}

void AGeneratedMesh::GenerateTerrainMesh() {
	UE_LOG(LogTemp, Warning, TEXT("\n\nGenerateTerrainMesh CALLED:\n"));

	// The following if-statements 'clamp' our noiseMap values:
	if (noiseScale <= 0) { noiseScale = 0.0001f; }
	if (lacunarity < 1) { lacunarity = 1; }
	if (octaves < 0) { octaves = 0; }
	else if (octaves > 7) { octaves = 7; }
	if (persistance < 0) { persistance = 0; }
	else if (persistance > 1) { persistance = 1; }
	if (weightCurveExponent < 1) { weightCurveExponent = 1.0f; }

	// Clamp our Level of Detail Value:
	if (levelOfDetail < 1) { levelOfDetail = 1; }
	else if (levelOfDetail > 6) { levelOfDetail = 6; }

	// Sets the starting 
	float topLeftX = (mapChunkSize - 1) / -2.f;
	float topLeftZ = (mapChunkSize - 1) / 2.f;

	// Calculates the increment for mesh LODs (ensures 'levelOfDetail' is NOT 0):
	int32 meshSimplificationIncrement = levelOfDetail == 0 ? 1 : levelOfDetail * 2;
	// Calculates correct number of vertices for our 'vertices' array:
	int32 verticesPerLine = (mapChunkSize - 1) / meshSimplificationIncrement + 1;

	TArray<FArray2D> noiseMap = UGenerateNoiseMap::GenerateNoiseMap(mapChunkSize, noiseScale, octaves, persistance, lacunarity);

	int32 vertexIndex = 0;

	UE_LOG(LogTemp, Warning, TEXT("\n\nWORKING TILL HERE 1\n"));

	// Reset all our meshData:
	/*for (int32 count = meshData.vertices.Num() - 1; count >= 0; count--) {
		meshData.vertices.RemoveAtSwap(count, 1, true);
		meshData.uvs.RemoveAtSwap(count, 1, true);
		meshData.normals.RemoveAtSwap(count, 1, true);
		meshData.tangents.RemoveAtSwap(count, 1, true);
		meshData.vertexColors.RemoveAtSwap(count, 1, true);
	}*/

	meshData.vertices.Reset();
	meshData.uvs.Reset();
	meshData.normals.Reset();
	meshData.tangents.Reset();
	meshData.vertexColors.Reset();

	UE_LOG(LogTemp, Warning, TEXT("\n\nWORKING TILL HERE 2\n"));
	UE_LOG(LogTemp, Warning, TEXT("\n\nPRE:  meshData.triangles.Num() = : %d\n"), meshData.triangles.Num());
	/*for (int32 count = meshData.triangles.Num() - 1; count >= 0; count--) {
		meshData.triangles.RemoveAtSwap(count, 1, true);
	}*/

	meshData.triangles.Reset();

	UE_LOG(LogTemp, Warning, TEXT("\n\nWORKING TILL HERE 3\n"));
	UE_LOG(LogTemp, Warning, TEXT("\n\nPRE:  meshData.triangles.Num() = : %d\n"), meshData.triangles.Num());

	for (int32 y = 0; y < mapChunkSize; y += meshSimplificationIncrement) {
		for (int32 x = 0; x < mapChunkSize; x += meshSimplificationIncrement) {

			// Define the height for the current vertex in our iteration:
			float currentVertexHeight = calculateWeightCurve(noiseMap[y].secondArray[x], weightCurveExponent) * heightMultiplier;
			
			// Add each vertex, uv, normal, tangent, & vertexColor:
			meshData.vertices.Add(FVector(topLeftX + x, topLeftZ - y, currentVertexHeight));
			meshData.uvs.Add(FVector2D(x / (float)mapChunkSize, y / (float)mapChunkSize));
			meshData.normals.Add(FVector(1, 0, 0));
			meshData.tangents.Add(FProcMeshTangent(0, 1, 0));
			meshData.vertexColorsNEW.Add(FColor(0.75, 0.75, 0.75, 1.0));
			

			// Check if we are still one vertex away from the bottom and right boundaries:
			if (x < mapChunkSize - 1 && y < mapChunkSize - 1) {
				// Add the two triangles that make up each square tile of mesh:
				meshData.AddTriangle(vertexIndex, vertexIndex + verticesPerLine + 1, vertexIndex + verticesPerLine);
				meshData.AddTriangle(vertexIndex + verticesPerLine + 1, vertexIndex, vertexIndex + 1);
			}
			
			vertexIndex++;
		}
	}
	UE_LOG(LogTemp, Warning, TEXT("\n\nWORKING TILL HERE 4\n"));
	
	ProceduralMesh->DestroyComponent();
	UE_LOG(LogTemp, Warning, TEXT("\n\nWORKING TILL HERE 5\n"));
	ProceduralMesh->CreateMeshSection(0, meshData.vertices, meshData.triangles, meshData.normals, meshData.uvs, meshData.vertexColorsNEW, meshData.tangents, true);
	UE_LOG(LogTemp, Warning, TEXT("\n\nWORKING TILL HERE 6\n"));
	// Enable collision data:
	ProceduralMesh->ContainsPhysicsTriMeshData(true);
}



void AGeneratedMesh::UpdateTerrainMesh() {
	UE_LOG(LogTemp, Warning, TEXT("\n\nUpdateTerrainMesh CALLED:\n"));

	// The following if-statements 'clamp' our noiseMap values:
	if (noiseScale <= 0) { noiseScale = 0.0001f; }
	if (lacunarity < 1) { lacunarity = 1; }
	if (octaves < 0) { octaves = 0; }
	else if (octaves > 7) { octaves = 7; }
	if (persistance < 0) { persistance = 0; }
	else if (persistance > 1) { persistance = 1; }
	if (weightCurveExponent < 1) { weightCurveExponent = 1.0f; }

	// Clamp our Level of Detail Value:
	if (levelOfDetail < 1) { levelOfDetail = 1; }
	else if (levelOfDetail > 6) { levelOfDetail = 6; }

	// Sets the starting 
	float topLeftX = (mapChunkSize - 1) / -2.f;
	float topLeftZ = (mapChunkSize - 1) / 2.f;

	/*int32 index = 0;*/
	int32 meshSimplificationIncrement = levelOfDetail == 0 ? 1 : levelOfDetail * 2;
	// Calculates correct number of vertices for our 'vertices' array:
	int32 verticesPerLine = (mapChunkSize - 1) / meshSimplificationIncrement + 1;
	
	TArray<FArray2D> newNoiseMap = UGenerateNoiseMap::GenerateNoiseMap(mapChunkSize, noiseScale, octaves, persistance, lacunarity);
	
	int32 vertexIndex = 0;

	// Reset the meshData:
	UE_LOG(LogTemp, Warning, TEXT("\n\nUPDATE PRE:  meshData.vertices.Num() = : %d\n"), meshData.vertices.Num());
	for (int32 count = meshData.vertices.Num() - 1; count >= 0; count--) {
		meshData.vertices.RemoveAtSwap(count, 1, true);
		meshData.uvs.RemoveAtSwap(count, 1, true);
		meshData.normals.RemoveAtSwap(count, 1, true);
		meshData.tangents.RemoveAtSwap(count, 1, true);
		meshData.vertexColorsNEW.RemoveAtSwap(count, 1, true);
	}
	UE_LOG(LogTemp, Warning, TEXT("\n\nUPDATE POST:  meshData.vertices.Num() = : %d\n"), meshData.vertices.Num());

	for (int32 y = 0; y < mapChunkSize; y += meshSimplificationIncrement) {
		for (int32 x = 0; x < mapChunkSize; x += meshSimplificationIncrement) {

			float newVertexHeight = calculateWeightCurve(newNoiseMap[y].secondArray[x], weightCurveExponent) * heightMultiplier;

			// Add each vertex, uv, normal, tangent, & vertexColor:
			meshData.vertices.Add(FVector(topLeftX + x, topLeftZ - y, newVertexHeight));
			meshData.uvs.Add(FVector2D(x / (float)mapChunkSize, y / (float)mapChunkSize));
			meshData.normals.Add(FVector(1, 0, 0));
			meshData.tangents.Add(FProcMeshTangent(0, 1, 0));
			meshData.vertexColorsNEW.Add(FColor(0.75, 0.75, 0.75, 1.0));

			vertexIndex++;
		}
	}

	ProceduralMesh->UpdateMeshSection(0, meshData.vertices, meshData.normals, meshData.uvs, meshData.vertexColorsNEW, meshData.tangents);
	ProceduralMesh->ContainsPhysicsTriMeshData(true);
}



float AGeneratedMesh::calculateWeightCurve(float vertexHeight, float exponent) {
	float output = pow(vertexHeight, exponent);
	return output;
}
