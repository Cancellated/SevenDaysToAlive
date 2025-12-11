// Fill out your copyright notice in the Description page of Project Settings.

#include "Variant_Survival/UI/SDTAPlayerHUD.h"

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
