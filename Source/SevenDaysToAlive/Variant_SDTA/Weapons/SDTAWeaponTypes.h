#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "SDTAWeaponTypes.generated.h"

UENUM(BlueprintType)
enum class ESDTABulletType : uint8
{
	Projectile UMETA(DisplayName = "Projectile", Tooltip = "实体子弹"),
	HitScan UMETA(DisplayName = "HitScan", Tooltip = "即时弹道"),
	EnergyBeam UMETA(DisplayName = "EnergyBeam", Tooltip = "能量束"),
	Shotgun UMETA(DisplayName = "Shotgun", Tooltip = "霰弹")
};

USTRUCT(BlueprintType)
struct FSDTAWeaponTableRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Data")
	FString WeaponName = TEXT("Default Weapon");

	/** 第一人称武器模型 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Data")
	TSoftObjectPtr<USkeletalMesh> FirstPersonMesh;

	/** 第三人称武器模型 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Data")
	TSoftObjectPtr<USkeletalMesh> ThirdPersonMesh;

	/** 武器在库存中的显示模型 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Data")
	TSoftObjectPtr<UStaticMesh> InventoryMesh;

	/** 武器开火动画 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Data")
	TSoftObjectPtr<UAnimMontage> FiringMontage;

	/** 武器装弹动画 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Data")
	TSoftObjectPtr<UAnimMontage> ReloadMontage;

	/** 第一人称武器动画实例类 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Data")
	TSubclassOf<UAnimInstance> FirstPersonAnimInstanceClass;

	/** 第三人称武器动画实例类 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Data")
	TSubclassOf<UAnimInstance> ThirdPersonAnimInstanceClass;

	/** 武器弹匣容量 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Data", Meta = (ClampMin = 1, ClampMax = 100))
	int32 MagazineSize = 30;

	/** 武器伤害 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Data", Meta = (ClampMin = 0))
	float Damage = 20.0f;

	/** 武器射速(发/秒) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Data", Meta = (ClampMin = 0.01, ClampMax = 5, Units = "s"))
	float FireRate = 0.15f;

	/** 武器射程 */	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Data", Meta = (ClampMin = 0, ClampMax = 100000, Units = "cm"))
	float Range = 10000.0f;

	/** 武器子弹类型 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Data")
	ESDTABulletType BulletType = ESDTABulletType::Projectile;

	/** 武器子弹类 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Data")
	TSubclassOf<class ASDTABullet> BulletClass;

	/** 武器弹丸数量(霰弹用) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Data", Meta = (ClampMin = 1, ClampMax = 20))
	int32 PelletCount = 1;

	/** 武器扩散角度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Data", Meta = (ClampMin = 0, ClampMax = 90, Units = "Degrees"))
	float SpreadAngle = 0.0f;

	/** 武器枪口Socket名称 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Data")
	FName MuzzleSocketName = FName("Muzzle");
	
	/** 武器枪口偏移距离 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Data", Meta = (ClampMin = 0, ClampMax = 1000, Units = "cm"))
	float MuzzleOffset = 0.0f;

	/** 武器后坐力 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Data", Meta = (ClampMin = 0, ClampMax = 100))
	float FiringRecoil = 0.0f;

	/** 是否全自动 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Data")
	bool bFullAuto = false;
};
