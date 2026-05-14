// Fill out your copyright notice in the Description page of Project Settings.

#include "Variant_SDTA\Core\Game\SDTAPlayerState.h"
#include "Variant_SDTA\Weapons\SDTAWeaponManager.h"
#include "Net/UnrealNetwork.h"

ASDTAPlayerState::ASDTAPlayerState()
{
	// 初始化默认值
	PlayerSoulFragments = 0;
	Kills = 0;
	Deaths = 0;
	WavesSurvived = 0;
}

void ASDTAPlayerState::BeginPlay()
{
	Super::BeginPlay();

	WeaponManager = NewObject<USDTAWeaponManager>();
	WeaponManager->Initialize(this);

	UE_LOG(LogTemp, Log, TEXT("玩家状态初始化武器管理器"));
}

void ASDTAPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 复制玩家状态属性
	DOREPLIFETIME(ASDTAPlayerState, PlayerSoulFragments);
	DOREPLIFETIME(ASDTAPlayerState, PersonalUpgrades);
	DOREPLIFETIME(ASDTAPlayerState, Kills);
	DOREPLIFETIME(ASDTAPlayerState, Deaths);
	DOREPLIFETIME(ASDTAPlayerState, WavesSurvived);
}

void ASDTAPlayerState::AddSoulFragments(int32 Amount)
{
	PlayerSoulFragments += Amount;
}

void ASDTAPlayerState::AddPersonalUpgrade(const FName& UpgradeName)
{
	if (!PersonalUpgrades.Contains(UpgradeName))
	{
		PersonalUpgrades.Add(UpgradeName);
	}
}

void ASDTAPlayerState::IncrementKills()
{
	Kills++;
}

void ASDTAPlayerState::IncrementDeaths()
{
	Deaths++;
}

void ASDTAPlayerState::IncrementWavesSurvived()
{
	WavesSurvived++;
}
