// Fill out your copyright notice in the Description page of Project Settings.


#include "Variant_Survival/Characters/SDTAPlayer.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"

// Sets default values
ASDTAPlayer::ASDTAPlayer()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// 启用角色的网络复制
	bReplicates = true;

	// 初始化角色统计属性
	MaxHealth = 100.0f;
	MaxStamina = 100.0f;
	Health = MaxHealth;
	Stamina = MaxStamina;



	// 配置角色移动
	if (GetCharacterMovement())
	{
		// 角色朝向移动方向
		GetCharacterMovement()->bOrientRotationToMovement = true;
		// 设置旋转速度
		GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
		// 禁用控制器俯仰旋转
		bUseControllerRotationPitch = false;
		bUseControllerRotationRoll = false;
	}
}

// Called when the game starts or when spawned
void ASDTAPlayer::BeginPlay()
{
	Super::BeginPlay();
	
	// 广播初始健康值和能量值
	OnHealthChanged.Broadcast(Health / MaxHealth);
	OnStaminaChanged.Broadcast(Stamina / MaxStamina);
}

// Called every frame
void ASDTAPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called when the actor is removed from the world
void ASDTAPlayer::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// 清理所有委托，防止访问已销毁的对象
	OnHealthChanged.Clear();
	OnStaminaChanged.Clear();
	OnDeath.Clear();
}

// Called to bind functionality to input
void ASDTAPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// 基类已经设置了增强输入组件
	// 可以在这里添加额外的输入绑定
}

// 设置需要复制的属性
void ASDTAPlayer::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	#define ThisClass ASDTAPlayer
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 使用DOREPLIFETIME宏添加需要复制的属性
	DOREPLIFETIME(ASDTAPlayer, Health);
	DOREPLIFETIME(ASDTAPlayer, Stamina);
	DOREPLIFETIME(ASDTAPlayer, MaxHealth);
	DOREPLIFETIME(ASDTAPlayer, MaxStamina);
	#undef ThisClass
}

// Server_SetHealth的实现
void ASDTAPlayer::Server_SetHealth_Implementation(float NewHealth)
{
	Health = FMath::Clamp(NewHealth, 0.0f, MaxHealth);
	// 通知所有客户端更新HUD
	Client_UpdateHUD(Health, Stamina);
	// 广播健康值变化事件
	OnHealthChanged.Broadcast(Health / MaxHealth);
}

bool ASDTAPlayer::Server_SetHealth_Validate(float NewHealth)
{
	// 验证输入值是否有效
	return NewHealth >= 0.0f && NewHealth <= MaxHealth;
}

// Server_SetStamina的实现
void ASDTAPlayer::Server_SetStamina_Implementation(float NewStamina)
{
	Stamina = FMath::Clamp(NewStamina, 0.0f, MaxStamina);
	// 通知所有客户端更新HUD
	Client_UpdateHUD(Health, Stamina);
	// 广播能量值变化事件
	OnStaminaChanged.Broadcast(Stamina / MaxStamina);
}

bool ASDTAPlayer::Server_SetStamina_Validate(float NewStamina)
{
	// 验证输入值是否有效
	return NewStamina >= 0.0f && NewStamina <= MaxStamina;
}



// 角色状态检查
bool ASDTAPlayer::IsAlive() const
{
	return Health > 0.0f;
}

// 处理伤害
float ASDTAPlayer::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (!IsAlive()) return 0.0f;
	
	// 调用父类的TakeDamage方法
	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	
	// 在服务器上处理伤害（确保网络一致性）
	if (IsServer())
	{
		Health = FMath::Clamp(Health - ActualDamage, 0.0f, MaxHealth);
		
		// 通知所有客户端更新HUD
		Client_UpdateHUD(Health, Stamina);
		
		// 广播健康值变化事件
		OnHealthChanged.Broadcast(Health / MaxHealth);
		
		// 检查是否死亡
		if (Health <= 0.0f)
		{
			Die();
		}
	}
	
	return ActualDamage;
}

// 角色死亡处理
void ASDTAPlayer::Die()
{
	// 仅在服务器上处理死亡逻辑
	if (!IsServer()) return;
	
	// 停止角色移动
	GetCharacterMovement()->StopMovementImmediately();
	
	// 禁用输入
	DisableInput(nullptr);
	
	// 广播死亡事件
	OnDeath.Broadcast();
	
	// 销毁角色，控制器会处理重生逻辑
	Destroy();
}

// 治疗角色
void ASDTAPlayer::Heal(float HealAmount)
{
	if (!IsAlive() || HealAmount <= 0.0f) return;
	
	// 在服务器上处理治疗（确保网络一致性）
	if (IsServer())
	{
		Health = FMath::Clamp(Health + HealAmount, 0.0f, MaxHealth);
		
		// 通知所有客户端更新HUD
		Client_UpdateHUD(Health, Stamina);
		
		// 广播健康值变化事件
		OnHealthChanged.Broadcast(Health / MaxHealth);
	}
	else
	{
		// 客户端请求服务器处理治疗
		Server_SetHealth(Health + HealAmount);
	}
}

// 消耗能量值
void ASDTAPlayer::ConsumeStamina(float Amount)
{
	if (!IsAlive() || Amount <= 0.0f) return;
	
	// 在服务器上处理能量消耗（确保网络一致性）
	if (IsServer())
	{
		Stamina = FMath::Clamp(Stamina - Amount, 0.0f, MaxStamina);
		
		// 通知所有客户端更新HUD
		Client_UpdateHUD(Health, Stamina);
		
		// 广播能量值变化事件
		OnStaminaChanged.Broadcast(Stamina / MaxStamina);
	}
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

