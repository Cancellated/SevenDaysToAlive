// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Net/UnrealNetwork.h"
#include "Variant_Survival/Weapons/SDTAWeaponHolder.h"
#include "WeaponComponent.generated.h"

class ASDTAWeapon;
class ASDTAPlayer;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class SEVENDAYSTOALIVE_API UWeaponComponent : public UActorComponent, public ISDTAWeaponHolder
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UWeaponComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// 网络复制相关
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** 初始化武器组件，设置持有武器的角色 */
	UFUNCTION(BlueprintCallable, Category = "Weapon Component")
	void InitializeWeaponHolder(ASDTAPlayer* InWeaponHolder);

	/** 获取武器持有者 */
	UFUNCTION(BlueprintCallable, Category = "Weapon Component")
	ASDTAPlayer* GetWeaponHolder() const;

	// 武器输入处理方法
	/** 处理开火开始输入 */
	UFUNCTION(BlueprintCallable, Category = "Weapon Input")
	void DoFireStart();

	/** 处理开火结束输入 */
	UFUNCTION(BlueprintCallable, Category = "Weapon Input")
	void DoFireEnd();

	/** 处理重新装填输入 */
	UFUNCTION(BlueprintCallable, Category = "Weapon Input")
	void DoReload();

	/** 处理武器切换输入 */
	UFUNCTION(BlueprintCallable, Category = "Weapon Input")
	void DoSwitchWeapon();

	/** 切换到下一个武器 */
	UFUNCTION(BlueprintCallable, Category = "Weapon Management")
	void SwitchToNextWeapon();

	/** 开始开火 */
	UFUNCTION()
	void StartFiring();

	/** 停止开火 */
	UFUNCTION()
	void StopFiring();

	/** 重新装填 */
	UFUNCTION()
	void Reload();

	// 武器管理方法
	/** 在武器持有者的物品栏中查找指定类型的武器 */
	UFUNCTION(BlueprintCallable, Category = "Weapon Management")
	ASDTAWeapon* FindWeaponOfType(TSubclassOf<ASDTAWeapon> WeaponClass) const;

	/** 获取当前武器 */
	UFUNCTION(BlueprintCallable, Category = "Weapon Management")
	ASDTAWeapon* GetCurrentWeapon() const;

	/** 获取所有武器 */
	UFUNCTION(BlueprintCallable, Category = "Weapon Management")
	TArray<ASDTAWeapon*> GetWeapons() const;

	// 清理武器资源
	void CleanupWeapons();

protected:
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

	/** 武器持有者 */
	UPROPERTY(BlueprintReadWrite, Category = "Weapon Component")
	ASDTAPlayer* WeaponHolder;

	//~Begin ISDTAWeaponHolder interface
public:
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
};
