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

    /** 当前健康值，用于UI文本显示 */
    UPROPERTY(BlueprintReadWrite, Category = "SDTA HUD", Meta = (ExposeOnSpawn = true, DisplayName = "Current Health"))
    int32 CurrentHealth = 100;

    /** 最大健康值，用于UI文本显示 */
    UPROPERTY(BlueprintReadWrite, Category = "SDTA HUD", Meta = (ExposeOnSpawn = true, DisplayName = "Max Health"))
    int32 MaxHealth = 100;

    /** 能量值百分比，用于UI绑定 */
    UPROPERTY(BlueprintReadWrite, Category = "SDTA HUD", Meta = (ExposeOnSpawn = true, DisplayName = "Stamina Percent", OnChanged="OnStaminaPercentChanged"))
    float StaminaPercent = 1.0f;

    /** 当前能量值，用于UI文本显示 */
    UPROPERTY(BlueprintReadWrite, Category = "SDTA HUD", Meta = (ExposeOnSpawn = true, DisplayName = "Current Stamina"))
    int32 CurrentStamina = 100;

    /** 最大能量值，用于UI文本显示 */
    UPROPERTY(BlueprintReadWrite, Category = "SDTA HUD", Meta = (ExposeOnSpawn = true, DisplayName = "Max Stamina"))
    int32 MaxStamina = 100;

protected:
	/** 健康值百分比变化时的回调 */
	UFUNCTION()
	void OnHealthPercentChanged();

	/** 能量值百分比变化时的回调 */
	UFUNCTION()
	void OnStaminaPercentChanged();

	/** 时间百分比变化时的回调 */
	UFUNCTION()
	void OnTimePercentChanged();

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

public:
    /** 当前是否为夜晚阶段 */
    UPROPERTY(BlueprintReadWrite, Category = "SDTA HUD", Meta = (ExposeOnSpawn = true, DisplayName = "Is Night"))
    bool bIsNight = false;

    /** 当前天数 */
    UPROPERTY(BlueprintReadWrite, Category = "SDTA HUD", Meta = (ExposeOnSpawn = true, DisplayName = "Current Day"))
    int32 CurrentDay = 1;

    /** 剩余时间（秒） */
    UPROPERTY(BlueprintReadWrite, Category = "SDTA HUD", Meta = (ExposeOnSpawn = true, DisplayName = "Remaining Time"))
    float RemainingTime = 0.0f;

    /** 剩余时间百分比（用于进度条） */
	UPROPERTY(BlueprintReadWrite, Category = "SDTA HUD", Meta = (ExposeOnSpawn = true, DisplayName = "Time Percent", OnChanged="OnTimePercentChanged"))
	float TimePercent = 1.0f;

	/** 灵魂碎片数量 */
	UPROPERTY(BlueprintReadWrite, Category = "SDTA HUD", Meta = (ExposeOnSpawn = true, DisplayName = "Soul Fragments"))
	int32 SoulFragments = 0;

    /** 更新昼夜循环显示 */
    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "SDTA HUD")
    void BP_UpdateDayNightCycle(bool bInIsNight, int32 InCurrentDay, float InRemainingTime, float InTimePercent);

    /** 更新灵魂碎片显示 */
    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "SDTA HUD")
    void BP_UpdateSoulFragments();
};