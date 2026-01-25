// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "SDTAWeaponTypes.generated.h"

class ASDTAWeapon;
class ASDTABullet;

/**
 * 子弹类型枚举
 */
UENUM(BlueprintType)
enum class ESDTABulletType : uint8
{
    /** 实体子弹（有物理存在，需要碰撞检测） */
    Projectile UMETA(DisplayName = "实体子弹"),
    
    /** 无实体子弹（即时弹道，使用射线检测） */
    HitScan UMETA(DisplayName = "即时弹道"),
    
    /** 能量束（持续伤害，如激光） */
    EnergyBeam UMETA(DisplayName = "能量束"),
    
    /** 霰弹（多射线检测） */
    Shotgun UMETA(DisplayName = "霰弹"),
    
    /** 预留扩展类型 */
    Custom1 UMETA(DisplayName = "自定义1"),
    Custom2 UMETA(DisplayName = "自定义2")
};

/**
 * 武器数据表行结构体
 */
USTRUCT(BlueprintType)
struct FSDTAWeaponTableRow : public FTableRowBase
{
    GENERATED_BODY()

    /** 武器名称 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FString WeaponName;

    /** 武器静态网格（用于拾取物） */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TSoftObjectPtr<UStaticMesh> StaticMesh;

    /** 武器骨骼网格（用于装备） */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TSoftObjectPtr<USkeletalMesh> SkeletalMesh;

    /** 武器类 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TSubclassOf<ASDTAWeapon> WeaponClass;

    /** 弹匣容量 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = 1))
    int32 MagazineSize;

    /** 伤害值 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = 0))
    float Damage;

    /** 射速（秒/发） */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = 0.1))
    float FireRate;

    /** 射程 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = 0))
    float Range;

    /** 子弹类型 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile")
    ESDTABulletType BulletType;
    
    /** 实体子弹类（仅当 BulletType 为 Projectile 时使用） */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile", meta = (EditCondition = "BulletType == ESDTABulletType::Projectile"))
    TSubclassOf<ASDTABullet> BulletClass;
    
    /** 霰弹枪射线数量（仅当 BulletType 为 Shotgun 时使用） */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile", meta = (EditCondition = "BulletType == ESDTABulletType::Shotgun", ClampMin = 1))
    int32 PelletCount;
    
    /** 子弹扩散角度（仅当 BulletType 为 Projectile 或 Shotgun 时使用） */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile", meta = (ClampMin = 0))
    float SpreadAngle;
};
