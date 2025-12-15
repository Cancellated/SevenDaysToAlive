// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SDTADebugUI.generated.h"

/**
 * DebugUI类，用于显示角色当前速度等调试信息
 */
UCLASS()
class SEVENDAYSTOALIVE_API USDTADebugUI : public UUserWidget
{
    GENERATED_BODY()

public:
    /** 当前角色速度，用于UI文本显示 */
    UPROPERTY(BlueprintReadWrite, Category = "SDTA Debug", Meta = (ExposeOnSpawn = true, DisplayName = "Current Speed"))
    float CurrentSpeed = 0.0f;

    /** 更新速度显示 */
    UFUNCTION(BlueprintCallable, Category = "SDTA Debug")
    void UpdateSpeed(float NewSpeed);
};
