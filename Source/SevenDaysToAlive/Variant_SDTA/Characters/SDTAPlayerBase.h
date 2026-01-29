// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SevenDaysToAliveCharacter.h"
#include "Variant_SDTA/Components/HealthComponent.h"
#include "Variant_SDTA/Components/StaminaComponent.h"
#include "Variant_SDTA/Weapons/SDTAWeaponHolderInterface.h"
#include "SDTAPlayerBase.generated.h"

// 健康值变化委托
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealthChanged, float, HealthPercent);

// 死亡委托
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeath);

// 耐力值变化委托
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStaminaChanged, float, StaminaPercent);

// 武器动画实例类变化委托
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeaponAnimInstanceChanged, TSubclassOf<UAnimInstance>, FirstPersonClass, TSubclassOf<UAnimInstance>, ThirdPersonClass);

// 前向声明
class UDashComponent;

UCLASS()
class SEVENDAYSTOALIVE_API ASDTAPlayerBase : public ASevenDaysToAliveCharacter, public ISDTAWeaponHolder
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
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UHealthComponent* HealthComponent;

	// 耐力组件
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UStaminaComponent* StaminaComponent;

	// 冲刺组件
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UDashComponent* DashComponent;

	// 武器管理组件
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class USDTAWeaponManagerComponent* WeaponManagerComponent;

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

public:
	// ISDTAWeaponHolder 接口实现
	
	// 附加武器网格
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Weapon Holder")
	void AttachWeaponMeshes(ASDTAWeapon* Weapon);
	virtual void AttachWeaponMeshes_Implementation(ASDTAWeapon* Weapon) override;

	// 播放开火动画
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Weapon Holder")
	void PlayFiringMontage(UAnimMontage* Montage);
	virtual void PlayFiringMontage_Implementation(UAnimMontage* Montage) override;

	// 应用武器后坐力
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Weapon Holder")
	void AddWeaponRecoil(float RecoilAmount);
	virtual void AddWeaponRecoil_Implementation(float RecoilAmount) override;

	// 更新武器HUD
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Weapon Holder")
	void UpdateWeaponHUD(int32 CurrentAmmo, int32 MaxAmmo);
	virtual void UpdateWeaponHUD_Implementation(int32 CurrentAmmo, int32 MaxAmmo) override;

	// 获取武器瞄准目标位置
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Weapon Holder")
	FVector GetWeaponTargetLocation();
	virtual FVector GetWeaponTargetLocation_Implementation() override;

	// 添加武器类
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Weapon Holder")
	void AddWeaponClass(TSubclassOf<ASDTAWeapon> WeaponClass);
	virtual void AddWeaponClass_Implementation(TSubclassOf<ASDTAWeapon> WeaponClass) override;

	// 武器激活时调用
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Weapon Holder")
	void OnWeaponActivated(ASDTAWeapon* Weapon);
	virtual void OnWeaponActivated_Implementation(ASDTAWeapon* Weapon) override;

	// 武器停用时调用
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Weapon Holder")
	void OnWeaponDeactivated(ASDTAWeapon* Weapon);
	virtual void OnWeaponDeactivated_Implementation(ASDTAWeapon* Weapon) override;

	// 动画切换委托（网络同步）
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnWeaponAnimInstanceChanged OnWeaponAnimInstanceChanged;

	// 获取武器管理组件
	UFUNCTION(BlueprintPure, Category = "Weapon")
	USDTAWeaponManagerComponent* GetWeaponManagerComponent() const { return WeaponManagerComponent; }

protected:
	// 切换动画实例类（网络同步）
	UFUNCTION(NetMulticast, Reliable)
	void MulticastSwitchAnimInstanceClass(TSubclassOf<UAnimInstance> FirstPersonClass, TSubclassOf<UAnimInstance> ThirdPersonClass);

	// 播放蒙太奇动画（网络同步）
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayMontage(UAnimMontage* Montage);

	// 应用后坐力（网络同步）
	UFUNCTION(NetMulticast, Reliable)
	void MulticastAddRecoil(float RecoilAmount);

private:
	// 内部动画实例类切换方法
	void SwitchAnimInstanceClass(TSubclassOf<UAnimInstance> FirstPersonClass, TSubclassOf<UAnimInstance> ThirdPersonClass);
};
