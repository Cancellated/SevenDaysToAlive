// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SDTAWeaponHolder.generated.h"

class ASDTAWeapon;
class UAnimMontage;

// 此类不需要修改。
UINTERFACE(MinimalAPI)
class USDTAWeaponHolder : public UInterface
{
	GENERATED_BODY()
};

/**
 *  Common interface for Survival Game weapon holder classes
 */
class SEVENDAYSTOALIVE_API ISDTAWeaponHolder
{
	GENERATED_BODY()

public:

	/** 将武器的网格附加到所有者 */
	virtual void AttachWeaponMeshes(ASDTAWeapon* Weapon) = 0;

	/** 播放武器的开火动画蒙太奇 */
	virtual void PlayFiringMontage(UAnimMontage* Montage) = 0;

	/** 向所有者应用武器后坐力 */
	virtual void AddWeaponRecoil(float Recoil) = 0;

	/** 使用当前弹药数量更新武器HUD */
	virtual void UpdateWeaponHUD(int32 CurrentAmmo, int32 MagazineSize) = 0;

	/** 计算并返回武器的瞄准位置 */
	virtual FVector GetWeaponTargetLocation() = 0;

	/** 给所有者一个此类的武器 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Weapon")
	void AddWeaponClass(TSubclassOf<ASDTAWeapon> WeaponClass);
	virtual void AddWeaponClass_Implementation(TSubclassOf<ASDTAWeapon> WeaponClass) = 0;

	/** 激活传入的武器 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Weapon")
	void OnWeaponActivated(ASDTAWeapon* Weapon);
	virtual void OnWeaponActivated_Implementation(ASDTAWeapon* Weapon) = 0;

	/** 停用传入的武器 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Weapon")
	void OnWeaponDeactivated(ASDTAWeapon* Weapon);
	virtual void OnWeaponDeactivated_Implementation(ASDTAWeapon* Weapon) = 0;
};