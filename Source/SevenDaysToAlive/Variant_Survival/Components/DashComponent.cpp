// Fill out your copyright notice in the Description page of Project Settings.


#include "Variant_Survival/Components/DashComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "SevenDaysToAlive.h"

// Sets default values for this component's properties
UDashComponent::UDashComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// 启用组件的网络复制
	SetIsReplicated(true);

	// 初始化冲刺相关属性
	bIsDashing = false;
	DashSpeedMultiplier = 2.0f; // 2倍移动速度
	DashStaminaCost = 30.0f; // 冲刺消耗30点体力
	DashDuration = 0.5f; // 冲刺持续0.5秒
	DashCooldown = 1.0f; // 冲刺冷却1秒
	LastDashTime = 0.0f;
	OriginalMaxWalkSpeed = 0.0f;

	UE_LOG(LogSevenDaysToAlive, Log, TEXT("[DashComponent] 初始化: 速度倍率=%.2f, 消耗=%.2f, 持续=%.2f, 冷却=%.2f"), 
		DashSpeedMultiplier, DashStaminaCost, DashDuration, DashCooldown);
}

// Called when the game starts
void UDashComponent::BeginPlay()
{
	Super::BeginPlay();

	// 初始化原始移动速度
	if (GetCharacterMovement())
	{
		OriginalMaxWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;
		UE_LOG(LogSevenDaysToAlive, Log, TEXT("[DashComponent] 原始移动速度已保存: %.2f"), OriginalMaxWalkSpeed);
	}
}

// Called every frame
void UDashComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 可以在这里添加冲刺相关的每帧逻辑
}

// 网络复制相关
void UDashComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 冲刺相关属性的网络复制
	DOREPLIFETIME(UDashComponent, bIsDashing);
	DOREPLIFETIME(UDashComponent, LastDashTime);
	DOREPLIFETIME(UDashComponent, OriginalMaxWalkSpeed);
}

// 开始冲刺
void UDashComponent::StartDash()
{
	UE_LOG(LogSevenDaysToAlive, Log, TEXT("[DashComponent] 开始冲刺请求"));

	// 如果是客户端，请求服务器开始冲刺
	if (!IsServer())
	{
		Server_StartDash();
		return;
	}

	// 服务器端直接执行
	Server_StartDash_Implementation();
}

// 结束冲刺
void UDashComponent::EndDash()
{
	UE_LOG(LogSevenDaysToAlive, Log, TEXT("[DashComponent] 结束冲刺请求"));

	// 如果是客户端，请求服务器结束冲刺
	if (!IsServer())
	{
		Server_EndDash();
		return;
	}

	// 服务器端直接执行
	Server_EndDash_Implementation();
}

// 检查是否可以冲刺
bool UDashComponent::CanDash() const
{
	if (bIsDashing)
	{
		UE_LOG(LogSevenDaysToAlive, Log, TEXT("[DashComponent] 不能冲刺: 已在冲刺中"));
		return false;
	}

	float CurrentTime = GetWorld()->GetTimeSeconds();
	float TimeSinceLastDash = CurrentTime - LastDashTime;
	if (TimeSinceLastDash < DashCooldown)
	{
		UE_LOG(LogSevenDaysToAlive, Log, TEXT("[DashComponent] 不能冲刺: 冷却中 (%.2f < %.2f)"), TimeSinceLastDash, DashCooldown);
		return false;
	}

	if (!StaminaComponent)
	{
		UE_LOG(LogSevenDaysToAlive, Log, TEXT("[DashComponent] 不能冲刺: 耐力组件无效"));
		return false;
	}

	// 检查耐力是否足够
	float CurrentStamina = StaminaComponent->Stamina;
	if (CurrentStamina < DashStaminaCost)
	{
		UE_LOG(LogSevenDaysToAlive, Log, TEXT("[DashComponent] 不能冲刺: 耐力不足 (%.2f < %.2f)"), CurrentStamina, DashStaminaCost);
		return false;
	}

	UE_LOG(LogSevenDaysToAlive, Log, TEXT("[DashComponent] 可以冲刺: 耐力充足 (%.2f >= %.2f), 冷却结束 (%.2f >= %.2f)"), 
		CurrentStamina, DashStaminaCost, TimeSinceLastDash, DashCooldown);
	return true;
}

// 服务器端开始冲刺实现
void UDashComponent::Server_StartDash_Implementation()
{
	UE_LOG(LogSevenDaysToAlive, Log, TEXT("[DashComponent] 服务器端开始冲刺"));

	// 检查是否可以冲刺
	if (!CanDash())
	{
		// UE_LOG(LogSevenDaysToAlive, Warning, TEXT("[DashComponent] 冲刺失败: 条件不满足"));
		return;
	}

	// 消耗体力
	if (!StaminaComponent->ConsumeStamina(DashStaminaCost))
	{
		UE_LOG(LogSevenDaysToAlive, Warning, TEXT("[DashComponent] 冲刺失败: 耐力不足，需要 %.2f"), DashStaminaCost);
		return;
	}

	UE_LOG(LogSevenDaysToAlive, Log, TEXT("[DashComponent] 冲刺开始，消耗 %.2f 耐力，速度倍率 %.2f"), 
		DashStaminaCost, DashSpeedMultiplier);

	// 设置冲刺状态
	bIsDashing = true;
	LastDashTime = GetWorld()->GetTimeSeconds();

	// 应用冲刺速度
	if (GetCharacterMovement())
	{
		// 保存原始移动速度
		OriginalMaxWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;
		// 设置冲刺速度
		GetCharacterMovement()->MaxWalkSpeed *= DashSpeedMultiplier;
		UE_LOG(LogSevenDaysToAlive, Log, TEXT("[DashComponent] 移动速度: %.2f -> %.2f"), 
			OriginalMaxWalkSpeed, GetCharacterMovement()->MaxWalkSpeed);
	}

	// 设置冲刺结束定时器
	GetWorld()->GetTimerManager().SetTimer(FDashTimerHandle, this, &UDashComponent::EndDash, DashDuration, false);
	UE_LOG(LogSevenDaysToAlive, Log, TEXT("[DashComponent] 冲刺将在 %.2f 秒后结束"), DashDuration);
}

// 服务器端开始冲刺验证
bool UDashComponent::Server_StartDash_Validate()
{
	// 基本验证
	return true;
}

// 服务器端结束冲刺实现
void UDashComponent::Server_EndDash_Implementation()
{
	UE_LOG(LogSevenDaysToAlive, Log, TEXT("[DashComponent] 服务器端结束冲刺"));

	// 检查是否处于冲刺状态
	if (!bIsDashing)
	{
		UE_LOG(LogSevenDaysToAlive, Warning, TEXT("[DashComponent] 冲刺结束失败: 未在冲刺中"));
		return;
	}

	UE_LOG(LogSevenDaysToAlive, Log, TEXT("[DashComponent] 冲刺结束"));

	// 重置冲刺状态
	bIsDashing = false;

	// 恢复原始移动速度
	if (GetCharacterMovement())
	{
		// 使用保存的原始移动速度恢复
		GetCharacterMovement()->MaxWalkSpeed = OriginalMaxWalkSpeed;
		UE_LOG(LogSevenDaysToAlive, Log, TEXT("[DashComponent] 移动速度恢复: %.2f"), OriginalMaxWalkSpeed);
	}
}

// 服务器端结束冲刺验证
bool UDashComponent::Server_EndDash_Validate()
{
	// 基本验证
	return true;
}

// 初始化组件
void UDashComponent::SetStaminaComponent(class UStaminaComponent* InStaminaComponent)
{
	StaminaComponent = InStaminaComponent;
	UE_LOG(LogSevenDaysToAlive, Log, TEXT("[DashComponent] 组件已初始化，耐力组件: %s"), 
		StaminaComponent ? *StaminaComponent->GetName() : TEXT("无"));
}

// 获取拥有者的角色移动组件
class UCharacterMovementComponent* UDashComponent::GetCharacterMovement() const
{
	APawn* OwnerPawn = GetOwnerPawn();
	if (!OwnerPawn)
	{
		return nullptr;
	}

	return Cast<UCharacterMovementComponent>(OwnerPawn->GetMovementComponent());
}

// 获取拥有者
class APawn* UDashComponent::GetOwnerPawn() const
{
	return Cast<APawn>(GetOwner());
}

// 检查是否在服务器上
bool UDashComponent::IsServer() const
{
	APawn* OwnerPawn = GetOwnerPawn();
	if (!OwnerPawn)
	{
		return false;
	}

	return OwnerPawn->GetLocalRole() == ROLE_Authority;
}
