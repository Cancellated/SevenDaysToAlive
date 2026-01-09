// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "SDTAUpgradeUIModel.generated.h"

/**
 * 升级UI数据模型类
 * 
 * 核心功能：
 * 1. 存储升级相关的数据
 * 2. 提供升级效果的应用方法
 * 3. 管理升级的基本属性
 */
UCLASS(BlueprintType, Blueprintable)
class SEVENDAYSTOALIVE_API USDTAUpgradeUIModel : public UObject
{
	GENERATED_BODY()
	
public:
    /**
     * 升级ID
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade")
    FString UpgradeID;
    
    /**
     * 升级名称
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade")
    FText UpgradeName;
    
    /**
     * 升级描述
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade")
    FText UpgradeDescription;
    
    /**
     * 升级图标
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade")
    UTexture2D* UpgradeIcon;
    
    /**
     * 灵魂消耗
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade")
    int32 SoulCost;
    
    /**
     * 应用升级效果
     * @param TargetActor 目标Actor
     * @return 是否成功应用
     */
    UFUNCTION(BlueprintCallable, Category = "Upgrade")
    virtual bool ApplyUpgrade(AActor* TargetActor);
};
