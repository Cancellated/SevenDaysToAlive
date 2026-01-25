// Fill out your copyright notice in the Description page of Project Settings.

#include "Variant_Survival/Weapons/SDTAWeapon.h"
#include "Variant_Survival/Weapons/SDTABullet.h"
#include "Variant_Survival/Components/HealthComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "TimerManager.h"
#include "SevenDaysToAlive.h"

// 设置默认值
ASDTAWeapon::ASDTAWeapon()
{
    // 设置此 actor 为每帧调用 Tick()
    PrimaryActorTick.bCanEverTick = true;

    // 创建第一人称武器网格
    FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FirstPersonMesh"));
    FirstPersonMesh->SetOnlyOwnerSee(true);
    RootComponent = FirstPersonMesh;

    // 创建第三人称武器网格
    ThirdPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ThirdPersonMesh"));
    ThirdPersonMesh->SetOwnerNoSee(true);
    ThirdPersonMesh->SetupAttachment(RootComponent);

    // 默认值
    CurrentAmmo = 0;
    bIsFiring = false;
    LastFireTime = 0.0f;
    Recoil = 1.0f;
    MuzzleSocketName = "MuzzleSocket";  // 枪口套接字名称
}

// Called when the game starts or when spawned
void ASDTAWeapon::BeginPlay()
{
    Super::BeginPlay();

    // 从数据表加载武器数据
    if (WeaponData.DataTable != nullptr && !WeaponData.RowName.IsNone())
    {
        if (const UDataTable* DataTable = WeaponData.DataTable)
        {
            if (const FSDTAWeaponTableRow* Row = DataTable->FindRow<FSDTAWeaponTableRow>(WeaponData.RowName, TEXT("")))
            {
                WeaponDataRow = *Row;
                CurrentAmmo = WeaponDataRow.MagazineSize;

                // 加载武器骨骼网格
                if (Row->SkeletalMesh.IsValid())
                {
                    if (USkeletalMesh* Mesh = Row->SkeletalMesh.LoadSynchronous())
                    {
                        FirstPersonMesh->SetSkeletalMesh(Mesh);
                        ThirdPersonMesh->SetSkeletalMesh(Mesh);
                    }
                }

                UE_LOG(LogTemp, Log, TEXT("武器数据加载成功: %s, 弹匣容量: %d, 伤害: %.2f"), *Row->WeaponName, Row->MagazineSize, Row->Damage);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("武器数据表行不存在: %s"), *WeaponData.RowName.ToString());
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("武器数据表无效"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("武器数据表未设置"));
    }
}

/** 激活武器 */
void ASDTAWeapon::ActivateWeapon()
{
    // 显示武器网格
    FirstPersonMesh->SetHiddenInGame(false);
    ThirdPersonMesh->SetHiddenInGame(false);

    // 通知持有者武器已激活
    if (WeaponOwner)
    {
        WeaponOwner->OnWeaponActivated(this);
        WeaponOwner->UpdateWeaponHUD(CurrentAmmo, WeaponDataRow.MagazineSize);
    }

    UE_LOG(LogTemp, Log, TEXT("武器激活: %s"), *WeaponDataRow.WeaponName);
}

/** 停用武器 */
void ASDTAWeapon::DeactivateWeapon()
{
    // 隐藏武器网格
    FirstPersonMesh->SetHiddenInGame(true);
    ThirdPersonMesh->SetHiddenInGame(true);

    // 停止开火
    StopFiring();

    // 通知持有者武器已停用
    if (WeaponOwner)
    {
        WeaponOwner->OnWeaponDeactivated(this);
    }

    UE_LOG(LogTemp, Log, TEXT("武器停用: %s"), *WeaponDataRow.WeaponName);
}

/** 开始开火 */
void ASDTAWeapon::StartFiring()
{
    if (!bIsFiring && CurrentAmmo > 0)
    {
        bIsFiring = true;
        Fire();
    }
}

/** 停止开火 */
void ASDTAWeapon::StopFiring()
{
    bIsFiring = false;
}

/** 开火 */
void ASDTAWeapon::Fire()
{
    // 检查是否可以开火
    float CurrentTime = GetWorld()->GetTimeSeconds();
    if (CurrentTime - LastFireTime < WeaponDataRow.FireRate || CurrentAmmo <= 0)
    {
        return;
    }

    // 消耗弹药
    CurrentAmmo--;

    // 更新HUD
    if (WeaponOwner)
    {
        WeaponOwner->UpdateWeaponHUD(CurrentAmmo, WeaponDataRow.MagazineSize);
        WeaponOwner->PlayFiringMontage(FiringMontage);
        WeaponOwner->AddWeaponRecoil(Recoil);
    }

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
            
        default:
            UE_LOG(LogTemp, Warning, TEXT("未实现的子弹类型: %d"), static_cast<int32>(WeaponDataRow.BulletType));
            break;
    }

    // 更新最后开火时间
    LastFireTime = CurrentTime;

    UE_LOG(LogTemp, Log, TEXT("武器开火: %s, 剩余弹药: %d, 子弹类型: %d"), *WeaponDataRow.WeaponName, CurrentAmmo, static_cast<int32>(WeaponDataRow.BulletType));
}

/** 实体子弹开火 */
void ASDTAWeapon::FireProjectile()
{
    if (WeaponOwner && WeaponDataRow.BulletClass)
    {
        FVector MuzzleLocation = FirstPersonMesh->GetSocketLocation(MuzzleSocketName);
        FRotator MuzzleRotation = FirstPersonMesh->GetSocketRotation(MuzzleSocketName);
        
        // 获取目标位置并添加扩散
        FVector TargetLocation = WeaponOwner->GetWeaponTargetLocation();
        FRotator FireRotation = (TargetLocation - MuzzleLocation).Rotation();
        
        // 添加随机扩散
        if (WeaponDataRow.SpreadAngle > 0.0f)
        {
            FireRotation.Pitch += FMath::FRandRange(-WeaponDataRow.SpreadAngle, WeaponDataRow.SpreadAngle);
            FireRotation.Yaw += FMath::FRandRange(-WeaponDataRow.SpreadAngle, WeaponDataRow.SpreadAngle);
        }
        
        // 从对象池获取子弹
        if (ASDTABullet* Bullet = ASDTABullet::GetBullet())
        {
            Bullet->SetOwner(GetOwner());
            Bullet->SetInstigator(Cast<APawn>(GetOwner()));
            Bullet->SetDamage(WeaponDataRow.Damage);
            Bullet->SetRange(WeaponDataRow.Range);
            Bullet->ActivateBullet(MuzzleLocation, FireRotation);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("无法从对象池获取子弹"));
        }
    }

    // 播放枪口特效
    PlayMuzzleEffect();
}

/** 即时弹道开火 */
void ASDTAWeapon::FireHitScan()
{
    if (WeaponOwner)
    {
        FVector MuzzleLocation = FirstPersonMesh->GetSocketLocation(MuzzleSocketName);
        FVector TargetLocation = WeaponOwner->GetWeaponTargetLocation();
        FVector FireDirection = (TargetLocation - MuzzleLocation).GetSafeNormal();
        
        // 执行射线检测
        FHitResult HitResult;
        FCollisionQueryParams Params;
        Params.AddIgnoredActor(GetOwner());
        Params.AddIgnoredActor(this);
        
        if (GetWorld()->LineTraceSingleByChannel(HitResult, MuzzleLocation, MuzzleLocation + FireDirection * WeaponDataRow.Range, ECC_Visibility, Params))
        {
            // 命中处理
            if (UHealthComponent* HealthComp = HitResult.GetActor()->FindComponentByClass<UHealthComponent>())
            {
                HealthComp->RemoveHealth(WeaponDataRow.Damage);
                UE_LOG(LogTemp, Log, TEXT("射线命中: %s, 伤害: %.2f"), *HitResult.GetActor()->GetName(), WeaponDataRow.Damage);
            }
            // 播放命中特效
            PlayHitEffect(HitResult.Location, HitResult.Normal);
        }
    }

    // 播放枪口特效
    PlayMuzzleEffect();
}

/** 能量束开火 */
void ASDTAWeapon::FireEnergyBeam()
{
    // 简化实现，使用射线检测模拟能量束
    if (WeaponOwner)
    {
        FVector MuzzleLocation = FirstPersonMesh->GetSocketLocation(MuzzleSocketName);
        FVector TargetLocation = WeaponOwner->GetWeaponTargetLocation();
        FVector FireDirection = (TargetLocation - MuzzleLocation).GetSafeNormal();
        
        // 执行射线检测
        FHitResult HitResult;
        FCollisionQueryParams Params;
        Params.AddIgnoredActor(GetOwner());
        Params.AddIgnoredActor(this);
        
        if (GetWorld()->LineTraceSingleByChannel(HitResult, MuzzleLocation, MuzzleLocation + FireDirection * WeaponDataRow.Range, ECC_Visibility, Params))
        {
            // 命中处理
            if (UHealthComponent* HealthComp = HitResult.GetActor()->FindComponentByClass<UHealthComponent>())
            {
                HealthComp->RemoveHealth(WeaponDataRow.Damage);
                UE_LOG(LogTemp, Log, TEXT("能量束命中: %s, 伤害: %.2f"), *HitResult.GetActor()->GetName(), WeaponDataRow.Damage);
            }
            // 播放命中特效
            PlayHitEffect(HitResult.Location, HitResult.Normal);
        }
    }

    // 播放枪口特效
    PlayMuzzleEffect();
}

/** 霰弹开火 */
void ASDTAWeapon::FireShotgun()
{
    if (WeaponOwner)
    {
        FVector MuzzleLocation = FirstPersonMesh->GetSocketLocation(MuzzleSocketName);
        FVector TargetLocation = WeaponOwner->GetWeaponTargetLocation();
        FVector BaseDirection = (TargetLocation - MuzzleLocation).GetSafeNormal();
        
        // 发射多颗霰弹
        int32 PelletCount = FMath::Max(1, WeaponDataRow.PelletCount);
        for (int32 i = 0; i < PelletCount; i++)
        {
            // 每颗霰弹的随机方向
            FVector FireDirection = BaseDirection;
            if (WeaponDataRow.SpreadAngle > 0.0f)
            {
                FireDirection = FireDirection.RotateAngleAxis(FMath::FRandRange(-WeaponDataRow.SpreadAngle, WeaponDataRow.SpreadAngle), FVector::UpVector);
                FireDirection = FireDirection.RotateAngleAxis(FMath::FRandRange(-WeaponDataRow.SpreadAngle, WeaponDataRow.SpreadAngle), FVector::RightVector);
            }
            
            // 执行射线检测
            FHitResult HitResult;
            FCollisionQueryParams Params;
            Params.AddIgnoredActor(GetOwner());
            Params.AddIgnoredActor(this);
            
            if (GetWorld()->LineTraceSingleByChannel(HitResult, MuzzleLocation, MuzzleLocation + FireDirection * WeaponDataRow.Range, ECC_Visibility, Params))
            {
                // 命中处理（每颗霰弹伤害较低）
                float PelletDamage = WeaponDataRow.Damage / PelletCount;
                if (UHealthComponent* HealthComp = HitResult.GetActor()->FindComponentByClass<UHealthComponent>())
                {
                    HealthComp->RemoveHealth(PelletDamage);
                    UE_LOG(LogTemp, Log, TEXT("霰弹命中: %s, 伤害: %.2f"), *HitResult.GetActor()->GetName(), PelletDamage);
                }
                // 播放命中特效
                PlayHitEffect(HitResult.Location, HitResult.Normal);
            }
        }
    }

    // 播放枪口特效
    PlayMuzzleEffect();
}

/** 播放枪口特效 */
void ASDTAWeapon::PlayMuzzleEffect()
{
    // 简化实现，实际项目中应使用粒子特效和音效
    UE_LOG(LogTemp, Log, TEXT("播放枪口特效"));
}

/** 播放命中特效 */
void ASDTAWeapon::PlayHitEffect(const FVector& HitLocation, const FVector& HitNormal)
{
    // 简化实现，实际项目中应使用粒子特效和音效
    UE_LOG(LogTemp, Log, TEXT("播放命中特效: 位置=(%.2f, %.2f, %.2f), 法线=(%.2f, %.2f, %.2f)"), 
        HitLocation.X, HitLocation.Y, HitLocation.Z, 
        HitNormal.X, HitNormal.Y, HitNormal.Z);
}

/** 装填弹药 */
void ASDTAWeapon::Reload()
{
    CurrentAmmo = WeaponDataRow.MagazineSize;

    // 更新HUD
    if (WeaponOwner)
    {
        WeaponOwner->UpdateWeaponHUD(CurrentAmmo, WeaponDataRow.MagazineSize);
    }

    UE_LOG(LogTemp, Log, TEXT("武器装填: %s, 弹药: %d/%d"), *WeaponDataRow.WeaponName, CurrentAmmo, WeaponDataRow.MagazineSize);
}

/** 设置武器持有者 */
void ASDTAWeapon::SetWeaponOwner(ISDTAWeaponHolder* NewOwner)
{
    WeaponOwner = NewOwner;
}
