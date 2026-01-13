// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "SDTAPlayerState.generated.h"

/**
 * 七日求生玩家状态类
 * 
 * 核心功能：
 * 1. 存储和管理单个玩家的持久状态
 * 2. 处理玩家数据的网络同步
 * 3. 提供玩家特定数据的访问接口
 */
UCLASS()
class SEVENDAYSTOALIVE_API ASDTAPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	ASDTAPlayerState();

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	// 玩家个人灵魂碎片
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Player Resources")
	int32 PlayerSoulFragments;

	// 玩家个人升级
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Player Upgrades")
	TArray<FName> PersonalUpgrades;

	// 玩家统计数据
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Player Stats")
	int32 Kills;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Player Stats")
	int32 Deaths;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Player Stats")
	int32 WavesSurvived;

	// 玩家状态方法
	UFUNCTION(BlueprintCallable, Category = "Player Resources")
	void AddSoulFragments(int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "Player Upgrades")
	void AddPersonalUpgrade(const FName& UpgradeName);

	UFUNCTION(BlueprintCallable, Category = "Player Stats")
	void IncrementKills();

	UFUNCTION(BlueprintCallable, Category = "Player Stats")
	void IncrementDeaths();

	UFUNCTION(BlueprintCallable, Category = "Player Stats")
	void IncrementWavesSurvived();
};
