// Fill out your copyright notice in the Description page of Project Settings.

#include "Variant_SDTA\Core\Game\SDTAGameState.h"
#include "Net/UnrealNetwork.h"

ASDTAGameState::ASDTAGameState()
{
	// 初始化默认值
	bIsNight = false;	// 初始为白天
	GameTime = 0.0f;	// 初始游戏时间为0
	RemainingTime = 0.0f;	// 初始剩余时间为0
	TimePercent = 0.0f;	// 初始时间百分比为0
	
	// 初始化过渡状态
	bIsTransitioning = false;	// 初始不在过渡中
	bTransitionToNight = false;	// 初始不向夜晚过渡
	TransitionProgress = 0.0f;	// 初始过渡进度为0
	
	CurrentDay = 1;	// 初始为第1天
	CurrentEnemyCount = 0;	// 初始敌人数量为0
	MaxEnemyCount = 20;	// 最大敌人数量为20
	GlobalSoulFragments = 0;	// 初始灵魂碎片数量为0
	TeamScore = 0;	// 初始团队分数为0
	bGameStarted = false;	// 初始游戏未开始
	bGameOver = false;	// 初始游戏未结束
	bVictory = false;	// 初始未胜利

	// 初始化昼夜配置参数
	DayLightIntensity = 3.0f;	// 白天光照强度
	NightLightIntensity = 0.5f;	// 夜晚光照强度
	DayLightColor = FLinearColor(1.0f, 0.95f, 0.9f);	// 白天暖白色
	NightLightColor = FLinearColor(0.7f, 0.8f, 1.0f);	// 夜晚蓝白色
	DayAtmosphereColor = FLinearColor(0.5f, 0.7f, 1.0f);	// 白天蓝色
	NightAtmosphereColor = FLinearColor(0.1f, 0.1f, 0.3f);	// 夜晚深蓝色
}

void ASDTAGameState::BeginPlay()
{
	Super::BeginPlay();
}

void ASDTAGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 复制游戏状态属性
	DOREPLIFETIME(ASDTAGameState, bIsNight);
	DOREPLIFETIME(ASDTAGameState, GameTime);
	DOREPLIFETIME(ASDTAGameState, RemainingTime);
	DOREPLIFETIME(ASDTAGameState, TimePercent);
	DOREPLIFETIME(ASDTAGameState, bIsTransitioning);
	DOREPLIFETIME(ASDTAGameState, bTransitionToNight);
	DOREPLIFETIME(ASDTAGameState, TransitionProgress);
	DOREPLIFETIME(ASDTAGameState, DayLightIntensity);
	DOREPLIFETIME(ASDTAGameState, NightLightIntensity);
	DOREPLIFETIME(ASDTAGameState, DayLightColor);
	DOREPLIFETIME(ASDTAGameState, NightLightColor);
	DOREPLIFETIME(ASDTAGameState, DayAtmosphereColor);
	DOREPLIFETIME(ASDTAGameState, NightAtmosphereColor);
	DOREPLIFETIME(ASDTAGameState, CurrentDay);
	DOREPLIFETIME(ASDTAGameState, CurrentEnemyCount);
	DOREPLIFETIME(ASDTAGameState, MaxEnemyCount);
	DOREPLIFETIME(ASDTAGameState, GlobalSoulFragments);
	DOREPLIFETIME(ASDTAGameState, GlobalUpgrades);
	DOREPLIFETIME(ASDTAGameState, ConnectedPlayers);
	DOREPLIFETIME(ASDTAGameState, TeamScore);
	DOREPLIFETIME(ASDTAGameState, bGameStarted);
	DOREPLIFETIME(ASDTAGameState, bGameOver);
	DOREPLIFETIME(ASDTAGameState, bVictory);
}
