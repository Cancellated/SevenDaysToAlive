// Fill out your copyright notice in the Description page of Project Settings.

#include "Variant_SDTA/Characters/SDTAPlayerBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Variant_SDTA/Components/HealthComponent.h"
#include "Variant_SDTA/Components/StaminaComponent.h"
#include "Variant_SDTA/Components/DashComponent.h"
#include "Variant_SDTA/Components/USDTAWeaponManagerComponent.h"
#include "Variant_SDTA/UI/SDTAWeaponUI.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "SevenDaysToAlive.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Kismet/GameplayStatics.h"

// 设置默认值
ASDTAPlayerBase::ASDTAPlayerBase()
{
	// 设置网络复制
	bReplicates = true;
	SetReplicateMovement(true);
	
	// 设置此角色每一帧调用Tick()
	PrimaryActorTick.bCanEverTick = true;

	if (GetCharacterMovement())
	{
		// 设置旋转速度
		GetCharacterMovement()->RotationRate = FRotator(0.0f, 600.0f, 0.0f);
	}

	// 创建健康组件
	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
	UE_LOG(LogTemp, Log, TEXT("生命组件创建成功"));

	// 创建耐力组件
	StaminaComponent = CreateDefaultSubobject<UStaminaComponent>(TEXT("StaminaComponent"));
	UE_LOG(LogTemp, Log, TEXT("耐力组件创建成功"));

	// 创建冲刺组件
	DashComponent = CreateDefaultSubobject<UDashComponent>(TEXT("DashComponent"));
	UE_LOG(LogTemp, Log, TEXT("冲刺组件创建成功"));

	// 创建武器管理组件
	WeaponManagerComponent = CreateDefaultSubobject<USDTAWeaponManagerComponent>(TEXT("WeaponManagerComponent"));
	UE_LOG(LogTemp, Log, TEXT("武器管理组件创建成功"));
}

// Called when the game starts or when spawned
void ASDTAPlayerBase::BeginPlay()
{
	Super::BeginPlay();

	// 绑定健康组件的委托
	if (HealthComponent)
	{
		// 绑定健康值变化委托
		HealthComponent->OnHealthChanged.AddDynamic(this, &ASDTAPlayerBase::OnHealthComponentChanged);
		
		// 绑定死亡委托
		HealthComponent->OnDeath.AddDynamic(this, &ASDTAPlayerBase::OnHealthComponentDeath);
	}

	// 绑定耐力组件的委托
	if (StaminaComponent)
	{
		// 绑定耐力值变化委托
		StaminaComponent->OnStaminaChanged.AddDynamic(this, &ASDTAPlayerBase::OnStaminaComponentChanged);
	}

	// 初始化冲刺组件
	if (DashComponent && StaminaComponent)
	{
		// 设置冲刺组件的耐力组件引用
		DashComponent->SetStaminaComponent(StaminaComponent);
		UE_LOG(LogTemp, Log, TEXT("冲刺组件初始化成功，已设置耐力组件引用"));
	}

	// 装备初始武器
	if (WeaponManagerComponent)
	{
		WeaponManagerComponent->EquipInitialWeapon();
	}
	
	// 生成武器UI
	SpawnWeaponUI();
}

// Called every frame
void ASDTAPlayerBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// 绑定玩家输入组件
void ASDTAPlayerBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// 添加冲刺输入绑定
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (DashAction)
		{
			EnhancedInputComponent->BindAction(DashAction, ETriggerEvent::Started, this, &ASDTAPlayerBase::DoDashStart);
			UE_LOG(LogTemp, Log, TEXT("冲刺输入绑定成功"));
		}

		// 添加开火输入绑定
		if (FireAction)
		{
			EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &ASDTAPlayerBase::DoFireStart);
			EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &ASDTAPlayerBase::DoFireEnd);
			UE_LOG(LogTemp, Log, TEXT("开火输入绑定成功（包含开始和结束事件）"));
		}

		// 添加换弹输入绑定
		if (ReloadAction)
		{
			EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Started, this, &ASDTAPlayerBase::DoReload);
			UE_LOG(LogTemp, Log, TEXT("换弹输入绑定成功"));
		}
	}
}

/** 处理换弹输入 */
void ASDTAPlayerBase::DoReload()
{
	// 处理换弹逻辑
	UE_LOG(LogTemp, Log, TEXT("换弹输入触发"));
	
	// 调用武器管理组件的换弹方法
	if (WeaponManagerComponent)
	{
		WeaponManagerComponent->Reload();
		UE_LOG(LogTemp, Log, TEXT("已调用武器管理组件换弹"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("武器管理组件未找到"));
	}
}

/** 处理瞄准输入 */
void ASDTAPlayerBase::DoAim(float Yaw, float Pitch)
{
	if (GetController())
	{
		// 使用Character的方法处理旋转输入
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

/** 处理移动输入 */
void ASDTAPlayerBase::DoMove(float Right, float Forward)
{
	if (GetController())
	{
		// 应用移动输入
		AddMovementInput(GetActorRightVector(), Right);
		AddMovementInput(GetActorForwardVector(), Forward);
	}
}

/** 处理跳跃开始输入 */
void ASDTAPlayerBase::DoJumpStart()
{
	// 开始跳跃
	Jump();
}

/** 处理跳跃结束输入 */
void ASDTAPlayerBase::DoJumpEnd()
{
	// 结束跳跃
	StopJumping();
}

/** 处理冲刺输入 */
void ASDTAPlayerBase::DoDashStart()
{
	// 检查冲刺组件是否有效
	if (DashComponent)
	{
		// 开始冲刺
		DashComponent->StartDash();
	}
}

/** 处理开火输入 */
void ASDTAPlayerBase::DoFireStart()
{
	// 这里可以添加开火逻辑
	// 例如：检查武器是否装备，消耗弹药，播放开火动画，生成子弹等
	UE_LOG(LogTemp, Log, TEXT("开火输入触发"));
	
	// 调用武器管理组件的开火方法
	if (WeaponManagerComponent)
	{
		WeaponManagerComponent->StartFire();
		UE_LOG(LogTemp, Log, TEXT("已调用武器管理组件开火"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("武器管理组件未找到"));
	}
}

/** 处理停止开火输入 */
void ASDTAPlayerBase::DoFireEnd()
{
	// 这里可以添加停止开火逻辑
	UE_LOG(LogTemp, Log, TEXT("停止开火输入触发"));
	
	// 调用武器管理组件的停止开火方法
	if (WeaponManagerComponent)
	{
		WeaponManagerComponent->StopFire();
		UE_LOG(LogTemp, Log, TEXT("已调用武器管理组件停止开火"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("武器管理组件未找到"));
	}
}

/** 健康值变化回调 */
void ASDTAPlayerBase::OnHealthComponentChanged(float HealthPercent)
{
	// 广播健康值变化委托
	OnHealthChanged.Broadcast(HealthPercent);
}

/** 死亡回调 */
void ASDTAPlayerBase::OnHealthComponentDeath()
{
	// 广播死亡委托
	OnDeath.Broadcast();
}

/** 耐力值变化回调 */
void ASDTAPlayerBase::OnStaminaComponentChanged(float StaminaPercent)
{
	// UE_LOG(LogSevenDaysToAlive, Log, TEXT("[SDTAPlayerBase] 耐力值变化回调，百分比: %.2f, 广播事件"), StaminaPercent);
	// 广播耐力值变化委托
	OnStaminaChanged.Broadcast(StaminaPercent);
}

/** 当Actor结束生命周期时调用 */
void ASDTAPlayerBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// 清理健康组件的委托绑定
	if (HealthComponent)
	{
		HealthComponent->OnHealthChanged.RemoveDynamic(this, &ASDTAPlayerBase::OnHealthComponentChanged);
		HealthComponent->OnDeath.RemoveDynamic(this, &ASDTAPlayerBase::OnHealthComponentDeath);
	}
	
	// 销毁武器UI
	DestroyWeaponUI();
}

// ============================================================================
// ISDTAWeaponHolder 接口实现
// ============================================================================

/** 附加武器网格 */
void ASDTAPlayerBase::AttachWeaponMeshes_Implementation(ASDTAWeapon* Weapon)
{
	if (!Weapon || !WeaponManagerComponent)
	{
		return;
	}

	// 调用武器管理组件的附加方法
	WeaponManagerComponent->AttachWeaponMeshes(Weapon);
}

/** 播放开火动画 */
void ASDTAPlayerBase::PlayFiringMontage_Implementation(UAnimMontage* Montage)
{
	UE_LOG(LogTemp, Log, TEXT("角色类接收到播放蒙太奇请求"));
	
	if (!Montage)
	{
		UE_LOG(LogTemp, Warning, TEXT("蒙太奇资源为空"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("准备播放蒙太奇: %s, 角色权限: %s"), *Montage->GetName(), HasAuthority() ? TEXT("服务器") : TEXT("客户端"));

	// 在服务器端播放动画
	if (HasAuthority())
	{
		UE_LOG(LogTemp, Log, TEXT("服务器端播放蒙太奇，准备广播"));
		// 广播到所有客户端
		MulticastPlayMontage(Montage);
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("客户端直接播放蒙太奇"));
		// 客户端直接播放
		if (GetMesh() && GetMesh()->GetAnimInstance())
		{
			GetMesh()->GetAnimInstance()->Montage_Play(Montage);
			UE_LOG(LogTemp, Log, TEXT("客户端蒙太奇播放成功"));
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("客户端网格或动画实例无效"));
		}
	}
}

/** 应用武器后坐力 */
void ASDTAPlayerBase::AddWeaponRecoil_Implementation(float RecoilAmount)
{
	if (RecoilAmount <= 0.0f)
	{
		return;
	}

	// 在服务器端应用后坐力
	if (HasAuthority())
	{
		// 广播到所有客户端
		MulticastAddRecoil(RecoilAmount);
	}
	else
	{
		// 客户端直接应用后坐力
		// 这里可以添加后坐力效果，比如相机抖动等
		UE_LOG(LogTemp, Log, TEXT("应用后坐力: %.2f"), RecoilAmount);
	}
}

/** 更新武器HUD */
void ASDTAPlayerBase::UpdateWeaponHUD_Implementation(int32 CurrentAmmo, int32 MaxAmmo)
{
	// 更新武器UI的弹药显示
	UpdateWeaponUI(CurrentAmmo, MaxAmmo);
	
	UE_LOG(LogTemp, Log, TEXT("更新HUD: %d/%d"), CurrentAmmo, MaxAmmo);
}

/** 获取武器瞄准目标位置 */
FVector ASDTAPlayerBase::GetWeaponTargetLocation_Implementation()
{
	// 获取摄像机位置和方向
	FVector CameraLocation;
	FRotator CameraRotation;
	
	if (GetController())
	{
		GetController()->GetPlayerViewPoint(CameraLocation, CameraRotation);
	}
	else
	{
		CameraLocation = GetActorLocation();
		CameraRotation = GetActorRotation();
	}

	// 计算瞄准目标位置
	FVector TargetLocation = CameraLocation + CameraRotation.Vector() * 10000.0f;
	
	return TargetLocation;
}

/** 添加武器类 */
void ASDTAPlayerBase::AddWeaponClass_Implementation(TSubclassOf<ASDTAWeapon> WeaponClass)
{
	if (!WeaponClass || !WeaponManagerComponent)
	{
		return;
	}

	// 调用武器管理组件的添加方法
	WeaponManagerComponent->AddWeaponClass(WeaponClass);
}

/** 武器激活时调用 */
void ASDTAPlayerBase::OnWeaponActivated_Implementation(ASDTAWeapon* Weapon)
{
	if (!Weapon)
	{
		return;
	}

	// 切换动画实例类
	TSubclassOf<UAnimInstance> FirstPersonClass = Weapon->GetFirstPersonAnimInstanceClass();
	TSubclassOf<UAnimInstance> ThirdPersonClass = Weapon->GetThirdPersonAnimInstanceClass();

	// 在服务器端切换动画
	if (HasAuthority())
	{
		// 广播到所有客户端
		MulticastSwitchAnimInstanceClass(FirstPersonClass, ThirdPersonClass);
	}
	else
	{
		// 客户端直接切换
		SwitchAnimInstanceClass(FirstPersonClass, ThirdPersonClass);
	}

	// 广播动画切换事件
	OnWeaponAnimInstanceChanged.Broadcast(FirstPersonClass, ThirdPersonClass);
}

/** 武器停用时调用 */
void ASDTAPlayerBase::OnWeaponDeactivated_Implementation(ASDTAWeapon* Weapon)
{
	if (!Weapon)
	{
		return;
	}

	// 切换到默认动画实例类（空手）
	TSubclassOf<UAnimInstance> DefaultClass = nullptr;

	// 在服务器端切换动画
	if (HasAuthority())
	{
		// 广播到所有客户端
		MulticastSwitchAnimInstanceClass(DefaultClass, DefaultClass);
	}
	else
	{
		// 客户端直接切换
		SwitchAnimInstanceClass(DefaultClass, DefaultClass);
	}

	// 广播动画切换事件
	OnWeaponAnimInstanceChanged.Broadcast(DefaultClass, DefaultClass);
}

// ============================================================================
// 网络同步方法实现
// ============================================================================

/** 切换动画实例类（网络同步） */
void ASDTAPlayerBase::MulticastSwitchAnimInstanceClass_Implementation(TSubclassOf<UAnimInstance> FirstPersonClass, TSubclassOf<UAnimInstance> ThirdPersonClass)
{
	// 所有客户端执行动画切换
	SwitchAnimInstanceClass(FirstPersonClass, ThirdPersonClass);
}

/** 播放动画蒙太奇（网络同步） */
void ASDTAPlayerBase::MulticastPlayMontage_Implementation(UAnimMontage* Montage)
{
	UE_LOG(LogTemp, Log, TEXT("客户端接收到蒙太奇广播"));
	
	if (!Montage)
	{
		UE_LOG(LogTemp, Warning, TEXT("客户端接收到的蒙太奇为空"));
		return;
	}
	
	UE_LOG(LogTemp, Log, TEXT("客户端准备播放蒙太奇: %s"), *Montage->GetName());

	// 根据当前视角决定使用哪个网格播放动画
	USkeletalMeshComponent* TargetMesh = nullptr;
	FString MeshType = TEXT("未知");
	
	// 检查是否是第一人称视角（拥有者视角）
	if (IsLocallyControlled() && GetFirstPersonMesh() && GetFirstPersonMesh()->GetAnimInstance())
	{
		TargetMesh = GetFirstPersonMesh();
		MeshType = TEXT("第一人称");
	}
	// 否则使用第三人称网格
	else if (GetMesh() && GetMesh()->GetAnimInstance())
	{
		TargetMesh = GetMesh();
		MeshType = TEXT("第三人称");
	}
	
	if (TargetMesh && TargetMesh->GetAnimInstance())
	{
		UE_LOG(LogTemp, Log, TEXT("在%s网格上播放蒙太奇"), *MeshType);
		TargetMesh->GetAnimInstance()->Montage_Play(Montage);
		UE_LOG(LogTemp, Log, TEXT("客户端蒙太奇播放完成"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("客户端网格或动画实例无效"));
	}
}

/** 应用后坐力（网络同步） */
void ASDTAPlayerBase::MulticastAddRecoil_Implementation(float RecoilAmount)
{
	// 所有客户端应用后坐力
	UE_LOG(LogTemp, Log, TEXT("网络同步应用后坐力: %.2f"), RecoilAmount);
}

/** 生成武器UI */
void ASDTAPlayerBase::SpawnWeaponUI()
{
	// 检查是否已经生成了武器UI
	if (WeaponUIInstance)
	{
		return;
	}
	
	// 检查是否配置了武器UI蓝图类
	if (!WeaponUIClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("未配置武器UI蓝图类，无法生成武器UI"));
		return;
	}
	
	// 检查是否是本地控制的玩家（只在客户端生成UI）
	if (!IsLocallyControlled())
	{
		return;
	}
	
	// 获取玩家控制器
	APlayerController* PlayerController = GetController<APlayerController>();
	if (!PlayerController)
	{
		UE_LOG(LogTemp, Warning, TEXT("无法获取玩家控制器，无法生成武器UI"));
		return;
	}
	
	// 获取玩家UI层
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("无法获取世界上下文，无法生成武器UI"));
		return;
	}
	
	// 创建武器UI实例
	WeaponUIInstance = CreateWidget<USDTAWeaponUI>(PlayerController, WeaponUIClass);
	
	if (WeaponUIInstance)
	{
		// 添加到玩家UI层
		WeaponUIInstance->AddToViewport();
		
		UE_LOG(LogTemp, Log, TEXT("武器UI生成成功: %s"), *WeaponUIInstance->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("武器UI生成失败"));
	}
}

/** 销毁武器UI */
void ASDTAPlayerBase::DestroyWeaponUI()
{
	if (WeaponUIInstance)
	{
		// 从父节点移除（替代RemoveFromViewport）
		WeaponUIInstance->RemoveFromParent();
		
		// 清空引用，让垃圾回收处理
		WeaponUIInstance = nullptr;
		
		UE_LOG(LogTemp, Log, TEXT("武器UI已销毁"));
	}
}

/** 获取武器UI实例（蓝图可调用） */
USDTAWeaponUI* ASDTAPlayerBase::GetWeaponUIInstance() const
{
	return WeaponUIInstance;
}

/** 更新武器UI的弹药显示 */
void ASDTAPlayerBase::UpdateWeaponUI(int32 CurrentAmmo, int32 MaxAmmo)
{
	if (WeaponUIInstance)
	{
		// 更新武器UI的弹药信息
		WeaponUIInstance->CurrentAmmo = CurrentAmmo;
		WeaponUIInstance->MagazineSize = MaxAmmo;
		
		UE_LOG(LogTemp, Log, TEXT("更新武器UI: %d/%d"), CurrentAmmo, MaxAmmo);
	}
}

// ============================================================================
// 内部实现方法
// ============================================================================

/** 内部动画实例类切换方法 */
void ASDTAPlayerBase::SwitchAnimInstanceClass(TSubclassOf<UAnimInstance> FirstPersonClass, TSubclassOf<UAnimInstance> ThirdPersonClass)
{
	// 切换第一人称动画实例类
	if (FirstPersonClass)
	{
		// 这里需要获取第一人称网格并设置动画实例类
		// 例如：FirstPersonMesh->SetAnimInstanceClass(FirstPersonClass);
		UE_LOG(LogTemp, Log, TEXT("切换到第一人称动画类"));
	}

	// 切换第三人称动画实例类
	if (ThirdPersonClass && GetMesh())
	{
		GetMesh()->SetAnimInstanceClass(ThirdPersonClass);
		UE_LOG(LogTemp, Log, TEXT("切换到第三人称动画类"));
	}
}
