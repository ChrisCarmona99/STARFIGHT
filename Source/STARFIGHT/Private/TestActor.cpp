// Fill out your copyright notice in the Description page of Project Settings.

#include "TestActor.h"


// Sets default values (CONSTRUCTOR):
ATestActor::ATestActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SuperMesh = CreateDefaultSubobject<UStaticMeshComponent>("Landscape Mesh");
}

// Called when the game starts or when spawned
void ATestActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ATestActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FVector NewLocation = GetActorLocation();
	NewLocation.Z += 1.f;
	
	SetActorLocation(NewLocation);

}

FVector2D ATestActor::GenerateNoiseMap() {


	//for (TActorIterator<ATestActor> ActorItr(GetWorld()); ActorItr; ++ActorItr)
	//{ // Same as with the Object Iterator, access the subclass instance with the * or -> operators.

	//}


	return FVector2D();
}
