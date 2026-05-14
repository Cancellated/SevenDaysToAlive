// Fill out your copyright notice in the Description page of Project Settings.

#include "Variant_SDTA/Characters/SDTAPlayerBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Variant_SDTA/Components/HealthComponent.h"
#include "Variant_SDTA/Components/StaminaComponent.h"
#include "Variant_SDTA/Components/DashComponent.h"

#include "Variant_SDTA/Core/Game/SDTAPlayerState.h"
#include "Variant_SDTA/Core/Game/SDTAGameMode.h"
#include "Variant_SDTA/Core/Game/SDTAGameState.h"
#include "Variant_SDTA/Weapons/SDTAWeaponManager.h"
#include "Variant_SDTA/Weapons/SDTAWeapon.h"
#include "Variant_SDTA/UI/SDTAWeaponUI.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "SevenDaysToAlive.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"

DEFINE_LOG_CATEGORY_STATIC(LogDiagnose, Log, All);
// #region 构造函数与组件初始化

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
	UE_LOG(LogTemp, Log, TEXT("[PlayerBase]生命组件创建成功"));

	// 创建耐力组件
	StaminaComponent = CreateDefaultSubobject<UStaminaComponent>(TEXT("StaminaComponent"));
	UE_LOG(LogTemp, Log, TEXT("[PlayerBase]耐力组件创建成功"));

	// 创建冲刺组件
	DashComponent = CreateDefaultSubobject<UDashComponent>(TEXT("DashComponent"));
	UE_LOG(LogTemp, Log, TEXT("[PlayerBase]冲刺组件创建成功"));


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
		UE_LOG(LogTemp, Log, TEXT("[PlayerBase]冲刺组件初始化成功，已设置耐力组件引用"));
	}

	// 尝试初始化武器（PlayerState可能延迟就绪）
	TryInitializeWeapon();
	
	// 生成武器UI
	SpawnWeaponUI();
}

/** 延迟重试武器初始化（等待PlayerState和数据表复制完成） */
void ASDTAPlayerBase::TryInitializeWeapon()
{
	if (bWeaponInitialized)
	{
		return;
	}

	ASDTAPlayerState* SDTAState = GetPlayerState<ASDTAPlayerState>();

	UE_LOG(LogDiagnose, Log, TEXT("[Init] TryInitializeWeapon: State=%p Manager=%p Role=%s"),
		SDTAState,
		(SDTAState ? SDTAState->WeaponManager : nullptr),
		*UEnum::GetValueAsString(TEXT("Engine.ENetRole"), GetLocalRole()));

	if (SDTAState && SDTAState->WeaponManager)
	{
		UDataTable* DataTable = nullptr;

		if (!SDTAState->WeaponManager->GetWeaponDataTable())
		{
			if (HasAuthority() && GetWorld())
			{
				ASDTAGameMode* GM = GetWorld()->GetAuthGameMode<ASDTAGameMode>();
				if (GM && GM->WeaponDataTable)
				{
					DataTable = GM->WeaponDataTable;
					UE_LOG(LogTemp, Log, TEXT("[PlayerBase]从GameMode获取武器数据表: %s"), *GM->WeaponDataTable->GetName());
				}
			}
			else if (GetWorld())
			{
				ASDTAGameState* GS = GetWorld()->GetGameState<ASDTAGameState>();
				if (GS && GS->WeaponDataTable)
				{
					DataTable = GS->WeaponDataTable;
					UE_LOG(LogTemp, Log, TEXT("[PlayerBase]从GameState获取武器数据表: %s"), *GS->WeaponDataTable->GetName());
				}
				else
				{
					GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ASDTAPlayerBase::TryInitializeWeapon);
					return;
				}
			}

			if (DataTable)
			{
				SDTAState->WeaponManager->SetWeaponDataTable(DataTable);
			}
		}

		if (!SDTAState->WeaponManager->GetWeaponDataTable())
		{
			if (HasAuthority())
			{
				GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ASDTAPlayerBase::TryInitializeWeapon);
			}
			else
			{
				GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ASDTAPlayerBase::TryInitializeWeapon);
			}
			return;
		}

		bWeaponInitialized = true;

		SDTAState->WeaponManager->OnWeaponDataReady.AddDynamic(this, &ASDTAPlayerBase::OnWeaponDataReady);

		SDTAState->WeaponManager->EquipInitialWeaponWithActor(
			InitialWeaponClass,
			InitialWeaponName
		);

		if (!HasAuthority())
		{
			GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ASDTAPlayerBase::WaitForWeaponActor);
		}

		UE_LOG(LogTemp, Log, TEXT("[PlayerBase]武器初始化成功: 数据表已就绪"));
	}
	else if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ASDTAPlayerBase::TryInitializeWeapon);
	}
}

/** 数据表复制到客户端后的回调 */
void ASDTAPlayerBase::OnDataTableReady()
{
	if (bWeaponInitialized)
	{
		return;
	}

	ASDTAPlayerState* SDTAState = GetPlayerState<ASDTAPlayerState>();
	if (SDTAState && SDTAState->WeaponManager && SDTAState->WeaponManager->GetWeaponDataTable())
	{
		bWeaponInitialized = true;

		SDTAState->WeaponManager->EquipInitialWeaponWithActor(
			InitialWeaponClass,
			InitialWeaponName
		);
		UE_LOG(LogTemp, Log, TEXT("[PlayerBase]武器初始化成功: 数据表已同步"));
	}
}

/** 客户端等待武器Actor到达并挂载Mesh */
void ASDTAPlayerBase::WaitForWeaponActor()
{
	if (HasAuthority())
	{
		return;
	}

	ASDTAPlayerState* SDTAState = GetPlayerState<ASDTAPlayerState>();
	if (!SDTAState || !SDTAState->WeaponManager)
	{
		if (GetWorld())
		{
			GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ASDTAPlayerBase::WaitForWeaponActor);
		}
		return;
	}

	ASDTAWeapon* WeaponActor = SDTAState->WeaponManager->GetCurrentWeaponActor();

	if (!WeaponActor)
	{
		for (TActorIterator<ASDTAWeapon> It(GetWorld()); It; ++It)
		{
			ASDTAWeapon* FoundWeapon = *It;
			if (FoundWeapon && FoundWeapon->GetOwner() == this)
			{
				WeaponActor = FoundWeapon;
				UE_LOG(LogDiagnose, Log, TEXT("[Attach] 客户端从世界中找到武器Actor: %s (Owner匹配)"), *FoundWeapon->GetName());
				break;
			}
		}
	}

	if (WeaponActor)
	{
		ISDTAWeaponHolder::Execute_AttachWeaponMeshes(this, WeaponActor);
		UE_LOG(LogDiagnose, Log, TEXT("[Attach] 客户端武器Actor已到达并挂载: %s"), *WeaponActor->GetName());
		return;
	}

	static int32 WaitCount = 0;
	WaitCount++;
	if (WaitCount > 300)
	{
		UE_LOG(LogDiagnose, Warning, TEXT("[Attach] 客户端等待武器Actor超时（5秒）"));
		WaitCount = 0;
		return;
	}

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ASDTAPlayerBase::WaitForWeaponActor);
	}
}

// Called every frame
void ASDTAPlayerBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// #endregion 构造函数与组件初始化

// #region 输入处理方法

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
			UE_LOG(LogTemp, Log, TEXT("[PlayerBase]冲刺输入绑定成功"));
		}

		// 添加开火输入绑定
		if (FireAction)
		{
			EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &ASDTAPlayerBase::DoFireStart);
			EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &ASDTAPlayerBase::DoFireEnd);
			UE_LOG(LogTemp, Log, TEXT("[PlayerBase]开火输入绑定成功（包含开始和结束事件）"));
		}

		// 添加换弹输入绑定
		if (ReloadAction)
		{
			EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Started, this, &ASDTAPlayerBase::DoReload);
			UE_LOG(LogTemp, Log, TEXT("[PlayerBase]换弹输入绑定成功"));
		}
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
	// 开始冲刺
	if (DashComponent)
	{
		DashComponent->StartDash();
	}
}

// #endregion 输入处理方法

// #region 武器相关方法

/** 处理开火输入 */
void ASDTAPlayerBase::DoFireStart()
{
	UE_LOG(LogTemp, Log, TEXT("[PlayerBase]开火输入触发"));

	if (HasAuthority())
	{
		ASDTAPlayerState* SDTAState = GetPlayerState<ASDTAPlayerState>();
		if (SDTAState && SDTAState->WeaponManager)
		{
			SDTAState->WeaponManager->StartFiring();
			UE_LOG(LogTemp, Log, TEXT("[PlayerBase]已调用武器管理器开火"));
		}
	}
	else
	{
		ServerStartFire();
		UE_LOG(LogTemp, Log, TEXT("[PlayerBase]已发送开火RPC到服务器"));
	}
}

/** 处理停止开火输入 */
void ASDTAPlayerBase::DoFireEnd()
{
	UE_LOG(LogTemp, Log, TEXT("[PlayerBase]停止开火输入触发"));

	if (HasAuthority())
	{
		ASDTAPlayerState* SDTAState = GetPlayerState<ASDTAPlayerState>();
		if (SDTAState && SDTAState->WeaponManager)
		{
			SDTAState->WeaponManager->StopFiring();
			UE_LOG(LogTemp, Log, TEXT("[PlayerBase]已调用武器管理器停止开火"));
		}
	}
	else
	{
		ServerStopFire();
		UE_LOG(LogTemp, Log, TEXT("[PlayerBase]已发送停止开火RPC到服务器"));
	}
}

/** 处理换弹输入 */
void ASDTAPlayerBase::DoReload()
{
	UE_LOG(LogTemp, Log, TEXT("[PlayerBase]换弹输入触发"));

	if (HasAuthority())
	{
		ASDTAPlayerState* SDTAState = GetPlayerState<ASDTAPlayerState>();
		if (SDTAState && SDTAState->WeaponManager)
		{
			SDTAState->WeaponManager->ReloadCurrentWeapon();
			UE_LOG(LogTemp, Log, TEXT("[PlayerBase]已调用武器管理器换弹"));
		}
	}
	else
	{
		ServerReload();
		UE_LOG(LogTemp, Log, TEXT("[PlayerBase]已发送换弹RPC到服务器"));
	}
}

/** 服务器端开火RPC实现 */
void ASDTAPlayerBase::ServerStartFire_Implementation()
{
	ASDTAPlayerState* SDTAState = GetPlayerState<ASDTAPlayerState>();
	if (SDTAState && SDTAState->WeaponManager)
	{
		SDTAState->WeaponManager->StartFiring();
		UE_LOG(LogTemp, Log, TEXT("[PlayerBase]服务器收到开火RPC并执行"));
	}
}

bool ASDTAPlayerBase::ServerStartFire_Validate()
{
	return true;
}

/** 服务器端停止开火RPC实现 */
void ASDTAPlayerBase::ServerStopFire_Implementation()
{
	ASDTAPlayerState* SDTAState = GetPlayerState<ASDTAPlayerState>();
	if (SDTAState && SDTAState->WeaponManager)
	{
		SDTAState->WeaponManager->StopFiring();
		UE_LOG(LogTemp, Log, TEXT("[PlayerBase]服务器收到停止开火RPC并执行"));
	}
}

bool ASDTAPlayerBase::ServerStopFire_Validate()
{
	return true;
}

/** 服务器端换弹RPC实现 */
void ASDTAPlayerBase::ServerReload_Implementation()
{
	ASDTAPlayerState* SDTAState = GetPlayerState<ASDTAPlayerState>();
	if (SDTAState && SDTAState->WeaponManager)
	{
		SDTAState->WeaponManager->ReloadCurrentWeapon();
		UE_LOG(LogTemp, Log, TEXT("[PlayerBase]服务器收到换弹RPC并执行"));
	}
}

bool ASDTAPlayerBase::ServerReload_Validate()
{
	return true;
}

/** 客户端更新武器弹药UI的RPC实现 */
void ASDTAPlayerBase::ClientUpdateWeaponHUD_Implementation(int32 CurrentAmmo, int32 MaxAmmo)
{
	UpdateWeaponUI(CurrentAmmo, MaxAmmo);
	UE_LOG(LogTemp, Log, TEXT("[PlayerBase]客户端收到弹药UI更新: %d/%d"), CurrentAmmo, MaxAmmo);
}

// #endregion 武器相关方法

// #region 组件回调方法

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

// #endregion 组件回调方法

// #region ISDTAWeaponHolder 接口实现


/** 附加武器网格 */
void ASDTAPlayerBase::AttachWeaponMeshes_Implementation(ASDTAWeapon* Weapon)
{
	if (!Weapon)
	{
		UE_LOG(LogDiagnose, Warning, TEXT("[Attach] AttachWeaponMeshes: Weapon为空"));
		return;
	}

	const FAttachmentTransformRules AttachmentRule(EAttachmentRule::SnapToTarget, false);

	USkeletalMeshComponent* FPMesh = GetFirstPersonMesh();
	USkeletalMeshComponent* TPMesh = GetMesh();

	UE_LOG(LogDiagnose, Log, TEXT("[Attach] AttachWeaponMeshes: %s → Pawn=%s FPMesh=%p TPMesh=%p Role=%s"),
		*Weapon->GetName(),
		*GetName(),
		FPMesh,
		TPMesh,
		*UEnum::GetValueAsString(TEXT("Engine.ENetRole"), GetLocalRole()));

	Weapon->AttachToActor(this, AttachmentRule);
	UE_LOG(LogDiagnose, Log, TEXT("[Attach] Actor已附加到Pawn"));

	if (Weapon->GetFirstPersonMesh() && FPMesh)
	{
		Weapon->GetFirstPersonMesh()->AttachToComponent(FPMesh, AttachmentRule, FName("HandGrip_R"));
		UE_LOG(LogDiagnose, Log, TEXT("[Attach] FP Mesh已挂载到HandGrip_R"));
	}
	else
	{
		UE_LOG(LogDiagnose, Warning, TEXT("[Attach] FP跳过: WeaponFP=%p CharacterFP=%p"),
			Weapon->GetFirstPersonMesh(), FPMesh);
	}

	if (Weapon->GetThirdPersonMesh() && TPMesh)
	{
		Weapon->GetThirdPersonMesh()->AttachToComponent(TPMesh, AttachmentRule, FName("HandGrip_R"));
		UE_LOG(LogDiagnose, Log, TEXT("[Attach] TP Mesh已挂载到HandGrip_R"));
	}
	else
	{
		UE_LOG(LogDiagnose, Warning, TEXT("[Attach] TP跳过: WeaponTP=%p CharacterTP=%p"),
			Weapon->GetThirdPersonMesh(), TPMesh);
	}

	UE_LOG(LogTemp, Log, TEXT("[PlayerBase]武器网格挂载完成: %s"), *Weapon->GetName());
}

/** 播放开火动画 */
void ASDTAPlayerBase::PlayFiringMontage_Implementation(UAnimMontage* Montage)
{
	UE_LOG(LogTemp, Log, TEXT("[PlayerBase]角色类接收到播放蒙太奇请求"));
	
	if (!Montage)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PlayerBase]蒙太奇资源为空"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[PlayerBase]准备播放蒙太奇: %s, 角色权限: %s"), *Montage->GetName(), HasAuthority() ? TEXT("服务器") : TEXT("客户端"));

	// 在服务器端播放动画
	if (HasAuthority())
	{
		UE_LOG(LogTemp, Log, TEXT("[PlayerBase]服务器端播放蒙太奇，准备广播"));
		// 广播到所有客户端
		MulticastPlayMontage(Montage);
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("[PlayerBase]客户端直接播放蒙太奇"));
		// 客户端直接播放
		if (GetMesh() && GetMesh()->GetAnimInstance())
		{
			GetMesh()->GetAnimInstance()->Montage_Play(Montage);
			UE_LOG(LogTemp, Log, TEXT("[PlayerBase]客户端蒙太奇播放成功"));
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[PlayerBase]客户端网格或动画实例无效"));
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
		UE_LOG(LogTemp, Log, TEXT("[PlayerBase]应用后坐力: %.2f"), RecoilAmount);
	}
}

/** 更新武器HUD */
void ASDTAPlayerBase::UpdateWeaponHUD_Implementation(int32 CurrentAmmo, int32 MaxAmmo)
{
	UpdateWeaponUI(CurrentAmmo, MaxAmmo);

	UE_LOG(LogTemp, Log, TEXT("[PlayerBase]更新HUD: %d/%d"), CurrentAmmo, MaxAmmo);

	if (HasAuthority())
	{
		ClientUpdateWeaponHUD(CurrentAmmo, MaxAmmo);
	}
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
	if (!WeaponClass)
	{
		return;
	}

	// 使用新的武器管理器
	ASDTAPlayerState* SDTAState = GetPlayerState<ASDTAPlayerState>();
	if (SDTAState && SDTAState->WeaponManager)
	{
		// 从武器类获取武器名称
		// 注意：这里假设WeaponClass有一个获取武器名称的方法或属性
		// 实际实现需要根据ASDTAWeapon类的具体设计来调整
		FName WeaponName = TEXT("[PlayerBase]UnknownWeapon");
		
		// 如果可以通过WeaponClass获取DefaultObject，我们可以从中提取武器名称
		ASDTAWeapon* DefaultWeapon = WeaponClass->GetDefaultObject<ASDTAWeapon>();
		if (DefaultWeapon)
		{
			// 假设ASDTAWeapon类有一个GetWeaponName方法
			// WeaponName = DefaultWeapon->GetWeaponName();
			UE_LOG(LogTemp, Warning, TEXT("[PlayerBase]AddWeaponClass: 从DefaultObject获取武器名称的逻辑需要实现"));
		}
		
		// 添加武器到武器管理器
		SDTAState->WeaponManager->AddWeapon(WeaponName);
		UE_LOG(LogTemp, Log, TEXT("[PlayerBase]添加武器: %s"), *WeaponName.ToString());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[PlayerBase]AddWeaponClass: 无法获取玩家状态或武器管理器"));
	}
}

/** 武器激活时调用 */
void ASDTAPlayerBase::OnWeaponActivated_Implementation(ASDTAWeapon* Weapon)
{
	if (!Weapon)
	{
		return;
	}

	TSubclassOf<UAnimInstance> FPClass = Weapon->GetFirstPersonAnimInstanceClass();
	TSubclassOf<UAnimInstance> TPClass = Weapon->GetThirdPersonAnimInstanceClass();

	UE_LOG(LogTemp, Log, TEXT("[PlayerBase]OnWeaponActivated: %s (FP=%s TP=%s) — 动画由WeaponDataReady统一管理"),
		*Weapon->GetName(),
		FPClass ? *FPClass->GetName() : TEXT("None"),
		TPClass ? *TPClass->GetName() : TEXT("None"));
}

/** 武器停用时调用 */
void ASDTAPlayerBase::OnWeaponDeactivated_Implementation(ASDTAWeapon* Weapon)
{
	if (!Weapon)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[PlayerBase]OnWeaponDeactivated: 武器已停用（动画由WeaponDataReady统一管理）"));
}

// #endregion ISDTAWeaponHolder 接口实现

// #region 网络同步方法实现

/** 切换动画实例类（网络同步） */
void ASDTAPlayerBase::MulticastSwitchAnimInstanceClass_Implementation(TSubclassOf<UAnimInstance> FirstPersonClass, TSubclassOf<UAnimInstance> ThirdPersonClass)
{
	// 所有客户端执行动画切换
	SwitchAnimInstanceClass(FirstPersonClass, ThirdPersonClass);
}

/** 播放动画蒙太奇（网络同步） */
void ASDTAPlayerBase::MulticastPlayMontage_Implementation(UAnimMontage* Montage)
{
	UE_LOG(LogTemp, Log, TEXT("[PlayerBase]客户端接收到蒙太奇广播"));
	
	if (!Montage)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PlayerBase]客户端接收到的蒙太奇为空"));
		return;
	}
	
	UE_LOG(LogTemp, Log, TEXT("[PlayerBase]客户端准备播放蒙太奇: %s"), *Montage->GetName());

	// 根据当前视角决定使用哪个网格播放动画
	USkeletalMeshComponent* TargetMesh = nullptr;
	FString MeshType = TEXT("[PlayerBase]未知");
	
	// 检查是否是第一人称视角（拥有者视角）
	if (IsLocallyControlled() && GetFirstPersonMesh() && GetFirstPersonMesh()->GetAnimInstance())
	{
		TargetMesh = GetFirstPersonMesh();
		MeshType = TEXT("[PlayerBase]第一人称");
	}
	// 否则使用第三人称网格
	else if (GetMesh() && GetMesh()->GetAnimInstance())
	{
		TargetMesh = GetMesh();
		MeshType = TEXT("[PlayerBase]第三人称");
	}
	
	if (TargetMesh && TargetMesh->GetAnimInstance())
	{
		UE_LOG(LogTemp, Log, TEXT("[PlayerBase]客户端在%s网格上播放蒙太奇"), *MeshType);
		TargetMesh->GetAnimInstance()->Montage_Play(Montage);
		UE_LOG(LogTemp, Log, TEXT("[PlayerBase]客户端蒙太奇播放完成"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[PlayerBase]客户端网格或动画实例无效"));
	}
}

/** 应用后坐力（网络同步） */
void ASDTAPlayerBase::MulticastAddRecoil_Implementation(float RecoilAmount)
{
	// 所有客户端应用后坐力
	UE_LOG(LogTemp, Log, TEXT("[PlayerBase]客户端网络同步应用后坐力: %.2f"), RecoilAmount);
}

// #endregion 网络同步方法实现

// #region UI相关方法

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
		UE_LOG(LogTemp, Warning, TEXT("[PlayerBase]客户端未配置武器UI蓝图类，无法生成武器UI"));
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
		UE_LOG(LogTemp, Warning, TEXT("[PlayerBase]客户端无法获取玩家控制器，无法生成武器UI"));
		return;
	}
	
	// 获取玩家UI层
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PlayerBase]客户端无法获取世界上下文，无法生成武器UI"));
		return;
	}
	
	// 创建武器UI实例
	WeaponUIInstance = CreateWidget<USDTAWeaponUI>(PlayerController, WeaponUIClass);
	
	if (WeaponUIInstance)
	{
		// 添加到玩家UI层
		WeaponUIInstance->AddToViewport();
		
		UE_LOG(LogTemp, Log, TEXT("[PlayerBase]客户端武器UI生成成功: %s"), *WeaponUIInstance->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[PlayerBase]客户端武器UI生成失败"));
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
		
		UE_LOG(LogTemp, Log, TEXT("[PlayerBase]客户端武器UI已销毁"));
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
		WeaponUIInstance->BP_UpdateBulletCounter(MaxAmmo, CurrentAmmo);
		
		UE_LOG(LogTemp, Log, TEXT("[PlayerBase]更新武器UI: %d/%d"), CurrentAmmo, MaxAmmo);
	}
}

// #endregion UI相关方法

// #region 内部实现方法

/** 内部动画实例类切换方法 */
void ASDTAPlayerBase::SwitchAnimInstanceClass(TSubclassOf<UAnimInstance> FirstPersonClass, TSubclassOf<UAnimInstance> ThirdPersonClass)
{
	USkeletalMeshComponent* FPMesh = GetFirstPersonMesh();
	USkeletalMeshComponent* TPMesh = GetMesh();

	UE_LOG(LogDiagnose, Log, TEXT("[Anim] SwitchAnimInstance: FPMesh=%p TPMesh=%p FPClass=%s TPClass=%s"),
		FPMesh, TPMesh,
		FirstPersonClass ? *FirstPersonClass->GetName() : TEXT("None"),
		ThirdPersonClass ? *ThirdPersonClass->GetName() : TEXT("None"));

	if (FirstPersonClass && FPMesh)
	{
		FPMesh->SetAnimInstanceClass(FirstPersonClass);
		UE_LOG(LogTemp, Log, TEXT("[PlayerBase]FP AnimInstance已设置到Mesh: %s"), *FPMesh->GetName());
	}
	else
	{
		UE_LOG(LogDiagnose, Warning, TEXT("[Anim] FP跳过: Class=%d Mesh=%d"),
			(!!FirstPersonClass), (!!FPMesh));
	}

	if (ThirdPersonClass && TPMesh)
	{
		TPMesh->SetAnimInstanceClass(ThirdPersonClass);
		UE_LOG(LogTemp, Log, TEXT("[PlayerBase]TP AnimInstance已设置到Mesh: %s"), *TPMesh->GetName());
	}
	else
	{
		UE_LOG(LogDiagnose, Warning, TEXT("[Anim] TP跳过: Class=%d Mesh=%d"),
			(!!ThirdPersonClass), (!!TPMesh));
	}
}

/** 武器数据就绪回调 */
void ASDTAPlayerBase::OnWeaponDataReady(TSubclassOf<UAnimInstance> FirstPersonAnimClass, TSubclassOf<UAnimInstance> ThirdPersonAnimClass)
{
	UE_LOG(LogTemp, Log, TEXT("[PlayerBase]客户端收到武器数据就绪广播，切换人物动画蓝图"));
	SwitchAnimInstanceClass(FirstPersonAnimClass, ThirdPersonAnimClass);
}

// #endregion 内部实现方法
