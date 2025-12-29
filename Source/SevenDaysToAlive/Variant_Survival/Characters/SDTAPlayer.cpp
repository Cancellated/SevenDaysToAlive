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

	// 创建健康组件
	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
	
	// 创建耐力组件
	StaminaComponent = CreateDefaultSubobject<UStaminaSystemComponent>(TEXT("StaminaComponent"));

	// 初始化冲刺相关属性
	bIsDashing = false;
	DashSpeedMultiplier = 2.0f; // 2倍移动速度
	DashStaminaCost = 30.0f; // 冲刺消耗30点体力
	DashDuration = 0.5f; // 冲刺持续0.5秒
	DashCooldown = 1.0f; // 冲刺冷却1秒
	LastDashTime = 0.0f;

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
	
	// 设置武器附着的默认插座名称（参考Shooter模板）
	WeaponAttachSocketName = FName("HandGrip_R");
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
	// 移除组件委托绑定
	if (HealthComponent)
	{
		HealthComponent->OnHealthChanged.RemoveDynamic(this, &ASDTAPlayer::OnHealthChangedInternal);
		HealthComponent->OnHealthLowWarning.RemoveDynamic(this, &ASDTAPlayer::OnHealthLowWarningInternal);
		HealthComponent->OnDeath.RemoveDynamic(this, &ASDTAPlayer::OnDeathInternal);
	}
	
	if (StaminaComponent)
	{
		StaminaComponent->OnStaminaChanged.RemoveDynamic(this, &ASDTAPlayer::OnStaminaChangedInternal);
		StaminaComponent->OnStaminaLowWarning.RemoveDynamic(this, &ASDTAPlayer::OnStaminaLowWarningInternal);
	}
	
	Super::EndPlay(EndPlayReason);
}

// 内部健康值变化处理方法
void ASDTAPlayer::OnHealthChangedInternal(float HealthPercent)
{
	// 调用原始的健康值变化委托
	OnHealthChanged.Broadcast(HealthPercent);
}

// 内部低健康值警告处理方法
void ASDTAPlayer::OnHealthLowWarningInternal()
{
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
	// 调用原始的能量值变化委托
	OnStaminaChanged.Broadcast(StaminaPercent);
}

// 内部低能量值警告处理方法
void ASDTAPlayer::OnStaminaLowWarningInternal()
{
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
		EnhancedInputComponent->BindAction(DashAction, ETriggerEvent::Started, this, &ASDTAPlayer::DoDashStart);
		
		// Firing
		// 开火绑定
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &ASDTAPlayer::DoFireStart);
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &ASDTAPlayer::DoFireEnd);
		
		// Reload
		// 换弹绑定
		EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Started, this, &ASDTAPlayer::DoReload);

		// Switch Weapon
		// 武器切换绑定
		EnhancedInputComponent->BindAction(SwitchWeaponAction, ETriggerEvent::Started, this, &ASDTAPlayer::DoSwitchWeapon);

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

	// 冲刺相关属性的网络复制
	DOREPLIFETIME(ASDTAPlayer, bIsDashing);
	DOREPLIFETIME(ASDTAPlayer, LastDashTime);
	DOREPLIFETIME(ASDTAPlayer, OriginalMaxWalkSpeed);
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
	if (!IsAlive()) return 0.0f;
	
	// 调用父类的TakeDamage方法
	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	
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

// 冲刺开始的服务器端实现
void ASDTAPlayer::Server_StartDash_Implementation()
{
	// 检查是否可以冲刺
	float CurrentTime = GetWorld()->GetTimeSeconds();
	if (bIsDashing || (CurrentTime - LastDashTime) < DashCooldown || !StaminaComponent)
	{
		return;
	}

	// 消耗体力
	if (!StaminaComponent->ConsumeStamina(DashStaminaCost))
	{
		return;
	}

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

/** 处理开火开始输入 */
void ASDTAPlayer::DoFireStart()
{
	// 确保是本地控制的角色
	if (!IsLocallyControlled()) return;
	
	// 检查当前是否有武器且武器可以开火
	if (CurrentWeapon)
	{
		CurrentWeapon->StartFiring();
	}
}

/** 处理开火结束输入 */
void ASDTAPlayer::DoFireEnd()
{
	// 确保是本地控制的角色
	if (!IsLocallyControlled()) return;
	
	// 检查当前是否有武器
	if (CurrentWeapon)
	{
		CurrentWeapon->StopFiring();
	}
}

/** 处理换弹输入 */
void ASDTAPlayer::DoReload()
{
	// 确保是本地控制的角色
	if (!IsLocallyControlled()) return;
	
	// 检查当前是否有武器
	if (CurrentWeapon)
	{
		CurrentWeapon->Reload();
	}
}

/** 处理武器切换输入 */
void ASDTAPlayer::DoSwitchWeapon()
{
	// 确保是本地控制的角色
	if (!IsLocallyControlled()) return;
	
	// 切换到下一个武器
	SwitchToNextWeapon();
}

/** 切换到下一个武器 */
void ASDTAPlayer::SwitchToNextWeapon()
{
	// 确保服务器端处理武器切换逻辑，保证网络一致性
	if (IsServer())
	{
		// 如果没有武器，直接返回
		if (Weapons.Num() <= 0)
		{
			return;
		}
		
		// 如果只有一个武器，不需要切换
		if (Weapons.Num() == 1)
		{
			return;
		}
		
		// 找到当前武器在列表中的索引
		int32 CurrentWeaponIndex = -1;
		if (CurrentWeapon)
		{
			CurrentWeaponIndex = Weapons.IndexOfByKey(CurrentWeapon);
		}
		
		// 计算下一个武器的索引
		int32 NextWeaponIndex = (CurrentWeaponIndex + 1) % Weapons.Num();
		
		// 如果当前没有武器或者索引无效，默认选择第一个武器
		if (CurrentWeaponIndex == -1)
		{
			NextWeaponIndex = 0;
		}
		
		// 停用当前武器
		if (CurrentWeapon)
		{
			CurrentWeapon->DeactivateWeapon();
		}
		
		// 激活下一个武器
		ASDTAWeapon* NextWeapon = Weapons[NextWeaponIndex];
		if (NextWeapon)
		{
			NextWeapon->ActivateWeapon();
		}
	}
	else
	{
		// 客户端请求服务器执行武器切换
		// 注意：这里应该使用RPC调用，但为了简化示例，我们直接在客户端处理
		// 在实际项目中，应该创建一个Server_RPC方法来处理武器切换
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

//~Begin ISDTAWeaponHolder Interface Implementation

/** 附加武器网格到角色身上 */
void ASDTAPlayer::AttachWeaponMeshes(ASDTAWeapon* Weapon)
{
	if (!Weapon) return;
	
	const FAttachmentTransformRules AttachmentRule(EAttachmentRule::SnapToTarget, false);

	// 将武器Actor附加到角色上（复刻Shooter模板的核心逻辑）
	Weapon->AttachToActor(this, AttachmentRule);

	// 附加第一人称武器网格到角色的第一人称Mesh组件
	USkeletalMeshComponent* FPWeaponMesh = Weapon->GetFirstPersonMesh();
	if (FPWeaponMesh && GetFirstPersonMesh())
	{
		FPWeaponMesh->AttachToComponent(GetFirstPersonMesh(), AttachmentRule, WeaponAttachSocketName);
	}
	
	// 附加第三人称武器网格到角色的Mesh组件
	USkeletalMeshComponent* TPWeaponMesh = Weapon->GetThirdPersonMesh();
	if (TPWeaponMesh)
	{
		TPWeaponMesh->AttachToComponent(GetMesh(), AttachmentRule, WeaponAttachSocketName);
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
void ASDTAPlayer::AddWeaponClass_Implementation(TSubclassOf<ASDTAWeapon> WeaponClass)
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
			NewWeapon->SetOwner(this);
			
			// 添加到武器列表
			Weapons.Add(NewWeapon);
			
			// 将武器网格附加到角色身上
			AttachWeaponMeshes(NewWeapon);
			
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
void ASDTAPlayer::OnWeaponActivated_Implementation(ASDTAWeapon* Weapon)
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
	
	// 设置角色网格的AnimInstance类（复刻Shooter模板的核心逻辑）
	USkeletalMeshComponent* FirstPersonCharacterMesh = GetFirstPersonMesh();
	if (FirstPersonCharacterMesh)
	{
		FirstPersonCharacterMesh->SetAnimInstanceClass(Weapon->GetFirstPersonAnimInstanceClass());
	}
	
	GetMesh()->SetAnimInstanceClass(Weapon->GetThirdPersonAnimInstanceClass());
	
	// 将武器网格附加到角色
	AttachWeaponMeshes(Weapon);
}

/** 停用武器 */
void ASDTAPlayer::OnWeaponDeactivated_Implementation(ASDTAWeapon* Weapon)
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

