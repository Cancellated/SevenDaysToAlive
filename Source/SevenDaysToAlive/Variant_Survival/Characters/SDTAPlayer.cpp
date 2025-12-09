// Fill out your copyright notice in the Description page of Project Settings.


#include "Variant_Survival/Characters/SDTAPlayer.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
ASDTAPlayer::ASDTAPlayer()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// 启用角色的网络复制
	bReplicates = true;

	// 初始化复制属性
	Health = 100.0f;
	Stamina = 100.0f;
}

// Called when the game starts or when spawned
void ASDTAPlayer::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ASDTAPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void ASDTAPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

// 设置需要复制的属性
void ASDTAPlayer::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 使用DOREPLIFETIME宏添加需要复制的属性
	DOREPLIFETIME(ASDTAPlayer, Health);
	DOREPLIFETIME(ASDTAPlayer, Stamina);
}

// Server_SetHealth的实现
void ASDTAPlayer::Server_SetHealth_Implementation(float NewHealth)
{
	Health = FMath::Clamp(NewHealth, 0.0f, 100.0f);
	// 通知所有客户端更新HUD
	Client_UpdateHUD(Health, Stamina);
}

bool ASDTAPlayer::Server_SetHealth_Validate(float NewHealth)
{
	// 验证输入值是否有效
	return NewHealth >= 0.0f && NewHealth <= 100.0f;
}

// Client_UpdateHUD的实现
void ASDTAPlayer::Client_UpdateHUD_Implementation(float NewHealth, float NewStamina)
{
	// 客户端更新HUD的实现
	// 这里可以添加UI更新逻辑
}

// Multicast_PlaySound的实现
void ASDTAPlayer::Multicast_PlaySound_Implementation(USoundBase* SoundToPlay)
{
	if (SoundToPlay)
	{
		// 在客户端播放声音
		UGameplayStatics::PlaySoundAtLocation(this, SoundToPlay, GetActorLocation());
	}
}

// 网络角色检查辅助方法
bool ASDTAPlayer::IsLocallyControlled() const
{
	return GetLocalRole() == ROLE_AutonomousProxy || GetLocalRole() == ROLE_Authority;
}

bool ASDTAPlayer::IsServer() const
{
	return GetLocalRole() == ROLE_Authority;
}

