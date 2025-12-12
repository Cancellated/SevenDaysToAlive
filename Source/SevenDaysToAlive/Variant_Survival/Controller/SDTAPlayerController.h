// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SevenDaysToAlivePlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "Variant_Survival/Characters/SDTAPlayer.h"
#include "Variant_Survival/UI/SDTAPlayerHUD.h"
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

	/** 接管角色时调用 */
	virtual void OnPossess(APawn* InPawn) override;

	/** 释放角色时调用 */
	virtual void OnUnPossess() override;

	/** 当Actor结束生命周期时调用 */
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

	/** 角色被销毁时的回调 */
	UFUNCTION()
	void OnPawnDestroyed(AActor* DestroyedActor);

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
	TSubclassOf<ASDTAPlayer> CharacterClass;

	/** 玩家标签 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character", meta = (AllowPrivateAccess = "true"))
	FName PlayerPawnTag;

	/** HUD界面类 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<USDTAPlayerHUD> PlayerHUDClass;

	/** HUD界面对象 */
	UPROPERTY(BlueprintReadOnly, Category = "UI")
	TObjectPtr<USDTAPlayerHUD> PlayerHUD;

public:
	/** 获取当前控制的SDTA角色 */
	ASDTAPlayer* GetControlledSDTAPlayer() const;
};