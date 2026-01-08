// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SDTADayNightManager.generated.h"

/**
 * 昼夜管理器
 * 
 * 核心功能：
 * 1. 管理昼夜循环系统
 * 2. 处理昼夜过渡效果
 * 3. 提供事件系统触发状态变化
 * 4. 管理光源和大气颜色
 * 
 * 使用说明：
 * - 作为独立模块管理昼夜系统
 * - 通过事件系统与其他模块通信
 * - 支持蓝图和C++双重调用
 */

// 昼夜状态变化事件
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDayNightStateChanged, bool, bIsNowNight);

// 过渡开始事件
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTransitionStarted, bool, bTransitioningToNight);

// 过渡结束事件
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTransitionCompleted);

// 时间更新事件
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTimeUpdated, float, RemainingTime, float, TimePercent);

UCLASS(Blueprintable, BlueprintType)
class SEVENDAYSTOALIVE_API USDTADayNightManager : public UObject
{
	GENERATED_BODY()
	
public:
	USDTADayNightManager();
	
	/**
	 * 初始化昼夜管理器
	 * 
	 * 功能：设置初始状态并开始昼夜循环
	 * 
	 * @param InWorld 游戏世界指针
	 */
	void Initialize(UWorld* InWorld);
	
	/**
	 * 开始昼夜循环
	 * 
	 * 功能：启动定时器并开始时间计算
	 */
	void StartDayNightCycle();
	
	/**
	 * 停止昼夜循环
	 * 
	 * 功能：暂停时间计算和过渡效果
	 */
	void StopDayNightCycle();
	
	/**
	 * 更新昼夜系统
	 * 
	 * 功能：在每帧调用，更新时间和处理过渡效果
	 * 
	 * @param DeltaTime 帧间隔时间（秒）
	 */
	void Tick(float DeltaTime);
	
	/**
	 * 开始夜晚阶段
	 * 
	 * 功能：启动从白天到夜晚的过渡
	 */
	void StartNightPhase();
	
	/**
	 * 开始白天阶段
	 * 
	 * 功能：启动从夜晚到白天的过渡
	 */
	void StartDayPhase();
	
	/**
	 * 获取当前昼夜状态
	 * 
	 * @return 是否为夜晚
	 */
	UFUNCTION(BlueprintCallable, Category = "Day Night System")
	bool IsNight() const;
	
	/**
	 * 获取当前天数
	 * 
	 * @return 当前天数
	 */
	UFUNCTION(BlueprintCallable, Category = "Day Night System")
	int32 GetCurrentDay() const;
	
	/**
	 * 获取游戏时间
	 * 
	 * @return 游戏时间（秒）
	 */
	UFUNCTION(BlueprintCallable, Category = "Day Night System")
	float GetGameTime() const;
	
	/**
	 * 获取剩余时间
	 * 
	 * @return 剩余时间（秒）
	 */
	UFUNCTION(BlueprintCallable, Category = "Day Night System")
	float GetRemainingTime() const;
	
	/**
	 * 获取时间百分比
	 * 
	 * @return 时间百分比（0-1）
	 */
	UFUNCTION(BlueprintCallable, Category = "Day Night System")
	float GetTimePercent() const;
	
	/**
	 * 是否正在过渡
	 * 
	 * @return 是否正在过渡
	 */
	UFUNCTION(BlueprintCallable, Category = "Day Night System")
	bool IsTransitioning() const;
	
	/**
	 * 获取世界中的光源列表
	 * 
	 * 功能：遍历世界中的所有光源，可选择按标签筛选
	 * 
	 * @param OptionalTag 可选的光源标签筛选
	 * @return 返回符合条件的光源列表
	 */
	UFUNCTION(BlueprintCallable, Category = "Light Management")
	TArray<class ULightComponent*> GetWorldLights(const FName& OptionalTag = NAME_None);
	
	/**
	 * 设置光源亮度和颜色
	 * 
	 * 功能：根据昼夜状态调整光源的亮度和颜色
	 * 
	 * @param bNight 是否为夜晚
	 */
	UFUNCTION(BlueprintCallable, Category = "Light Management")
	void SetLightIntensityBasedOnTime(bool bNight);
	
	/**
	 * 设置大气颜色
	 * 
	 * 功能：根据昼夜状态调整大气的瑞利散射颜色
	 * 
	 * @param bNight 是否为夜晚
	 */
	UFUNCTION(BlueprintCallable, Category = "Light Management")
	void SetAtmosphereColorBasedOnTime(bool bNight);

	/**
	 * 获取需要复制的属性
	 * 
	 * 功能：定义需要在网络上同步的属性
	 */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	// 游戏时间管理
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Day Night System")
	float GameTime; // 游戏内时间（秒）
	
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Day Night System")
	int32 CurrentDay; // 当前天数（1-7）
	
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Day Night System")
	bool bIsNight; // 是否为夜晚阶段
	
	// 时间配置
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Day Night System")
	float DayDuration; // 白天持续时间（秒）
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Day Night System")
	float NightDuration; // 夜晚持续时间（秒）
	
	// 渐变系统
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Transition")
	float TransitionDuration; // 昼夜过渡持续时间（秒）
	
	UPROPERTY(BlueprintReadOnly, Category = "Transition")
	bool bIsTransitioning; // 是否正在过渡
	
	// 光源配置
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Light Management")
	float DayLightIntensity; // 白天光源亮度
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Light Management")
	float NightLightIntensity; // 夜晚光源亮度
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Light Management")
	FLinearColor DayLightColor; // 白天光源颜色
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Light Management")
	FLinearColor NightLightColor; // 夜晚光源颜色
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Light Management")
	FName LightTag; // 光源标签
	
	// 大气配置
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Light Management")
	FLinearColor DayAtmosphereColor; // 白天大气瑞利散射颜色
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Light Management")
	FLinearColor NightAtmosphereColor; // 夜晚大气瑞利散射颜色
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Light Management")
	FName AtmosphereTag; // 大气标签
	
	// 事件系统
	UPROPERTY(BlueprintAssignable, Category = "Day Night System")
	FOnDayNightStateChanged OnDayNightStateChanged; // 昼夜状态变化事件
	
	UPROPERTY(BlueprintAssignable, Category = "Day Night System")
	FOnTransitionStarted OnTransitionStarted; // 过渡开始事件
	
	UPROPERTY(BlueprintAssignable, Category = "Day Night System")
	FOnTransitionCompleted OnTransitionCompleted; // 过渡结束事件
	
	UPROPERTY(BlueprintAssignable, Category = "Day Night System")
	FOnTimeUpdated OnTimeUpdated; // 时间更新事件

private:
	// 世界指针
	UWorld* World;
	
	// 过渡系统
	float TransitionProgress; // 过渡进度（0-1）
	bool bTransitionToNight; // 是否过渡到夜晚
	
	// 计算剩余时间
	float GetRemainingTimeInternal() const;
	
	// 计算时间百分比
	float CalculateTimePercent() const;
	
	// 应用过渡效果
	void ApplyTransitionEffects(float Progress, bool bToNight);
	
	// 检查昼夜切换
	void CheckDayNightTransition();
	
	// 更新游戏时间
	void UpdateGameTime(float DeltaTime);
};
