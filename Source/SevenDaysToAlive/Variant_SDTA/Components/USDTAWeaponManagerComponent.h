// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Character.h"
#include "Animation/AnimInstance.h"
#include "../Weapons/SDTAWeapon.h"
#include "../Weapons/SDTAWeaponHolderInterface.h"
#include "../Characters/SDTAPlayerBase.h"
#include "USDTAWeaponManagerComponent.generated.h"

// 前向声明
class UEnhancedInputComponent;

/**
 * 武器管理组件，负责管理玩家的武器系统
 * 包括武器切换、开火控制、输入绑定等功能
 */
UCLASS(ClassGroup=(Custom), Blueprintable, BlueprintType, meta=(BlueprintSpawnableComponent))
class SEVENDAYSTOALIVE_API USDTAWeaponManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	USDTAWeaponManagerComponent();

protected:
	// 拥有的武器列表
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Weapon Manager")
	TArray<ASDTAWeapon*> OwnedWeapons;

	// 当前装备的武器
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Weapon Manager")
	ASDTAWeapon* CurrentWeapon;

	// 第一人称武器装备socket名称
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Manager")
	FName FirstPersonWeaponSocket;

	// 第三人称武器装备socket名称
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Manager")
	FName ThirdPersonWeaponSocket;

	// 武器瞄准最大距离
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Manager")
	float MaxAimDistance;

	// 初始武器类（玩家出生时自动装备）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Manager")
	TSubclassOf<ASDTAWeapon> InitialWeaponClass;

	// 是否正在开火
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Weapon Manager")
	bool bIsFiring;

	// 是否正在装填
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Weapon Manager")
	bool bIsReloading;

	// 是否正在切换武器
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Weapon Manager")
	bool bIsSwitchingWeapon;

public:
	// 添加武器类
	UFUNCTION(BlueprintCallable, Category = "Weapon Manager")
	void AddWeaponClass(TSubclassOf<ASDTAWeapon> WeaponClass);

	// 切换到下一个武器
	UFUNCTION(BlueprintCallable, Category = "Weapon Manager")
	void SwitchToNextWeapon();

	// 切换到上一个武器
	UFUNCTION(BlueprintCallable, Category = "Weapon Manager")
	void SwitchToPreviousWeapon();

	// 直接切换到指定索引的武器
	UFUNCTION(BlueprintCallable, Category = "Weapon Manager")
	void SwitchToWeapon(int32 WeaponIndex);

	// 开始开火
	UFUNCTION(BlueprintCallable, Category = "Weapon Manager")
	void StartFire();

	// 停止开火
	UFUNCTION(BlueprintCallable, Category = "Weapon Manager")
	void StopFire();

	// 装填弹药
	UFUNCTION(BlueprintCallable, Category = "Weapon Manager")
	void Reload();

	// 设置输入绑定
	UFUNCTION(BlueprintCallable, Category = "Weapon Manager")
	void SetupInputBindings(UEnhancedInputComponent* InputComponent);

	// 装备初始武器
	UFUNCTION(BlueprintCallable, Category = "Weapon Manager")
	void EquipInitialWeapon();

	// 附加武器网格到角色
	UFUNCTION(BlueprintCallable, Category = "Weapon Manager")
	void AttachWeaponMeshes(ASDTAWeapon* Weapon);

	// 获取当前武器
	UFUNCTION(BlueprintPure, Category = "Weapon Manager")
	ASDTAWeapon* GetCurrentWeapon() const { return CurrentWeapon; }

	// 获取拥有的武器列表
	UFUNCTION(BlueprintPure, Category = "Weapon Manager")
	const TArray<ASDTAWeapon*>& GetOwnedWeapons() const { return OwnedWeapons; }

	// 获取武器数量
	UFUNCTION(BlueprintPure, Category = "Weapon Manager")
	int32 GetWeaponCount() const { return OwnedWeapons.Num(); }

	// 检查是否正在开火
	UFUNCTION(BlueprintPure, Category = "Weapon Manager")
	bool IsFiring() const { return bIsFiring; }

	// 检查是否正在装填
	UFUNCTION(BlueprintPure, Category = "Weapon Manager")
	bool IsReloading() const { return bIsReloading; }

	// 切换武器动画蓝图
	UFUNCTION(BlueprintCallable, Category = "Weapon Manager")
	void SwitchWeaponAnimInstance(ASDTAWeapon* Weapon);

	// 恢复默认动画蓝图
	UFUNCTION(BlueprintCallable, Category = "Weapon Manager")
	void RestoreDefaultAnimInstance();

	// 检查是否正在切换武器
	UFUNCTION(BlueprintPure, Category = "Weapon Manager")
	bool IsSwitchingWeapon() const { return bIsSwitchingWeapon; }

protected:
	// 服务器端添加武器类
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerAddWeaponClass(TSubclassOf<ASDTAWeapon> WeaponClass);

	// 服务器端切换到下一个武器
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSwitchToNextWeapon();

	// 服务器端切换到上一个武器
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSwitchToPreviousWeapon();

	// 服务器端直接切换到指定武器
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSwitchToWeapon(int32 WeaponIndex);

	// 服务器端开始开火
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerStartFire();

	// 服务器端停止开火
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerStopFire();

	// 服务器端装填弹药
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerReload();

	// 当组件开始播放时调用
	virtual void BeginPlay() override;

	// 网络同步
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	// 内部切换武器实现
	void InternalSwitchWeapon(ASDTAWeapon* NewWeapon);

	// 处理开火输入
	void HandleFireInput();

	// 处理装填输入
	void HandleReloadInput();

	// 处理武器切换输入
	void HandleSwitchWeaponInput();

	// 换弹完成回调
	UFUNCTION()
	void OnReloadCompleted();
};
