// Fill out your copyright notice in the Description page of Project Settings.

#include "Variant_SDTA/UI/SDTAPlayerHUD.h"

// 健康值百分比变化时的回调实现
void USDTAPlayerHUD::OnHealthPercentChanged()
{
    BP_UpdateHealthBar(HealthPercent);
}

// 能量值百分比变化时的回调实现
void USDTAPlayerHUD::OnStaminaPercentChanged()
{
    BP_UpdateStaminaBar(StaminaPercent);
}

// 时间百分比变化时的回调实现
void USDTAPlayerHUD::OnTimePercentChanged()
{
    // 调用BP_UpdateDayNightCycle更新昼夜循环显示
    BP_UpdateDayNightCycle(bIsNight, CurrentDay, RemainingTime, TimePercent);
}
