// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SDTAWeaponHolder.generated.h"

class ASDTAWeapon;
class UAnimMontage;

// 武器持有者接口
UINTERFACE(MinimalAPI)
class USDTAWeaponHolder : public UInterface
{
    GENERATED_BODY()
};

/**
 * 武器持有者接口
 */
class SEVENDAYSTOALIVE_API ISDTAWeaponHolder
{
    GENERATED_BODY()

public:
    /** 附加武器网格 */
    virtual void AttachWeaponMeshes(ASDTAWeapon* Weapon) = 0;

    /** 播放开火动画 */
    virtual void PlayFiringMontage(UAnimMontage* Montage) = 0;

    /** 应用武器后坐力 */
    virtual void AddWeaponRecoil(float Recoil) = 0;

    /** 更新武器HUD */
    virtual void UpdateWeaponHUD(int32 CurrentAmmo, int32 MagazineSize) = 0;

    /** 获取武器瞄准目标位置 */
    virtual FVector GetWeaponTargetLocation() = 0;

    /** 添加武器类 */
    virtual void AddWeaponClass(const TSubclassOf<ASDTAWeapon>& WeaponClass) = 0;

    /** 武器激活时调用 */
    virtual void OnWeaponActivated(ASDTAWeapon* Weapon) = 0;

    /** 武器停用时调用 */
    virtual void OnWeaponDeactivated(ASDTAWeapon* Weapon) = 0;
};
