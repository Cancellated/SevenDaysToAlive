// Fill out your copyright notice in the Description page of Project Settings.

#include "Variant_Survival/Weapons/SDTAWeapon.h"
#include "Variant_Survival/Components/WeaponComponent.h"
#include "SevenDaysToAlive.h"

// 设置默认值
ASDTAWeapon::ASDTAWeapon()
{
	// 设置此Actor每一帧调用Tick()。如果不需要，可以关闭以提高性能。
	PrimaryActorTick.bCanEverTick = true;

	// 创建第一人称网格组件
	FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FirstPersonMesh"));
	FirstPersonMesh->SetOnlyOwnerSee(true);
	FirstPersonMesh->SetHiddenInGame(true);
	FirstPersonMesh->SetupAttachment(GetRootComponent());

	// 创建第三人称网格组件
	ThirdPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ThirdPersonMesh"));
	ThirdPersonMesh->SetupAttachment(GetRootComponent());
}

// 当游戏开始或生成时调用
void ASDTAWeapon::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogSevenDaysToAlive, Log, TEXT("[SDTAWeapon] %s - BeginPlay开始"), *GetName());

	// 初始化武器属性
	CurrentBullets = MagazineSize;

	// 记录动画实例类配置
	UE_LOG(LogSevenDaysToAlive, Log, TEXT("[SDTAWeapon] %s - FirstPersonAnimInstanceClass: %s, ThirdPersonAnimInstanceClass: %s"), 
		*GetName(), 
		FirstPersonAnimInstanceClass ? *FirstPersonAnimInstanceClass->GetName() : TEXT("未设置"),
		ThirdPersonAnimInstanceClass ? *ThirdPersonAnimInstanceClass->GetName() : TEXT("未设置"));

	// 初始化对象池管理器
	if (ProjectileClass)
	{
		UE_LOG(LogSevenDaysToAlive, Log, TEXT("[SDTAWeapon] %s - 投射物类: %s"), *GetName(), *ProjectileClass->GetName());
		
		// 创建或获取对象池管理器
		PoolManager = NewObject<USDTAPoolManager>(GetTransientPackage());
		if (PoolManager)
		{
			PoolManager->Initialize(GetWorld());
			// 为投射物初始化对象池，初始大小10，最大大小50
			PoolManager->InitPoolForClass(ProjectileClass, 10, 50);
			UE_LOG(LogSevenDaysToAlive, Log, TEXT("[SDTAWeapon] %s - 对象池初始化成功"), *GetName());
		}
	}
}

// 当Actor被销毁或从关卡中移除时调用
void ASDTAWeapon::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// 清除定时器句柄
	GetWorldTimerManager().ClearTimer(RefireTimer);
}

// 初始化武器所有者（在SetOwner之后调用）
void ASDTAWeapon::InitializeWeaponOwner()
{
	UE_LOG(LogSevenDaysToAlive, Log, TEXT("[SDTAWeapon] %s - InitializeWeaponOwner开始"), *GetName());

	// 查找武器所有者
	AActor* OwnerActor = GetOwner();
	if (OwnerActor)
	{
		// 首先尝试直接转换所有者为ISDTAWeaponHolder
		WeaponOwner = Cast<ISDTAWeaponHolder>(OwnerActor);
		
		// 如果所有者不是ISDTAWeaponHolder，尝试从所有者的组件中查找
		if (!WeaponOwner)
		{
			// 直接尝试获取WeaponComponent
			UWeaponComponent* WeaponComp = OwnerActor->FindComponentByClass<UWeaponComponent>();
			if (WeaponComp)
			{
				WeaponOwner = Cast<ISDTAWeaponHolder>(WeaponComp);
				if (WeaponOwner)
				{
					UE_LOG(LogSevenDaysToAlive, Log, TEXT("[SDTAWeapon] %s - 找到WeaponComponent: %s"), *GetName(), *WeaponComp->GetName());
				}
			}
			else
			{
				// 如果没有找到WeaponComponent，遍历所有组件
				const TSet<UActorComponent*>& AllComponents = OwnerActor->GetComponents();
				for (UActorComponent* Component : AllComponents)
				{
					if (Component)
					{
						// 尝试将组件转换为ISDTAWeaponHolder
						ISDTAWeaponHolder* PotentialHolder = Cast<ISDTAWeaponHolder>(Component);
						if (PotentialHolder)
						{
							WeaponOwner = PotentialHolder;
							UE_LOG(LogSevenDaysToAlive, Log, TEXT("[SDTAWeapon] %s - 从组件中找到WeaponHolder: %s"), *GetName(), *Component->GetName());
							break;
						}
					}
				}
			}
		}
		
		PawnOwner = Cast<APawn>(OwnerActor);

		UE_LOG(LogSevenDaysToAlive, Log, TEXT("[SDTAWeapon] %s - 找到所有者: %s, WeaponHolder: %s"), 
			*GetName(), *OwnerActor->GetName(), WeaponOwner ? TEXT("有效") : TEXT("无效"));

		// 监听所有者销毁事件
		OwnerActor->OnDestroyed.AddDynamic(this, &ASDTAWeapon::OnOwnerDestroyed);
	}
	else
	{
		UE_LOG(LogSevenDaysToAlive, Warning, TEXT("[SDTAWeapon] %s - 未找到所有者"), *GetName());
	}
}

// 当武器的所有者被销毁时调用
void ASDTAWeapon::OnOwnerDestroyed(AActor* DestroyedActor)
{
	// 清除引用以防止访问无效内存
	WeaponOwner = nullptr;
	PawnOwner = nullptr;
}

// 激活此武器并准备开火
void ASDTAWeapon::ActivateWeapon()
{
	UE_LOG(LogSevenDaysToAlive, Log, TEXT("[SDTAWeapon] %s - ActivateWeapon开始"), *GetName());

	// 设置武器为可见
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	SetActorTickEnabled(true);

	// 激活第一人称网格
	if (FirstPersonMesh)
	{
		FirstPersonMesh->SetHiddenInGame(false);
		FirstPersonMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		UE_LOG(LogSevenDaysToAlive, Log, TEXT("[SDTAWeapon] %s - 第一人称网格已激活"), *GetName());
	}
	else
	{
		UE_LOG(LogSevenDaysToAlive, Error, TEXT("[SDTAWeapon] %s - 第一人称网格无效"), *GetName());
	}

	// 激活第三人称网格
	if (ThirdPersonMesh)
	{
		ThirdPersonMesh->SetHiddenInGame(false);
		ThirdPersonMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		UE_LOG(LogSevenDaysToAlive, Log, TEXT("[SDTAWeapon] %s - 第三人称网格已激活"), *GetName());
	}
	else
	{
		UE_LOG(LogSevenDaysToAlive, Error, TEXT("[SDTAWeapon] %s - 第三人称网格无效"), *GetName());
	}

	// 通知武器所有者武器已激活
	if (WeaponOwner)
	{
		UE_LOG(LogSevenDaysToAlive, Log, TEXT("[SDTAWeapon] %s - 通知武器所有者激活"), *GetName());
		// 由于WeaponOwner可能是组件，使用它作为接口调用的目标
		if (UObject* WeaponOwnerObj = Cast<UObject>(WeaponOwner))
		{
			ISDTAWeaponHolder::Execute_OnWeaponActivated(WeaponOwnerObj, this);
		}
		else
		{
			UE_LOG(LogSevenDaysToAlive, Error, TEXT("[SDTAWeapon] %s - WeaponOwner无法转换为UObject"), *GetName());
		}
	}
	else
	{
		UE_LOG(LogSevenDaysToAlive, Error, TEXT("[SDTAWeapon] %s - 武器所有者无效，无法通知激活"), *GetName());
	}

	UE_LOG(LogSevenDaysToAlive, Log, TEXT("[SDTAWeapon] %s - ActivateWeapon完成"), *GetName());
}

// 停用此武器
void ASDTAWeapon::DeactivateWeapon()
{
	// 停止当前的开火动作
	StopFiring();

	// 设置武器为隐藏
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	SetActorTickEnabled(false);

	// 隐藏第一人称网格
	if (FirstPersonMesh)
	{
		FirstPersonMesh->SetHiddenInGame(true);
		FirstPersonMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// 隐藏第三人称网格
	if (ThirdPersonMesh)
	{
		ThirdPersonMesh->SetHiddenInGame(true);
		ThirdPersonMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// 通知武器所有者武器已停用
	if (WeaponOwner)
	{
		// 由于WeaponOwner可能是组件，使用它作为接口调用的目标
		if (UObject* WeaponOwnerObj = Cast<UObject>(WeaponOwner))
		{
			ISDTAWeaponHolder::Execute_OnWeaponDeactivated(WeaponOwnerObj, this);
		}
		else
		{
			UE_LOG(LogSevenDaysToAlive, Error, TEXT("[SDTAWeapon] %s - WeaponOwner无法转换为UObject"), *GetName());
		}
	}
}

// 开始开火
void ASDTAWeapon::StartFiring()
{
	// Basic implementation placeholder
	if (bIsFiring)
	{
		return;
	}

	bIsFiring = true;

	// 发射初始子弹
	Fire();

	// 如果是全自动，设置自动开火定时器
	if (bFullAuto)
	{
		GetWorldTimerManager().SetTimer(RefireTimer, this, &ASDTAWeapon::Fire, RefireRate, true);
	}
}

// 停止开火
void ASDTAWeapon::StopFiring()
{
	// Basic implementation placeholder
	bIsFiring = false;
	GetWorldTimerManager().ClearTimer(RefireTimer);
}

// 重新装填武器
void ASDTAWeapon::Reload()
{
	// 停止当前的开火动作
	StopFiring();

	// 重新装填子弹到弹夹容量
	CurrentBullets = MagazineSize;

	// 更新HUD显示新的子弹数量
	if (WeaponOwner)
	{
		WeaponOwner->UpdateWeaponHUD(CurrentBullets, MagazineSize);
	}
}

// 开火
void ASDTAWeapon::Fire()
{
	// 检查武器是否有子弹
	if (CurrentBullets <= 0)
	{
		StopFiring();
		return;
	}

	// 播放开火动画蒙太奇
	if (WeaponOwner && FiringMontage)
	{
		WeaponOwner->PlayFiringMontage(FiringMontage);
	}

	// 应用后坐力
	if (WeaponOwner)
	{
		WeaponOwner->AddWeaponRecoil(FiringRecoil);
	}

	// 计算目标位置（当前实现为武器前方10000单位）
	FVector MuzzleLocation = FirstPersonMesh->GetSocketLocation(MuzzleSocketName);
	FVector FireDirection = FirstPersonMesh->GetForwardVector();
	FVector TargetLocation = MuzzleLocation + FireDirection * 10000.0f;

	// 发射投射物
	FireProjectile(TargetLocation);

	// 减少子弹数量
	CurrentBullets--;

	// 更新武器HUD
	if (WeaponOwner)
	{
		WeaponOwner->UpdateWeaponHUD(CurrentBullets, MagazineSize);
	}

	// 保存最后一次射击时间
	TimeOfLastShot = GetWorld()->TimeSeconds;
}

// 在射击半自动武器时，当自动开火间隔时间过去时调用
void ASDTAWeapon::FireCooldownExpired()
{
	// 占位符不需要实现
}

// 向目标位置发射投射物
void ASDTAWeapon::FireProjectile(const FVector& TargetLocation)
{
	// 检查投射物类和对象池管理器是否有效
	if (!ProjectileClass || !PoolManager)
	{
		return;
	}

	// 计算投射物生成变换
	FTransform SpawnTransform = CalculateProjectileSpawnTransform(TargetLocation);

	// 从对象池获取投射物
	ASDTAProjectiles* Projectile = Cast<ASDTAProjectiles>(PoolManager->GetObject(ProjectileClass));
	if (Projectile)
	{
		// 重置投射物位置和旋转
		Projectile->SetActorTransform(SpawnTransform);
		
		// 设置投射物的参数
		Projectile->SetInstigator(PawnOwner);
		Projectile->SetOwner(this);
		
		// 设置对象池管理器引用
		Projectile->SetPoolManager(PoolManager);
		
		// 设置射击方向
		FVector FireDirection = (TargetLocation - SpawnTransform.GetLocation()).GetSafeNormal();
		Projectile->SetFireDirection(FireDirection);
		
		// 触发射击
		Projectile->Fire();
	}
}

// 为此武器发射的投射物计算生成变换
FTransform ASDTAWeapon::CalculateProjectileSpawnTransform(const FVector& TargetLocation) const
{
	// 获取枪口位置
	FVector MuzzleLocation = FirstPersonMesh->GetSocketLocation(MuzzleSocketName);
	
	// 计算射击方向
	FVector FireDirection = (TargetLocation - MuzzleLocation).GetSafeNormal();
	
	// 计算发射位置（在枪口前方一定距离）
	FVector SpawnLocation = MuzzleLocation + FireDirection * MuzzleOffset;
	
	// 计算旋转角度
	FRotator SpawnRotation = FireDirection.Rotation();
	
	// 创建并返回变换
	return FTransform(SpawnRotation, SpawnLocation);
}

// 返回第一人称动画实例类
const TSubclassOf<UAnimInstance>& ASDTAWeapon::GetFirstPersonAnimInstanceClass() const
{
	return FirstPersonAnimInstanceClass;
}

// 返回第三人称动画实例类
const TSubclassOf<UAnimInstance>& ASDTAWeapon::GetThirdPersonAnimInstanceClass() const
{
	return ThirdPersonAnimInstanceClass;
}
