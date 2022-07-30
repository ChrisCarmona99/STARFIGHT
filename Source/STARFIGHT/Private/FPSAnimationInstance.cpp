// Fill out your copyright notice in the Description page of Project Settings.


#include "FPSAnimationInstance.h"
#include "FPSCharacter.h"
#include "Camera/CameraComponent.h"
#include "Runtime/Engine/Classes/Kismet/KismetMathLibrary.h"


// Constructor implementation:
UFPSAnimationInstance::UFPSAnimationInstance() {}



void UFPSAnimationInstance::NativeBeginPlay()
{
	Super::NativeBeginPlay();
	// *** OLD... LEAVE COMMENTED *** :
	/*
	Character = Cast<AFPSCharacter>(TryGetPawnOwner());
	if (Character) { // If character is valid, set up all the other references:
		Mesh = Character->GetMesh();
		Character->CurrentWeaponChangedDelegate.AddDynamic(this, &UFPSAnimationInstance::CurrentWeaponChanged);
		CurrentWeaponChanged(Character->CurrentWeapon, nullptr); // Initialize our character to have the current weapon and no previous weapon ('PreviousWeapon' set to 'nullptr')
	}
	*/
}



// This is our 'Tick' function (This functions are called every single frame!):
void UFPSAnimationInstance::NativeUpdateAnimation(float DeltaTime)
{
	//Super::NativeUpdateAnimation(DeltaTime);

	//if (!Character) // If character valid... keep trying to set Character until it IS valid:
	//{ 
	//	Character = Cast<AFPSCharacter>(TryGetPawnOwner());
	//	if (Character) // Once the character is finally valid:
	//	{ 
	//		Mesh = Character->GetMesh();
	//		Character->CurrentWeaponChangedDelegate.AddDynamic(this, &UFPSAnimationInstance::CurrentWeaponChanged);
	//		CurrentWeaponChanged(Character->CurrentWeapon, nullptr); // Initialize our character to have the current weapon and no previous weapon ('PreviousWeapon' set to 'nullptr')
	//	}
	//	else return;
	//}

	//SetVars(DeltaTime);
	//CalculateWeaponSway(DeltaTime);

	//LastRotation = CameraTransform.Rotator();
}



void UFPSAnimationInstance::CurrentWeaponChanged(AWeapon* NewWeapon, const AWeapon* OldWeapon) 
{
	CurrentWeapon = NewWeapon;
	if (CurrentWeapon) 
	{
		IKProperties = CurrentWeapon->IKProperties;
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UFPSAnimationInstance::SetIKTransforms);
	}
}



// Sets a variety of our Animation Rig's variables for the current owning player:
void UFPSAnimationInstance::SetVars(const float DeltaTime) 
{
	CameraTransform = FTransform(Character->GetBaseAimRotation(), Character->Camera->GetComponentLocation()); // Gets the players relative transform for the camera
	
	const FTransform& RootOffset = Mesh->GetSocketTransform(FName("root"), RTS_Component).Inverse() * Mesh->GetSocketTransform(FName("ik_hand_root"));
	RelativeCameraTransform = CameraTransform.GetRelativeTransform(RootOffset);
	
	ADSWeight = Character->ADSWeight;

	/*
	*  OFFSETS:
	*/

	// Accumulative Rotation:
	constexpr float AngleClamp = 6.f; // Basically just limit the amout of sway we will actually have in the case we spin our character really fast
	const FRotator& AddRotation = CameraTransform.Rotator() - LastRotation; // We want to decrement our offset to the original location so that our character will 'reposition' its gun back to its original position after swaying
	FRotator AddRotationClamped = FRotator( FMath::ClampAngle(AddRotation.Pitch, -AngleClamp, AngleClamp) * 1.5f, // Clamp our Pitch Rotation AND multiply it by 1.5 to increase its effect
		FMath::ClampAngle(AddRotation.Yaw, -AngleClamp, AngleClamp), 0.f); // Clamp our Yaw Rotation
	AddRotationClamped.Roll = AddRotationClamped.Yaw * 0.7f; // Add 70% of our Yaw rotation to our Roll so we can also have a little weapon tilt from side to side as we move around!

	AccumulativeRotation += AddRotationClamped;
	AccumulativeRotation = UKismetMathLibrary::RInterpTo(AccumulativeRotation, FRotator::ZeroRotator, DeltaTime, 30.f); // Interpolate back to our original weapon position overtime (NOTE: Inputs: (start, end, 'DeltaTime', InterpSpeed = '30.f'))
	AccumulativeRotationInterp = UKismetMathLibrary::RInterpTo(AccumulativeRotationInterp, AccumulativeRotation, DeltaTime, 5.f);  // NOTE: Inputs: ( current value = 'AccumulativeRotationInterp' (itself), target = 'AccumulativeRotation', 'DeltaTime', InterpSpeed = '5.f' )
}


// NOTE: All weapon sway offsets are oriented around our 'Sights" socket! This is so no matter the sway values, any movement will always be centered about the "Sights" socket, this ensures everything looks normal!
void UFPSAnimationInstance::CalculateWeaponSway(const float DeltaTime) 
{
	FVector LocationOffset = FVector::ZeroVector;
	FRotator RotationOffset = FRotator::ZeroRotator;

	const FRotator& AccumulativeRotationInterpInverse = AccumulativeRotationInterp.GetInverse(); // Get the inverse of our rotation offset so it appears that our gun is "lagging behind" and not "in the future" lol
	RotationOffset += AccumulativeRotationInterpInverse;

	LocationOffset += FVector(0.f, AccumulativeRotationInterpInverse.Yaw, AccumulativeRotationInterpInverse.Pitch) / 6.f; // NOTE: Normalize our 'LocationOffset' vector by dividing by some number (6 is chosen just because it happens to work well)

	// The following lines apply the weapon's 'WeightScale':
	LocationOffset *= IKProperties.WeightScale; 
	RotationOffset.Pitch *= IKProperties.WeightScale;
	RotationOffset.Yaw *= IKProperties.WeightScale;
	RotationOffset.Roll *= IKProperties.WeightScale;

	// Our final calculated offset based off all of our offset logic above: 
	OffsetTransform = FTransform(RotationOffset, LocationOffset); 
}



void UFPSAnimationInstance::SetIKTransforms() 
{
	RHandToSightsTransform = CurrentWeapon->GetSightsWorldTransform().GetRelativeTransform(Mesh->GetSocketTransform(FName("hand_r")));
}

