// Fill out your copyright notice in the Description page of Project Settings.

#include "Variant_Survival/Characters/SDTAPlayerBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Variant_Survival/Components/HealthComponent.h"
#include "Variant_Survival/Components/StaminaComponent.h"
#include "Variant_Survival/Components/DashComponent.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"

// 设置默认值
ASDTAPlayerBase::ASDTAPlayerBase()
{
	// 设置此角色每一帧调用Tick()
	PrimaryActorTick.bCanEverTick = true;

	if (GetCharacterMovement())
	{
		// 设置旋转速度
		GetCharacterMovement()->RotationRate = FRotator(0.0f, 600.0f, 0.0f);
	}

	// 创建健康组件
	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
	UE_LOG(LogTemp, Log, TEXT("生命组件创建成功"));

	// 创建耐力组件
	StaminaComponent = CreateDefaultSubobject<UStaminaComponent>(TEXT("StaminaComponent"));
	UE_LOG(LogTemp, Log, TEXT("耐力组件创建成功"));

	// 创建冲刺组件
	DashComponent = CreateDefaultSubobject<UDashComponent>(TEXT("DashComponent"));
	UE_LOG(LogTemp, Log, TEXT("冲刺组件创建成功"));
}

// Called when the game starts or when spawned
void ASDTAPlayerBase::BeginPlay()
{
	Super::BeginPlay();

	// 绑定健康组件的委托
	if (HealthComponent)
	{
		// 绑定健康值变化委托
		HealthComponent->OnHealthChanged.AddDynamic(this, &ASDTAPlayerBase::OnHealthComponentChanged);
		
		// 绑定死亡委托
		HealthComponent->OnDeath.AddDynamic(this, &ASDTAPlayerBase::OnHealthComponentDeath);
	}

	// 绑定耐力组件的委托
	if (StaminaComponent)
	{
		// 绑定耐力值变化委托
		StaminaComponent->OnStaminaChanged.AddDynamic(this, &ASDTAPlayerBase::OnStaminaComponentChanged);
	}

	// 初始化冲刺组件
	if (DashComponent && StaminaComponent)
	{
		// 设置冲刺组件的耐力组件引用
		DashComponent->SetStaminaComponent(StaminaComponent);
		UE_LOG(LogTemp, Log, TEXT("冲刺组件初始化成功，已设置耐力组件引用"));
	}
}

// Called every frame
void ASDTAPlayerBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// 绑定玩家输入组件
void ASDTAPlayerBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// 添加冲刺输入绑定
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (DashAction)
		{
			EnhancedInputComponent->BindAction(DashAction, ETriggerEvent::Started, this, &ASDTAPlayerBase::DoDashStart);
			UE_LOG(LogTemp, Log, TEXT("冲刺输入绑定成功"));
		}
	}
}

/** 处理瞄准输入 */
void ASDTAPlayerBase::DoAim(float Yaw, float Pitch)
{
	if (GetController())
	{
		// 使用Character的方法处理旋转输入
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

/** 处理移动输入 */
void ASDTAPlayerBase::DoMove(float Right, float Forward)
{
	if (GetController())
	{
		// 应用移动输入
		AddMovementInput(GetActorRightVector(), Right);
		AddMovementInput(GetActorForwardVector(), Forward);
	}
}

/** 处理跳跃开始输入 */
void ASDTAPlayerBase::DoJumpStart()
{
	// 开始跳跃
	Jump();
}

/** 处理跳跃结束输入 */
void ASDTAPlayerBase::DoJumpEnd()
{
	// 结束跳跃
	StopJumping();
}

/** 处理冲刺输入 */
void ASDTAPlayerBase::DoDashStart()
{
	// 检查冲刺组件是否有效
	if (DashComponent)
	{
		// 开始冲刺
		DashComponent->StartDash();
	}
}

/** 健康值变化回调 */
void ASDTAPlayerBase::OnHealthComponentChanged(float HealthPercent)
{
	// 广播健康值变化委托
	OnHealthChanged.Broadcast(HealthPercent);
}

/** 死亡回调 */
void ASDTAPlayerBase::OnHealthComponentDeath()
{
	// 广播死亡委托
	OnDeath.Broadcast();
}

/** 耐力值变化回调 */
void ASDTAPlayerBase::OnStaminaComponentChanged(float StaminaPercent)
{
	// 广播耐力值变化委托
	OnStaminaChanged.Broadcast(StaminaPercent);
}

/** 当Actor结束生命周期时调用 */
void ASDTAPlayerBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// 清理健康组件的委托绑定
	if (HealthComponent)
	{
		HealthComponent->OnHealthChanged.RemoveDynamic(this, &ASDTAPlayerBase::OnHealthComponentChanged);
		HealthComponent->OnDeath.RemoveDynamic(this, &ASDTAPlayerBase::OnHealthComponentDeath);
	}

	// 清理耐力组件的委托绑定
	if (StaminaComponent)
	{
		StaminaComponent->OnStaminaChanged.RemoveDynamic(this, &ASDTAPlayerBase::OnStaminaComponentChanged);
	}
}
