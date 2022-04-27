// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Runtime/Engine/Public/Net/UnrealNetwork.h"
#include "Runtime/Engine/Classes/Components/TimelineComponent.h"

#include "FPSCharacter.generated.h"

// Not sure what this does but it was made @ 7:06 Vid #2:

// BACKUP: 
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCurrentWeaponChangedDelegate, class AWeapon*, CurrentWeapon, const class AWeapon*, OldWeapon);



UCLASS()
class STARFIGHT_API AFPSCharacter : public ACharacter
{
	GENERATED_BODY()





	/*
	*  BUILT IN FUNCTIONALITIES + SETUP:
	*/

public:
	// Constructor...sets default values for this character's properties:
	AFPSCharacter(); 

protected:
	// The following 4 functions are built-in Unreal Engine functions we are overriding to add/adjust functionality!
	virtual void BeginPlay() override;
	virtual void Tick(const float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker) override;

public:	
	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Components")
		class UCameraComponent* Camera;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Components")
		class USkeletalMeshComponent* ClientMesh;





	/*
	*  ATTRIBUTES:
	*/

protected:
	// An array containing a client character's weapons:
	/*UPROPERTY(EditDefaultsOnly, Category = "Configurations")
		TArray<TSubclassOf<class AWeapon>> DefaultWeapons;*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configurations")
		TArray<TSubclassOf<class AWeapon>> DefaultWeapons;

public:
	// BACKUP:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "State")
		TArray<class AWeapon*> Weapons; // An Array of all the possible weapons our character can hold... as an array of weapon references which are replicated 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_CurrentWeapon, Category = "State")
		class AWeapon* CurrentWeapon;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "State")
	//	TArray<TSubclassOf<class AWeapon>*> Weapons; // An Array of all the possible weapons our character can hold... as an array of weapon references which are replicated 

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_CurrentWeapon, Category = "State")
	//	class TSubclassOf<class AWeapon>* CurrentWeapon;











	UPROPERTY(BlueprintAssignable, Category = "Delegates")
		FCurrentWeaponChangedDelegate CurrentWeaponChangedDelegate; // Called whenever 'CurrentWeapon' is changed:

	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "State")
		int32 CurrentIndex = 0; // Keeps track of the current index we are on in the weapons array (This is so that we can switch our equiped gun by simply changing our index in the weapons array


	UPROPERTY(BlueprintReadWrite, Replicated, Category = "Attributes")
		float Health;
	UPROPERTY(BlueprintReadWrite, Category = "Attributes")
		float MaxHealth;

	UPROPERTY(BlueprintReadWrite, Replicated, Category = "Attributes")
		float Armor;
	UPROPERTY(BlueprintReadWrite, Category = "Attributes")
		float MaxArmor;

	UPROPERTY(BlueprintReadWrite, Replicated, Category = "Attributes")
		int32 KillCount;




	/*
	*  GENERAL FUNCTIONS:
	*/
	UFUNCTION(BlueprintCallable, Category = "Character")
		virtual void EquipWeapon(const int32 Index);
	








protected:

	// BACKUP:
	/*
	* When we switch weapons, 'OldWeapon' will be a reference to the previous weapon we just had equiped, 
	* and because it is replicated, will have the currnt weapon as well:
	*/
	UFUNCTION()
		virtual void OnRep_CurrentWeapon(const class AWeapon* OldWeapon);
	/*
	* The following function is an RPC call for our server to update the owner player's current weapon to all of the other clients
	* NOTE: An RPC is a function that is called on a client to run on the server with the intention of updating a variable that calling client wants to update,
	*		which will be reflected to all the other clients
	*/     
	UFUNCTION(Server, Reliable)
		void Server_SetCurrentWeapon(class AWeapon* WeaponREF);
		virtual void Server_SetCurrentWeapon_Implementation(class AWeapon* NewWeapon);



	//    MODIFIED (Currently not working):
	///*
	//* When we switch weapons, 'OldWeapon' will be a reference to the previous weapon we just had equiped,
	//* and because it is replicated, will have the currnt weapon as well:
	//*/
	//UFUNCTION()
	//	virtual void OnRep_CurrentWeapon(const class TSubclassOf<class AWeapon>* OldWeapon);

	///*
	//* The following function is an RPC call for our server to update the owner player's current weapon to all of the other clients
	//* NOTE: An RPC is a function that is called on a client to run on the server with the intention of updating a variable that calling client wants to update,
	//*		which will be reflected to all the other clients
	//*/
	//UFUNCTION(Server, Reliable)
	//	void Server_SetCurrentWeapon(class TSubclassOf<class AWeapon>* WeaponREF);
	//virtual void Server_SetCurrentWeapon_Implementation(class TSubclassOf<class AWeapon>* NewWeapon);

















	/*
	*  CHARACTER CONTROLS + RELEVENT VARIABLES:
	*/
public:
	UFUNCTION(BlueprintCallable, Category = "Animation")
		virtual void StartAiming();

	UFUNCTION(BlueprintCallable, Category = "Animation")
		virtual void ReverseAiming();

	// THIS IS ARE ACTUAL AIMING VALUE (Must be replicated so that other clients will see the owner client aim!):
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Animation")
		float ADSWeight = 0.f;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Configurations|Animation")
		class UCurveFloat* AimingCurve; // Just a timeline for interpolating between our animations

	FTimeline AimingTimeline;



	UFUNCTION(Server, Reliable)
		void Server_Aim(const bool bForward = true);
	virtual FORCEINLINE void Server_Aim_Implementation(const bool bForward) { // Just calls our 'Multi_Aim' function and its implementation:
		Multi_Aim(bForward); // This is the actual RPC passed in 
		Multi_Aim_Implementation(bForward); // Will also run its implementation locally! This is because the net multicast will not run on the server by default, we we gotta call it separately for our local client!
	}

	UFUNCTION(NetMulticast, Reliable)
		void Multi_Aim(const bool bForward);
	virtual void Multi_Aim_Implementation(const bool bForward);












	UFUNCTION()
		virtual void TimelineProgress(const float Value);






	virtual void NextWeapon();
	virtual void PreviousWeapon();

	void MoveForward(const float Value);
	void MoveRight(const float Value);
	void LookUp(const float Value);
	void LookRight(const float Value);

	void CheckJumping();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Actions")
		bool isAiming;
	UPROPERTY()
		bool isJumping;
};
