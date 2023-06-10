// Fill out your copyright notice in the Description page of Project Settings.


#include "ProceduralTerrain.h"
#include "Components/StaticMeshComponent.h"
#include <chrono>
#include <thread>
#include "HAL/ThreadManager.h"



auto debugPrint = [](const FString ThreadName, float*& NoiseMap, FProceduralMeshInputs& Inputs) {
	float min = 0;
	float max = 0;
	UE_LOG(LogTemp, Warning, TEXT("| %s | #: TESTING NoiseMap POST NoiseMapComputeShader:"), *ThreadName);
	//for (int index = 0; index < Inputs.mapChunkSize * Inputs.mapChunkSize; index += FMath::DivideAndRoundUp(Inputs.mapChunkSize * Inputs.mapChunkSize, 50))
	for (int index = 0; index < 500; index++)
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
	GENERATE_IN_EDITOR(true),
	APPLY_FALLOFF_MAP(true),
	APPLY_EROSION_MAP(true)

{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	//_ProceduralTerrainMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("GeneratedMesh"));
	RootComponent = _ProceduralTerrainMesh;

	// Setup the terrain material:
	_TerrainMaterial = CreateDefaultSubobject<UMaterialInterface>("TerrainMaterial");

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

	// Update Terrain Material:
	_ProceduralTerrainMesh->SetMaterial(0, _TerrainMaterial);

	// Perform input value boundry checks:
	MapChunkSize = (MapChunkSize < 2) ? 2 : MapChunkSize;
	NoiseScale = (NoiseScale < 0.0001f) ? 0.0001f : NoiseScale;
	Lacurnarity = (Lacurnarity < 1) ? 1 : Lacurnarity;
	Octaves = (Octaves < 1) ? 1 : Octaves;
	Persistence = (Persistence < 0) ? 0 : (Persistence > 1) ? 1 : Persistence;
	WeightCurveExponent = (WeightCurveExponent < 1.0f) ? 1.0f : WeightCurveExponent;
	LevelOfDetail = (LevelOfDetail < 0) ? 0 : (LevelOfDetail > 6) ? 6 : LevelOfDetail;

	// Set our `Inputs` struct:
	FProceduralMeshInputs Inputs;
	Inputs.proceduralTerrainMesh = _ProceduralTerrainMesh;
	Inputs.APPLY_FALLOFF_MAP = APPLY_FALLOFF_MAP;
	Inputs.APPLY_EROSION_MAP = APPLY_EROSION_MAP;

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


			// At this point, our terrain mesh values have been generated, so queue the mesh creation on the game thread passing in those parameters:
			UE_LOG(LogTemp, Warning, TEXT("| %s | 19: CALLING ASYNC TASK ON GAME THREAD..."), *ThreadName);
			AsyncTask(ENamedThreads::GameThread,
				[Inputs, MeshData]()
				{
					uint32 ThreadId = FPlatformTLS::GetCurrentThreadId();
					const FString ThreadName = FThreadManager::Get().GetThreadName(ThreadId);

					UE_LOG(LogTemp, Warning, TEXT("| %s | 20: BUILDING MESH"), *ThreadName);
					//AProceduralTerrain::CreateTestTriangle(Inputs.proceduralTerrainMesh);
					AProceduralTerrain::BuildTerrainMesh(Inputs.proceduralTerrainMesh, MeshData);
					UE_LOG(LogTemp, Warning, TEXT("| %s | 21: BUILDING MESH DONE!!!"), *ThreadName);
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
	ProceduralGeneration::GenerateNoiseMap(NoiseMap, Inputs.mapChunkSize, Inputs.seed, Inputs.offset, Inputs.noiseScale, Inputs.octaves, Inputs.persistence, Inputs.lacunarity, Inputs.heightMultiplier, Inputs.weightCurveExponent);


	// Apply the Falloff map:
	if (Inputs.APPLY_FALLOFF_MAP == true)
	{
		ProceduralGeneration::ApplyFalloffMap(NoiseMap, Inputs.mapChunkSize, Inputs.a, Inputs.b, Inputs.c);
	}
	else {
		UE_LOG(LogTemp, Warning, TEXT("| %s | 15: ApplyFalloffMap NOT applied..."), *ThreadName);
	}
	

	// Apply the Erosion map:
	if (Inputs.APPLY_EROSION_MAP == true)
	{
		ProceduralGeneration::ApplyErosionMap(NoiseMap, Inputs.mapChunkSize, Inputs.seed, Inputs.dropletLifetime, Inputs.numIterations);
	}
	

	// Generate Normals and Tangets:
	float* Normals = new float[Inputs.mapChunkSize * Inputs.mapChunkSize * 3];
	float* Tangents = new float[Inputs.mapChunkSize * Inputs.mapChunkSize * 3];

	float* debug1 = new float[Inputs.mapChunkSize * Inputs.mapChunkSize * 2];
	AProceduralTerrain::AddNormalsAndTangents(Normals, Tangents, NoiseMap, Inputs.mapChunkSize);
	for (int index = 0; index < 500; index++)
	{	
		int y = index % Inputs.mapChunkSize; // inner vector, traverses the y-axis (left to right)
		int x = std::floor(index / Inputs.mapChunkSize); // outer vector, traveres the x-axis (top to bottom)
		UE_LOG(LogTemp, Warning, TEXT("        index: %d   |   vertex == (%f, %f, %f)    |    normal == <%f, %f, %f>    |    tangent == <%f, %f, %f>"), 
								      index, (float)x, (float)y, NoiseMap[index], Normals[index * 3], Normals[index * 3 + 1], Normals[index * 3 + 2], Tangents[index * 3], Tangents[index * 3 + 1], Tangents[index * 3 + 2]);
	}


	// Sets the starting traversal point for all the vertices (NOTE: In UE5, the x-axis and y-axis are flipped... +x is 'forward' and +y is 'to the right'):
	float topMostX = (float)(Inputs.mapChunkSize - 1) / 2.0f; // +x
	float leftMostY = (float)(Inputs.mapChunkSize - 1) / -2.0f; // -y

	// Calculates the increment for mesh LODs (ensures 'levelOfDetail' is NOT 0):
	int32 LODincrement = Inputs.levelOfDetail == 0 ? 1 : Inputs.levelOfDetail * 2;
	// Calculates correct number of vertices for our 'vertices' array:
	int32 verticesPerLine = ((Inputs.mapChunkSize - 1) / LODincrement) + 1;

	// Populate MeshData (TODO: Make HLSL):
	UE_LOG(LogTemp, Warning, TEXT("| %s | 17: Populating `MeshData`:"), *ThreadName);
	for (int i = 0; i < Inputs.mapChunkSize * Inputs.mapChunkSize; i++)
	{
		// Define our proportionate 'x' and 'y' indices so that we can map our 1D vector too an 'x' and 'y' coordinate for our vertex
		int y = i % Inputs.mapChunkSize; // inner vector, traverses the y-axis (left to right)
		int x = std::floor(i / Inputs.mapChunkSize); // outer vector, traveres the x-axis (top to bottom)

		// Define the height for the current vertex in our iteration
		//float currentHeight = ProceduralGeneration::calculateWeightCurve(NoiseMap[i], Inputs.weightCurveExponent) * Inputs.heightMultiplier; // WARNING: This probably needs to be applied before we calculate the normals & tangents...

		// Add Vertices, UVs, VertexColors, and Triangles:
		MeshData->vertices.Add(FVector(topMostX - x, leftMostY + y, NoiseMap[i]));
		MeshData->uvs.Add(FVector2D(x, y));
		MeshData->vertexColors.Add(FLinearColor(0.75, 0.75, 0.75, 1.0));

		// Add Normals and Tangents:
		int i2 = i * 3;
		MeshData->normals.Add(FVector(Normals[i2], Normals[i2 + 1], Normals[i2 + 2]));
		MeshData->tangents.Add(FProcMeshTangent(Tangents[i2], Tangents[i2 + 1], Tangents[i2 + 2]));

		// Add Triangles:
		if (y < Inputs.mapChunkSize - 1 && x < Inputs.mapChunkSize - 1)
		{
			// NOTE: UE5 render mode is counter-clockwise when looking from +z -> =z... the order in which we pass these vertices in is based off that 
			MeshData->AddTriangle(i, i + verticesPerLine, i + verticesPerLine + 1);
			MeshData->AddTriangle(i + verticesPerLine + 1, i + 1, i);
		}

		if (i < 500) {
			UE_LOG(LogTemp, Warning, TEXT("        index: %d    |    raw vertex == (%f, %f, %f)    |    centered vertex == (%f, %f, %f)"), i, (float)x, (float)y, NoiseMap[i], (float)(topMostX - x), (float)(leftMostY + y), NoiseMap[i]);
		}
	}

	// DONE WITH ALL NESH GENERATION TASKS... DEALLOCATE EVERTHING THAT'S NOT MeshData:
	UE_LOG(LogTemp, Warning, TEXT("| %s | 17.1: Deleting NoiseMap, Normals, and Tangents"), *ThreadName);
	delete[] NoiseMap;
	delete[] Normals;
	delete[] Tangents;
	UE_LOG(LogTemp, Warning, TEXT("| %s | 17.2: NoiseMap, Normals, and Tangents deleted!"), *ThreadName);
}



void AProceduralTerrain::AddNormalsAndTangents(float*& normals, float*& tangents, float*& noiseMap, const int32& mapChunkSize)
{
	uint32 ThreadId = FPlatformTLS::GetCurrentThreadId();
	const FString ThreadName = FThreadManager::Get().GetThreadName(ThreadId);

	UE_LOG(LogTemp, Warning, TEXT(""));
	UE_LOG(LogTemp, Warning, TEXT("| %s | 6: AddNormalsAndTangents CALLED"), *ThreadName);


	// Define our Compute Shader's input parameters:
	int THREADS_X = 1024;
	int THREADS_Y = 1;
	int THREADS_Z = 1;
	int THREAD_GROUPS_X = FMath::DivideAndRoundUp(mapChunkSize * mapChunkSize, THREADS_X);

	FNormalsAndTangentsCSDispatchParams Params(THREADS_X, THREADS_Y, THREADS_Z);
	Params.THREAD_GROUPS_X = THREAD_GROUPS_X;

	Params.mapChunkSize[0] = mapChunkSize;
	Params.noiseMap = noiseMap;

	Params.normals = normals;
	Params.tangents = tangents;


	// Create a completion event to be signaled when the Compute Shader has completed:
	FEvent* NormalsAndTangentsCompletionEvent = FPlatformProcess::GetSynchEventFromPool(true);

	UE_LOG(LogTemp, Warning, TEXT("| %s | 7: Launching ENQUEUE_RENDER_COMMAND thread..."), *ThreadName);
	if (!IsInRenderingThread())
	{
		UE_LOG(LogTemp, Warning, TEXT("|    |	 : NOT IN RENDERING THREAD... WE GOOD!"), *ThreadName);
		// Enqueue a render command to call MyFunction on the render thread
		ENQUEUE_RENDER_COMMAND(MyRenderCommand)(
			[&Params, normals, tangents, NormalsAndTangentsCompletionEvent](FRHICommandListImmediate& RHICmdList) mutable
			{
				FNormalsAndTangentsCSInterface::ExecuteNormalsAndTangentsCS(
					GetImmediateCommandList_ForRenderCommand(),
					Params,
					normals,
					tangents,
					NormalsAndTangentsCompletionEvent);
			});
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("|    |	 : IN RENDERING THREAD WHEN WE SHOULDN'T BE..."), *ThreadName);
	}

	UE_LOG(LogTemp, Warning, TEXT("| %s | 7.5: NormalsAndTangentsCompletionEvent->Wait() CALLED"), *ThreadName);
	NormalsAndTangentsCompletionEvent->Wait();
	UE_LOG(LogTemp, Warning, TEXT("| %s | 7.6: NormalsAndTangentsCompletionEvent->Wait() COMPLETED"), *ThreadName);


	UE_LOG(LogTemp, Warning, TEXT("| %s | 14: ** Dispatch DONE **  | NormalsAndTangentsCS DONE"), *ThreadName);
}

