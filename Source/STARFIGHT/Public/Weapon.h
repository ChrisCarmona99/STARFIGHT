// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Runtime/Engine/Public/Net/UnrealNetwork.h"

#include "Weapon.generated.h"



USTRUCT(BlueprintType)
struct FIKProperties {
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		class UAnimSequence* AnimPose; // This is our base pose for the inverse kinematics

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FTransform CustomOffsetTransform; // The offset from our base post to where we actually want the weapon to be

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float AimOffset = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float WeightScale = 1.f; // NOTE: A higher weight scale will mean the gun is HEAVIER, thus a greater offset when moving, making the gun appear heavier!... and Vise versa for a lower weight scale
};



UCLASS(Abstract)
class STARFIGHT_API AWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AWeapon();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Component")
		class USceneComponent* Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Category")
		class USkeletalMeshComponent* WeaponMesh;  // Skeletal Mesh of our Character


	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "State")
		class AFPSCharacter* CurrentOwner; // Just a reference to our current owner (which is our 'FPSCharacter')


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configurations")
		FIKProperties IKProperties;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configurations")
		FTransform PlacementTransform; // The orientation offset from where the gun is connected to our skeletans hand component

	

	/*
	*  ATTRIBUTES:
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Attributes", Meta = (ExposeOnSpawn=true), Meta = (InstanceEditable=true))
		int32 CurrentClip;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
		int32 ClipSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Attributes", Meta = (ExposeOnSpawn = true), Meta = (InstanceEditable = true))
		int32 Reserves;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
		int32 MaxReserves;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
		int32 AmmoDifference;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
		float FireRate;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
		float WeaponDamage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
		bool MarkForReloading;



	UFUNCTION(BlueprintCallable, Category = "Functionality")
		bool Reload();


	/*
	* Will return the transform of our weapon's sights!
	* NOTE: 'BlueprintNativeEvent' means that this function can have two implementations, one in C++ & one in Blueprint
	*		 Basically, if we create a new blueprint weapon (like our rifle & shotgun), then we can give it a different 
	*		 sights transform apart from the one we are going to define as a 'base' sights transform in our C++ implementation
	*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "IK")
		FTransform GetSightsWorldTransform() const;
	virtual FORCEINLINE FTransform GetSightsWorldTransform_Implementation() const { return WeaponMesh->GetSocketTransform(FName("Sights")); } // the "Sights" is just whatever the socket name for our "Sights" socket is!


	// 'UpdateCurrentClip()' Server & Multicast definitions:
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Functionality")
		void Server_UpdateCurrentClip();
	virtual FORCEINLINE void Server_UpdateCurrentClip_Implementation() {
		Multicast_UpdateCurrentClip();
		Multicast_UpdateCurrentClip_Implementation();
	}

	UFUNCTION(NetMulticast, Reliable)
		void Multicast_UpdateCurrentClip();
	virtual void Multicast_UpdateCurrentClip_Implementation();

	UFUNCTION(BlueprintCallable, Category = "Functionality")
		void Base_UpdateCurrentClip();



	// 'AddAmmo()' Server & Multicast Definitions:
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Functionality")
		void Server_AddAmmo(const int32 AddedAmmo = 50);
	virtual FORCEINLINE void Server_AddAmmo_Implementation(const int32 AddedAmmo) {
		Multicast_AddAmmo(AddedAmmo);
		Multicast_AddAmmo_Implementation(AddedAmmo);
	}

	UFUNCTION(NetMulticast, Reliable)
		void Multicast_AddAmmo(const int32 AddedAmmo);
	virtual void Multicast_AddAmmo_Implementation(int32 AddedAmmo);



	UFUNCTION(BlueprintCallable, Category = "Functionality")
		void Base_AddAmmo(const int32 AddedAmmo);
};
