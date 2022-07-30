// Fill out your copyright notice in the Description page of Project Settings.


#include "FPSCharacter.h"
#include "Camera/CameraComponent.h"

#include "Weapon.h"
#include "Runtime/Engine/Classes/Kismet/KismetMathLibrary.h"

// CONSTRUCTOR
AFPSCharacter::AFPSCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	

	GetMesh()->SetTickGroup(ETickingGroup::TG_PostUpdateWork); // Fixes latency issues with the camera (Renders the camera at the last tick so that everything can be positioned first):
	GetMesh()->bVisibleInReflectionCaptures = true;
	GetMesh()->bCastHiddenShadow = true;

	ClientMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ClientMesh"));
	ClientMesh->SetCastShadow(false); // We only want our main mesh to cast a shadow (so that way our head from the mesh the player can't see will cast a shadow
	ClientMesh->bCastHiddenShadow = false;
	ClientMesh->bVisibleInReflectionCaptures = false; // This is so that we cannot see our client mesh in any mirrors/reflections
	ClientMesh->SetTickGroup(ETickingGroup::TG_PostUpdateWork);
	ClientMesh->SetupAttachment(GetMesh());

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera")); // Get the Camera Object
	Camera->bUsePawnControlRotation = true; // Sets camera to always face where the controller is
	Camera->SetupAttachment(GetMesh(), FName("head")); // Attach Camera to our pawn's "head" component

	isJumping = false;

	// SET STARTING CHARACTER ATTRIBUTES:
	Health = 100.0f;
	MaxHealth = 100.0f;
	Armor = 100.0f;
	MaxArmor = 100.0f;
	KillCount = 0;
}



// Called when the game starts or when spawned:
void AFPSCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Setup ADS timeline:
	if (AimingCurve) // Check if we have an aiming curve:
	{
		FOnTimelineFloat TimelineFloat;
		TimelineFloat.BindDynamic(this, &AFPSCharacter::TimelineProgress);

		AimingTimeline.AddInterpFloat(AimingCurve, TimelineFloat);
	}
	

	// Client Mesh Logic:
	if (IsLocallyControlled()) // Check if we are the owner of the current pawn... if so, Make the head on our client mesh!
	{ 
		ClientMesh->HideBoneByName(FName("neck_01"), EPhysBodyOp::PBO_None);
		GetMesh()->SetVisibility(false);
	}
	else // If Not... delete our 'ClientMesh' component (since we don't need it, we will just see the other player's main mesh)
	{ 
		ClientMesh->DestroyComponent();
	}

	//// Spawning weapons:
	//if (HasAuthority()) 
	//{
	//	for (const TSubclassOf<AWeapon>& WeaponClass : DefaultWeapons) // Loop over each weapon in our 'DefaultWeapons' array
	//	{ 
	//		if (!WeaponClass) continue; // If Weapon Class isn't valid, skip over that iteration!
	//		FActorSpawnParameters SpawnParams;
	//		SpawnParams.Owner = this;
	//		AWeapon* SpawnedWeapon = GetWorld()->SpawnActor<AWeapon>(WeaponClass, SpawnParams); // 'SpawnedWeapon' = The current weapon in our 'DefaultWeapons' array
	//		const int32 Index = Weapons.Add(SpawnedWeapon); // 'Index' = the index reference
	//		
	//		if (Index == CurrentIndex) 
	//		{
	//			CurrentWeapon = SpawnedWeapon;
	//			OnRep_CurrentWeapon(nullptr);  // This just sets our previous weapon to nothing (using nullptr), and is replicated to the server
	//		}
	//	}
	//}
	
}



void AFPSCharacter::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (isJumping)
	{
		Jump(); // Built-in UE Jump function
	}

	AimingTimeline.TickTimeline(DeltaTime); // This makes it so the track in our Timeline Curve will actually run
}



// Network/Server Functions:
void AFPSCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Set these variables to replicate on the server (NOTE: We are using the 'DOREPLIFETIME' that includes a condition... we don't need this but we are doing it anyways cause why not :) ... at least for now ):
	
	/*DOREPLIFETIME_CONDITION(AFPSCharacter, Weapons, COND_None);
	DOREPLIFETIME_CONDITION(AFPSCharacter, CurrentWeapon, COND_None);*/
	DOREPLIFETIME_CONDITION(AFPSCharacter, ADSWeight, COND_None);

	DOREPLIFETIME_CONDITION(AFPSCharacter, Health, COND_None);
	DOREPLIFETIME_CONDITION(AFPSCharacter, Armor, COND_None);
	DOREPLIFETIME_CONDITION(AFPSCharacter, KillCount, COND_None)
}

void AFPSCharacter::PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker)
{
	Super::PreReplication(ChangedPropertyTracker);

	DOREPLIFETIME_ACTIVE_OVERRIDE(AFPSCharacter, ADSWeight, ADSWeight >= 1.f || ADSWeight <= 0.f) // NOTE: 3rd parameter == a boolean that will tell us whether we should replicate or not (true == replicate, false == don't replicate)
}



// Called to bind functionality to input
void AFPSCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAction(FName("AimDownSights"), EInputEvent::IE_Pressed, this, &AFPSCharacter::StartAiming);
	PlayerInputComponent->BindAction(FName("AimDownSights"), EInputEvent::IE_Released, this, &AFPSCharacter::ReverseAiming);

	/*PlayerInputComponent->BindAction(FName("NextWeapon"), EInputEvent::IE_Pressed, this, &AFPSCharacter::NextWeapon);
	PlayerInputComponent->BindAction(FName("PreviousWeapon"), EInputEvent::IE_Pressed, this, &AFPSCharacter::PreviousWeapon);*/

	// Bind our Movement Functions to our FPSCharacter:
	PlayerInputComponent->BindAxis(FName("MoveForward"), this, &AFPSCharacter::MoveForward);
	PlayerInputComponent->BindAxis(FName("MoveRight"), this, &AFPSCharacter::MoveRight);
	PlayerInputComponent->BindAxis(FName("LookUp"), this, &AFPSCharacter::LookUp);
	PlayerInputComponent->BindAxis(FName("LookRight"), this, &AFPSCharacter::LookRight);

	PlayerInputComponent->BindAction(FName("Jump"), EInputEvent::IE_Pressed, this, &AFPSCharacter::CheckJumping);
	PlayerInputComponent->BindAction(FName("Jump"), EInputEvent::IE_Released, this, &AFPSCharacter::CheckJumping);
}







//void AFPSCharacter::EquipWeapon(const int32 Index)
//{
//	if (!Weapons.IsValidIndex(Index) || CurrentWeapon == Weapons[Index]) return;// First check if the current index is actually a valid index OR if the next weapon is the same weapon we currently have equipped, if not, return & exit the function, if it is, then continue!
//
//	if (IsLocallyControlled() || HasAuthority())
//	{
//		CurrentIndex = Index; // Just set our 'CurrentIndex' variable to the new 'Index' passed in
//
//		const AWeapon* OldWeaponREF = CurrentWeapon; // Get a reference to the previously equiped weapon
//		CurrentWeapon = Weapons[Index]; // Assign weapon to the gun at our specified index
//		OnRep_CurrentWeapon(OldWeaponREF);
//	}
//
//	if (!HasAuthority())
//	{
//		Server_SetCurrentWeapon(Weapons[Index]);
//	}
//}



//void AFPSCharacter::DropWeapon()
//{
//	if (HasAuthority()) 
//	{
//
//	}
//}



//FTransform AFPSCharacter::GetPickupSpawn()
//{
//	FTransform output;
//
//	FVector OutLocation;
//	FRotator OutRotation;
//	GetController()->GetActorEyesViewPoint(OutLocation, OutRotation);
//	
//	output.SetLocation(OutLocation + (UKismetMathLibrary::GetForwardVector(OutRotation) * 200));
//
//
//	return output;
//}



//void AFPSCharacter::OnRep_CurrentWeapon(const AWeapon* OldWeapon) 
//{
//	if (CurrentWeapon) 
//	{
//		if(!CurrentWeapon->CurrentOwner) // Checks if there IS a 'CurrentOwner' for the 'CurrentWeapon'
//		{ 
//			const FTransform& PlacementTransform = CurrentWeapon->PlacementTransform * GetMesh()->GetSocketTransform(FName("weapon_socket_r"));
//			CurrentWeapon->SetActorTransform(PlacementTransform, false, nullptr, ETeleportType::TeleportPhysics);
//			CurrentWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepWorldTransform, FName("weapon_socket_r"));
//
//			CurrentWeapon->CurrentOwner = this; // Set the owner of the current weapon to ourselves
//		}
//
//		CurrentWeapon->Mesh->SetVisibility(true);
//	}
//
//	if (OldWeapon) 
//	{
//		OldWeapon->Mesh->SetVisibility(false);
//	}
//
//	// Broadcast our Delegate function made in Vid #2:
//	CurrentWeaponChangedDelegate.Broadcast(CurrentWeapon, OldWeapon);
//}
//
//
//
//void AFPSCharacter::Server_SetCurrentWeapon_Implementation(AWeapon* NewWeapon) 
//{
//	const AWeapon* OldWeaponREF = CurrentWeapon; // Get a reference to the previously equiped weapon
//	CurrentWeapon = NewWeapon; // The 'Weapon' passed in will be the previously equipped weapon
//	OnRep_CurrentWeapon(OldWeaponREF);
//}








// AIMING STUFF:

void AFPSCharacter::StartAiming() 
{
	if (IsLocallyControlled() || HasAuthority()) 
	{
		isAiming = true;
		Multi_Aim_Implementation(true); // Set to true because we ARE aiming forward, and thus are starting the aiming logic... this will just run on the 
	}
	if (!HasAuthority()) 
	{
		isAiming = true;
		Server_Aim(true); // Will make a "round-trip" to the server if was are the client, which will run our aiming logic on the server, and broadcast it to every other client inluding ourselves, making the client who called it aim and every other client actually see that client's aiming animation!
	}
}

void AFPSCharacter::ReverseAiming() 
{
	if (IsLocallyControlled() || HasAuthority()) 
	{
		isAiming = false;
		Multi_Aim_Implementation(false); // Set to true because we ARE aiming forward, and thus are starting the aiming logic... this will just run on the 
	}
	if (!HasAuthority()) 
	{
		isAiming = false;
		Server_Aim(false); // Will make a "round-trip" to the server if was are the client, which will run our aiming logic on the server, and broadcast it to every other client inluding ourselves, making the client who called it aim and every other client actually see that client's aiming animation!
	}
}

void AFPSCharacter::Multi_Aim_Implementation(const bool bForward) 
{
	if (bForward) 
	{
		AimingTimeline.Play(); // Will play forward
	}
	else 
	{
		AimingTimeline.Reverse(); // Will just reverse our Timeline
	}
}



void AFPSCharacter::TimelineProgress(const float Value) 
{
	ADSWeight = Value;
}





// CHARACTER CONTROLS:

//void AFPSCharacter::NextWeapon() 
//{
//	const int32 Index = Weapons.IsValidIndex(CurrentIndex + 1) ? CurrentIndex + 1 : 0; // Checks if we are at the last index, IF SO, wrap around and reset the index to 0, ELSE add '1' to our index
//	EquipWeapon(Index);
//}
//void AFPSCharacter::PreviousWeapon() 
//{
//	const int32 Index = Weapons.IsValidIndex(CurrentIndex - 1) ? CurrentIndex - 1 : Weapons.Num() - 1;
//	EquipWeapon(Index);
//}
void AFPSCharacter::MoveForward(const float Value) 
{
	const FVector& Direction = FRotationMatrix(FRotator(0.f, GetControlRotation().Yaw, 0.f)).GetUnitAxis(EAxis::X); // Gets a unit vector is our pawns current facing direction and add a value to its position
	AddMovementInput(Direction, Value);
}
void AFPSCharacter::MoveRight(const float Value) 
{
	const FVector& Direction = FRotationMatrix(FRotator(0.f, GetControlRotation().Yaw, 0.f)).GetUnitAxis(EAxis::Y);
	AddMovementInput(Direction, Value);
}
void AFPSCharacter::LookUp(const float Value) 
{
	AddControllerPitchInput(Value);
}
void AFPSCharacter::LookRight(const float Value) 
{
	AddControllerYawInput(Value);
}
void AFPSCharacter::CheckJumping()
{
	isJumping ? isJumping = false : isJumping = true;
}
