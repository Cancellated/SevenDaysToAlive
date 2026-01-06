// Fill out your copyright notice in the Description page of Project Settings.


#include "Variant_Survival/Characters/SDTAPlayer.h"
#include "Variant_Survival/Controller/SDTAPlayerController.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "SevenDaysToAlive.h"

// Sets default values
ASDTAPlayer::ASDTAPlayer()
{
	UE_LOG(LogSevenDaysToAlive, Log, TEXT("[SDTAPlayer] %s - 构造函数初始化"), *GetName());

 	// 设置此角色每一帧调用Tick()。如果不需要，可以关闭以提高性能。
	PrimaryActorTick.bCanEverTick = true;

	// 启用角色的网络复制
	bReplicates = true;

	// 创建健康组件
	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
	UE_LOG(LogSevenDaysToAlive, Log, TEXT("[SDTAPlayer] %s - 健康组件已创建"), *GetName());
	
	// 创建耐力组件
	StaminaComponent = CreateDefaultSubobject<UStaminaSystemComponent>(TEXT("StaminaComponent"));
	UE_LOG(LogSevenDaysToAlive, Log, TEXT("[SDTAPlayer] %s - 耐力组件已创建"), *GetName());
	
	// 创建冲刺组件
	DashComponent = CreateDefaultSubobject<UDashComponent>(TEXT("DashComponent"));
	UE_LOG(LogSevenDaysToAlive, Log, TEXT("[SDTAPlayer] %s - 冲刺组件已创建"), *GetName());
	
	// 创建武器组件
	WeaponComponent = CreateDefaultSubobject<UWeaponComponent>(TEXT("WeaponComponent"));
	UE_LOG(LogSevenDaysToAlive, Log, TEXT("[SDTAPlayer] %s - 武器组件已创建"), *GetName());

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
		UE_LOG(LogSevenDaysToAlive, Log, TEXT("[SDTAPlayer] %s - 角色移动组件已配置"), *GetName());
	}
}

// Called when the game starts or when spawned
void ASDTAPlayer::BeginPlay()
{
	Super::BeginPlay();
	
	UE_LOG(LogSevenDaysToAlive, Log, TEXT("[SDTAPlayer] %s - BeginPlay开始"), *GetName());
	
	// 设置组件委托回调
	if (HealthComponent)
	{
		// 绑定健康值变化委托
		HealthComponent->OnHealthChanged.AddDynamic(this, &ASDTAPlayer::OnHealthChangedInternal);
		
		// 绑定低健康值警告委托
		HealthComponent->OnHealthLowWarning.AddDynamic(this, &ASDTAPlayer::OnHealthLowWarningInternal);
		
		// 绑定死亡委托
		HealthComponent->OnDeath.AddDynamic(this, &ASDTAPlayer::OnDeathInternal);
		
		// 广播初始健康值
		float HealthPercent = HealthComponent->MaxHealth > 0.0f ? HealthComponent->Health / HealthComponent->MaxHealth : 0.0f;
		OnHealthChanged.Broadcast(HealthPercent);
		
		UE_LOG(LogSevenDaysToAlive, Log, TEXT("[SDTAPlayer] %s - 健康组件委托已绑定，初始健康: %.2f/%.2f (%.1f%%)"), 
			*GetName(), HealthComponent->Health, HealthComponent->MaxHealth, HealthPercent * 100.0f);
	}
	
	if (StaminaComponent)
	{
		// 绑定能量值变化委托
		StaminaComponent->OnStaminaChanged.AddDynamic(this, &ASDTAPlayer::OnStaminaChangedInternal);
		
		// 绑定低能量值警告委托
		StaminaComponent->OnStaminaLowWarning.AddDynamic(this, &ASDTAPlayer::OnStaminaLowWarningInternal);
		
		// 广播初始能量值
		float StaminaPercent = StaminaComponent->MaxStamina > 0.0f ? StaminaComponent->Stamina / StaminaComponent->MaxStamina : 0.0f;
		OnStaminaChanged.Broadcast(StaminaPercent);
		
		UE_LOG(LogSevenDaysToAlive, Log, TEXT("[SDTAPlayer] %s - 耐力组件委托已绑定，初始耐力: %.2f/%.2f (%.1f%%)"), 
			*GetName(), StaminaComponent->Stamina, StaminaComponent->MaxStamina, StaminaPercent * 100.0f);
		
		// 初始化冲刺组件
		if (DashComponent)
		{
			DashComponent->SetStaminaComponent(StaminaComponent);
			UE_LOG(LogSevenDaysToAlive, Log, TEXT("[SDTAPlayer] %s - 冲刺组件已初始化"), *GetName());
		}
	}
	
	// 初始化武器组件
	if (WeaponComponent)
	{
		UE_LOG(LogSevenDaysToAlive, Log, TEXT("[SDTAPlayer] %s - 武器组件已初始化"), *GetName());
	}
}

// Called every frame
void ASDTAPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called when the actor is removed from the world
void ASDTAPlayer::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	UE_LOG(LogSevenDaysToAlive, Log, TEXT("[SDTAPlayer] %s - EndPlay，原因: %d"), *GetName(), (int32)EndPlayReason);

	// 移除组件委托绑定
	if (HealthComponent)
	{
		HealthComponent->OnHealthChanged.RemoveDynamic(this, &ASDTAPlayer::OnHealthChangedInternal);
		HealthComponent->OnHealthLowWarning.RemoveDynamic(this, &ASDTAPlayer::OnHealthLowWarningInternal);
		HealthComponent->OnDeath.RemoveDynamic(this, &ASDTAPlayer::OnDeathInternal);
		UE_LOG(LogSevenDaysToAlive, Log, TEXT("[SDTAPlayer] %s - 健康组件委托已解绑"), *GetName());
	}
	
	if (StaminaComponent)
	{
		StaminaComponent->OnStaminaChanged.RemoveDynamic(this, &ASDTAPlayer::OnStaminaChangedInternal);
		StaminaComponent->OnStaminaLowWarning.RemoveDynamic(this, &ASDTAPlayer::OnStaminaLowWarningInternal);
		UE_LOG(LogSevenDaysToAlive, Log, TEXT("[SDTAPlayer] %s - 耐力组件委托已解绑"), *GetName());
	}
	
	// 武器组件会自动处理武器资源的清理
	
	Super::EndPlay(EndPlayReason);
}

// 内部健康值变化处理方法
void ASDTAPlayer::OnHealthChangedInternal(float HealthPercent)
{
	UE_LOG(LogSevenDaysToAlive, Log, TEXT("[SDTAPlayer] %s - 健康值变化: %.1f%%"), *GetName(), HealthPercent * 100.0f);

	// 调用原始的健康值变化委托
	OnHealthChanged.Broadcast(HealthPercent);
}

// 内部低健康值警告处理方法
void ASDTAPlayer::OnHealthLowWarningInternal()
{
	UE_LOG(LogSevenDaysToAlive, Warning, TEXT("[SDTAPlayer] %s - 低健康值警告"), *GetName());

	// 调用原始的低健康值警告委托
	OnHealthLowWarning.Broadcast();
}

// 内部死亡处理方法
void ASDTAPlayer::OnDeathInternal()
{
	// 调用原始的死亡委托
	OnDeath.Broadcast();
	
	// 执行玩家死亡逻辑
	Die();
}

// 内部能量值变化处理方法
void ASDTAPlayer::OnStaminaChangedInternal(float StaminaPercent)
{
	// UE_LOG(LogSevenDaysToAlive, Log, TEXT("[SDTAPlayer] %s - 耐力值变化: %.1f%%"), *GetName(), StaminaPercent * 100.0f);

	// 调用原始的能量值变化委托
	OnStaminaChanged.Broadcast(StaminaPercent);
}

// 内部低能量值警告处理方法
void ASDTAPlayer::OnStaminaLowWarningInternal()
{
	UE_LOG(LogSevenDaysToAlive, Warning, TEXT("[SDTAPlayer] %s - 低耐力值警告"), *GetName());

	// 调用原始的低能量值警告委托
	OnStaminaLowWarning.Broadcast();
}

// Called to bind functionality to input
void ASDTAPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// 基类已经设置了增强输入组件
	// 可以在这里添加额外的输入绑定
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Dashing
		// 冲刺绑定
		EnhancedInputComponent->BindAction(DashAction, ETriggerEvent::Started, this, &ASDTAPlayer::DoDash);
		
		// 武器相关输入绑定到WeaponComponent
		if (WeaponComponent)
		{
			// Firing
			// 开火绑定
			EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, WeaponComponent, &UWeaponComponent::StartFiring);
			EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, WeaponComponent, &UWeaponComponent::StopFiring);
			
			// Reload
			// 换弹绑定
			EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Started, WeaponComponent, &UWeaponComponent::Reload);

			// Switch Weapon
			// 武器切换绑定
			EnhancedInputComponent->BindAction(SwitchWeaponAction, ETriggerEvent::Started, WeaponComponent, &UWeaponComponent::SwitchToNextWeapon);
		}

	}
	else
	{
		UE_LOG(LogSevenDaysToAlive, Error, TEXT("'%s' Failed to find an Enhanced Input Component!"), *GetNameSafe(this));
	}
}

// 设置需要复制的属性
void ASDTAPlayer::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}





// 角色状态检查
bool ASDTAPlayer::IsAlive() const
{
	if (HealthComponent)
	{
		return !HealthComponent->IsDead();
	}
	return false;
}

// 处理伤害
float ASDTAPlayer::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (!IsAlive()) 
	{
		UE_LOG(LogSevenDaysToAlive, Warning, TEXT("[SDTAPlayer] %s - 受到伤害失败: 角色已死亡"), *GetName());
		return 0.0f;
	}
	
	UE_LOG(LogSevenDaysToAlive, Log, TEXT("[SDTAPlayer] %s - 受到 %.2f 伤害，来源: %s，造成者: %s"), 
		*GetName(), 
		DamageAmount, 
		EventInstigator ? *EventInstigator->GetName() : TEXT("无"), 
		DamageCauser ? *DamageCauser->GetName() : TEXT("无"));
	
	// 调用父类的TakeDamage方法
	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	
	UE_LOG(LogSevenDaysToAlive, Log, TEXT("[SDTAPlayer] %s - 实际受到 %.2f 伤害"), *GetName(), ActualDamage);
	
	// 在服务器上处理伤害（确保网络一致性）
	if (IsServer() && HealthComponent)
	{
		// 使用健康组件处理伤害
		HealthComponent->RemoveHealth(ActualDamage);
		
		// 健康组件会自动处理死亡事件，不需要在这里检查
	}
	
	return ActualDamage;
}

// 角色死亡处理
void ASDTAPlayer::Die()
{
	// 仅在服务器上处理死亡逻辑
	if (!IsServer()) 
	{
		UE_LOG(LogSevenDaysToAlive, Warning, TEXT("[SDTAPlayer] %s - 死亡处理: 非服务器端，跳过"), *GetName());
		return;
	}
	
	UE_LOG(LogSevenDaysToAlive, Warning, TEXT("[SDTAPlayer] %s - 角色死亡，位置: %s"), 
		*GetName(), *GetActorLocation().ToString());
	
	// 停止角色移动
	GetCharacterMovement()->StopMovementImmediately();
	UE_LOG(LogSevenDaysToAlive, Log, TEXT("[SDTAPlayer] %s - 已停止移动"), *GetName());
	
	// 禁用输入
	DisableInput(nullptr);
	UE_LOG(LogSevenDaysToAlive, Log, TEXT("[SDTAPlayer] %s - 已禁用输入"), *GetName());
	
	// 广播死亡事件
	OnDeath.Broadcast();
	UE_LOG(LogSevenDaysToAlive, Log, TEXT("[SDTAPlayer] %s - 已广播死亡事件"), *GetName());
	
	// 销毁角色，控制器会处理重生逻辑
	Destroy();
	UE_LOG(LogSevenDaysToAlive, Log, TEXT("[SDTAPlayer] %s - 角色已销毁"), *GetName());
}

// 治疗角色
void ASDTAPlayer::Heal(float HealAmount)
{
	if (!IsAlive() || HealAmount <= 0.0f) return;
	
	// 在服务器上处理治疗（确保网络一致性）
	if (IsServer() && HealthComponent)
	{
		// 使用健康组件处理治疗
		HealthComponent->AddHealth(HealAmount);
		
		// 健康组件会自动广播健康值变化事件，不需要在这里重复广播
	}
}

// 消耗能量值
void ASDTAPlayer::ConsumeStamina(float Amount)
{
	if (!IsAlive() || Amount <= 0.0f) return;
	
	// 在服务器上处理能量消耗（确保网络一致性）
	if (IsServer() && StaminaComponent)
	{
		// 使用耐力组件处理能量消耗
		StaminaComponent->ConsumeStamina(Amount);
		
		// 耐力组件会自动处理能量回复和广播事件，不需要在这里重复处理
	}
}



// Client_UpdateHUD的实现
void ASDTAPlayer::Client_UpdateHUD_Implementation()
{
	// 从组件中获取当前的健康和耐力值
	float CurrentHealth = HealthComponent ? HealthComponent->Health : 0.0f;
	float CurrentStamina = StaminaComponent ? StaminaComponent->Stamina : 0.0f;
	
	// 客户端更新HUD的实现
	// 这里可以添加UI更新逻辑，使用CurrentHealth和CurrentStamina
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



// 标准化输入处理方法实现

/** 处理瞄准输入 */
void ASDTAPlayer::DoAim(float Yaw, float Pitch)
{
	// 确保是本地控制的角色
	if (!IsLocallyControlled()) return;

	// UE_LOG(LogSevenDaysToAlive, Log, TEXT("[SDTAPlayer] %s - 瞄准输入: Yaw=%.2f, Pitch=%.2f"), *GetName(), Yaw, Pitch);

	// 获取控制器
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;

	// 应用旋转
	PC->AddYawInput(Yaw);
	PC->AddPitchInput(Pitch);
}

/** 处理移动输入 */
void ASDTAPlayer::DoMove(float Right, float Forward)
{
	// 确保控制器有效（与父类保持一致的实现）
	if (!GetController()) return;

	// UE_LOG(LogSevenDaysToAlive, Log, TEXT("[SDTAPlayer] %s - 移动输入: Right=%.2f, Forward=%.2f"), *GetName(), Right, Forward);

	// 获取移动方向（与父类保持一致的实现）
	AddMovementInput(GetActorRightVector(), Right);
	AddMovementInput(GetActorForwardVector(), Forward);
}

/** 处理跳跃开始输入 */
void ASDTAPlayer::DoJumpStart()
{
	// 确保是本地控制的角色
	if (!IsLocallyControlled()) return;

	UE_LOG(LogSevenDaysToAlive, Log, TEXT("[SDTAPlayer] %s - 跳跃开始"), *GetName());

	// 开始跳跃
	Jump();
}

/** 处理跳跃结束输入 */
void ASDTAPlayer::DoJumpEnd()
{
	// 确保是本地控制的角色
	if (!IsLocallyControlled()) return;

	UE_LOG(LogSevenDaysToAlive, Log, TEXT("[SDTAPlayer] %s - 跳跃结束"), *GetName());

	// 结束跳跃
	StopJumping();
}

/** 处理冲刺输入 */
void ASDTAPlayer::DoDash()
{
	// 确保是本地控制的角色
	if (!IsLocallyControlled()) 
	{
		UE_LOG(LogSevenDaysToAlive, Warning, TEXT("[SDTAPlayer] %s - 冲刺输入: 非本地控制，跳过"), *GetName());
		return;
	}

	UE_LOG(LogSevenDaysToAlive, Log, TEXT("[SDTAPlayer] %s - 冲刺输入: 调用冲刺组件"), *GetName());
	
	// 使用冲刺组件开始冲刺
	if (DashComponent)
	{
		DashComponent->StartDash();
	}
	else
	{
		UE_LOG(LogSevenDaysToAlive, Warning, TEXT("[SDTAPlayer] %s - 冲刺失败: 冲刺组件无效"), *GetName());
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

// 获取当前移动速度
float ASDTAPlayer::GetCurrentSpeed() const
{
	// 返回当前速度向量的大小（绝对值）
	return GetVelocity().Size();
}


		






