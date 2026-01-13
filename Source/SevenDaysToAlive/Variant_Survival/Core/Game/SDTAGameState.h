// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "SDTAGameState.generated.h"

/**
 * 七日求生游戏状态类
 * 
 * 核心功能：
 * 1. 存储和管理游戏的全局状态
 * 2. 处理状态的网络同步
 * 3. 提供客户端访问游戏状态的接口
 */
UCLASS()
class SEVENDAYSTOALIVE_API ASDTAGameState : public AGameState
{
	GENERATED_BODY()

public:
	ASDTAGameState();

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	// 昼夜状态
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game State")
	bool bIsNight;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game State")
	float GameTime;

	// 昼夜管理器时间数据
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Day Night")
	float RemainingTime;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Day Night")
	float TimePercent;

	// 过渡状态
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Day Night")
	bool bIsTransitioning;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Day Night")
	bool bTransitionToNight;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Day Night")
	float TransitionProgress;

	// 昼夜管理器配置参数
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Day Night Configuration")
	float DayLightIntensity;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Day Night Configuration")
	float NightLightIntensity;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Day Night Configuration")
	FLinearColor DayLightColor;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Day Night Configuration")
	FLinearColor NightLightColor;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Day Night Configuration")
	FLinearColor DayAtmosphereColor;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Day Night Configuration")
	FLinearColor NightAtmosphereColor;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game State")
	int32 CurrentDay;

	// 敌人生成状态
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Enemy Spawn")
	int32 CurrentEnemyCount;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Enemy Spawn")
	int32 MaxEnemyCount;

	// 资源系统
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Resource System")
	int32 GlobalSoulFragments;

	// 升级系统
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Upgrade System")
	TArray<FName> GlobalUpgrades;

	// 多人游戏
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Multiplayer")
	TArray<class APlayerState*> ConnectedPlayers;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Multiplayer")
	int32 TeamScore;

	// 游戏状态
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game State")
	bool bGameStarted;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game State")
	bool bGameOver;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game State")
	bool bVictory;
};
