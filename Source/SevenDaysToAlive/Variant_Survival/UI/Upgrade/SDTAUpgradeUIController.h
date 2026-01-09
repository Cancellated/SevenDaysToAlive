// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Variant_Survival/UI/Upgrade/SDTAUpgradeUIModel.h"
#include "SDTAUpgradeUIController.generated.h"

/**
 * 升级UI控制器类
 * 
 * 核心功能：
 * 1. 管理升级选项的生成和显示
 * 2. 处理用户的升级选择
 * 3. 连接升级模型和视图
 * 4. 管理升级UI的显示和隐藏
 */
UCLASS()
class SEVENDAYSTOALIVE_API USDTAUpgradeUIController : public UUserWidget
{
    GENERATED_BODY()
    
public:
    /**
     * 设置升级选项
     * @param UpgradeOptions 升级选项数组
     */
    UFUNCTION(BlueprintCallable, Category = "Upgrade UI")
    void SetUpgradeOptions(TArray<USDTAUpgradeUIModel*> UpgradeOptions);
    
    /**
     * 显示升级UI
     */
    UFUNCTION(BlueprintCallable, Category = "Upgrade UI")
    void ShowUpgradeUI();
    
    /**
     * 隐藏升级UI
     */
    UFUNCTION(BlueprintCallable, Category = "Upgrade UI")
    void HideUpgradeUI();
    
    /**
     * 选择升级选项
     * @param OptionIndex 选项索引
     */
    UFUNCTION(BlueprintCallable, Category = "Upgrade UI")
    void SelectUpgradeOption(int32 OptionIndex);
    
    /**
     * 获取当前升级选项
     * @return 升级选项数组
     */
    UFUNCTION(BlueprintCallable, Category = "Upgrade UI")
    TArray<USDTAUpgradeUIModel*> GetCurrentUpgradeOptions() const;
    
    /**
     * 设置灵魂总量
     * @param SoulFragments 灵魂数量
     */
    UFUNCTION(BlueprintCallable, Category = "Upgrade UI")
    void SetSoulFragments(int32 SoulFragments);
    
    /**
	 * 获取当前灵魂总量
	 * @return 灵魂数量
	 */
	UFUNCTION(BlueprintCallable, Category = "Upgrade UI")
	int32 GetSoulFragments() const;
    
    /**
     * 刷新升级选项
     */
    UFUNCTION(BlueprintCallable, Category = "Upgrade UI")
    void RefreshUpgradeOptions();
    
    /**
     * 检查是否有足够的灵魂
     * @param Cost 消耗的灵魂数量
     * @return 是否有足够的灵魂
     */
    UFUNCTION(BlueprintCallable, Category = "Upgrade UI")
    bool HasEnoughSouls(int32 Cost) const;
    
protected:
    // 升级选项数组
    TArray<USDTAUpgradeUIModel*> CurrentUpgradeOptions;
    
    // 目标玩家
    UPROPERTY()
    AActor* TargetPlayer;
    
    // 灵魂数量
    UPROPERTY()
    int32 SoulFragments;
    
    /**
     * 升级选择委托
     */
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUpgradeSelected, USDTAUpgradeUIModel*, SelectedUpgrade);
    
    /**
     * 当选择升级时触发
     */
    UPROPERTY(BlueprintAssignable, Category = "Upgrade UI")
    FOnUpgradeSelected OnUpgradeSelected;
    
    /**
     * 当升级UI关闭时触发
     */
    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnUpgradeUIClosed);
    
    /**
     * 当升级UI关闭时触发
     */
    UPROPERTY(BlueprintAssignable, Category = "Upgrade UI")
    FOnUpgradeUIClosed OnUpgradeUIClosed;
};
