// 七日求生游戏模式实现
// 主机权威制的昼夜循环生存游戏

/**
 * SDTAGameMode.cpp - 游戏模式实现文件
 * 
 * 核心功能：
 * 1. 管理游戏的整体流程和状态
 * 2. 实现昼夜循环系统
 * 3. 管理敌人的生成和回收
 * 4. 处理UI更新和事件广播
 * 5. 管理对象池系统
 * 
 * 设计要点：
 * - 采用主机权威制设计，确保游戏逻辑的一致性
 * - 集成对象池管理器，实现敌人等游戏对象的高效复用
 * - 提供安全的访问接口，避免直接访问私有成员变量
 * 
 * 使用说明：
 * - 作为游戏的核心控制器，协调各个系统的工作
 * - 通过GetPoolManager()方法提供对象池访问接口
 * - 管理游戏内的时间流逝和昼夜变化
 */

#include "Variant_Survival/Core/Game/SDTAGameMode.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

// 包含对象池管理器头文件
#include "Variant_Survival/Core/Pool/SDTAPoolManager.h"

// 包含HUD头文件
#include "Variant_Survival/UI/SDTAPlayerHUD.h"
#include "Variant_Survival/Controller/SDTAPlayerController.h"
#include "Blueprint/UserWidget.h"
#include "Components/LightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SpotLightComponent.h"
// 包含天空大气组件头文件
#include "Components/SkyAtmosphereComponent.h"
#include "Components/ExponentialHeightFogComponent.h"

#pragma region 构造函数和基础方法
ASDTAGameMode::ASDTAGameMode()
{
	// 设置默认值
	GameTime = 0.0f;
	CurrentDay = 1;
	bIsNight = false;
	
	DayDuration = 120.0f; // 2分钟白天（用于升级准备）
	NightDuration = 300.0f; // 5分钟夜晚（用于战斗）
	
	CurrentEnemyCount = 0;
	MaxEnemyCount = 20;
	
	SoulFragments = 0;
	
	MaxPlayers = 4;
	TeamScore = 0;
	
	bGameStarted = false;
	bGameOver = false;
	bVictory = false;
	
	// 初始化光源配置
	DayLightIntensity = 1.0f;
	NightLightIntensity = 0.3f;
	DayLightColor = FLinearColor::White;
	NightLightColor = FLinearColor(0.7f, 0.7f, 1.0f); // 夜晚偏蓝色
	LightTag = FName("WorldLight");

	// 初始化大气配置
	DayAtmosphereColor = FLinearColor(0.5f, 0.7f, 1.0f); // 白天蓝色天空
	NightAtmosphereColor = FLinearColor(0.1f, 0.1f, 0.3f); // 夜晚深蓝色天空
	AtmosphereTag = FName("WorldAtmosphere");
	
	// 初始化渐变系统
	TransitionDuration = 5.0f; // 默认5秒过渡时间
	bIsTransitioning = false;
	TransitionProgress = 0.0f;
	bTransitionToNight = false;
}

void ASDTAGameMode::BeginPlay()
{
	Super::BeginPlay();
	
	// 初始化对象池管理器
	PoolManager = NewObject<USDTAPoolManager>(this, USDTAPoolManager::StaticClass());
	if (PoolManager)
	{
		PoolManager->Initialize(GetWorld());
	}
	
	// 游戏开始逻辑
	StartGame();
}

void ASDTAGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if (bGameStarted && !bGameOver)
	{
		// 检查是否正在过渡
		if (bIsTransitioning)
		{
			// 更新过渡进度
			TransitionProgress += DeltaTime / TransitionDuration;
			TransitionProgress = FMath::Clamp(TransitionProgress, 0.0f, 1.0f);
			
			// 应用过渡效果
			ApplyTransitionEffects(TransitionProgress, bTransitionToNight);
			
			// 检查过渡是否完成
			if (TransitionProgress >= 1.0f)
			{
				bIsTransitioning = false;
				TransitionProgress = 0.0f;
				
				// 过渡完成后设置最终状态
				if (bTransitionToNight)
				{
					SetLightIntensityBasedOnTime(true);
					SetAtmosphereColorBasedOnTime(true);
				}
				else
				{
					SetLightIntensityBasedOnTime(false);
					SetAtmosphereColorBasedOnTime(false);
				}
			}
		}
		else
		{
			// 更新游戏时间
			UpdateGameTime(DeltaTime);
			
			// 检查昼夜切换
			CheckDayNightTransition();
			
			// 检查游戏结束条件
			CheckWinCondition();
			CheckLoseCondition();
			
			// 更新UI
			UpdateGameUI();
		}
	}
}

void ASDTAGameMode::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	// 复制所有需要同步的属性
	DOREPLIFETIME(ASDTAGameMode, GameTime);						// 游戏时间
	DOREPLIFETIME(ASDTAGameMode, CurrentDay);					// 当前天数
	DOREPLIFETIME(ASDTAGameMode, bIsNight);						// 是否夜晚
	DOREPLIFETIME(ASDTAGameMode, CurrentEnemyCount);			// 当前敌人数量
	DOREPLIFETIME(ASDTAGameMode, MaxEnemyCount);				// 最大敌人数量
	DOREPLIFETIME(ASDTAGameMode, SoulFragments);				// 灵魂碎片数量
	DOREPLIFETIME(ASDTAGameMode, PlayerUpgrades);				// 玩家升级信息
	DOREPLIFETIME(ASDTAGameMode, ConnectedPlayers);				// 已连接玩家列表
	DOREPLIFETIME(ASDTAGameMode, MaxPlayers);					// 最大玩家数量
	DOREPLIFETIME(ASDTAGameMode, TeamScore);					// 团队分数
	DOREPLIFETIME(ASDTAGameMode, bGameStarted);					// 是否游戏已开始
	DOREPLIFETIME(ASDTAGameMode, bGameOver);					// 是否游戏已结束
	DOREPLIFETIME(ASDTAGameMode, bVictory);						// 是否胜利
}
#pragma endregion

#pragma region 光源管理系统 - 方法实现
/**
 * 获取世界中的光源列表
 * 
 * 功能：遍历世界中的所有光源，可选择按标签筛选
 * 实现细节：
 * - 获取世界中的所有Actor
 * - 遍历每个Actor的所有组件
 * - 筛选出LightComponent类型的组件
 * - 如果指定了标签，则只返回带有该标签的光源
 * 
 * @param OptionalTag 可选的光源标签筛选
 * @return 返回符合条件的光源列表
 */
TArray<class ULightComponent*> ASDTAGameMode::GetWorldLights(const FName& OptionalTag)
{
	TArray<class ULightComponent*> LightComponents;
	
	if (UWorld* World = GetWorld())
	{
		// 获取世界中的所有Actor
		TArray<AActor*> AllActors;
		UGameplayStatics::GetAllActorsOfClass(World, AActor::StaticClass(), AllActors);
		
		// 遍历每个Actor
		for (AActor* Actor : AllActors)
		{
			if (!Actor) continue;
			
			// 检查是否需要按标签筛选
			if (OptionalTag != NAME_None && !Actor->ActorHasTag(OptionalTag))
			{
				continue;
			}
			
			// 获取Actor的所有组件
			TArray<UActorComponent*> Components;
			Actor->GetComponents(ULightComponent::StaticClass(), Components);
			
			// 将符合条件的光源组件添加到列表中
			for (UActorComponent* Component : Components)
			{
				if (ULightComponent* LightComp = Cast<ULightComponent>(Component))
				{
					LightComponents.Add(LightComp);
				}
			}
		}
	}
	
	return LightComponents;
}

/**
 * 设置光源亮度和颜色
 * 
 * 功能：根据昼夜状态调整光源的亮度和颜色
 * 实现细节：
 * - 使用LightTag获取世界中的光源
 * - 根据昼夜状态设置不同的亮度和颜色
 * - 记录调整结果
 * 
 * @param bNight 是否为夜晚
 */
void ASDTAGameMode::SetLightIntensityBasedOnTime(bool bNight)
{
	// 使用LightTag获取世界中的光源
	TArray<class ULightComponent*> Lights = GetWorldLights(LightTag);
	
	// 根据昼夜状态确定目标亮度和颜色
	float TargetIntensity = bNight ? NightLightIntensity : DayLightIntensity;
	FLinearColor TargetColor = bNight ? NightLightColor : DayLightColor;
	
	// 遍历并设置每个光源的亮度和颜色
	for (ULightComponent* Light : Lights)
	{
		if (Light)
		{
			// 设置光源的亮度
			Light->SetIntensity(TargetIntensity);
			// 设置光源的颜色
			Light->SetLightColor(TargetColor);
		}
	}
	
	// 输出调试信息
	UE_LOG(LogTemp, Log, TEXT("光源亮度和颜色已调整：昼夜=%d, 亮度值=%.2f, 影响光源数量=%d"), bNight, TargetIntensity, Lights.Num());
}

/**
 * 设置大气瑞利散射颜色
 * 
 * 功能：根据昼夜状态调整大气的瑞利散射颜色
 * 实现细节：
 * - 使用AtmosphereTag获取世界中的大气组件
 * - 根据昼夜状态设置不同的瑞利散射颜色
 * - 记录调整结果
 * 
 * @param bNight 是否为夜晚
 */
void ASDTAGameMode::SetAtmosphereColorBasedOnTime(bool bNight)
{
	if (UWorld* World = GetWorld())
	{
		// 获取世界中带有AtmosphereTag标签的Actor
		TArray<AActor*> AtmosphereActors;
		UGameplayStatics::GetAllActorsWithTag(World, AtmosphereTag, AtmosphereActors);
		
		// 根据昼夜状态确定目标颜色
		FLinearColor TargetColor = bNight ? NightAtmosphereColor : DayAtmosphereColor;
		
		// 遍历并设置每个大气组件的瑞利散射颜色
		for (AActor* Actor : AtmosphereActors)
		{
			if (!Actor) continue;
			
			// 尝试获取天空大气组件
			TArray<UActorComponent*> Components;
			Actor->GetComponents(USkyAtmosphereComponent::StaticClass(), Components);
			
			for (UActorComponent* Component : Components)
			{
				if (USkyAtmosphereComponent* AtmosphereComp = Cast<USkyAtmosphereComponent>(Component))
				{
					// 设置瑞利散射颜色
					// 在UE5中，瑞利散射颜色通过RayleighScattering属性设置
					AtmosphereComp->RayleighScattering = TargetColor;
					
					// 输出调试信息
					UE_LOG(LogTemp, Log, TEXT("已设置天空大气组件的瑞利散射：昼夜=%d, 颜色=%s"), bNight, *TargetColor.ToString());
				}
			}
		}
		
		// 输出调试信息
		UE_LOG(LogTemp, Log, TEXT("大气颜色已调整：昼夜=%d, 影响大气数量=%d"), bNight, AtmosphereActors.Num());
	}
}

/**
 * 应用过渡效果
 * 
 * 功能：在昼夜过渡期间应用平滑的颜色和亮度变化
 * 实现细节：
 * - 根据过渡进度计算当前的亮度和颜色值
 * - 应用到光源和大气组件
 * - 使用线性插值实现平滑过渡
 * 
 * @param Progress 过渡进度（0-1）
 * @param bToNight 是否过渡到夜晚
 */
void ASDTAGameMode::ApplyTransitionEffects(float Progress, bool bToNight)
{
	if (UWorld* World = GetWorld())
	{
		// 计算过渡的亮度和颜色值
		float StartIntensity = bToNight ? DayLightIntensity : NightLightIntensity;
		float EndIntensity = bToNight ? NightLightIntensity : DayLightIntensity;
		FLinearColor StartColor = bToNight ? DayLightColor : NightLightColor;
		FLinearColor EndColor = bToNight ? NightLightColor : DayLightColor;
		FLinearColor StartAtmosphereColor = bToNight ? DayAtmosphereColor : NightAtmosphereColor;
		FLinearColor EndAtmosphereColor = bToNight ? NightAtmosphereColor : DayAtmosphereColor;
		
		// 线性插值计算当前值
		float CurrentIntensity = FMath::Lerp(StartIntensity, EndIntensity, Progress);
		FLinearColor CurrentLightColor = FLinearColor::LerpUsingHSV(StartColor, EndColor, Progress);
		FLinearColor CurrentAtmosphereColor = FLinearColor::LerpUsingHSV(StartAtmosphereColor, EndAtmosphereColor, Progress);
		
		// 应用光源过渡效果
		TArray<class ULightComponent*> Lights = GetWorldLights(LightTag);
		for (ULightComponent* Light : Lights)
		{
			if (Light)
			{
				Light->SetIntensity(CurrentIntensity);
				Light->SetLightColor(CurrentLightColor);
			}
		}
		
		// 应用大气过渡效果
		TArray<AActor*> AtmosphereActors;
		UGameplayStatics::GetAllActorsWithTag(World, AtmosphereTag, AtmosphereActors);
		
		for (AActor* Actor : AtmosphereActors)
		{
			if (!Actor) continue;
			
			TArray<UActorComponent*> Components;
			Actor->GetComponents(USkyAtmosphereComponent::StaticClass(), Components);
			
			for (UActorComponent* Component : Components)
			{
				if (USkyAtmosphereComponent* AtmosphereComp = Cast<USkyAtmosphereComponent>(Component))
				{
					AtmosphereComp->RayleighScattering = CurrentAtmosphereColor;
				}
			}
		}
	}
}
#pragma endregion



#pragma region 昼夜循环系统 - 方法实现
/**
 * 更新游戏时间
 * 
 * 功能：根据DeltaTime更新游戏内时间
 * 实现细节：简单累加DeltaTime到GameTime变量
 * 
 * @param DeltaTime 帧间隔时间（秒）
 */
void ASDTAGameMode::UpdateGameTime(float DeltaTime)
{
	GameTime += DeltaTime;
}

/**
 * 检查昼夜切换
 * 
 * 功能：根据当前游戏时间判断是否需要进行昼夜切换
 * 实现细节：
 * - 白天：检查是否达到白天持续时间
 * - 夜晚：检查是否达到夜晚持续时间
 * - 达到时间后调用相应的阶段开始方法
 */
void ASDTAGameMode::CheckDayNightTransition()
{
	if (!bIsNight)
	{
		// 白天阶段：检查是否达到白天持续时间
		if (GameTime >= DayDuration)
		{
			// 重置游戏时间并开始夜晚阶段
			GameTime = 0.0f;
			StartNightPhase();
		}
	}
	else
	{
		// 夜晚阶段：检查是否达到夜晚持续时间
		if (GameTime >= NightDuration)
		{
			// 重置游戏时间并开始白天阶段
			GameTime = 0.0f;
			CurrentDay++;
			StartDayPhase();
		}
	}
}

/**
 * 开始夜晚阶段
 * 
 * 功能：处理夜晚开始时的逻辑
 * 实现细节：
 * - 设置bIsNight为true
 * - 启动昼夜过渡效果
 * - 广播夜晚开始事件
 * - 准备敌人生成（暂时注释，后续实现）
 */
void ASDTAGameMode::StartNightPhase()
{
	bIsNight = true;
	
	// 启动过渡效果
	bIsTransitioning = true;
	TransitionProgress = 0.0f;
	bTransitionToNight = true;
	
	// 广播夜晚开始事件
	UE_LOG(LogTemp, Log, TEXT("夜晚阶段开始！第 %d 天"), CurrentDay);
	
	// 广播游戏状态更新
	BroadcastGameState();
	
	// 后续实现：开始敌人生成
	// StartEnemySpawning();
}

/**
 * 开始白天阶段
 * 
 * 功能：处理白天开始时的逻辑
 * 实现细节：
 * - 设置bIsNight为false
 * - 启动昼夜过渡效果
 * - 广播白天开始事件
 * - 分配灵魂碎片用于升级
 * - 停止敌人生成（暂时注释，后续实现）
 */
void ASDTAGameMode::StartDayPhase()
{
	bIsNight = false;
	
	// 启动过渡效果
	bIsTransitioning = true;
	TransitionProgress = 0.0f;
	bTransitionToNight = false;
	
	// 广播白天开始事件
	UE_LOG(LogTemp, Log, TEXT("白天阶段开始！第 %d 天"), CurrentDay);
	
	// 分配灵魂碎片用于升级
	DistributeSoulFragments();
	
	// 广播游戏状态更新
	BroadcastGameState();
	
	// 后续实现：停止敌人生成
	// StopEnemySpawning();
}
#pragma endregion

#pragma region 敌人生成系统 - 方法声明
void ASDTAGameMode::SpawnEnemyWave()
{
	// TODO: 实现敌人生成波次逻辑
}

void ASDTAGameMode::StartEnemySpawning()
{
	// TODO: 开始敌人生成
}

void ASDTAGameMode::StopEnemySpawning()
{
	// TODO: 停止敌人生成
}

void ASDTAGameMode::OnEnemyDestroyed(class AEnemyBase* DestroyedEnemy)
{
	// TODO: 处理敌人被击败事件
}

void ASDTAGameMode::CleanupDeadEnemies()
{
	// TODO: 清理死亡敌人
}
#pragma endregion

#pragma region 资源与升级系统 - 方法声明
/**
 * 收集灵魂碎片
 * 
 * 功能：收集敌人掉落的灵魂碎片
 * 实现细节：
 * - 将收集到的碎片直接添加到可用灵魂碎片中
 * - 记录收集日志
 * 
 * @param Amount 收集的灵魂碎片数量
 */
void ASDTAGameMode::CollectSoulFragments(int32 Amount)
{
	SoulFragments += Amount;
	
	UE_LOG(LogTemp, Log, TEXT("收集了 %d 个灵魂碎片。可用数量：%d"), Amount, SoulFragments);
}

/**
 * 分配灵魂碎片
 * 
 * 功能：在白天开始时准备灵魂碎片用于升级
 * 实现细节：
 * - 由于现在直接使用可用灵魂碎片，此方法主要用于日志记录和未来扩展
 */
void ASDTAGameMode::DistributeSoulFragments()
{
	// 直接使用可用灵魂碎片，无需额外分配
	UE_LOG(LogTemp, Log, TEXT("灵魂碎片已准备就绪：%d 个可用用于升级"), SoulFragments);
}

void ASDTAGameMode::ApplyPlayerUpgrade(const FName& UpgradeName)
{
	// TODO: 实现玩家升级应用
}

bool ASDTAGameMode::CanAffordUpgrade(int32 Cost) const
{
	// TODO: 实现升级 affordability 检查
	return false;
}
#pragma endregion

#pragma region 多人游戏系统 - 方法声明
void ASDTAGameMode::OnPlayerJoined(class APlayerController* NewPlayer)
{
	// TODO: 实现玩家加入处理
}

void ASDTAGameMode::OnPlayerLeft(class APlayerController* LeavingPlayer)
{
	// TODO: 实现玩家离开处理
}

void ASDTAGameMode::UpdateTeamScore(int32 Points)
{
	// TODO: 实现团队分数更新
}

void ASDTAGameMode::ServerCollectFragments_Implementation(class APlayerState* Player, int32 Amount)
{
	// 服务器端碎片收集验证逻辑
	if (HasAuthority() && Player)
	{
		// TODO: 实现碎片收集逻辑
	}
}

void ASDTAGameMode::ServerApplyUpgrade_Implementation(class APlayerState* Player, const FName& UpgradeName)
{
	// 服务器端升级验证逻辑
	if (HasAuthority() && Player)
	{
		// TODO: 实现升级验证逻辑
	}
}
#pragma endregion

#pragma region 游戏状态管理 - 方法声明
/**
 * 实现GetPoolManager方法
 * 
 * 功能：提供对私有成员变量PoolManager的安全访问接口
 * 实现细节：直接返回内部对象池管理器实例的指针
 * 
 * @return 返回对象池管理器实例，如果未初始化则返回nullptr
 * 
 * 设计说明：
 * - 采用只读访问（const），确保外部无法修改内部对象池管理器
 * - 实现了封装原则，避免直接访问私有成员变量
 * - 支持蓝图调用，便于在蓝图中使用对象池功能
 */
USDTAPoolManager* ASDTAGameMode::GetPoolManager() const
{
	return PoolManager;
}

/**
 * 开始游戏
 * 
 * 功能：初始化游戏状态并开始游戏
 * 实现细节：
 * - 设置游戏状态为已开始
 * - 重置游戏时间
 * - 开始第一天白天阶段
 * - 分配初始灵魂碎片
 */
void ASDTAGameMode::StartGame()
{
	bGameStarted = true;
	bGameOver = false;
	bVictory = false;
	
	GameTime = 0.0f;
	CurrentDay = 1;
	bIsNight = false;
	
	// 设置初始光源状态为白天
	SetLightIntensityBasedOnTime(false);
	
	// 设置初始大气状态为白天
	SetAtmosphereColorBasedOnTime(false);
	
	// 分配初始灵魂碎片
	DistributeSoulFragments();
	
	// 开始第一天白天阶段
	UE_LOG(LogTemp, Log, TEXT("游戏开始！第 1 天开始。"));
	
	// 广播游戏状态更新
	BroadcastGameState();
}

void ASDTAGameMode::EndGame(bool bWin)
{
	// TODO: 实现游戏结束逻辑
}

void ASDTAGameMode::CheckWinCondition()
{
	// TODO: 实现胜利条件检查
}

void ASDTAGameMode::CheckLoseCondition()
{
	// TODO: 实现失败条件检查
}

void ASDTAGameMode::SaveGameProgress()
{
	// TODO: 实现游戏进度保存
}

void ASDTAGameMode::LoadGameProgress()
{
	// TODO: 实现游戏进度加载
}
#pragma endregion

#pragma region UI与事件系统 - 方法声明
/**
 * 更新游戏UI
 * 
 * 功能：计算游戏状态数据并调用BroadcastGameState更新UI
 * 实现细节：
 * - 计算当前阶段的剩余时间
 * - 计算时间百分比（用于进度条显示）
 * - 调用BroadcastGameState广播游戏状态
 */
void ASDTAGameMode::UpdateGameUI()
{
	// 计算剩余时间
	float TotalDuration = bIsNight ? NightDuration : DayDuration;
	float RemainingTime = FMath::Max(0.0f, TotalDuration - GameTime);
	
	// 计算时间百分比（用于进度条，从100%开始递减）
	float TimePercent = FMath::Clamp(1.0f - (GameTime / TotalDuration), 0.0f, 1.0f);
	
	// 广播游戏状态更新
	BroadcastGameState();
}

/**
 * 广播游戏状态
 * 
 * 功能：将游戏状态数据同步到所有玩家的HUD
 * 实现细节：
 * - 遍历所有玩家控制器
 * - 获取每个玩家的HUD实例
 * - 计算当前阶段的剩余时间和百分比
 * - 调用HUD的更新方法更新UI
 */
void ASDTAGameMode::BroadcastGameState()
{
	// 计算当前阶段的剩余时间和百分比
	float TotalDuration = bIsNight ? NightDuration : DayDuration;
	float RemainingTime = FMath::Max(0.0f, TotalDuration - GameTime);
	float TimePercent = FMath::Clamp(1.0f - (GameTime / TotalDuration), 0.0f, 1.0f);
	
	// 遍历所有玩家控制器，更新他们的HUD
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (PC)
		{
			// 将PC转换为ASDTAPlayerController类型
			ASDTAPlayerController* SDTAPC = Cast<ASDTAPlayerController>(PC);
			if (SDTAPC)
			{
				// 获取PlayerHUD实例
				USDTAPlayerHUD* PlayerHUD = SDTAPC->GetPlayerHUD();
				if (PlayerHUD)
				{
					// 更新HUD的属性
					PlayerHUD->bIsNight = bIsNight;
					PlayerHUD->CurrentDay = CurrentDay;
					PlayerHUD->RemainingTime = RemainingTime;
					PlayerHUD->TimePercent = TimePercent;
					
					// 调用HUD的更新方法更新UI
					PlayerHUD->BP_UpdateDayNightCycle(bIsNight, CurrentDay, RemainingTime, TimePercent);
				}
				
				// 输出游戏状态日志
				// UE_LOG(LogTemp, Log, TEXT("游戏状态更新：昼夜=%d, 天数=%d, 剩余时间=%.2f, 百分比=%.2f"), bIsNight, CurrentDay, RemainingTime, TimePercent);
			}
		}
	}
}
#pragma endregion

