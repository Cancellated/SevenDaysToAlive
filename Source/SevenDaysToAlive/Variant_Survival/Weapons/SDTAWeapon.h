// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SDTAWeaponTypes.h"
#include "SDTAWeaponHolder.h"
#include "SDTAWeapon.generated.h"

class USkeletalMeshComponent;
class UStaticMeshComponent;
class UAnimMontage;

/**
 * 武器基类
 */
UCLASS(abstract)
class SEVENDAYSTOALIVE_API ASDTAWeapon : public AActor
{
    GENERATED_BODY()

public:
    /** 构造函数 */
    ASDTAWeapon();

protected:
    /** 武器持有者接口 */
    ISDTAWeaponHolder* WeaponOwner;

    /** 武器数据表行 */
    UPROPERTY(EditAnywhere, Category = "Weapon Data")
    FDataTableRowHandle WeaponData;

    /** 武器数据 */
    FSDTAWeaponTableRow WeaponDataRow;

    /** 第一人称武器网格 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    USkeletalMeshComponent* FirstPersonMesh;

    /** 第三人称武器网格 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    USkeletalMeshComponent* ThirdPersonMesh;

    /** 当前弹药数量 */
    int32 CurrentAmmo;

    /** 武器是否正在开火 */
    bool bIsFiring;

    /** 上次开火时间 */
    float LastFireTime;

    /** 开火动画 */
    UPROPERTY(EditAnywhere, Category = "Animation")
    UAnimMontage* FiringMontage;

    /** 后坐力 */
    UPROPERTY(EditAnywhere, Category = "Weapon", meta = (ClampMin = 0))
    float Recoil;

    /** 子弹类 */
    UPROPERTY(EditAnywhere, Category = "Projectile")
    TSubclassOf<class ASDTABullet> BulletClass;

    /** 枪口 socket 名称 */
    UPROPERTY(EditAnywhere, Category = "Projectile")
    FName MuzzleSocketName;

protected:
    /** 游戏开始时调用 */
    virtual void BeginPlay() override;

public:
    /** 激活武器 */
    void ActivateWeapon();

    /** 停用武器 */
    void DeactivateWeapon();

    /** 开始开火 */
    void StartFiring();

    /** 停止开火 */
    void StopFiring();

    /** 开火 */
    virtual void Fire();

    /** 实体子弹开火 */
    void FireProjectile();

    /** 即时弹道开火 */
    void FireHitScan();

    /** 能量束开火 */
    void FireEnergyBeam();

    /** 霰弹开火 */
    void FireShotgun();

    /** 播放枪口特效 */
    void PlayMuzzleEffect();

    /** 播放命中特效 */
    void PlayHitEffect(const FVector& HitLocation, const FVector& HitNormal);

    /** 装填弹药 */
    void Reload();

    /** 设置武器持有者 */
    void SetWeaponOwner(ISDTAWeaponHolder* Owner);

    /** 获取当前弹药数量 */
    int32 GetCurrentAmmo() const { return CurrentAmmo; }

    /** 获取弹匣容量 */
    int32 GetMagazineSize() const { return WeaponDataRow.MagazineSize; }

    /** 获取武器名称 */
    FString GetWeaponName() const { return WeaponDataRow.WeaponName; }

    /** 获取第一人称武器网格 */
    USkeletalMeshComponent* GetFirstPersonMesh() const { return FirstPersonMesh; }

    /** 获取第三人称武器网格 */
    USkeletalMeshComponent* GetThirdPersonMesh() const { return ThirdPersonMesh; }

    /** 获取武器伤害 */
    float GetDamage() const { return WeaponDataRow.Damage; }

    /** 获取武器射速 */
    float GetFireRate() const { return WeaponDataRow.FireRate; }

    /** 获取武器射程 */
    float GetRange() const { return WeaponDataRow.Range; }
};
