// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SDTAPlayerHUD.generated.h"

/**
 * 生存模式玩家HUD类，负责显示健康值、能量值等UI元素
 */
UCLASS()
class SEVENDAYSTOALIVE_API USDTAPlayerHUD : public UUserWidget
{
    GENERATED_BODY()

public:
    /** 健康值百分比，用于UI绑定 */
    UPROPERTY(BlueprintReadWrite, Category = "SDTA HUD", Meta = (ExposeOnSpawn = true, DisplayName = "Health Percent", OnChanged="OnHealthPercentChanged"))
    float HealthPercent = 1.0f;

    /** 能量值百分比，用于UI绑定 */
    UPROPERTY(BlueprintReadWrite, Category = "SDTA HUD", Meta = (ExposeOnSpawn = true, DisplayName = "Stamina Percent", OnChanged="OnStaminaPercentChanged"))
    float StaminaPercent = 1.0f;

protected:
    /** 健康值百分比变化时的回调 */
    UFUNCTION()
    void OnHealthPercentChanged();

    /** 能量值百分比变化时的回调 */
    UFUNCTION()
    void OnStaminaPercentChanged();

public:
    /** 更新健康值显示 */
    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "SDTA HUD")
    void BP_UpdateHealthBar(float InHealthPercent);

    /** 更新能量值显示 */
    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "SDTA HUD")
    void BP_UpdateStaminaBar(float InStaminaPercent);

    /** 显示死亡画面 */
    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "SDTA HUD")
    void BP_ShowDeathScreen();

    /** 重置HUD显示 */
    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "SDTA HUD")
    void BP_ResetHUD();
};