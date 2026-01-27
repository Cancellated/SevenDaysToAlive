// Fill out your copyright notice in the Description page of Project Settings.

#include "Variant_SDTA/Components/StaminaComponent.h"
#include "TimerManager.h"
#include "SevenDaysToAlive.h"

// Sets default values for this component's properties
UStaminaComponent::UStaminaComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.
	PrimaryComponentTick.bCanEverTick = true;

	// 启用组件的网络复制
	SetIsReplicated(true);

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
        float NewStamina = FMath::Clamp(Stamina + (StaminaRegenerationRate * DeltaTime), 0.0f, MaxStamina);
        
        // 更新耐力值
        if (NewStamina != OldStamina)
        {
            SetStamina(NewStamina);
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
	// UE_LOG(LogSevenDaysToAlive, Log, TEXT("[StaminaComponent] 设置耐力值，旧值: %.2f, 新值: %.2f, 百分比: %.2f, 广播事件"), OldStamina, Stamina, StaminaPercent);
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

// 当Stamina属性在网络上复制时调用
void UStaminaComponent::OnRep_Stamina()
{
    // 计算耐力值百分比
    float StaminaPercent = MaxStamina > 0.0f ? Stamina / MaxStamina : 0.0f;
    // UE_LOG(LogSevenDaysToAlive, Log, TEXT("[StaminaComponent] 网络复制回调，当前耐力: %.2f, 最大耐力: %.2f, 百分比: %.2f, 广播事件"), Stamina, MaxStamina, StaminaPercent);
    
    // 广播耐力值变化事件
    OnStaminaChanged.Broadcast(StaminaPercent);
}
