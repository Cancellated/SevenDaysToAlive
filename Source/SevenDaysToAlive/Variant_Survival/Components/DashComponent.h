// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Variant_Survival/Components/StaminaSystemComponent.h"
#include "DashComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SEVENDAYSTOALIVE_API UDashComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UDashComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

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

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Character Dash")
	float OriginalMaxWalkSpeed;

	// 冲刺结束定时器句柄
	FTimerHandle FDashTimerHandle;

	// 耐力组件引用
	UPROPERTY()
	class UStaminaSystemComponent* StaminaComponent;

	// 网络复制相关
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 冲刺相关方法
	/** 开始冲刺 */
	UFUNCTION(BlueprintCallable, Category = "Dash")
	void StartDash();

	/** 结束冲刺 */
	UFUNCTION(BlueprintCallable, Category = "Dash")
	void EndDash();

	/** 检查是否可以冲刺 */
	UFUNCTION(BlueprintCallable, Category = "Dash")
	bool CanDash() const;

	// 服务器端RPC方法
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_StartDash();
	void Server_StartDash_Implementation();
	bool Server_StartDash_Validate();

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_EndDash();
	void Server_EndDash_Implementation();
	bool Server_EndDash_Validate();

	// 初始化组件
	UFUNCTION(BlueprintCallable, Category = "Dash")
	void SetStaminaComponent(class UStaminaSystemComponent* InStaminaComponent);

private:
	// 获取拥有者的角色移动组件
	class UCharacterMovementComponent* GetCharacterMovement() const;

	// 获取拥有者
	class APawn* GetOwnerPawn() const;

	// 检查是否在服务器上
	bool IsServer() const;
};
