// Fill out your copyright notice in the Description page of Project Settings.

#include "SDTAWeapon.h"
#include "SDTAWeaponManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
ASDTAWeapon::ASDTAWeapon()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	SetNetUpdateFrequency(30.0f);

	// 创建第一人称武器网格
	FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FirstPersonMesh"));
	FirstPersonMesh->SetOnlyOwnerSee(true);
	FirstPersonMesh->SetCollisionProfileName(FName("NoCollision"));
	FirstPersonMesh->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::FirstPerson);
	FirstPersonMesh->SetupAttachment(RootComponent);

	// 创建第三人称武器网格
	ThirdPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ThirdPersonMesh"));
	ThirdPersonMesh->SetOwnerNoSee(true);
	ThirdPersonMesh->SetCollisionProfileName(FName("NoCollision"));
	ThirdPersonMesh->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::WorldSpaceRepresentation);
	ThirdPersonMesh->SetupAttachment(RootComponent);

	// 设置默认值
	CurrentAmmo = 0;
	bIsFiring = false;
	LastFireTime = 0.0f;
	Recoil = 1.0f;
	MuzzleSocketName = TEXT("Muzzle");
}

// 激活武器
void ASDTAWeapon::ActivateWeapon()
{
	// 启用武器网格
	if (FirstPersonMesh)
	{
		FirstPersonMesh->SetHiddenInGame(false);
	}
	if (ThirdPersonMesh)
	{
		ThirdPersonMesh->SetHiddenInGame(false);
	}

	// 通知武器持有者
	if (WeaponOwner)
	{
		ISDTAWeaponHolder::Execute_OnWeaponActivated(WeaponOwner.GetObject(), this);
	}
}

// 停用武器
void ASDTAWeapon::DeactivateWeapon()
{
	// 禁用武器网格
	if (FirstPersonMesh)
	{
		FirstPersonMesh->SetHiddenInGame(true);
	}
	if (ThirdPersonMesh)
	{
		ThirdPersonMesh->SetHiddenInGame(true);
	}

	// 停止开火
	StopFiring();

	// 通知武器持有者
	if (WeaponOwner)
	{
		ISDTAWeaponHolder::Execute_OnWeaponDeactivated(WeaponOwner.GetObject(), this);
	}
}

// 开始开火
void ASDTAWeapon::StartFiring()
{
	// 如果有武器管理器，调用管理器的开始开火方法
	if (WeaponManager)
	{
		WeaponManager->StartFiring();
	}
	// 否则使用旧逻辑
	else
	{
		if (!bIsFiring)
		{
			bIsFiring = true;

			// 服务器端同步
			if (GetLocalRole() < ROLE_Authority)
			{
				ServerStartFire();
			}

			// 立即开火一次
			if (CanFire())
			{
				Fire();
			}

			// 设置定时器用于持续开火
			if (WeaponDataRow.FireRate > 0.0f)
			{
				GetWorldTimerManager().SetTimer(
					FireTimerHandle,
					this,
					&ASDTAWeapon::OnFireTimer,
					WeaponDataRow.FireRate,
					true // 循环执行
				);
			}
		}
	}
}

// 停止开火
void ASDTAWeapon::StopFiring()
{
	// 如果有武器管理器，调用管理器的停止开火方法
	if (WeaponManager)
	{
		WeaponManager->StopFiring();
	}
	// 否则使用旧逻辑
	else
	{
		bIsFiring = false;

		// 清理开火定时器
		GetWorldTimerManager().ClearTimer(FireTimerHandle);

		// 服务器端同步
		if (GetLocalRole() < ROLE_Authority)
		{
			ServerStopFire();
		}
	}
}

// 开火逻辑
void ASDTAWeapon::Fire()
{
	// 武器管理器已经处理了开火条件检查和弹药消耗
	// 这里只需要处理视觉效果和子弹生成

	// 根据子弹类型执行不同逻辑
	switch (WeaponDataRow.BulletType)
	{
	case ESDTABulletType::Projectile:
		FireProjectile();
		break;
	case ESDTABulletType::HitScan:
		FireHitScan();
		break;
	case ESDTABulletType::EnergyBeam:
		FireEnergyBeam();
		break;
	case ESDTABulletType::Shotgun:
		FireShotgun();
		break;
	}

	// 应用后坐力
	ApplyRecoil();

	// 播放开火动画
	PlayFiringMontage();

	// 播放开火特效
	PlayFiringEffects();

	// 更新最后开火时间（用于客户端视觉效果同步）
	LastFireTime = GetWorld()->GetTimeSeconds();
}

// 发射实体子弹
void ASDTAWeapon::FireProjectile()
{
	if (!BulletClass || !WeaponOwner)
	{
		return;
	}

	// 获取枪口位置和旋转
	FVector MuzzleLocation = FirstPersonMesh->GetSocketLocation(MuzzleSocketName);
	FRotator MuzzleRotation = FirstPersonMesh->GetSocketRotation(MuzzleSocketName);

	// 获取目标方向
	FVector TargetLocation = ISDTAWeaponHolder::Execute_GetWeaponTargetLocation(WeaponOwner.GetObject());
	FRotator FireRotation = (TargetLocation - MuzzleLocation).Rotation();

	// 生成子弹
	ASDTABullet* Bullet = GetWorld()->SpawnActor<ASDTABullet>(BulletClass, MuzzleLocation, FireRotation);
	if (Bullet)
	{
		// 设置子弹属性
		Bullet->SetBulletDamage(GetWeaponDamage());
		Bullet->SetBulletRange(GetWeaponRange());
		
		// 激活子弹
		FVector FireDirection = FireRotation.Vector();
		Bullet->ActivateBullet(FireDirection);
	}
}

// 即时弹道开火
void ASDTAWeapon::FireHitScan()
{
	if (!WeaponOwner)
	{
		return;
	}

	// 获取枪口位置
	FVector MuzzleLocation = FirstPersonMesh->GetSocketLocation(MuzzleSocketName);
	
	// 获取目标方向
	FVector TargetLocation = ISDTAWeaponHolder::Execute_GetWeaponTargetLocation(WeaponOwner.GetObject());
	FVector FireDirection = (TargetLocation - MuzzleLocation).GetSafeNormal();

	// 射线检测
	FHitResult HitResult;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	Params.AddIgnoredActor(GetOwner());

	if (GetWorld()->LineTraceSingleByChannel(HitResult, MuzzleLocation, MuzzleLocation + FireDirection * GetWeaponRange(), ECC_Visibility, Params))
	{
		// 应用伤害
		if (ACharacter* HitCharacter = Cast<ACharacter>(HitResult.GetActor()))
		{
			UGameplayStatics::ApplyPointDamage(HitCharacter, GetWeaponDamage(), FireDirection, HitResult, GetInstigatorController(), this, nullptr);
		}
	}
}

// 能量束开火
void ASDTAWeapon::FireEnergyBeam()
{
	// 暂时实现为HitScan
	FireHitScan();
}

// 霰弹开火
void ASDTAWeapon::FireShotgun()
{
	if (!WeaponOwner)
	{
		return;
	}

	// 获取枪口位置
	FVector MuzzleLocation = FirstPersonMesh->GetSocketLocation(MuzzleSocketName);
	
	// 获取目标方向
	FVector TargetLocation = ISDTAWeaponHolder::Execute_GetWeaponTargetLocation(WeaponOwner.GetObject());
	FVector BaseDirection = (TargetLocation - MuzzleLocation).GetSafeNormal();

	// 发射多个射线
	for (int32 i = 0; i < WeaponDataRow.PelletCount; i++)
	{
		// 计算随机扩散
		float SpreadAngle = FMath::DegreesToRadians(WeaponDataRow.SpreadAngle);
		FVector RandomDirection = FRotationMatrix(FRotator(
			FMath::FRandRange(-SpreadAngle, SpreadAngle),
			FMath::FRandRange(-SpreadAngle, SpreadAngle),
			0.0f
		)).TransformVector(BaseDirection);

		// 射线检测
		FHitResult HitResult;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(this);
		Params.AddIgnoredActor(GetOwner());

		if (GetWorld()->LineTraceSingleByChannel(HitResult, MuzzleLocation, MuzzleLocation + RandomDirection * GetWeaponRange(), ECC_Visibility, Params))
		{
			// 应用伤害
			if (ACharacter* HitCharacter = Cast<ACharacter>(HitResult.GetActor()))
			{
				float PelletDamage = GetWeaponDamage() / WeaponDataRow.PelletCount;
				UGameplayStatics::ApplyPointDamage(HitCharacter, PelletDamage, RandomDirection, HitResult, GetInstigatorController(), this, nullptr);
			}
		}
	}
}

// 装填弹药
void ASDTAWeapon::Reload()
{
	// 服务器端同步
	if (GetLocalRole() < ROLE_Authority)
	{
		ServerReload();
		return;
	}

	// 填满弹药
	CurrentAmmo = WeaponDataRow.MagazineSize;

	// 广播换弹完成委托
	OnReloadCompleted.Broadcast();

	// 注意：UI更新现在由武器管理器处理，这里不再直接更新UI
}

// 设置武器数据
void ASDTAWeapon::SetWeaponData(const FDataTableRowHandle& NewWeaponData)
{
	WeaponData = NewWeaponData;
	LoadWeaponData();
}

// 设置武器数据行
void ASDTAWeapon::SetWeaponDataRow(const FSDTAWeaponTableRow& NewWeaponDataRow)
{
	WeaponDataRow = NewWeaponDataRow;
	CurrentAmmo = WeaponDataRow.MagazineSize;

	LoadWeaponData();

	UE_LOG(LogTemp, Log, TEXT("武器数据行已设置并应用: %s"), *WeaponDataRow.WeaponName);
}

// 设置武器持有者
void ASDTAWeapon::SetWeaponOwner(TScriptInterface<ISDTAWeaponHolder> NewOwner)
{
	WeaponOwner = NewOwner;
}

// 服务器端开始开火
void ASDTAWeapon::ServerStartFire_Implementation()
{
	StartFiring();
}

// 服务器端开始开火验证
bool ASDTAWeapon::ServerStartFire_Validate()
{
	return true;
}

// 服务器端停止开火
void ASDTAWeapon::ServerStopFire_Implementation()
{
	StopFiring();
}

// 服务器端停止开火验证
bool ASDTAWeapon::ServerStopFire_Validate()
{
	return true;
}

// 服务器端装填弹药
void ASDTAWeapon::ServerReload_Implementation()
{
	Reload();
}

// 服务器端装填弹药验证
bool ASDTAWeapon::ServerReload_Validate()
{
	return true;
}

// 获取当前弹药数量
int32 ASDTAWeapon::GetCurrentAmmo() const
{
	if (WeaponManager)
	{
		return WeaponManager->GetCurrentWeaponAmmo();
	}
	return CurrentAmmo;
}

// 获取弹匣容量
int32 ASDTAWeapon::GetMagazineSize() const
{
	if (WeaponManager)
	{
		return WeaponManager->GetCurrentWeaponMagazineSize();
	}
	return WeaponDataRow.MagazineSize;
}

// 获取武器伤害
float ASDTAWeapon::GetWeaponDamage() const
{
	if (WeaponManager)
	{
		return WeaponManager->GetCurrentWeaponDamage();
	}
	return WeaponDataRow.Damage;
}

// 获取武器射程
float ASDTAWeapon::GetWeaponRange() const
{
	if (WeaponManager)
	{
		return WeaponManager->GetCurrentWeaponRange();
	}
	return WeaponDataRow.Range;
}

// 获取武器名称
FString ASDTAWeapon::GetWeaponName() const
{
	if (WeaponManager)
	{
		return WeaponManager->GetCurrentWeaponName();
	}
	return WeaponDataRow.WeaponName;
}

// 加载武器数据
void ASDTAWeapon::LoadWeaponData()
{
	if (WeaponData.DataTable)
	{
		if (const FSDTAWeaponTableRow* Row = WeaponData.DataTable->FindRow<FSDTAWeaponTableRow>(WeaponData.RowName, TEXT("")))
		{
			WeaponDataRow = *Row;
			CurrentAmmo = WeaponDataRow.MagazineSize;

			if (FirstPersonMesh && WeaponDataRow.FirstPersonMesh.IsValid())
			{
				FirstPersonMesh->SetSkeletalMesh(WeaponDataRow.FirstPersonMesh.LoadSynchronous());
				UE_LOG(LogTemp, Log, TEXT("武器FP Mesh已设置: %s"), *WeaponDataRow.FirstPersonMesh.ToString());
			}

			if (ThirdPersonMesh && WeaponDataRow.ThirdPersonMesh.IsValid())
			{
				ThirdPersonMesh->SetSkeletalMesh(WeaponDataRow.ThirdPersonMesh.LoadSynchronous());
				UE_LOG(LogTemp, Log, TEXT("武器TP Mesh已设置: %s"), *WeaponDataRow.ThirdPersonMesh.ToString());
			}

			UE_LOG(LogTemp, Log, TEXT("武器数据已加载: 武器Mesh已应用"));
		}
	}
}

// 检查是否可以开火
bool ASDTAWeapon::CanFire() const
{
	// 检查弹药
	if (CurrentAmmo <= 0)
	{
		return false;
	}

	// 检查射速
	float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastFireTime < WeaponDataRow.FireRate)
	{
		return false;
	}

	return true;
}

// 应用后坐力
void ASDTAWeapon::ApplyRecoil()
{
	if (WeaponOwner)
	{
		ISDTAWeaponHolder::Execute_AddWeaponRecoil(WeaponOwner.GetObject(), Recoil);
	}
}

// 播放开火特效
void ASDTAWeapon::PlayFiringEffects()
{
	// 这里可以添加枪口闪光、音效等特效
}

// 播放开火动画
void ASDTAWeapon::PlayFiringMontage()
{
	UE_LOG(LogTemp, Log, TEXT("开始播放开火蒙太奇"));
	
	if (!WeaponOwner)
	{
		UE_LOG(LogTemp, Warning, TEXT("武器持有者无效，无法播放蒙太奇"));
		return;
	}
	
	// 尝试直接加载蒙太奇资源，不检查有效性
	UAnimMontage* FiringMontage = WeaponDataRow.FiringMontage.LoadSynchronous();
	if (!FiringMontage)
	{
		UE_LOG(LogTemp, Error, TEXT("开火蒙太奇加载失败，路径: %s"), *WeaponDataRow.FiringMontage.ToString());
		
		// 尝试使用软对象路径重新加载
		FSoftObjectPath MontagePath = WeaponDataRow.FiringMontage.ToSoftObjectPath();
		UE_LOG(LogTemp, Warning, TEXT("软对象路径: %s"), *MontagePath.ToString());
		
		// 尝试手动加载
		FiringMontage = Cast<UAnimMontage>(MontagePath.TryLoad());
		if (!FiringMontage)
		{
			UE_LOG(LogTemp, Error, TEXT("手动加载蒙太奇也失败"));
			return;
		}
		UE_LOG(LogTemp, Log, TEXT("手动加载蒙太奇成功"));
	}
	
	UE_LOG(LogTemp, Log, TEXT("成功加载开火蒙太奇: %s"), *FiringMontage->GetName());
	ISDTAWeaponHolder::Execute_PlayFiringMontage(WeaponOwner.GetObject(), FiringMontage);
	UE_LOG(LogTemp, Log, TEXT("已调用播放蒙太奇接口"));
}

// 定时器开火回调
void ASDTAWeapon::OnFireTimer()
{
	// 如果有武器管理器，定时器逻辑由管理器处理
	if (WeaponManager)
	{
		return;
	}
	
	// 否则使用旧逻辑
	// 检查是否可以继续开火
	if (!bIsFiring || !CanFire())
	{
		// 停止开火
		StopFiring();
		return;
	}

	// 执行开火逻辑
	Fire();
}

// 获取第一人称动画实例类
TSubclassOf<UAnimInstance> ASDTAWeapon::GetFirstPersonAnimInstanceClass() const
{
	return WeaponDataRow.FirstPersonAnimInstanceClass;
}

// 获取第三人称动画实例类
TSubclassOf<UAnimInstance> ASDTAWeapon::GetThirdPersonAnimInstanceClass() const
{
	return WeaponDataRow.ThirdPersonAnimInstanceClass;
}

// 设置武器管理器
void ASDTAWeapon::SetWeaponManager(USDTAWeaponManager* Manager)
{
	WeaponManager = Manager;
}

// 网络同步
void ASDTAWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}
