// Fill out your copyright notice in the Description page of Project Settings.

#include "HealthComponent.h"

// Sets default values for this component's properties
UHealthComponent::UHealthComponent()
{
    // Set this component to be initialized when the game starts, and to be ticked every frame.
    PrimaryComponentTick.bCanEverTick = true;

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
