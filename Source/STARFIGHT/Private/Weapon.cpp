// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon.h"



// Sets default values
AWeapon::AWeapon()
{
 //	// set this actor to call tick() every frame.  you can turn this off to improve performance if you don't need it.
	//primaryactortick.bcanevertick = false;

	//setreplicates(true);

	//root = createdefaultsubobject<uscenecomponent>(text("root"));
	//rootcomponent = root;

	//weaponmesh = createdefaultsubobject<uskeletalmeshcomponent>(text("weaponmesh"));
	//weaponmesh->setupattachment(root);

	//// preset all of our base weapon stats:
	//currentclip = 30;
	//clipsize = 30;

	//reserves = 1000;
	//maxreserves = 1000;

	//firerate = 0.15f;
	//weapondamage = 30.0f;
}

// Called when the game starts or when spawned
void AWeapon::BeginPlay()
{
	Super::BeginPlay();

	/*
	*  We will first spawn our weapon on the server, which will then replicate the spawned weapon to the client,
	*  and then if it is equiped, then it will be visible to the player / other connected players:
	*/
	// if (!CurrentOwner) { // Only call if the 'CurrentOwner' is not set...
	// 	WeaponMesh->SetVisibility(false); // ... so, this will only apply if it's our initial equiped weapon
	// }
	
}

void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Set these variables to replicate on the server (NOTE: We are using the 'DOREPLIFETIME' that includes a condition... we don't need this but we are doing it anyways cause why not :) ... at least for now ):
	DOREPLIFETIME_CONDITION(AWeapon, CurrentClip, COND_None);
	//DOREPLIFETIME_CONDITION(AWeapon, ClipSize, COND_None);
	DOREPLIFETIME_CONDITION(AWeapon, Reserves, COND_None);
	//DOREPLIFETIME_CONDITION(AWeapon, MaxReserves, COND_None);
}





bool AWeapon::Reload()
{
	if (CurrentClip < ClipSize) 
	{
		//AmmoDifference = ClipSize - CurrentClip;
		//CurrentClip = CurrentClip + AmmoDifference;

		if (Reserves > (ClipSize - CurrentClip)) 
		{
			AmmoDifference = ClipSize - CurrentClip;
			CurrentClip = CurrentClip + AmmoDifference;
			Reserves = Reserves - AmmoDifference;
			MarkForReloading = true;
		}
		else 
		{
			AmmoDifference = ClipSize - CurrentClip;
			CurrentClip = CurrentClip + Reserves;
			Reserves = 0;
			MarkForReloading = true;
		}
	}
	else {
		// Print "Clip is Full!" to the screen
	}
	return MarkForReloading;
}



void AWeapon::Multicast_UpdateCurrentClip_Implementation()
{
	//UE_LOG(LogTemp, Warning, TEXT("Called 1"));
	CurrentClip -= 1;
}

void AWeapon::Base_UpdateCurrentClip()
{
	//UE_LOG(LogTemp, Warning, TEXT("Called 2"));
	CurrentClip -= 1;
}

void AWeapon::Multicast_AddAmmo_Implementation(const int32 AddedAmmo)
{
	int32 NewReserves = Reserves + AddedAmmo;
	NewReserves > MaxReserves ? Reserves = MaxReserves : Reserves = NewReserves;
}

void AWeapon::Base_AddAmmo(int32 AddedAmmo)
{
	int32 NewReserves = Reserves + AddedAmmo;
	NewReserves > MaxReserves ? Reserves = MaxReserves : Reserves = NewReserves;
}
