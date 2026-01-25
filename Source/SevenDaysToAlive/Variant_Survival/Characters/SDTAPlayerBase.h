// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SevenDaysToAliveCharacter.h"
#include "Variant_Survival/Components/HealthComponent.h"
#include "Variant_Survival/Components/StaminaComponent.h"
#include "SDTAPlayerBase.generated.h"

// 健康值变化委托
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealthChanged, float, HealthPercent);

// 死亡委托
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeath);

// 耐力值变化委托
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStaminaChanged, float, StaminaPercent);

// 前向声明
class UDashComponent;

UCLASS()
class SEVENDAYSTOALIVE_API ASDTAPlayerBase : public ASevenDaysToAliveCharacter
{
	GENERATED_BODY()

public:
	/** 构造函数 */
	ASDTAPlayerBase();

protected:
	/** 冲刺输入动作 */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* DashAction;

	/** 开火输入动作 */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* FireAction;

protected:
	/** 游戏开始时调用 */
	virtual void BeginPlay() override;

public:
	/** 每帧调用 */
	virtual void Tick(float DeltaTime) override;

	/** 绑定输入功能 */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/** 处理瞄准输入 */
	virtual void DoAim(float Yaw, float Pitch) override;

	/** 处理移动输入 */
	virtual void DoMove(float Right, float Forward) override;

	/** 处理跳跃开始输入 */
	virtual void DoJumpStart() override;

	/** 处理跳跃结束输入 */
	virtual void DoJumpEnd() override;

	/** 处理冲刺输入 */
	virtual void DoDashStart();

	/** 处理开火输入 */
	virtual void DoFireStart();

	/** 当Actor结束生命周期时调用 */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 健康组件
	UPROPERTY(BlueprintReadOnly, Category = "Components")
	class UHealthComponent* HealthComponent;

	// 耐力组件
	UPROPERTY(BlueprintReadOnly, Category = "Components")
	class UStaminaComponent* StaminaComponent;

	// 冲刺组件
	UPROPERTY(BlueprintReadOnly, Category = "Components")
	class UDashComponent* DashComponent;

	// 健康值变化委托
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnHealthChanged OnHealthChanged;

	// 死亡委托
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnDeath OnDeath;

	// 耐力值变化委托
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnStaminaChanged OnStaminaChanged;

	// 健康值变化回调
	UFUNCTION()
	void OnHealthComponentChanged(float HealthPercent);

	// 死亡回调
	UFUNCTION()
	void OnHealthComponentDeath();

	// 耐力值变化回调
	UFUNCTION()
	void OnStaminaComponentChanged(float StaminaPercent);
};
