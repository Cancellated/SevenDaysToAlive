// Fill out your copyright notice in the Description page of Project Settings.

#include "Variant_Survival/Components/StaminaComponent.h"
#include "TimerManager.h"

// Sets default values for this component's properties
UStaminaComponent::UStaminaComponent()
{
    // Set this component to be initialized when the game starts, and to be ticked every frame.
    PrimaryComponentTick.bCanEverTick = true;

    // 设置默认能量值
    MaxStamina = 100.0f;
    Stamina = MaxStamina;
    StaminaRegenerationRate = 5.0f;
    StaminaRegenerationDelay = 2.0f;
    bIsStaminaRegenerating = false;
}

// Called when the game starts
void UStaminaComponent::BeginPlay()
{
    Super::BeginPlay();

    // 初始化能量值
    Stamina = MaxStamina;
    bIsStaminaRegenerating = true;
}

// Called every frame
void UStaminaComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // 能量回复逻辑
    if (bIsStaminaRegenerating && Stamina < MaxStamina)
    {
        float OldStamina = Stamina;
        Stamina = FMath::Clamp(Stamina + (StaminaRegenerationRate * DeltaTime), 0.0f, MaxStamina);

        // 如果能量值发生变化，广播事件
        if (FMath::Abs(Stamina - OldStamina) > 0.01f)
        {
            float StaminaPercent = MaxStamina > 0.0f ? Stamina / MaxStamina : 0.0f;
            OnStaminaChanged.Broadcast(StaminaPercent);
        }
    }
}

// 设置能量值
void UStaminaComponent::SetStamina(float NewStamina)
{
    float OldStamina = Stamina;
    Stamina = FMath::Clamp(NewStamina, 0.0f, MaxStamina);

    // 广播能量值变化
    float StaminaPercent = MaxStamina > 0.0f ? Stamina / MaxStamina : 0.0f;
    OnStaminaChanged.Broadcast(StaminaPercent);

    // 检查是否为低能量值
    if (Stamina < MaxStamina * 0.3f && OldStamina >= MaxStamina * 0.3f)
    {
        OnStaminaLowWarning.Broadcast();
    }
}

// 增加能量值
void UStaminaComponent::AddStamina(float StaminaToAdd)
{
    SetStamina(Stamina + StaminaToAdd);
}

// 减少能量值
void UStaminaComponent::RemoveStamina(float StaminaToRemove)
{
    SetStamina(Stamina - StaminaToRemove);
}

// 消耗能量值
bool UStaminaComponent::ConsumeStamina(float StaminaCost)
{
    if (Stamina >= StaminaCost)
    {
        SetStamina(Stamina - StaminaCost);
        return true;
    }
    return false;
}

// 开始能量回复
void UStaminaComponent::StartStaminaRegeneration()
{
    bIsStaminaRegenerating = true;
}

// 停止能量回复
void UStaminaComponent::StopStaminaRegeneration()
{
    bIsStaminaRegenerating = false;
}

// 实现网络复制属性配置
void UStaminaComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    
    // 复制最大体力和当前体力值
    DOREPLIFETIME(UStaminaComponent, MaxStamina);
    DOREPLIFETIME(UStaminaComponent, Stamina);
    
    // 复制体力回复状态
    DOREPLIFETIME(UStaminaComponent, bIsStaminaRegenerating);
}
