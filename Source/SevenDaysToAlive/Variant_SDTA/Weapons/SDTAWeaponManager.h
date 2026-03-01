// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
// 前置声明
class ASDTAPlayerState;
class ASDTAWeapon;
class ISDTAWeaponHolder;
#include "SDTAWeaponTypes.h"
#include "SDTAWeaponManager.generated.h"

// 武器状态更新委托
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeaponAmmoChanged, int32, CurrentAmmo, int32, MaxAmmo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCurrentWeaponChanged, const FName&, WeaponName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponFireStateChanged, bool, bIsFiring);

/**
 * 武器管理器数据结构
 */
USTRUCT(BlueprintType)
struct FWeaponInventoryData
{
	GENERATED_BODY()

	// 武器名称
	UPROPERTY(BlueprintReadWrite, Category = "Weapon Inventory")
	FName WeaponName;

	// 当前弹药数量
	UPROPERTY(BlueprintReadWrite, Category = "Weapon Inventory")
	int32 CurrentAmmo;

	// 后备弹药数量
	UPROPERTY(BlueprintReadWrite, Category = "Weapon Inventory")
	int32 ReserveAmmo;

	// 武器数据
	UPROPERTY(BlueprintReadWrite, Category = "Weapon Inventory")
	FSDTAWeaponTableRow WeaponData;
};

/**
 * 武器管理器类
 * 
 * 核心功能：
 * 1. 管理玩家的武器状态、弹药和开火控制
 * 2. 处理武器数据的网络同步
 * 3. 提供武器系统的公共接口
 */
UCLASS()
class SEVENDAYSTOALIVE_API USDTAWeaponManager : public UObject
{
	GENERATED_BODY()

public:
	USDTAWeaponManager();

	/**
	 * 初始化武器管理器
	 * @param InPlayerState 玩家状态引用
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon Manager")
	void Initialize(ASDTAPlayerState* InPlayerState);

	/**
	 * 设置武器数据表格
	 * @param InWeaponDataTable 武器数据表格引用
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon Manager")
	void SetWeaponDataTable(UDataTable* InWeaponDataTable);

	/**
	 * 获取武器数据表格
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon Manager")
	UDataTable* GetWeaponDataTable() const;

	/**
	 * 通过武器名称获取武器数据
	 * @param WeaponName 武器名称
	 * @param OutWeaponData 输出的武器数据
	 * @return 是否找到武器数据
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon Manager")
	bool GetWeaponDataByName(const FName& WeaponName, FSDTAWeaponTableRow& OutWeaponData) const;

	/**
	 * 更新武器管理器状态
	 * @param DeltaTime 帧间隔时间
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon Manager")
	void Tick(float DeltaTime);

	/**
	 * 获取当前武器
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon Manager")
	FName GetCurrentWeapon() const;

	/**
	 * 切换武器
	 * @param WeaponName 要切换的武器名称
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon Manager")
	void SwitchWeapon(const FName& WeaponName);

	/**
	 * 添加武器到库存
	 * @param WeaponName 武器名称
	 * @param InitialAmmo 初始弹药数量
	 * @param ReserveAmmo 后备弹药数量
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon Manager")
	void AddWeapon(const FName& WeaponName, int32 InitialAmmo = 0, int32 ReserveAmmo = 0);

	/**
	 * 移除武器从库存
	 * @param WeaponName 武器名称
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon Manager")
	void RemoveWeapon(const FName& WeaponName);

	/**
	 * 获取武器库存
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon Manager")
	TArray<FWeaponInventoryData> GetWeaponInventory() const;

public:
	/**
	 * 开始开火
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon Manager")
	void StartFiring();

	/**
	 * 停止开火
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon Manager")
	void StopFiring();

	/**
	 * 单次开火
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon Manager")
	void FireOnce();

	/**
	 * 检查是否可以开火
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon Manager")
	bool CanFire() const;

	/**
	 * 检查是否正在开火
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon Manager")
	bool IsFiring() const;

	/**
	 * 重新装填弹药
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon Manager")
	void ReloadCurrentWeapon();

	/**
	 * 检查弹药是否充足
	 * @param WeaponName 武器名称，默认为当前武器
	 * @param RequiredAmmo 需要的弹药数量
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon Manager")
	bool HasEnoughAmmo(const FName& WeaponName = NAME_None, int32 RequiredAmmo = 1) const;

	/**
	 * 获取当前武器的当前弹药数量
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon Manager")
	int32 GetCurrentWeaponAmmo() const;

	/**
	 * 获取当前武器的弹匣容量
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon Manager")
	int32 GetCurrentWeaponMagazineSize() const;

	/**
	 * 获取当前武器的伤害
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon Manager")
	float GetCurrentWeaponDamage() const;

	/**
	 * 获取当前武器的射程
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon Manager")
	float GetCurrentWeaponRange() const;

	/**
	 * 获取当前武器的名称
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon Manager")
	FString GetCurrentWeaponName() const;

	/**
	 * 获取当前武器的数据行
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon Manager")
	bool GetCurrentWeaponDataRow(FSDTAWeaponTableRow& OutWeaponDataRow) const;

protected:
	/**
	 * 辅助函数：通过武器名称查找武器数据
	 */
	FWeaponInventoryData* FindWeaponData(const FName& WeaponName);

	/**
	 * 辅助函数：通过武器名称查找武器数据（const版本）
	 */
	const FWeaponInventoryData* FindWeaponData(const FName& WeaponName) const;

	/**
	 * 检查是否拥有权限
	 */
	bool HasAuthority() const;

	/**
	 * 检查是否是本地角色
	 */
	bool IsLocalPlayer() const;

	/**
	 * 更新武器UI
	 */
	void UpdateWeaponUI();

	/**
	 * 获取武器持有者接口
	 */
	TScriptInterface<ISDTAWeaponHolder> GetWeaponHolder() const;

	// 玩家状态引用
	UPROPERTY()
	ASDTAPlayerState* PlayerState;

	// 世界引用
	UPROPERTY()
	UWorld* World;

	// 武器数据表格
	UPROPERTY(BlueprintReadOnly, Category = "Weapon Manager")
	UDataTable* WeaponDataTable;

	// 当前武器名称
	UPROPERTY(ReplicatedUsing = OnRep_CurrentWeapon, BlueprintReadOnly, Category = "Weapon Manager")
	FName CurrentWeaponName;

	// 武器库存（使用TArray代替TMap以支持网络复制）
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Weapon Manager")
	TArray<FWeaponInventoryData> WeaponInventory;

	// 开火状态
	UPROPERTY(ReplicatedUsing = OnRep_IsFiring, BlueprintReadOnly, Category = "Weapon Manager")
	bool bIsFiring;

	// 上次开火时间
	UPROPERTY(BlueprintReadOnly, Category = "Weapon Manager")
	float LastFireTime;

	// 后坐力值
	UPROPERTY(BlueprintReadOnly, Category = "Weapon Manager")
	float Recoil;

	// 后坐力衰减速度
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon Manager")
	float RecoilDecayRate;

	// 当前武器Actor
	UPROPERTY(BlueprintReadOnly, Category = "Weapon Manager")
	ASDTAWeapon* CurrentWeaponActor;

	// 委托：武器弹药变化时触发
	UPROPERTY(BlueprintAssignable, Category = "Weapon Manager Events")
	FOnWeaponAmmoChanged OnWeaponAmmoChanged;

	// 委托：当前武器变化时触发
	UPROPERTY(BlueprintAssignable, Category = "Weapon Manager Events")
	FOnCurrentWeaponChanged OnCurrentWeaponChanged;

	// 委托：武器开火状态变化时触发
	UPROPERTY(BlueprintAssignable, Category = "Weapon Manager Events")
	FOnWeaponFireStateChanged OnWeaponFireStateChanged;

	/**
	 * 处理开火逻辑
	 */
	void ProcessFire();

	/**
	 * 更新冷却时间
	 * @param DeltaTime 帧间隔时间
	 */
	void UpdateCooldown(float DeltaTime);

	/**
	 * 更新后坐力衰减
	 * @param DeltaTime 帧间隔时间
	 */
	void UpdateRecoilDecay(float DeltaTime);

	/**
	 * 加载武器数据
	 */
	void LoadWeaponData();

	/**
	 * 重置武器状态
	 */
	void ResetState();

	/**
	 * 检查是否是全自动武器
	 */
	bool IsFullAuto() const;

	/**
	 * 当前武器的射速
	 */
	float GetCurrentFireRate() const;

	/**
	 * 当前武器的弹匣容量
	 */
	int32 GetCurrentMagazineSize() const;

	/**
	 * 服务器端开始开火
	 */
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerStartFire();

	/**
	 * 服务器端停止开火
	 */
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerStopFire();

	/**
	 * 服务器端单次开火
	 */
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerFireOnce();

	/**
	 * 服务器端重新装填
	 */
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerReload();

	/**
	 * 服务器端切换武器
	 */
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSwitchWeapon(const FName& WeaponName);

	/**
	 * 服务器端添加武器
	 */
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerAddWeapon(const FName& WeaponName, int32 InitialAmmo, int32 ReserveAmmo);

	/**
	 * 服务器端移除武器
	 */
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerRemoveWeapon(const FName& WeaponName);

	/**
	 * 当前武器变更的RepNotify
	 */
	UFUNCTION()
	void OnRep_CurrentWeapon();

	/**
	 * 开火状态变更的RepNotify
	 */
	UFUNCTION()
	void OnRep_IsFiring();

	// 网络同步
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};