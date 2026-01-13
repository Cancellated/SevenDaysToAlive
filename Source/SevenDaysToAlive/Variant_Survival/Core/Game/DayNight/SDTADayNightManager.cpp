// Fill out your copyright notice in the Description page of Project Settings.

#include "CoreMinimal.h"
#include "Variant_Survival/Core/Game/DayNight/SDTADayNightManager.h"
#include "Kismet/GameplayStatics.h"
#include "Components/LightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Math/UnrealMathUtility.h"

USDTADayNightManager::USDTADayNightManager()
{
    // 初始化默认值
    GameTime = 0.0f;
    CurrentDay = 1;
    bIsNight = false;
    
    // 默认时间配置
    DayDuration = 120.0f; // 白天 2 分钟
    NightDuration = 300.0f; // 夜晚 5 分钟
    
    // 默认过渡时间
    TransitionDuration = 5.0f; // 5 秒过渡
    
    // 初始化过渡系统
    TransitionProgress = 0.0f;
    bIsTransitioning = false;
    bTransitionToNight = false;
    
    // 默认光源配置
    DayLightIntensity = 3.0f;
    NightLightIntensity = 0.5f;
    DayLightColor = FLinearColor(1.0f, 0.95f, 0.9f); // 白天暖白色
    NightLightColor = FLinearColor(0.7f, 0.8f, 1.0f); // 夜晚蓝白色
    LightTag = FName("WorldLight");
    
    // 默认大气配置
    DayAtmosphereColor = FLinearColor(0.5f, 0.7f, 1.0f); // 白天蓝色
    NightAtmosphereColor = FLinearColor(0.1f, 0.1f, 0.3f); // 夜晚深蓝色
    AtmosphereTag = FName("WorldAtmosphere");
}

void USDTADayNightManager::Initialize(UWorld* InWorld)
{
    World = InWorld;
    if (World)
    {
        // 初始设置为白天
        bIsNight = false;
        SetLightIntensityBasedOnTime(false);
        SetAtmosphereColorBasedOnTime(false);
        
        // 开始昼夜循环
        StartDayNightCycle();
    }
}

void USDTADayNightManager::StartDayNightCycle()
{
    // 重置时间
    GameTime = 0.0f;
}

void USDTADayNightManager::StopDayNightCycle()
{
    // 暂停时间计算
    // 这里可以添加额外的暂停逻辑
}

void USDTADayNightManager::Tick(float DeltaTime)
{
    if (!World) return;
    
    // 始终更新游戏时间，即使在过渡期间
    UpdateGameTime(DeltaTime);
    
    // 检查是否需要昼夜切换（仅在非过渡状态下）
    if (!bIsTransitioning)
    {
        CheckDayNightTransition();
    }
    
    // 如果正在过渡，更新过渡进度
    if (bIsTransitioning)
    {
        TransitionProgress += DeltaTime / TransitionDuration;
        TransitionProgress = FMath::Clamp(TransitionProgress, 0.0f, 1.0f);
        
        // 应用过渡效果
        ApplyTransitionEffects(TransitionProgress, bTransitionToNight);
        
        // 检查过渡是否完成
        if (TransitionProgress >= 1.0f)
        {
            bIsTransitioning = false;
            TransitionProgress = 0.0f;
            
            // 广播过渡完成事件
            OnTransitionCompleted.Broadcast();
        }
    }
    
    // 计算剩余时间和时间百分比
    float RemainingTime = GetRemainingTimeInternal();
    float TimePercent = CalculateTimePercent();
    
    // 广播时间更新事件
    OnTimeUpdated.Broadcast(RemainingTime, TimePercent);
}


void USDTADayNightManager::StartNightPhase()
{
    if (!bIsTransitioning)
    {
        bIsTransitioning = true;
        bTransitionToNight = true;
        TransitionProgress = 0.0f;
        
        // 立即更新昼夜状态，确保时间计算正确
        bIsNight = true;
        
        // 广播过渡开始事件
        OnTransitionStarted.Broadcast(true);
        
        // 广播昼夜状态变化事件（在过渡开始时）
        OnDayNightStateChanged.Broadcast(true);
    }
}

void USDTADayNightManager::StartDayPhase()
{
    if (!bIsTransitioning)
    {
        bIsTransitioning = true;
        bTransitionToNight = false;
        TransitionProgress = 0.0f;
        
        // 立即更新昼夜状态，确保时间计算正确
        bIsNight = false;
        
        // 广播过渡开始事件
        OnTransitionStarted.Broadcast(false);
        
        // 广播昼夜状态变化事件（在过渡开始时）
        OnDayNightStateChanged.Broadcast(false);
    }
}


bool USDTADayNightManager::IsNight() const
{
    return bIsNight;
}

int32 USDTADayNightManager::GetCurrentDay() const
{
    return CurrentDay;
}

float USDTADayNightManager::GetGameTime() const
{
    return GameTime;
}

float USDTADayNightManager::GetRemainingTime() const
{
    return GetRemainingTimeInternal();
}

float USDTADayNightManager::GetTimePercent() const
{
    return CalculateTimePercent();
}

bool USDTADayNightManager::IsTransitioning() const
{
    return bIsTransitioning;
}

bool USDTADayNightManager::IsTransitioningToNight() const
{
    return bTransitionToNight;
}

float USDTADayNightManager::GetTransitionProgress() const
{
    return TransitionProgress;
}

TArray<class ULightComponent*> USDTADayNightManager::GetWorldLights(const FName& OptionalTag)
{
    TArray<class ULightComponent*> Lights;
    
    if (!World) return Lights;
    
    // 获取所有带标签的光源
    TArray<AActor*> Actors;
    if (OptionalTag != NAME_None)
    {
        UGameplayStatics::GetAllActorsWithTag(World, OptionalTag, Actors);
    }
    else
    {
        UGameplayStatics::GetAllActorsWithTag(World, LightTag, Actors);
    }
    
    // 遍历所有演员，获取光源组件
    for (AActor* Actor : Actors)
    {
        if (!Actor) continue;
        
        TArray<ULightComponent*> LightComponents;
        Actor->GetComponents<ULightComponent>(LightComponents);
        
        for (ULightComponent* LightComp : LightComponents)
        {
            if (LightComp)
            {
                Lights.Add(LightComp);
            }
        }
    }
    
    return Lights;
}

void USDTADayNightManager::SetLightIntensityBasedOnTime(bool bNight)
{
    if (!World) return;
    
    // 获取所有光源
    TArray<ULightComponent*> Lights = GetWorldLights();
    
    // 设置光源属性
    float TargetIntensity = bNight ? NightLightIntensity : DayLightIntensity;
    FLinearColor TargetColor = bNight ? NightLightColor : DayLightColor;
    
    for (ULightComponent* Light : Lights)
    {
        if (Light)
        {
            Light->SetIntensity(TargetIntensity);
            Light->SetLightColor(TargetColor);
        }
    }
}

void USDTADayNightManager::SetAtmosphereColorBasedOnTime(bool bNight)
{
    if (!World) return;
    
    // 获取所有带大气标签的演员
    TArray<AActor*> AtmosphereActors;
    UGameplayStatics::GetAllActorsWithTag(World, AtmosphereTag, AtmosphereActors);
    
    // 设置大气属性
    FLinearColor TargetColor = bNight ? NightAtmosphereColor : DayAtmosphereColor;
    
    for (AActor* Actor : AtmosphereActors)
    {
        if (!Actor) continue;
        
        // 获取天空大气组件
        TArray<UActorComponent*> Components;
        Actor->GetComponents(USkyAtmosphereComponent::StaticClass(), Components);
        
        for (UActorComponent* Component : Components)
        {
            if (USkyAtmosphereComponent* AtmosphereComp = Cast<USkyAtmosphereComponent>(Component))
            {
                AtmosphereComp->RayleighScattering = TargetColor;
            }
        }
    }
}

float USDTADayNightManager::GetRemainingTimeInternal() const
{
    float TotalCycleDuration = DayDuration + NightDuration;
    float PhaseTime = FMath::Fmod(GameTime, TotalCycleDuration);
    
    if (bIsNight)
    {
        // 检查是否已经到了第二天的白天阶段
        if (PhaseTime < DayDuration)
        {
            // 已经到了第二天的白天阶段，返回白天剩余时间
            return FMath::Max(0.0f, DayDuration - PhaseTime);
        }
        else
        {
            // 仍然在夜晚阶段，返回夜晚剩余时间
            return FMath::Max(0.0f, NightDuration - (PhaseTime - DayDuration));
        }
    }
    else
    {
        return FMath::Max(0.0f, DayDuration - PhaseTime);
    }
}

float USDTADayNightManager::CalculateTimePercent() const
{
    float CurrentDuration = bIsNight ? NightDuration : DayDuration;
    float RemainingTime = GetRemainingTimeInternal();
    
    if (CurrentDuration > 0.0f)
    {
        // 确保时间百分比在0-1之间
        return FMath::Clamp(RemainingTime / CurrentDuration, 0.0f, 1.0f);
    }
    
    return 0.0f;
}

void USDTADayNightManager::ApplyTransitionEffects(float Progress, bool bToNight)
{
    if (!World) return;
    
    // 计算当前过渡值
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
    
    // 更新光源
    TArray<ULightComponent*> Lights = GetWorldLights();
    for (ULightComponent* Light : Lights)
    {
        if (Light)
        {
            Light->SetIntensity(CurrentIntensity);
            Light->SetLightColor(CurrentLightColor);
        }
    }
    
    // 更新大气
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

void USDTADayNightManager::CheckDayNightTransition()
{
    // 如果正在过渡，跳过检查
    if (bIsTransitioning) return;
    
    float TotalCycleDuration = DayDuration + NightDuration;
    float PhaseTime = FMath::Fmod(GameTime, TotalCycleDuration);
    
    // 检查是否需要从白天切换到夜晚
    if (!bIsNight && PhaseTime >= DayDuration)
    {
        StartNightPhase();
    }
    // 检查是否需要从夜晚切换到白天
    else if (bIsNight && PhaseTime < DayDuration)
    {
        StartDayPhase();
        // 增加天数
        CurrentDay++;
    }
}

void USDTADayNightManager::UpdateGameTime(float DeltaTime)
{
    GameTime += DeltaTime;
}

// 空实现，因为UObject不支持网络复制
void USDTADayNightManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    // 注意：UObject默认不支持网络复制，此方法仅为满足编译需求
}
