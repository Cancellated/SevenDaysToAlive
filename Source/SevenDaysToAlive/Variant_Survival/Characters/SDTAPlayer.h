// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SevenDaysToAliveCharacter.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "Delegates/DelegateCombinations.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Variant_Survival/Components/HealthComponent.h"
#include "Variant_Survival/Components/StaminaSystemComponent.h"
#include "Variant_Survival/Weapons/SDTAWeaponHolder.h"
#include "SDTAPlayer.generated.h"

class ASDTAWeapon;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealthChanged, float, HealthPercent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHealthLowWarning);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStaminaChanged, float, StaminaPercent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStaminaLowWarning);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeath);

UCLASS()
class SEVENDAYSTOALIVE_API ASDTAPlayer : public ASevenDaysToAliveCharacter, public ISDTAWeaponHolder
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ASDTAPlayer();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	/** Dash Input Action */
	UPROPERTY(EditAnywhere, Category = "Input")
	class UInputAction* DashAction;

	/** Fire Input Action */
	UPROPERTY(EditAnywhere, Category = "Input")
	class UInputAction* FireAction;

	/** Reload Input Action */
	UPROPERTY(EditAnywhere, Category = "Input")
	class UInputAction* ReloadAction;

	/** Switch Weapon Input Action */
	UPROPERTY(EditAnywhere, Category = "Input")
	class UInputAction* SwitchWeaponAction;

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

	// 健康组件
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Character Components")
	class UHealthComponent* HealthComponent;

	// 耐力组件
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Character Components")
	class UStaminaSystemComponent* StaminaComponent;

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
	void Client_UpdateHUD();
	void Client_UpdateHUD_Implementation();

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



	// 获取当前移动速度
	UFUNCTION(BlueprintCallable, Category = "Character")
	float GetCurrentSpeed() const;

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

	/** 处理开火开始输入 */
	virtual void DoFireStart();

	/** 处理开火结束输入 */
	virtual void DoFireEnd();

	/** 处理重新装填输入 */
	virtual void DoReload();

	/** 处理武器切换输入 */
	virtual void DoSwitchWeapon();

	/** 切换到下一个武器 */
	UFUNCTION(BlueprintCallable, Category = "Weapons")
	void SwitchToNextWeapon();

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

protected:
	// 内部健康值变化处理方法
	UFUNCTION()
	void OnHealthChangedInternal(float HealthPercent);
	
	// 内部低健康值警告处理方法
	UFUNCTION()
	void OnHealthLowWarningInternal();
	
	// 内部死亡处理方法
	UFUNCTION()
	void OnDeathInternal();
	
	// 内部能量值变化处理方法
	UFUNCTION()
	void OnStaminaChangedInternal(float StaminaPercent);
	
	// 内部低能量值警告处理方法
	UFUNCTION()
	void OnStaminaLowWarningInternal();

	// 武器相关属性
	/** 玩家当前持有的武器 */
	UPROPERTY(Replicated, BlueprintReadWrite, Category = "Weapons")
	class ASDTAWeapon* CurrentWeapon;

	/** 玩家拥有的所有武器的数组 */
	UPROPERTY(Replicated, BlueprintReadWrite, Category = "Weapons")
	TArray<class ASDTAWeapon*> Weapons;

	/** 武器附着的插座名称 */
	UPROPERTY(EditDefaultsOnly, Category = "Weapons")
	FName WeaponAttachSocketName;

	/** 初始武器类，游戏开始时自动装备 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapons")
	TSubclassOf<class ASDTAWeapon> StartingWeaponClass;

	/** 是否在游戏开始时自动装备初始武器 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapons")
	bool bEquipStartingWeaponOnSpawn;

	//~Begin ISDTAWeaponHolder interface

	/** 将武器的网格附加到所有者 */
	virtual void AttachWeaponMeshes(ASDTAWeapon* Weapon) override;

	/** 播放武器的开火动画蒙太奇 */
	virtual void PlayFiringMontage(UAnimMontage* Montage) override;

	/** 向所有者应用武器后坐力 */
	virtual void AddWeaponRecoil(float Recoil) override;

	/** 使用当前弹药数量更新武器HUD */
	virtual void UpdateWeaponHUD(int32 CurrentAmmo, int32 MagazineSize) override;

	/** 计算并返回武器的瞄准位置 */
	virtual FVector GetWeaponTargetLocation() override;

	/** 给所有者一个此类的武器 */
	virtual void AddWeaponClass_Implementation(TSubclassOf<ASDTAWeapon> WeaponClass) override;

	/** 激活传入的武器 */
	virtual void OnWeaponActivated_Implementation(ASDTAWeapon* Weapon) override;

	/** 停用传入的武器 */
	virtual void OnWeaponDeactivated_Implementation(ASDTAWeapon* Weapon) override;

	//~End ISDTAWeaponHolder interface

	/** 在玩家的物品栏中查找指定类型的武器 */
	ASDTAWeapon* FindWeaponOfType(TSubclassOf<ASDTAWeapon> WeaponClass) const;
};
