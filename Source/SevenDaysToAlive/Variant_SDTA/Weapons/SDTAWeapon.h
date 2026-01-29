// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"
#include "SDTAWeaponTypes.h"
#include "SDTABullet.h"
#include "SDTAWeaponHolderInterface.h"
#include "SDTAWeapon.generated.h"

// 前向声明
class ISDTAWeaponHolder;
class USkeletalMeshComponent;
class UAnimMontage;

/**
 * 武器基类，实现武器核心功能
 */
UCLASS()
class SEVENDAYSTOALIVE_API ASDTAWeapon : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ASDTAWeapon();

protected:
	// 第一人称武器网格
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USkeletalMeshComponent* FirstPersonMesh;

	// 第三人称武器网格
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USkeletalMeshComponent* ThirdPersonMesh;

	// 武器数据表行引用
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Data")
	FDataTableRowHandle WeaponData;

	// 武器数据
	UPROPERTY(BlueprintReadOnly, Category = "Weapon Data")
	FSDTAWeaponTableRow WeaponDataRow;

	// 当前弹药数量
	UPROPERTY(BlueprintReadWrite, Category = "Weapon Data")
	int32 CurrentAmmo;

	// 是否正在开火
	UPROPERTY(BlueprintReadOnly, Category = "Weapon State")
	bool bIsFiring;

	// 上次开火时间
	UPROPERTY(BlueprintReadOnly, Category = "Weapon State")
	float LastFireTime;

	// 武器后坐力
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Data")
	float Recoil;

	// 子弹类
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Data")
	TSubclassOf<ASDTABullet> BulletClass;

	// 枪口socket名称
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Data")
	FName MuzzleSocketName;

	// 武器持有者
	UPROPERTY(BlueprintReadOnly, Category = "Weapon Owner")
	TScriptInterface<ISDTAWeaponHolder> WeaponOwner;

	// 开火定时器句柄
	FTimerHandle FireTimerHandle;

public:
	// 激活武器
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void ActivateWeapon();

	// 停用武器
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void DeactivateWeapon();

	// 开始开火
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void StartFiring();

	// 停止开火
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void StopFiring();

	// 开火逻辑
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void Fire();

	// 定时器开火回调
	void OnFireTimer();

	// 发射实体子弹
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void FireProjectile();

	// 即时弹道开火
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void FireHitScan();

	// 能量束开火
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void FireEnergyBeam();

	// 霰弹开火
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void FireShotgun();

	// 装填弹药
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void Reload();

	// 设置武器数据
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void SetWeaponData(const FDataTableRowHandle& NewWeaponData);

	// 设置武器持有者
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void SetWeaponOwner(TScriptInterface<ISDTAWeaponHolder> NewOwner);

	// 设置武器数据行
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void SetWeaponDataRow(const FSDTAWeaponTableRow& NewWeaponDataRow);

	// 获取武器数据行
	UFUNCTION(BlueprintPure, Category = "Weapon")
	FSDTAWeaponTableRow GetWeaponDataRow() const { return WeaponDataRow; }

	// 获取武器数据表格句柄
	UFUNCTION(BlueprintPure, Category = "Weapon")
	FDataTableRowHandle GetWeaponData() const { return WeaponData; }

	// 获取当前弹药数量
	UFUNCTION(BlueprintPure, Category = "Weapon")
	int32 GetCurrentAmmo() const { return CurrentAmmo; }

	// 获取弹匣容量
	UFUNCTION(BlueprintPure, Category = "Weapon")
	int32 GetMagazineSize() const { return WeaponDataRow.MagazineSize; }

	// 获取武器伤害
	UFUNCTION(BlueprintPure, Category = "Weapon")
	float GetWeaponDamage() const { return WeaponDataRow.Damage; }

	// 获取武器射程
	UFUNCTION(BlueprintPure, Category = "Weapon")
	float GetWeaponRange() const { return WeaponDataRow.Range; }

	// 获取武器名称
	UFUNCTION(BlueprintPure, Category = "Weapon")
	FString GetWeaponName() const { return WeaponDataRow.WeaponName; }

	// 获取第一人称动画实例类
	UFUNCTION(BlueprintPure, Category = "Weapon")
	TSubclassOf<UAnimInstance> GetFirstPersonAnimInstanceClass() const;

	// 获取第三人称动画实例类
	UFUNCTION(BlueprintPure, Category = "Weapon")
	TSubclassOf<UAnimInstance> GetThirdPersonAnimInstanceClass() const;

	// 获取第一人称武器网格
	UFUNCTION(BlueprintPure, Category = "Weapon")
	USkeletalMeshComponent* GetFirstPersonMesh() const { return FirstPersonMesh; }

	// 获取第三人称武器网格
	UFUNCTION(BlueprintPure, Category = "Weapon")
	USkeletalMeshComponent* GetThirdPersonMesh() const { return ThirdPersonMesh; }

	// 服务器端开始开火
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerStartFire();

	// 服务器端停止开火
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerStopFire();

	// 服务器端装填弹药
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerReload();

protected:
	// 加载武器数据
	void LoadWeaponData();

	// 检查是否可以开火
	bool CanFire() const;

	// 应用后坐力
	void ApplyRecoil();

	// 播放开火特效
	void PlayFiringEffects();

	// 播放开火动画
	void PlayFiringMontage();

	// 网络同步
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};