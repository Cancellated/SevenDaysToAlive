// Fill out your copyright notice in the Description page of Project Settings.

#include "HealthComponent.h"
#include "SevenDaysToAlive.h"

// Sets default values for this component's properties
UHealthComponent::UHealthComponent()
{
    // Set this component to be initialized when the game starts, and to be ticked every frame.
    PrimaryComponentTick.bCanEverTick = true;

    // 启用组件的网络复制
    SetIsReplicated(true);

    // 设置默认健康值
    MaxHealth = 100.0f;
    Health = MaxHealth;
}

// Called when the game starts
void UHealthComponent::BeginPlay()
{
    Super::BeginPlay();

    // 初始化健康值
    Health = MaxHealth;
}

// Called every frame
void UHealthComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // 健康值组件不需要每帧更新
}

// 设置健康值
void UHealthComponent::SetHealth(float NewHealth)
{
    float OldHealth = Health;
    Health = FMath::Clamp(NewHealth, 0.0f, MaxHealth);

    // 广播健康值变化
    float HealthPercent = MaxHealth > 0.0f ? Health / MaxHealth : 0.0f;
    OnHealthChanged.Broadcast(HealthPercent);

    // 检查是否死亡
    if (Health <= 0.0f && OldHealth > 0.0f)
    {
        OnDeath.Broadcast();
    }
    // 检查是否为低健康值
    else if (Health < MaxHealth * 0.3f && OldHealth >= MaxHealth * 0.3f)
    {
        OnHealthLowWarning.Broadcast();
    }
}

// 增加健康值
void UHealthComponent::AddHealth(float HealthToAdd)
{
    SetHealth(Health + HealthToAdd);
}

// 减少健康值
void UHealthComponent::RemoveHealth(float HealthToRemove)
{
    SetHealth(Health - HealthToRemove);
}

// 检查是否死亡
bool UHealthComponent::IsDead() const
{
    return Health <= 0.0f;
}

// 实现网络复制属性配置
void UHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    
    // 复制最大健康值和当前健康值
    DOREPLIFETIME(UHealthComponent, MaxHealth);
    DOREPLIFETIME(UHealthComponent, Health);
}

// 当Health属性在网络上复制时调用
void UHealthComponent::OnRep_Health()
{
    // 计算健康值百分比
    float HealthPercent = MaxHealth > 0.0f ? Health / MaxHealth : 0.0f;
    UE_LOG(LogSevenDaysToAlive, Log, TEXT("[HealthComponent] 网络复制回调，当前健康值: %.2f, 最大健康值: %.2f, 百分比: %.2f, 广播事件"), Health, MaxHealth, HealthPercent);
    
    // 广播健康值变化事件
    OnHealthChanged.Broadcast(HealthPercent);
}
