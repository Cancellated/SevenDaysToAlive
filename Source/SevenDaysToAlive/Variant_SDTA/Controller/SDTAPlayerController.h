// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SevenDaysToAlivePlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "Variant_SDTA/Characters/SDTAPlayerBase.h"
#include "Variant_SDTA/UI/SDTAPlayerHUD.h"
#include "Variant_SDTA/UI/SDTADebugUI.h"
#include "Widgets/Input/SVirtualJoystick.h"
#include "SDTAPlayerController.generated.h"

/**
 * SDTA模式的玩家控制器类，负责输入管理、UI界面、角色控制和重生逻辑
 */
UCLASS()
class SEVENDAYSTOALIVE_API ASDTAPlayerController : public ASevenDaysToAlivePlayerController
{
	GENERATED_BODY()

public:
	/** 构造函数 */
	ASDTAPlayerController();

protected:
	/** 游戏开始时调用 */
	virtual void BeginPlay() override;

	/** 设置输入组件 */
	virtual void SetupInputComponent() override;

	/** 每帧调用一次 */
	virtual void Tick(float DeltaSeconds) override;

	

	/** 当Actor结束生命周期时调用 */
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

	/** 健康值变化时的回调 */
	UFUNCTION()
	void OnHealthChanged(float HealthPercent);

	/** 能量值变化时的回调 */
	UFUNCTION()
	void OnStaminaChanged(float StaminaPercent);

	/** 角色死亡时的回调 */
	UFUNCTION()
	void OnPawnDeath();

	/** 角色类 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<ASDTAPlayerBase> CharacterClass;

	/** 玩家标签 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character", meta = (AllowPrivateAccess = "true"))
	FName PlayerPawnTag;

	/** HUD界面类 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<USDTAPlayerHUD> PlayerHUDClass;

	/** HUD界面对象 */
	UPROPERTY(BlueprintReadOnly, Category = "UI")
	TObjectPtr<USDTAPlayerHUD> PlayerHUD;

	

	/** DebugUI界面类 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Debug", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<USDTADebugUI> DebugUIWidgetClass;

	/** DebugUI界面对象 */
	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	TObjectPtr<USDTADebugUI> DebugUIWidget;

protected:
	/** 跟踪上一次的整数值，用于减少不必要的HUD更新 */
	int32 LastHealthInt;
	int32 LastMaxHealthInt;
	int32 LastStaminaInt;
	int32 LastMaxStaminaInt;
	
	// 本地计时相关
	bool bIsNightPhase = false;
	float TotalPhaseDuration = 0.0f;
	float ElapsedPhaseTime = 0.0f;
	
	// 更新本地计时
	void UpdateLocalTiming(float DeltaTime);
	
	// 计算剩余时间和百分比
	float GetRemainingTime() const;
	float CalculateTimePercent() const;

public:
	/** 获取当前控制的SDTA角色 */
	ASDTAPlayerBase* GetControlledSDTAPlayer() const;

	/** 获取PlayerHUD实例 */
	USDTAPlayerHUD* GetPlayerHUD() const;

	/** 获取SDTA GameState */
	UFUNCTION(BlueprintCallable, Category = "Game State")
	class ASDTAGameState* GetSDTAGameState() const;

	/** 获取SDTA PlayerState */
	UFUNCTION(BlueprintCallable, Category = "Player State")
	class ASDTAPlayerState* GetSDTAPlayerState() const;

	/** 处理冲刺输入 */
	UFUNCTION()
	void HandleDashInput();
	
	/** 开始本地计时 */
	UFUNCTION(BlueprintCallable, Category = "SDTA Player")
	void StartLocalTiming(bool InIsNightPhase, float TotalDuration);

private:
	/**
	 * 客户端环境同步方法
	 * 用于在客户端根据GameState更新昼夜环境效果
	 */
	void UpdateClientEnvironment();
};