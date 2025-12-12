// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SevenDaysToAliveCharacter.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "Delegates/DelegateCombinations.h"
#include "Net/UnrealNetwork.h" // 添加网络相关头文件
#include "TimerManager.h" // 添加定时器头文件
#include "SDTAPlayer.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealthChanged, float, HealthPercent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHealthLowWarning);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStaminaChanged, float, StaminaPercent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStaminaLowWarning);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeath);

UCLASS()
class SEVENDAYSTOALIVE_API ASDTAPlayer : public ASevenDaysToAliveCharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ASDTAPlayer();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// 网络复制相关
	// 重写GetLifetimeReplicatedProps方法来设置需要复制的属性
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	// 清理资源和委托
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

	// 角色统计属性
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Replicated, Category = "Character Stats")
	float MaxHealth;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Replicated, Category = "Character Stats")
	float MaxStamina;

	// 需要在网络上复制的健康值
	UPROPERTY(Replicated, BlueprintReadWrite, Category = "Character Stats")
	float Health;

	// 需要在网络上复制的能量值
	UPROPERTY(Replicated, BlueprintReadWrite, Category = "Character Stats")
	float Stamina;

	// 冲刺相关属性
	UPROPERTY(Replicated, BlueprintReadWrite, Category = "Character Dash")
	bool bIsDashing;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Character Dash")
	float DashSpeedMultiplier;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Character Dash")
	float DashStaminaCost;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Character Dash")
	float DashDuration;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Character Dash")
	float DashCooldown;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Character Dash")
	float LastDashTime;

	// 冲刺结束定时器句柄
	FTimerHandle FDashTimerHandle;

	// RPC方法示例
	// 服务器端执行的方法（客户端调用）
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SetHealth(float NewHealth);
	void Server_SetHealth_Implementation(float NewHealth);
	bool Server_SetHealth_Validate(float NewHealth);
	
	// 服务器端执行的方法（客户端调用）
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SetStamina(float NewStamina);
	void Server_SetStamina_Implementation(float NewStamina);
	bool Server_SetStamina_Validate(float NewStamina);

	// 冲刺相关RPC方法
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_StartDash();
	void Server_StartDash_Implementation();
	bool Server_StartDash_Validate();

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_EndDash();
	void Server_EndDash_Implementation();
	bool Server_EndDash_Validate();

	// 客户端执行的方法（服务器调用）
	UFUNCTION(Client, Reliable)
	void Client_UpdateHUD(float NewHealth, float NewStamina);
	void Client_UpdateHUD_Implementation(float NewHealth, float NewStamina);

	// 所有客户端执行的方法（服务器调用）
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlaySound(USoundBase* SoundToPlay);
	void Multicast_PlaySound_Implementation(USoundBase* SoundToPlay);

	// 网络角色检查辅助方法
	bool IsLocallyControlled() const;
	bool IsServer() const;

	// 角色状态检查方法
	UFUNCTION(BlueprintCallable, Category = "Character")
	bool IsAlive() const;

	// 生命值相关方法
	UFUNCTION(BlueprintCallable, Category = "Character")
	float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	UFUNCTION(BlueprintCallable, Category = "Character")
	void Die();

	UFUNCTION(BlueprintCallable, Category = "Character")
	void Heal(float HealAmount);

	// 能量值相关方法
	UFUNCTION(BlueprintCallable, Category = "Character")
	void ConsumeStamina(float Amount);

	// 标准化输入处理方法（通用接口，支持来自控制器或UI的输入）
	/** 处理瞄准输入 */
	virtual void DoAim(float Yaw, float Pitch);

	/** 处理移动输入 */
	virtual void DoMove(float Right, float Forward);

	/** 处理跳跃开始输入 */
	virtual void DoJumpStart();

	/** 处理跳跃结束输入 */
	virtual void DoJumpEnd();

	/** 处理冲刺开始输入 */
	virtual void DoDashStart();
	
	/** 处理冲刺结束输入 */
	virtual void DoDashEnd();

	// 事件委托声明
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnHealthChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnHealthLowWarning OnHealthLowWarning;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnStaminaChanged OnStaminaChanged;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnStaminaLowWarning OnStaminaLowWarning;

public:
	// 死亡事件
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnDeath OnDeath;
};