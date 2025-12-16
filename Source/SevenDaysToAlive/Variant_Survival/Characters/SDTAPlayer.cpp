// Fill out your copyright notice in the Description page of Project Settings.


#include "Variant_Survival/Characters/SDTAPlayer.h"
#include "Variant_Survival/Weapons/SDTAWeapon.h"
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
 	// 设置此角色每一帧调用Tick()。如果不需要，可以关闭以提高性能。
	PrimaryActorTick.bCanEverTick = true;

	// 启用角色的网络复制
	bReplicates = true;

	// 初始化角色统计属性
	MaxHealth = 100.0f;
	MaxStamina = 100.0f;
	Health = MaxHealth;
	Stamina = MaxStamina;

	// 初始化冲刺相关属性
	bIsDashing = false;
	DashSpeedMultiplier = 2.0f; // 2倍移动速度
	DashStaminaCost = 30.0f; // 冲刺消耗30点体力
	DashDuration = 0.5f; // 冲刺持续0.5秒
	DashCooldown = 1.0f; // 冲刺冷却1秒
	LastDashTime = 0.0f;

	// 初始化能量回复相关属性
	StaminaRegenerationRate = 5.0f; // 每秒回复5点能量
	StaminaRegenerationDelay = 1.0f; // 消耗能量后1秒开始回复
	bIsStaminaRegenerating = true; // 默认正在回复能量

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
	
	// 初始化原始移动速度
	if (GetCharacterMovement())
	{
		OriginalMaxWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;
	}
	
	// 广播初始健康值和能量值
	OnHealthChanged.Broadcast(Health / MaxHealth);
	OnStaminaChanged.Broadcast(Stamina / MaxStamina);
}

// Called every frame
void ASDTAPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 在服务器上处理能量回复
	if (IsServer() && IsAlive() && bIsStaminaRegenerating)
	{
		float OldStamina = Stamina;
		Stamina = FMath::Clamp(Stamina + (StaminaRegenerationRate * DeltaTime), 0.0f, MaxStamina);

		// 如果能量值发生变化，通知客户端并广播事件
		if (FMath::Abs(Stamina - OldStamina) > 0.01f)
		{
			Client_UpdateHUD(Health, Stamina);
			OnStaminaChanged.Broadcast(Stamina / MaxStamina);
		}
	}
}

// Called when the actor is removed from the world
void ASDTAPlayer::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// 清理所有委托，防止访问已销毁的对象
	OnHealthChanged.Clear();
	OnHealthLowWarning.Clear();
	OnStaminaChanged.Clear();
	OnStaminaLowWarning.Clear();
	OnDeath.Clear();
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
		EnhancedInputComponent->BindAction(DashAction, ETriggerEvent::Started, this, &ASDTAPlayer::DoDashStart);

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

	// 使用DOREPLIFETIME宏添加需要复制的属性
	// 注意：属性的顺序必须与生成的代码中一致，否则会导致索引不匹配错误
	DOREPLIFETIME(ASDTAPlayer, MaxHealth);
	DOREPLIFETIME(ASDTAPlayer, MaxStamina);
	DOREPLIFETIME(ASDTAPlayer, Health);
	DOREPLIFETIME(ASDTAPlayer, Stamina);
	
	// 冲刺相关属性的网络复制
	DOREPLIFETIME(ASDTAPlayer, bIsDashing);
	DOREPLIFETIME(ASDTAPlayer, LastDashTime);
	DOREPLIFETIME(ASDTAPlayer, OriginalMaxWalkSpeed);
}

// Server_SetHealth的实现
void ASDTAPlayer::Server_SetHealth_Implementation(float NewHealth)
{
	float OldHealthPercent = Health / MaxHealth;
	Health = FMath::Clamp(NewHealth, 0.0f, MaxHealth);
	float NewHealthPercent = Health / MaxHealth;
	
	// 通知所有客户端更新HUD
	Client_UpdateHUD(Health, Stamina);
	// 广播健康值变化事件
	OnHealthChanged.Broadcast(NewHealthPercent);
	
	// 检查低血量警告
	const float HealthLowThreshold = 0.2f; // 20%
	if (NewHealthPercent <= HealthLowThreshold && OldHealthPercent > HealthLowThreshold)
	{
		OnHealthLowWarning.Broadcast();
	}
}

bool ASDTAPlayer::Server_SetHealth_Validate(float NewHealth)
{
	// 验证输入值是否有效
	return NewHealth >= 0.0f && NewHealth <= MaxHealth;
}

// Server_SetStamina的实现
void ASDTAPlayer::Server_SetStamina_Implementation(float NewStamina)
{
	float OldStaminaPercent = Stamina / MaxStamina;
	Stamina = FMath::Clamp(NewStamina, 0.0f, MaxStamina);
	float NewStaminaPercent = Stamina / MaxStamina;
	
	// 通知所有客户端更新HUD
	Client_UpdateHUD(Health, Stamina);
	// 广播能量值变化事件
	OnStaminaChanged.Broadcast(NewStaminaPercent);
	
	// 检查低体力警告
	const float StaminaLowThreshold = 0.2f; // 20%
	if (NewStaminaPercent <= StaminaLowThreshold && OldStaminaPercent > StaminaLowThreshold)
	{
		OnStaminaLowWarning.Broadcast();
	}

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
		
		// 消耗能量后暂停回复，并重置延迟计时器
		bIsStaminaRegenerating = false;
		GetWorldTimerManager().ClearTimer(StaminaRegenDelayTimerHandle);
		GetWorldTimerManager().SetTimer(StaminaRegenDelayTimerHandle, this, &ASDTAPlayer::StartStaminaRegeneration, StaminaRegenerationDelay, false);
		
		// 通知所有客户端更新HUD
		Client_UpdateHUD(Health, Stamina);
		
		// 广播能量值变化事件
		OnStaminaChanged.Broadcast(Stamina / MaxStamina);
	}
}

// 开始能量回复
void ASDTAPlayer::StartStaminaRegeneration()
{
	if (IsServer() && IsAlive())
	{
		bIsStaminaRegenerating = true;
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

// 冲刺开始的服务器端实现
void ASDTAPlayer::Server_StartDash_Implementation()
{
	// 检查是否可以冲刺
	float CurrentTime = GetWorld()->GetTimeSeconds();
	if (bIsDashing || (CurrentTime - LastDashTime) < DashCooldown || Stamina < DashStaminaCost)
	{
		return;
	}

	// 消耗体力
	Server_SetStamina(Stamina - DashStaminaCost);

	// 设置冲刺状态
	bIsDashing = true;
	LastDashTime = CurrentTime;

	// 应用冲刺速度
	if (GetCharacterMovement())
	{
		// 保存原始移动速度
		OriginalMaxWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;
		// 设置冲刺速度
		GetCharacterMovement()->MaxWalkSpeed *= DashSpeedMultiplier;
	}

	// 设置冲刺结束定时器
	GetWorld()->GetTimerManager().SetTimer(FDashTimerHandle, this, &ASDTAPlayer::Server_EndDash, DashDuration, false);
}

bool ASDTAPlayer::Server_StartDash_Validate()
{
	// 基本验证
	return true;
}

// 冲刺结束的服务器端实现
void ASDTAPlayer::Server_EndDash_Implementation()
{
	// 检查是否处于冲刺状态
	if (!bIsDashing)
	{
		return;
	}

	// 重置冲刺状态
	bIsDashing = false;

	// 恢复原始移动速度
	if (GetCharacterMovement())
	{
		// 使用保存的原始移动速度恢复
		GetCharacterMovement()->MaxWalkSpeed = OriginalMaxWalkSpeed;
	}
}

bool ASDTAPlayer::Server_EndDash_Validate()
{
	// 基本验证
	return true;
}

// 标准化输入处理方法实现

/** 处理瞄准输入 */
void ASDTAPlayer::DoAim(float Yaw, float Pitch)
{
	// 确保是本地控制的角色
	if (!IsLocallyControlled()) return;

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

	// 获取移动方向（与父类保持一致的实现）
	AddMovementInput(GetActorRightVector(), Right);
	AddMovementInput(GetActorForwardVector(), Forward);
}

/** 处理跳跃开始输入 */
void ASDTAPlayer::DoJumpStart()
{
	// 确保是本地控制的角色
	if (!IsLocallyControlled()) return;

	// 开始跳跃
	Jump();
}

/** 处理跳跃结束输入 */
void ASDTAPlayer::DoJumpEnd()
{
	// 确保是本地控制的角色
	if (!IsLocallyControlled()) return;

	// 结束跳跃
	StopJumping();
}

/** 处理冲刺开始输入 */
void ASDTAPlayer::DoDashStart()
{
	// 确保是本地控制的角色
	if (!IsLocallyControlled()) return;

	// 请求服务器开始冲刺
	Server_StartDash();
}

/** 处理冲刺结束输入 */
void ASDTAPlayer::DoDashEnd()
{
	// 确保是本地控制的角色
	if (!IsLocallyControlled()) return;

	// 请求服务器结束冲刺
	Server_EndDash();
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

//~Begin ISDTAWeaponHolder Interface Implementation

/** 附加武器网格到角色身上 */
void ASDTAPlayer::AttachWeaponMeshes(ASDTAWeapon* Weapon)
{
	if (!Weapon) return;
	
	// 附加第一人称武器网格到角色的第一人称Mesh组件
	USkeletalMeshComponent* FPWeaponMesh = Weapon->GetFirstPersonMesh();
	if (FPWeaponMesh)
	{
		FPWeaponMesh->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, WeaponAttachSocketName);
	}
	
	// 附加第三人称武器网格到角色的Mesh组件
	USkeletalMeshComponent* TPWeaponMesh = Weapon->GetThirdPersonMesh();
	if (TPWeaponMesh)
	{
		TPWeaponMesh->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, WeaponAttachSocketName);
	}
}

/** 播放武器射击动画 */
void ASDTAPlayer::PlayFiringMontage(UAnimMontage* Montage)
{
	if (!Montage) return;
	
	// 播放射击动画
	GetMesh()->GetAnimInstance()->Montage_Play(Montage);
}

/** 应用武器后坐力到角色 */
void ASDTAPlayer::AddWeaponRecoil(float Recoil)
{
	// 仅在本地角色上应用后坐力
	if (!IsLocallyControlled()) return;
	
	// 获取控制器
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;
	
	// 应用后坐力到控制器的旋转
	PC->AddPitchInput(-Recoil);
}

/** 更新武器HUD显示当前弹药数量 */
void ASDTAPlayer::UpdateWeaponHUD(int32 CurrentAmmo, int32 MagazineSize)
{
	// 在客户端上更新HUD
	if (IsLocallyControlled())
	{
		// 这里可以添加更新HUD的逻辑
		// 例如调用UI更新函数或触发事件
	}
}

/** 计算并返回武器的瞄准目标位置 */
FVector ASDTAPlayer::GetWeaponTargetLocation()
{
	// 获取控制器
	AController* LocalController = GetController();
	if (!LocalController) return FVector::ZeroVector;
	
	// 对于玩家控制器，使用鼠标位置进行射线检测
	APlayerController* PC = Cast<APlayerController>(LocalController);
	if (PC)
	{
		FHitResult HitResult;
		if (PC->GetHitResultUnderCursor(ECC_Visibility, false, HitResult))
		{
			return HitResult.Location;
		}
	}
	
	// 默认返回角色前方一定距离的位置
	return GetActorLocation() + GetActorForwardVector() * 1000.0f;
}

/** 给玩家添加指定类型的武器 */
void ASDTAPlayer::AddWeaponClass(const TSubclassOf<ASDTAWeapon>& WeaponClass)
{
	if (!WeaponClass) return;
	
	// 检查玩家是否已经拥有这种武器
	if (FindWeaponOfType(WeaponClass))
	{
		// 已经拥有这种武器，可以选择增加弹药或替换
		return;
	}
	
	// 仅在服务器上创建武器
	if (IsServer())
	{
		// 创建武器实例
		ASDTAWeapon* NewWeapon = GetWorld()->SpawnActor<ASDTAWeapon>(WeaponClass, GetActorLocation(), GetActorRotation());
		if (NewWeapon)
		{
			// 设置武器所有者
			// 设置武器的所有者为当前玩家
			NewWeapon->SetOwner(this);
			
			// 添加到武器列表
			Weapons.Add(NewWeapon);
			
			// 如果是第一个武器，自动装备
			if (Weapons.Num() == 1)
			{
				NewWeapon->ActivateWeapon();
				CurrentWeapon = NewWeapon;
			}
		}
	}
}

/** 激活武器 */
void ASDTAPlayer::OnWeaponActivated(ASDTAWeapon* Weapon)
{
	if (!Weapon) return;
	
	// 仅在服务器上处理激活逻辑
	if (IsServer())
	{
		// 如果有当前武器，先停用它
		if (CurrentWeapon && CurrentWeapon != Weapon)
		{
			CurrentWeapon->DeactivateWeapon();
		}
		
		// 设置当前武器
		CurrentWeapon = Weapon;
	}
}

/** 停用武器 */
void ASDTAPlayer::OnWeaponDeactivated(ASDTAWeapon* Weapon)
{
	if (!Weapon) return;
	
	// 仅在服务器上处理停用逻辑
	if (IsServer())
	{
		// 如果停用的是当前武器，清除当前武器
		if (CurrentWeapon == Weapon)
		{
			CurrentWeapon = nullptr;
		}
	}
}

//~End ISDTAWeaponHolder Interface Implementation

/** 在玩家的武器库中查找指定类型的武器 */
ASDTAWeapon* ASDTAPlayer::FindWeaponOfType(TSubclassOf<ASDTAWeapon> WeaponClass) const
{
	if (!WeaponClass) return nullptr;
	
	// 遍历玩家的武器列表，查找指定类型的武器
	for (ASDTAWeapon* Weapon : Weapons)
	{
		if (Weapon && Weapon->IsA(WeaponClass))
		{
			return Weapon;
		}
	}
	
	return nullptr;
}

