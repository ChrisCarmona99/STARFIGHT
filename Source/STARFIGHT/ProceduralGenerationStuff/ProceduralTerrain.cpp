// Fill out your copyright notice in the Description page of Project Settings.


#include "ProceduralTerrain.h"
#include <chrono>
#include <thread>
#include "HAL/ThreadManager.h"



auto debugPrint = [](const FString ThreadName, float*& NoiseMap, FProceduralMeshInputs& Inputs) {
	float min = 0;
	float max = 0;
	UE_LOG(LogTemp, Warning, TEXT("| %s | #: TESTING NoiseMap POST NoiseMapComputeShader:"), *ThreadName);
	for (int index = 0; index < Inputs.mapChunkSize * Inputs.mapChunkSize; index += FMath::DivideAndRoundUp(Inputs.mapChunkSize * Inputs.mapChunkSize, 50))
		//for (int index = 0; index < Inputs.mapChunkSize * Inputs.mapChunkSize; index++)
	{
		UE_LOG(LogTemp, Warning, TEXT("        index: %d   |   value: %f"), index, NoiseMap[index]);
		//min = (NoiseMap[index] < min) ? NoiseMap[index] : min;
		//max = (NoiseMap[index] > max) ? NoiseMap[index] : max;
	}
	//UE_LOG(LogTemp, Warning, TEXT("        (MIN | MAX) == (%f | %f)"), min, max);
};



AProceduralTerrain::AProceduralTerrain() :
	// Mesh Components:
	_ProceduralTerrainMesh(CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("GeneratedMesh"))),

	// Material:

	// Mesh Parameters:
	MapChunkSize(10),
	Seed(0),
	Offset(FVector2D(0.0, 0.0)),
	LevelOfDetail(0),
	SpawnLocation(FVector(0.0, 0.0, 0.0)),
	MeshScale(FVector(0.0, 0.0, 0.0)),
	UseRandomSeed(false),
	SeedRangeMAX(2147483647),

	// Noise Parameters:
	NoiseScale(27.6),
	Octaves(4),
	Persistence(0.6),
	Lacurnarity(1.9),
	HeighMultiplier(22.0),
	WeightCurveExponent(3.0),
	A(1.8),
	B(4.0),
	C(0.6),

	// Erosion Parameters:
	DropletLifetime(30), // NOTE: How many iterations a drop will traverse our mesh triangles until we stop manipulating the vertices
	NumIterations(1000), // NOTE: Control how many droplets to simulate
	
	// Execute
	GENERATE_IN_EDITOR(true)


{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	//_ProceduralTerrainMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("GeneratedMesh"));
	RootComponent = _ProceduralTerrainMesh;

	// New in UE 4.17, multi-threaded PhysX cooking:
	_ProceduralTerrainMesh->bUseAsyncCooking = true;
}



void AProceduralTerrain::BeginPlay()
{
	Super::BeginPlay();
	
}



void AProceduralTerrain::OnConstruction(const FTransform& Transform)
{
	uint32 ThreadId = FPlatformTLS::GetCurrentThreadId();
	const FString ThreadName = FThreadManager::Get().GetThreadName(ThreadId);

	Super::OnConstruction(Transform);
	UE_LOG(LogTemp, Warning, TEXT(""));
	UE_LOG(LogTemp, Warning, TEXT(""));
	UE_LOG(LogTemp, Warning, TEXT(""));
	UE_LOG(LogTemp, Warning, TEXT(""));
	UE_LOG(LogTemp, Warning, TEXT(""));
	UE_LOG(LogTemp, Warning, TEXT(""));
	UE_LOG(LogTemp, Warning, TEXT(""));
	UE_LOG(LogTemp, Warning, TEXT(""));
	UE_LOG(LogTemp, Warning, TEXT(""));
	UE_LOG(LogTemp, Warning, TEXT("| %s | 1: OnConstruction CALLED"), *ThreadName);

	// Perform input value boundry checks:
	MapChunkSize = (MapChunkSize < 2) ? 2 : MapChunkSize;
	NoiseScale = (NoiseScale < 0.0001f) ? 0.0001f : NoiseScale;
	Lacurnarity = (Lacurnarity < 1) ? 1 : Lacurnarity;
	Octaves = (Octaves < 0) ? 0 : Octaves;
	Persistence = (Persistence < 0) ? 0 : (Persistence > 1) ? 1 : Persistence;
	WeightCurveExponent = (WeightCurveExponent < 1.0f) ? 1.0f : WeightCurveExponent;
	LevelOfDetail = (LevelOfDetail < 0) ? 0 : (LevelOfDetail > 6) ? 6 : LevelOfDetail;

	// Set our `Inputs` struct:
	FProceduralMeshInputs Inputs;
	Inputs.proceduralTerrainMesh = _ProceduralTerrainMesh;

	Inputs.mapChunkSize = MapChunkSize;
	Inputs.seed = Seed;
	Inputs.offset = Offset;
	Inputs.levelOfDetail = LevelOfDetail;

	Inputs.noiseScale = NoiseScale;
	Inputs.octaves = Octaves;
	Inputs.persistence = Persistence;
	Inputs.lacunarity = Lacurnarity;
	Inputs.heightMultiplier = HeighMultiplier;
	Inputs.weightCurveExponent = WeightCurveExponent;
	Inputs.a = A;
	Inputs.b = B;
	Inputs.c = C;

	Inputs.dropletLifetime = DropletLifetime;
	Inputs.numIterations = NumIterations;

	// Log all the inputs (Debug purposes):
	auto printInputs = [Inputs]()
	{
		UE_LOG(LogTemp, Warning, TEXT(""));
		UE_LOG(LogTemp, Warning, TEXT("		printInputs:"));
		UE_LOG(LogTemp, Warning, TEXT("				mapChunkSize (int32) == %d"), Inputs.mapChunkSize);
		UE_LOG(LogTemp, Warning, TEXT("				seed (int32) == %d"), Inputs.seed);
		UE_LOG(LogTemp, Warning, TEXT("				offset (FVector2D) == (%f, %f)"), Inputs.offset.X, Inputs.offset.Y);
		UE_LOG(LogTemp, Warning, TEXT("				levelOfDetail (int32) == %d"), Inputs.levelOfDetail);
		UE_LOG(LogTemp, Warning, TEXT("				noiseScale (float) == %f"), Inputs.noiseScale);
		UE_LOG(LogTemp, Warning, TEXT("				octaves (int) == %d"), Inputs.octaves);
		UE_LOG(LogTemp, Warning, TEXT("				persistence (float) == %f"), Inputs.persistence);
		UE_LOG(LogTemp, Warning, TEXT("				lacunarity (float) == %f"), Inputs.lacunarity);
		UE_LOG(LogTemp, Warning, TEXT("				heightMultiplier (float) == %f"), Inputs.heightMultiplier);
		UE_LOG(LogTemp, Warning, TEXT("				weightCurveExponent (float) == %f"), Inputs.weightCurveExponent);
		UE_LOG(LogTemp, Warning, TEXT("				a (float) == %f"), Inputs.a);
		UE_LOG(LogTemp, Warning, TEXT("				b (float) == %f"), Inputs.b);
		UE_LOG(LogTemp, Warning, TEXT("				c (float) == %f"), Inputs.c);
		UE_LOG(LogTemp, Warning, TEXT("				dropletLifetime (int32) == %d"), Inputs.dropletLifetime);
		UE_LOG(LogTemp, Warning, TEXT("				numIterations (int32) == %d"), Inputs.numIterations);
		UE_LOG(LogTemp, Warning, TEXT(""));
	};
	printInputs();

	// Regenerate the terrain based off the new input values (This will be executed after the constructor):
	AProceduralTerrain::ExecuteProceduralMeshGeneration(Inputs);
	UE_LOG(LogTemp, Warning, TEXT("| %s | 4: OnConstruction DONE"), *ThreadName);
}



// Called every frame
void AProceduralTerrain::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}



void AProceduralTerrain::CreateTestTriangle(UProceduralMeshComponent* proceduralTerrainMesh)
{
	TArray<FVector> vertices;
	vertices.Add(FVector(0, 0, 0));
	vertices.Add(FVector(0, 100, 0));
	vertices.Add(FVector(0, 0, 100));

	TArray<int32> Triangles;
	Triangles.Add(0);
	Triangles.Add(1);
	Triangles.Add(2);

	TArray<FVector> normals;
	normals.Add(FVector(1, 0, 0));
	normals.Add(FVector(1, 0, 0));
	normals.Add(FVector(1, 0, 0));

	TArray<FVector2D> UV0;
	UV0.Add(FVector2D(0, 0));
	UV0.Add(FVector2D(10, 0));
	UV0.Add(FVector2D(0, 10));


	TArray<FProcMeshTangent> tangents;
	tangents.Add(FProcMeshTangent(0, 1, 0));
	tangents.Add(FProcMeshTangent(0, 1, 0));
	tangents.Add(FProcMeshTangent(0, 1, 0));

	TArray<FLinearColor> vertexColors;
	vertexColors.Add(FLinearColor(0.75, 0.75, 0.75, 1.0));
	vertexColors.Add(FLinearColor(0.75, 0.75, 0.75, 1.0));
	vertexColors.Add(FLinearColor(0.75, 0.75, 0.75, 1.0));

	proceduralTerrainMesh->CreateMeshSection_LinearColor(0, vertices, Triangles, normals, UV0, vertexColors, tangents, true);

	// Enable collision data
	proceduralTerrainMesh->ContainsPhysicsTriMeshData(true);
}



void AProceduralTerrain::BuildTerrainMesh(UProceduralMeshComponent* proceduralTerrainMesh, std::shared_ptr<FGeneratedMeshData> meshData)
{
	// Generate Mesh:
	proceduralTerrainMesh->CreateMeshSection_LinearColor(
		0, 
		meshData->vertices, 
		meshData->triangles, 
		meshData->normals, 
		meshData->uvs, 
		meshData->vertexColors, 
		meshData->tangents, 
		true);

	// Enable collision data
	proceduralTerrainMesh->ContainsPhysicsTriMeshData(true);
}



void AProceduralTerrain::ExecuteProceduralMeshGeneration(FProceduralMeshInputs& Inputs)
{
	uint32 ThreadId = FPlatformTLS::GetCurrentThreadId();
	const FString ThreadName = FThreadManager::Get().GetThreadName(ThreadId);

	UE_LOG(LogTemp, Warning, TEXT("| %s | 2: STARTING TerrainGenerationThread"), *ThreadName);

	// Launch the TerrainGenerationThread:
	std::thread TerrainGenerationThread(
		[Inputs]() mutable
		{
			uint32 ThreadId = FPlatformTLS::GetCurrentThreadId();
			const FString ThreadName = FThreadManager::Get().GetThreadName(ThreadId);

			// Create a `Mesh Data` struct:
			std::shared_ptr<FGeneratedMeshData> MeshData(new FGeneratedMeshData);

			// Execute GenerateProceduralMeshData on the TerrainGenerationThread
			UE_LOG(LogTemp, Warning, TEXT("| %s | 5: GenerateProceduralMeshData CALLED"), *ThreadName);
			AProceduralTerrain::GenerateProceduralMeshData(MeshData, Inputs);
			UE_LOG(LogTemp, Warning, TEXT("| %s | 18: GenerateProceduralMeshData FINISHED"), *ThreadName);


			UE_LOG(LogTemp, Warning, TEXT("| %s | 19: CALLING ASYNC TASK ON GAME THREAD..."), *ThreadName);
			// At this point, our terrain mesh values have been generated, so queue the mesh creation on the game thread passing in those parameters:
			AsyncTask(ENamedThreads::GameThread,
				[Inputs, MeshData]()
				{
					uint32 ThreadId = FPlatformTLS::GetCurrentThreadId();
					const FString ThreadName = FThreadManager::Get().GetThreadName(ThreadId);

					UE_LOG(LogTemp, Warning, TEXT("| %s | 20: BUILDING MESH"), *ThreadName);
					AProceduralTerrain::CreateTestTriangle(Inputs.proceduralTerrainMesh);
					//AProceduralTerrain::BuildTerrainMesh(Inputs.proceduralTerrainMesh, MeshData);
					UE_LOG(LogTemp, Warning, TEXT("| %s | 20: BUILDING MESH DONE!!!"), *ThreadName);
				});
		});
	
	// Detach the TerrainGenerationThread so we don't halt the GameThread while the terrain is being generated
	TerrainGenerationThread.detach();

	UE_LOG(LogTemp, Warning, TEXT("| %s | 3: ExecuteProceduralMeshGeneration EXITED"), *ThreadName);
}



void AProceduralTerrain::GenerateProceduralMeshData(std::shared_ptr<FGeneratedMeshData> MeshData, FProceduralMeshInputs& Inputs)
{
	uint32 ThreadId = FPlatformTLS::GetCurrentThreadId();
	const FString ThreadName = FThreadManager::Get().GetThreadName(ThreadId);


	// Create a new `Noise Map` array:
	float* NoiseMap = new float[Inputs.mapChunkSize * Inputs.mapChunkSize];

	// Generate the Perline Noise values:
	ProceduralGeneration::GenerateNoiseMap(NoiseMap, Inputs.mapChunkSize, Inputs.seed, Inputs.offset, Inputs.noiseScale, Inputs.octaves, Inputs.persistence, Inputs.lacunarity);

	debugPrint(ThreadName, NoiseMap, Inputs);

	// Apply the Falloff map:
	ProceduralGeneration::ApplyFalloffMap(NoiseMap, Inputs.mapChunkSize, Inputs.a, Inputs.b, Inputs.c);

	// Apply the Erosion map:
	ProceduralGeneration::ApplyErosionMap(NoiseMap, Inputs.mapChunkSize, Inputs.seed, Inputs.dropletLifetime, Inputs.numIterations);


	// Sets the starting traversal point for all the vertices
	//		In UE5, the x-axis and y-axis are flipped... +x is 'forward' and +y is 'to the right'
	float topMostX = (Inputs.mapChunkSize - 1) / 2.f; // +x
	float leftMostY = (Inputs.mapChunkSize - 1) / -2.f; // -y

	// Calculates the increment for mesh LODs (ensures 'levelOfDetail' is NOT 0):
	int32 LODincrement = Inputs.levelOfDetail == 0 ? 1 : Inputs.levelOfDetail * 2;

	// Calculates correct number of vertic  es for our 'vertices' array:
	int32 verticesPerLine = ((Inputs.mapChunkSize - 1) / LODincrement) + 1;

	// Populate MeshData (TODO: Make HLSL):
	UE_LOG(LogTemp, Warning, TEXT("| %s | 17: Populating `MeshData`:"), *ThreadName);
	int printIncrement = FMath::DivideAndRoundUp(Inputs.mapChunkSize * Inputs.mapChunkSize, 50);
	int printIndex = printIncrement;
	for (int i = 0; i < Inputs.mapChunkSize * Inputs.mapChunkSize; i++)
	{
		// Define our proportionate 'x' and 'y' indices so that we can map our 1D vector too an 'x' and 'y' coordinate for our vertex
		int y = i % Inputs.mapChunkSize; // inner vector, traverses the y-axis (left to right)
		int x = std::floor(i / Inputs.mapChunkSize); // outer vector, traveres the x-axis (top to bottom)

		if (i == printIndex) { 
			UE_LOG(LogTemp, Warning, TEXT("        index: %d   |   (X, Y): (%d, %d)"), i, x, y);
			printIndex += printIncrement;
		}

		// Define the height for the current vertex in our iteration
		float currentHeight = ProceduralGeneration::calculateWeightCurve(NoiseMap[i], Inputs.weightCurveExponent) * Inputs.heightMultiplier;

		// Add Vertices, UVs, VertexColors, and Triangles:
		MeshData->vertices.Add(FVector(topMostX - x, leftMostY + y, currentHeight));
		MeshData->uvs.Add(FVector2D(x, y));
		MeshData->vertexColorsNEW.Add(FColor(0.50, 0.75, 1.00, 1.0));

		// Add Normals and Tangents:
		AProceduralTerrain::AddNormalAndTangent(MeshData, i);

		// Add Triangles:
		if (y < Inputs.mapChunkSize - 1 && x < Inputs.mapChunkSize - 1)
		{
			// NOTE: UE5 render mode is counter-clockwise when looking from +z -> =z... the order in which we pass these vertices in is based off that 
			MeshData->AddTriangle(i, i + verticesPerLine, i + verticesPerLine + 1);
			MeshData->AddTriangle(i + verticesPerLine + 1, i + 1, i);
		}
	}

	// DONE WITH ALL NESH GENERATION TASKS... DEALLOCATE EVERTHING THAT'S NOT MeshData:
	UE_LOG(LogTemp, Warning, TEXT("| %s | 17.1: Deleting NoiseMap"), *ThreadName);
	delete[] NoiseMap;
	UE_LOG(LogTemp, Warning, TEXT("| %s | 17.2: NoiseMap deleted!"), *ThreadName);
}



void AProceduralTerrain::AddNormalAndTangent(std::shared_ptr<FGeneratedMeshData> meshData, int& vertexIndex)
{
	TArray<FVector> vertices = meshData->vertices;
	TArray<int32> triangles = meshData->triangles;

	meshData->normals.Add(FVector(0.0f, 0.0f, 0.0f));
	bool bFlipBitangent = true;
	meshData->tangents.Add(FProcMeshTangent(FVector(0.0f, 0.0f, 0.0f), bFlipBitangent));
}

