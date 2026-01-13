// Fill out your copyright notice in the Description page of Project Settings.


#include "Variant_Survival/Controller/SDTAPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "Variant_Survival/Characters/SDTAPlayerBase.h"
#include "Variant_Survival/Components/HealthComponent.h"
#include "Variant_Survival/Core/Game/SDTAGameState.h"
#include "Variant_Survival/Core/Game/SDTAPlayerState.h"
#include "SevenDaysToAlive.h"
#include "Widgets/Input/SVirtualJoystick.h"
#include "Components/LightComponent.h"
#include "Components/SkyAtmosphereComponent.h"

// Sets default values
ASDTAPlayerController::ASDTAPlayerController()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ASDTAPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// only spawn UI on local player controllers
	if (IsLocalPlayerController())
	{
		// spawn the player HUD
		PlayerHUD = CreateWidget<USDTAPlayerHUD>(this, PlayerHUDClass);

		if (PlayerHUD)
		{
			PlayerHUD->AddToPlayerScreen(0);
			UE_LOG(LogSevenDaysToAlive, Log, TEXT("玩家HUD已添加到视口"));
		}
		else
		{
			UE_LOG(LogSevenDaysToAlive, Error, TEXT("无法创建玩家HUD"));
		}

		// 初始化跟踪整数变量
		LastHealthInt = -1;
		LastMaxHealthInt = -1;
		LastStaminaInt = -1;
		LastMaxStaminaInt = -1;

		// 创建DebugUI（如果指定了UI类）
		if (DebugUIWidgetClass)
		{
			DebugUIWidget = CreateWidget<USDTADebugUI>(this, DebugUIWidgetClass);
			if (DebugUIWidget)
			{
				DebugUIWidget->AddToPlayerScreen(1); // 放在更高层级，确保能看到
				UE_LOG(LogSevenDaysToAlive, Log, TEXT("调试UI已添加到视口"));
			}
			else
			{
				UE_LOG(LogSevenDaysToAlive, Warning, TEXT("无法创建调试UI"));
			}
		}

		// 创建武器UI（如果指定了UI类）
		if (WeaponUIWidgetClass)
		{
			WeaponUI = CreateWidget<USDTAWeaponUI>(this, WeaponUIWidgetClass);
			if (WeaponUI)
			{
				WeaponUI->AddToPlayerScreen(0); // 与HUD同一层级
				UE_LOG(LogSevenDaysToAlive, Log, TEXT("武器UI已添加到视口"));
			}
			else
			{
				UE_LOG(LogSevenDaysToAlive, Warning, TEXT("无法创建武器UI"));
			}
		}
	}
}

// Called to bind functionality to input
void ASDTAPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// add the input mapping contexts
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!ShouldUseTouchControls())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}
	}
}

/** 处理冲刺输入 */
void ASDTAPlayerController::HandleDashInput()
{
	ASDTAPlayerBase* SDTAPlayer = GetControlledSDTAPlayer();
	if (SDTAPlayer)
	{
		SDTAPlayer->DoDashStart();
	}
}

void ASDTAPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
}

void ASDTAPlayerController::OnUnPossess()
{
	Super::OnUnPossess();
}

/**
 * 当Actor结束生命周期时调用，用于清理资源和委托
 * @param EndPlayReason 结束游戏的原因
 */
void ASDTAPlayerController::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	// 调用父类的EndPlay方法
	Super::EndPlay(EndPlayReason);
}



void ASDTAPlayerController::OnHealthChanged(float HealthPercent)
{
	// 更新健康值UI
	if (PlayerHUD)
	{
		ASDTAPlayerBase* SDTAPlayer = GetControlledSDTAPlayer();
		if (SDTAPlayer && SDTAPlayer->HealthComponent)
		{
			// 强制更新健康值百分比
			PlayerHUD->HealthPercent = HealthPercent;
			
			// 更新健康值文本显示
			int32 CurrentHealthInt = FMath::RoundToInt(SDTAPlayer->HealthComponent->Health);
			int32 MaxHealthInt = FMath::RoundToInt(SDTAPlayer->HealthComponent->MaxHealth);
			
			// 强制更新UI，不依赖于值的变化
			PlayerHUD->CurrentHealth = CurrentHealthInt;
			PlayerHUD->MaxHealth = MaxHealthInt;
			
			// 更新跟踪变量
			LastHealthInt = CurrentHealthInt;
			LastMaxHealthInt = MaxHealthInt;
			
			// 直接调用蓝图实现的更新方法，确保UI更新
			PlayerHUD->BP_UpdateHealthBar(HealthPercent);
		}
	}
}

void ASDTAPlayerController::OnStaminaChanged(float StaminaPercent)
{
	// 更新能量值UI
	if (PlayerHUD)
	{
		ASDTAPlayerBase* SDTAPlayer = GetControlledSDTAPlayer();
		if (SDTAPlayer && SDTAPlayer->StaminaComponent)
		{
			// 强制更新能量值百分比
			PlayerHUD->StaminaPercent = StaminaPercent;
			
			// 更新能量值文本显示
			int32 CurrentStaminaInt = FMath::RoundToInt(SDTAPlayer->StaminaComponent->Stamina);
			int32 MaxStaminaInt = FMath::RoundToInt(SDTAPlayer->StaminaComponent->MaxStamina);
			
			// 强制更新UI，不依赖于值的变化
			PlayerHUD->CurrentStamina = CurrentStaminaInt;
			PlayerHUD->MaxStamina = MaxStaminaInt;
			
			// 更新跟踪变量
			LastStaminaInt = CurrentStaminaInt;
			LastMaxStaminaInt = MaxStaminaInt;
			
			// 直接调用蓝图实现的更新方法，确保UI更新
			PlayerHUD->BP_UpdateStaminaBar(StaminaPercent);
		}
	}
}

void ASDTAPlayerController::OnPawnDeath()
{
	// 处理角色死亡时的UI逻辑
	if (PlayerHUD)
	{
		PlayerHUD->BP_ShowDeathScreen();
	}
}



void ASDTAPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 更新DebugUI中的速度显示
	if (DebugUIWidget)
	{
		ASDTAPlayerBase* SDTAPlayer = GetControlledSDTAPlayer();
		if (SDTAPlayer)
		{
			// 获取角色当前速度（使用Velocity的大小）
			float CurrentSpeed = SDTAPlayer->GetVelocity().Size();
			DebugUIWidget->UpdateSpeed(CurrentSpeed);
		}
	}

	// 从GameState获取游戏状态数据并更新UI
	if (PlayerHUD)
	{
		ASDTAGameState* GameState = GetSDTAGameState();
		if (GameState)
		{
			// 更新昼夜和天数
			PlayerHUD->bIsNight = GameState->bIsNight;
			PlayerHUD->CurrentDay = GameState->CurrentDay;
			PlayerHUD->RemainingTime = GameState->RemainingTime;
			PlayerHUD->TimePercent = GameState->TimePercent;
			
			// 从PlayerState获取玩家个人数据
			ASDTAPlayerState* SDTAPlayerState = GetSDTAPlayerState();
			if (SDTAPlayerState)
			{
				// 更新玩家个人灵魂碎片显示
				PlayerHUD->SoulFragments = SDTAPlayerState->PlayerSoulFragments;
			}
			else
			{
				// 如果没有PlayerState，使用全局灵魂碎片
				PlayerHUD->SoulFragments = GameState->GlobalSoulFragments;
			}
			
			// 触发UI更新
			PlayerHUD->BP_UpdateDayNightCycle(GameState->bIsNight, GameState->CurrentDay, GameState->RemainingTime, GameState->TimePercent);
			PlayerHUD->BP_UpdateSoulFragments();
		}
	}

	// 客户端环境同步
	if (IsLocalPlayerController())
	{
		UpdateClientEnvironment();
	}
}

ASDTAPlayerBase* ASDTAPlayerController::GetControlledSDTAPlayer() const
{
	return Cast<ASDTAPlayerBase>(GetPawn());
}

/**
 * 获取SDTA GameState
 * 
 * @return SDTA GameState实例
 */
ASDTAGameState* ASDTAPlayerController::GetSDTAGameState() const
{
	return GetWorld()->GetGameState<ASDTAGameState>();
}

/**
 * 获取SDTA PlayerState
 * 
 * @return SDTA PlayerState实例
 */
ASDTAPlayerState* ASDTAPlayerController::GetSDTAPlayerState() const
{
	return Cast<ASDTAPlayerState>(PlayerState);
}

void ASDTAPlayerController::UpdateWeaponCounterUI(int32 CurrentAmmo, int32 MagazineSize)
{
	if (WeaponUI)
	{
		// 直接设置属性，属性变化会自动触发回调更新UI
		WeaponUI->CurrentAmmo = CurrentAmmo;
		WeaponUI->MagazineSize = MagazineSize;
	}
}

void ASDTAPlayerController::ShowHitFeedback()
{
	if (WeaponUI)
	{
		WeaponUI->ShowHitFeedback();
	}
}

/**
 * 开始本地计时
 * 
 * 功能：在客户端本地开始计时，用于昼夜循环进度条
 * 实现细节：
 * - 设置当前是否为夜晚阶段
 * - 设置总持续时间
 * - 重置已用时间
 * - 输出调试日志
 * 
 * @param InIsNightPhase 是否为夜晚阶段
 * @param TotalDuration 总持续时间（秒）
 */
void ASDTAPlayerController::StartLocalTiming(bool InIsNightPhase, float TotalDuration)
{
	// 设置本地计时变量
	bIsNightPhase = InIsNightPhase;
	this->TotalPhaseDuration = TotalDuration;
	this->ElapsedPhaseTime = 0.0f;
	
	// 输出调试日志
	UE_LOG(LogTemp, Log, TEXT("开始本地计时：%s，持续时间：%.2f秒"), InIsNightPhase ? TEXT("夜晚") : TEXT("白天"), TotalDuration);
}

/**
 * 更新本地计时
 * 
 * 功能：每帧更新本地计时，用于昼夜循环进度条
 * 实现细节：
 * - 累加已用时间
 * - 计算剩余时间和百分比
 * - 更新HUD的属性
 * - 触发UI更新
 * 
 * @param DeltaTime 每帧的时间间隔（秒）
 */
void ASDTAPlayerController::UpdateLocalTiming(float DeltaTime)
{
	// 如果总持续时间为0，直接返回
	if (TotalPhaseDuration <= 0.0f)
	{
		return;
	}
	
	// 累加已用时间，确保不超过总持续时间
	ElapsedPhaseTime += DeltaTime;
	ElapsedPhaseTime = FMath::Min(ElapsedPhaseTime, TotalPhaseDuration);
	
	// 计算剩余时间和百分比
	float RemainingTime = GetRemainingTime();
	float TimePercent = CalculateTimePercent();
	
	// 更新HUD的属性
	if (PlayerHUD)
	{
		PlayerHUD->bIsNight = bIsNightPhase;
		PlayerHUD->RemainingTime = RemainingTime;
		PlayerHUD->TimePercent = TimePercent;
		
		// 手动触发属性变化回调，确保蓝图中的绑定能够更新
		PlayerHUD->OnTimePercentChanged();
	}
}

/**
 * 计算剩余时间
 * 
 * 功能：计算当前阶段的剩余时间
 * 
 * @return 剩余时间（秒）
 */
float ASDTAPlayerController::GetRemainingTime() const
{
	return FMath::Max(0.0f, TotalPhaseDuration - ElapsedPhaseTime);
}

/**
 * 计算时间百分比
 * 
 * 功能：计算当前阶段的时间百分比，用于进度条
 * 
 * @return 时间百分比（0.0-1.0）
 */
float ASDTAPlayerController::CalculateTimePercent() const
{
	if (TotalPhaseDuration <= 0.0f)
	{
		return 0.0f;
	}
	
	return ElapsedPhaseTime / TotalPhaseDuration;
}

/**
 * 获取PlayerHUD实例
 * 
 * @return PlayerHUD实例指针
 */
USDTAPlayerHUD* ASDTAPlayerController::GetPlayerHUD() const
{
	return PlayerHUD;
}

/**
 * 客户端环境同步方法
 * 用于在客户端根据GameState更新昼夜环境效果
 */
void ASDTAPlayerController::UpdateClientEnvironment()
{
	// 获取GameState
	ASDTAGameState* GameState = GetSDTAGameState();
	if (!GameState || !GetWorld())
	{
		return;
	}

	// 获取昼夜状态和过渡状态
	bool bIsNight = GameState->bIsNight;
	bool bIsTransitioning = GameState->bIsTransitioning;
	bool bTransitionToNight = GameState->bTransitionToNight;
	float TransitionProgress = GameState->TransitionProgress;

	// 1. 更新光源
	TArray<AActor*> LightActors;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("WorldLight"), LightActors);

	// 从GameState获取光源配置
	float DayLightIntensity = GameState->DayLightIntensity;
	float NightLightIntensity = GameState->NightLightIntensity;
	FLinearColor DayLightColor = GameState->DayLightColor;
	FLinearColor NightLightColor = GameState->NightLightColor;

	// 计算目标值
	float TargetIntensity = DayLightIntensity;
	FLinearColor TargetColor = DayLightColor;

	if (bIsTransitioning)
	{
		// 过渡期间使用插值
		float StartIntensity = bTransitionToNight ? DayLightIntensity : NightLightIntensity;
		float EndIntensity = bTransitionToNight ? NightLightIntensity : DayLightIntensity;
		FLinearColor StartColor = bTransitionToNight ? DayLightColor : NightLightColor;
		FLinearColor EndColor = bTransitionToNight ? NightLightColor : DayLightColor;

		TargetIntensity = FMath::Lerp(StartIntensity, EndIntensity, TransitionProgress);
		TargetColor = FLinearColor::LerpUsingHSV(StartColor, EndColor, TransitionProgress);
	}
	else
	{
		// 非过渡期间直接使用目标值
		TargetIntensity = bIsNight ? NightLightIntensity : DayLightIntensity;
		TargetColor = bIsNight ? NightLightColor : DayLightColor;
	}

	// 更新所有光源
	for (AActor* Actor : LightActors)
	{
		if (!Actor) continue;

		TArray<ULightComponent*> LightComponents;
		Actor->GetComponents<ULightComponent>(LightComponents);

		for (ULightComponent* Light : LightComponents)
		{
			if (Light)
			{
				Light->SetIntensity(TargetIntensity);
				Light->SetLightColor(TargetColor);
			}
		}
	}

	// 2. 更新大气效果
	TArray<AActor*> AtmosphereActors;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("WorldAtmosphere"), AtmosphereActors);

	// 从GameState获取大气配置
	FLinearColor DayAtmosphereColor = GameState->DayAtmosphereColor;
	FLinearColor NightAtmosphereColor = GameState->NightAtmosphereColor;

	// 计算目标大气颜色
	FLinearColor TargetAtmosphereColor = DayAtmosphereColor;

	if (bIsTransitioning)
	{
		// 过渡期间使用插值
		FLinearColor StartAtmosphereColor = bTransitionToNight ? DayAtmosphereColor : NightAtmosphereColor;
		FLinearColor EndAtmosphereColor = bTransitionToNight ? NightAtmosphereColor : DayAtmosphereColor;

		TargetAtmosphereColor = FLinearColor::LerpUsingHSV(StartAtmosphereColor, EndAtmosphereColor, TransitionProgress);
	}
	else
	{
		// 非过渡期间直接使用目标值
		TargetAtmosphereColor = bIsNight ? NightAtmosphereColor : DayAtmosphereColor;
	}

	// 更新所有大气组件
	for (AActor* Actor : AtmosphereActors)
	{
		if (!Actor) continue;

		TArray<UActorComponent*> Components;
		Actor->GetComponents(USkyAtmosphereComponent::StaticClass(), Components);

		for (UActorComponent* Component : Components)
		{
			if (USkyAtmosphereComponent* AtmosphereComp = Cast<USkyAtmosphereComponent>(Component))
			{
				AtmosphereComp->RayleighScattering = TargetAtmosphereColor;
			}
		}
	}
}

