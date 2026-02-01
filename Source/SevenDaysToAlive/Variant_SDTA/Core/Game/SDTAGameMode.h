// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "Variant_SDTA/Core/Game/SDTAGameState.h"
#include "Variant_SDTA/Core/Game/SDTAPlayerState.h"
#include "Variant_SDTA/Enemies/AI/EnemyBase.h"
#include "Variant_SDTA/Core/Game/DayNight/SDTADayNightManager.h"
#include "SDTAGameMode.generated.h"

/**
 * 七日求生游戏模式类
 * 
 * 核心功能：
 * 1. 管理游戏的整体流程和状态
 * 2. 控制昼夜循环系统
 * 3. 管理敌人的生成和对象池
 * 4. 处理UI更新和事件广播
 * 
 * 使用说明：
 * - 作为游戏的核心控制器，负责协调各个系统
 * - 提供对对象池管理器的访问接口
 * - 管理游戏内的时间流逝和状态变化
 */
UCLASS()
class SEVENDAYSTOALIVE_API ASDTAGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	ASDTAGameMode();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	// 获取GameState
	UFUNCTION(BlueprintCallable, Category = "Game State")
	ASDTAGameState* GetSDTAGameState() const;

#pragma region 昼夜循环系统
public:
	// 昼夜管理器
	UPROPERTY(BlueprintReadOnly, Category = "Day Night System")
	USDTADayNightManager* DayNightManager; // 昼夜管理器实例
	
	// 获取昼夜管理器实例
	UFUNCTION(BlueprintCallable, Category = "Day Night System")
	USDTADayNightManager* GetDayNightManager() const;
	
	// 昼夜状态获取方法（保持向后兼容）
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game State")
	bool bIsNight; // 是否为夜晚阶段
	
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game State")
	float GameTime; // 游戏内时间（秒）
	
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game State")
	int32 CurrentDay; // 当前天数（1-7）
	
	// 昼夜管理器事件回调
	UFUNCTION()
	void OnDayNightStateChanged(bool bIsNowNight);
	
	UFUNCTION()
	void OnTimeUpdated(float RemainingTime, float TimePercent);

	// 昼夜系统配置（直接暴露到GameMode以便在蓝图中配置）
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Day Night System|Time Configuration")
	float DayDuration; // 白天持续时间（秒）
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Day Night System|Time Configuration")
	float NightDuration; // 夜晚持续时间（秒）
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Day Night System|Transition")
	float TransitionDuration; // 昼夜过渡持续时间（秒）
	
	// 光源配置
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Day Night System|Light Configuration")
	float DayLightIntensity; // 白天光源亮度
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Day Night System|Light Configuration")
	float NightLightIntensity; // 夜晚光源亮度
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Day Night System|Light Configuration")
	FLinearColor DayLightColor; // 白天光源颜色
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Day Night System|Light Configuration")
	FLinearColor NightLightColor; // 夜晚光源颜色
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Day Night System|Light Configuration")
	FName LightTag; // 光源标签
	
	// 大气配置
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Day Night System|Atmosphere Configuration")
	FLinearColor DayAtmosphereColor; // 白天大气瑞利散射颜色
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Day Night System|Atmosphere Configuration")
	FLinearColor NightAtmosphereColor; // 夜晚大气瑞利散射颜色
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Day Night System|Atmosphere Configuration")
	FName AtmosphereTag; // 大气标签
#pragma endregion

#pragma region 敌人生成系统
public:
	// 敌人生成管理
	UPROPERTY(EditDefaultsOnly, Category = "Enemy Spawn")
	TSubclassOf<class AEnemyBase> EnemyClass; // 敌人类型
	
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Enemy Spawn")
	int32 CurrentEnemyCount; // 当前场景中敌人数量
	
	UPROPERTY(EditDefaultsOnly, Replicated, BlueprintReadWrite, Category = "Enemy Spawn")
	int32 MaxEnemyCount; // 最大敌人数量
	
	// 波次生成配置
	UPROPERTY(EditDefaultsOnly, Category = "Enemy Spawn")
	TArray<int32> WaveSizes; // 每天敌人生成数量
	
	UPROPERTY(EditDefaultsOnly, Category = "Enemy Spawn")
	TArray<float> WaveIntervals; // 敌人生成间隔
	
	// 敌人生成逻辑
	void SpawnEnemyWave();
	void StartEnemySpawning();
	void StopEnemySpawning();
	
	// 敌人管理
	void OnEnemyDestroyed(class AEnemyBase* DestroyedEnemy);
	void CleanupDeadEnemies();
	void ClearAllEnemies(); // 清理所有敌人（白天开始时调用）
#pragma endregion

#pragma region 资源与升级系统
public:
	// 灵魂碎片管理
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Resource System")
	int32 SoulFragments; // 可用灵魂碎片（白天升级用）
	
	// 升级系统
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Upgrade System")
	TArray<FName> PlayerUpgrades; // 玩家已选择的升级
	
	// 资源收集
	void CollectSoulFragments(int32 Amount);
	void DistributeSoulFragments(); // 白天开始时分配碎片
	
	// 升级逻辑
	void ApplyPlayerUpgrade(const FName& UpgradeName);
	bool CanAffordUpgrade(int32 Cost) const;
#pragma endregion

#pragma region 多人游戏系统
public:
	// 玩家管理
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Multiplayer")
	TArray<class APlayerState*> ConnectedPlayers;
	
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Multiplayer")
	int32 MaxPlayers; // 最大玩家数量
	
	// 分数系统
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Multiplayer")
	int32 TeamScore; // 团队总分
	
	// 合作机制
	void OnPlayerJoined(class APlayerController* NewPlayer);
	void OnPlayerLeft(class APlayerController* LeavingPlayer);
	void UpdateTeamScore(int32 Points);
	
	// 网络同步验证（主机权威）
	UFUNCTION(Server, Reliable)
	void ServerCollectFragments(class APlayerState* Player, int32 Amount);
	
	UFUNCTION(Server, Reliable)
	void ServerApplyUpgrade(class APlayerState* Player, const FName& UpgradeName);
#pragma endregion

#pragma region 游戏状态管理
public:
	// 游戏进度
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game State")
	bool bGameStarted; // 游戏是否开始
	
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game State")
	bool bGameOver; // 游戏是否结束
	
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game State")
	bool bVictory; // 是否胜利
	
	// 游戏流程控制
	void StartGame();
	void EndGame(bool bWin);
	void CheckWinCondition();
	void CheckLoseCondition();
	
	// 存档系统（预留）
	void SaveGameProgress();
	void LoadGameProgress();
	
	/**
	 * 获取对象池管理器实例
	 * 
	 * 功能：提供对游戏模式内部对象池管理器的安全访问接口
	 * 用途：用于获取和操作对象池，实现敌人等游戏对象的高效回收和复用
	 * 
	 * @return 返回对象池管理器实例，如果未初始化则返回nullptr
	 */
	UFUNCTION(BlueprintCallable, Category = "Game Systems")
	USDTAPoolManager* GetPoolManager() const;
#pragma endregion

#pragma region UI与事件系统
public:
	// UI更新方法
	void UpdateGameUI();
	void BroadcastGameState();
#pragma endregion



private:
	// 对象池管理器
	UPROPERTY()
	class USDTAPoolManager* PoolManager;
	
	// 内部计时器
	FTimerHandle EnemySpawnTimer;
	FTimerHandle DayNightTimer;
	
	// 敌人列表
	TArray<class AEnemyBase*> ActiveEnemies;
	
	// 日志输出控制
	float LastLogTime; // 上次输出游戏状态日志的时间
};