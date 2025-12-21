// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SDTAWeaponHolder.h"
#include "Animation/AnimInstance.h"
#include "Variant_Survival/Core/Pool/SDTAPoolManager.h"
#include "Variant_Survival/Weapons/SDTAProjectiles.h"
#include "SDTAWeapon.generated.h"

class ISDTAWeaponHolder;
class ASDTAProjectiles;
class USkeletalMeshComponent;
class UAnimMontage;
class UAnimInstance;
class USDTAPoolManager;

/**
 *  生存游戏武器的基类
 *  提供第一人称和第三人称视角的网格
 *  处理弹药和开火逻辑
 *  通过SDTAWeaponHolder接口与武器所有者交互
 */
UCLASS(abstract)
class SEVENDAYSTOALIVE_API ASDTAWeapon : public AActor
{
	GENERATED_BODY()
	
	/** 第一人称视角网格 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* FirstPersonMesh;

	/** 第三人称视角网格 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* ThirdPersonMesh;

protected:

	/** 投射到武器所有者的指针 */
	ISDTAWeaponHolder* WeaponOwner;

	/** 对象池管理器指针 */
	UPROPERTY()
	USDTAPoolManager* PoolManager;

	/** 此武器发射的投射物类型 */
	UPROPERTY(EditAnywhere, Category="弹药")
	TSubclassOf<ASDTAProjectiles> ProjectileClass;

	/** 弹夹中的子弹数量 */
	UPROPERTY(EditAnywhere, Category="弹药", meta = (ClampMin = 0, ClampMax = 100))
	int32 MagazineSize = 10;

	/** 当前弹夹中的子弹数量 */
	int32 CurrentBullets = 0;
	
	/** 开火时播放的动画蒙太奇 */
	UPROPERTY(EditAnywhere, Category="Animation")
	UAnimMontage* FiringMontage;

	/** 武器激活时为第一人称角色网格设置的AnimInstance类 */
	UPROPERTY(EditAnywhere, Category="Animation")
	TSubclassOf<UAnimInstance> FirstPersonAnimInstanceClass;

	/** 武器激活时为第三人称角色网格设置的AnimInstance类 */
	UPROPERTY(EditAnywhere, Category="Animation")
	TSubclassOf<UAnimInstance> ThirdPersonAnimInstanceClass;

	/** 瞄准时光束半角的偏差 */
	//注意，后坐力系统还没有实现
	UPROPERTY(EditAnywhere, Category="瞄准", meta = (ClampMin = 0, ClampMax = 90, Units = "Degrees"))
	float AimVariance = 0.0f;

	/** 应用到所有者的开火后坐力大小 */
	UPROPERTY(EditAnywhere, Category="瞄准", meta = (ClampMin = 0, ClampMax = 100))
	float FiringRecoil = 0.0f;

	/** 第一人称枪口插座的名称，投射物将在此生成 */
	UPROPERTY(EditAnywhere, Category="瞄准")
	FName MuzzleSocketName;

	/** 子弹在枪口前方生成的距离 */
	UPROPERTY(EditAnywhere, Category="瞄准", meta = (ClampMin = 0, ClampMax = 1000, Units = "cm"))
	float MuzzleOffset = 10.0f;

	/** 如果为true，此武器将以射速自动开火 */
	UPROPERTY(EditAnywhere, Category="射击模式")
	bool bFullAuto = false;

	/** 此武器射击之间的时间间隔，影响全自动和半自动模式 */
	UPROPERTY(EditAnywhere, Category="射击模式", meta = (ClampMin = 0, ClampMax = 5, Units = "s"))
	float RefireRate = 0.2f;

	/** 最后一次射击的游戏时间，用于在半自动模式下强制射速 */
	float TimeOfLastShot = 0.0f;

	/** 如果为true，武器当前正在开火 */
	bool bIsFiring = false;

	/** 处理全自动射击的定时器 */
	FTimerHandle RefireTimer;

	/** 投射到所有者的pawn指针，用于AI感知系统交互 */
	TObjectPtr<APawn> PawnOwner;

	/** 射击的音量，用于AI感知系统交互 */
	UPROPERTY(EditAnywhere, Category="Perception", meta = (ClampMin = 0, ClampMax = 100))
	float ShotLoudness = 1.0f;

	/** 射击AI感知噪音的最大范围 */
	UPROPERTY(EditAnywhere, Category="Perception", meta = (ClampMin = 0, ClampMax = 100000, Units = "cm"))
	float ShotNoiseRange = 3000.0f;

	/** 应用到此武器射击产生的噪音的标签 */
	UPROPERTY(EditAnywhere, Category="Perception")
	FName ShotNoiseTag = FName("Shot");

public:	

	/** 构造函数 */
	ASDTAWeapon();

protected:
	
	/** 游戏初始化 */
	virtual void BeginPlay() override;

	/** 游戏清理 */
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

protected:

	/** 当武器的所有者被销毁时调用 */
	UFUNCTION()
	void OnOwnerDestroyed(AActor* DestroyedActor);

public:

	/** 激活此武器并准备开火 */
	void ActivateWeapon();

	/** 停用此武器 */
	void DeactivateWeapon();

	/** 开始开火 */
	void StartFiring();

	/** 停止开火 */
	void StopFiring();

protected:

	/** 开火 */
	virtual void Fire();

	/** 在射击半自动武器时，当射速时间过去时调用 */
	void FireCooldownExpired();

	/** 向目标位置发射投射物 */
	virtual void FireProjectile(const FVector& TargetLocation);

	/** 为此武器发射的投射物计算生成变换 */
	FTransform CalculateProjectileSpawnTransform(const FVector& TargetLocation) const;

public:

	/** 返回第一人称网格 */
	UFUNCTION(BlueprintPure, Category="Weapon")
	USkeletalMeshComponent* GetFirstPersonMesh() const { return FirstPersonMesh; };

	/** 返回第三人称网格 */
	UFUNCTION(BlueprintPure, Category="Weapon")
	USkeletalMeshComponent* GetThirdPersonMesh() const { return ThirdPersonMesh; };

	/** 返回第一人称AnimInstance类 */
	const TSubclassOf<UAnimInstance>& GetFirstPersonAnimInstanceClass() const;

	/** 返回第三人称AnimInstance类 */
	const TSubclassOf<UAnimInstance>& GetThirdPersonAnimInstanceClass() const;

	/** 返回弹夹大小 */
	int32 GetMagazineSize() const { return MagazineSize; };

	/** 返回当前子弹数量 */
	int32 GetBulletCount() const { return CurrentBullets; }

	/** 重新装填武器 */
	UFUNCTION(BlueprintCallable, Category="Weapon")
	void Reload();
};