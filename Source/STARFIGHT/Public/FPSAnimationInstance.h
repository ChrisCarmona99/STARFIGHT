// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Weapon.h"
#include "FPSAnimationInstance.generated.h"

UCLASS()
class STARFIGHT_API UFPSAnimationInstance : public UAnimInstance
{
	GENERATED_BODY()
public:
	// Constructor definition:
	UFPSAnimationInstance();

protected:
	virtual void NativeBeginPlay() override;
	virtual void NativeUpdateAnimation(float DeltaTime) override;
	
	UFUNCTION()
	virtual void CurrentWeaponChanged(class AWeapon* NewWeapon, const class AWeapon* OldWeapon);
	UFUNCTION()
	virtual void SetVars(const float DeltaTime);
	UFUNCTION()
	virtual void CalculateWeaponSway(const float DeltaTime);

	// Call this once everytime we equip a weapon:
	UFUNCTION()
	virtual void SetIKTransforms();

public:
	/*
	*  REFERENCES from the character:
	*/
	UPROPERTY(BlueprintReadWrite, Category = "Animation")
		class AFPSCharacter* Character;

	UPROPERTY(BlueprintReadWrite, Category = "Animation")
		class USkeletalMeshComponent* Mesh;

	UPROPERTY(BlueprintReadWrite, Category = "Animation")
		class AWeapon* CurrentWeapon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
		FIKProperties IKProperties;

	/*
	*   STATE
	*/
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
		FRotator LastRotation; // Used to get the difference for our 'AccumulativeRotation' variable

	/*
	*  IK VARIABLES:
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
		FTransform CameraTransform;

	// World Transform of the camera, BUT relative to the base of the mesh:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
		FTransform RelativeCameraTransform;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
		FTransform RHandToSightsTransform;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
		FTransform OffsetTransform;

	UPROPERTY(EditAnywhere, BlueprintReadWRite, Category = "Animation")
		float ADSWeight = 0.f; // 0 = Idle, 1 = Aiming... we will shift bewteen our animations by interpolating this value between 0 and 1


	/*
	*  ACCUMULATIVE OFFSETS (just a bunch of variables we will use as addatives to adjust what effects our weapon sway (looking around fast, sudden stops, falling, etc.):
	*/
	UPROPERTY(BlueprintReadWrite, Category = "Animation")
		FRotator AccumulativeRotation;

	UPROPERTY(BlueprintReadWrite, Category = "Animation")
		FRotator AccumulativeRotationInterp; // This will allow for interpolation between our added weapon swaying offsets and the weapon's original position so that way there is a smooth transition from our weapon's offseted position to its original position



};

