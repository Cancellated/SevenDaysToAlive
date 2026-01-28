// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SDTAWeaponHolderInterface.generated.h"

// 前向声明
class ASDTAWeapon;
class UAnimMontage;

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class USDTAWeaponHolder : public UInterface
{
	GENERATED_BODY()
};

/**
 * 武器持有者接口，定义武器持有者需要实现的方法
 */
class SEVENDAYSTOALIVE_API ISDTAWeaponHolder
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	// 附加武器网格
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Weapon Holder")
	void AttachWeaponMeshes(ASDTAWeapon* Weapon);

	// 播放开火动画
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Weapon Holder")
	void PlayFiringMontage(UAnimMontage* Montage);

	// 应用武器后坐力
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Weapon Holder")
	void AddWeaponRecoil(float RecoilAmount);

	// 更新武器HUD
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Weapon Holder")
	void UpdateWeaponHUD(int32 CurrentAmmo, int32 MaxAmmo);

	// 获取武器瞄准目标位置
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Weapon Holder")
	FVector GetWeaponTargetLocation();

	// 添加武器类
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Weapon Holder")
	void AddWeaponClass(TSubclassOf<ASDTAWeapon> WeaponClass);

	// 武器激活时调用
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Weapon Holder")
	void OnWeaponActivated(ASDTAWeapon* Weapon);

	// 武器停用时调用
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Weapon Holder")
	void OnWeaponDeactivated(ASDTAWeapon* Weapon);
};
