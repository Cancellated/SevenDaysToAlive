// Fill out your copyright notice in the Description page of Project Settings.


#include "Variant_Survival/Controller/SDTAPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "Variant_Survival/Characters/SDTAPlayer.h"
#include "SevenDaysToAlive.h"
#include "Widgets/Input/SVirtualJoystick.h"

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
		}
		else
		{
			UE_LOG(LogSevenDaysToAlive, Error, TEXT("Could not spawn player HUD widget."));
		}

		// spawn mobile controls if needed
		if (ShouldUseTouchControls())
		{
			// spawn the mobile controls widget
			MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

			if (MobileControlsWidget)
			{
				MobileControlsWidget->AddToPlayerScreen(0);
			}
			else
			{
				UE_LOG(LogSevenDaysToAlive, Error, TEXT("Could not spawn mobile controls widget."));
			}
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
			}
			else
			{
				UE_LOG(LogSevenDaysToAlive, Warning, TEXT("Could not spawn Debug UI widget."));
			}
		}

		// 创建武器UI（如果指定了UI类）
		if (WeaponUIWidgetClass)
		{
			WeaponUI = CreateWidget<USDTAWeaponUI>(this, WeaponUIWidgetClass);
			if (WeaponUI)
			{
				WeaponUI->AddToPlayerScreen(0); // 与HUD同一层级
				UE_LOG(LogSevenDaysToAlive, Log, TEXT("Weapon UI created successfully"));
			}
			else
			{
				UE_LOG(LogSevenDaysToAlive, Warning, TEXT("Could not spawn Weapon UI widget."));
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

void ASDTAPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// subscribe to the pawn's OnDestroyed delegate
	InPawn->OnDestroyed.AddDynamic(this, &ASDTAPlayerController::OnPawnDestroyed);

	// is this a SDTA character?
	if (ASDTAPlayer* SDTAPlayer = Cast<ASDTAPlayer>(InPawn))
	{
		// add the player tag
		SDTAPlayer->Tags.Add(PlayerPawnTag);

		// subscribe to the pawn's delegates
		SDTAPlayer->OnHealthChanged.AddDynamic(this, &ASDTAPlayerController::OnHealthChanged);
		SDTAPlayer->OnStaminaChanged.AddDynamic(this, &ASDTAPlayerController::OnStaminaChanged);
		SDTAPlayer->OnDeath.AddDynamic(this, &ASDTAPlayerController::OnPawnDeath);

		// force update the health and stamina bars
		float HealthPercent = SDTAPlayer->HealthComponent ? (SDTAPlayer->HealthComponent->Health / SDTAPlayer->HealthComponent->MaxHealth) : 0.0f;
		SDTAPlayer->OnHealthChanged.Broadcast(HealthPercent);

		float StaminaPercent = SDTAPlayer->StaminaComponent ? (SDTAPlayer->StaminaComponent->Stamina / SDTAPlayer->StaminaComponent->MaxStamina) : 0.0f;
		SDTAPlayer->OnStaminaChanged.Broadcast(StaminaPercent);
	}
}

void ASDTAPlayerController::OnUnPossess()
{
	// Get the pawn that was just unpossessed from the parameter
	// Note: In Unreal Engine, GetPawn() may still return the pawn during OnUnPossess
	// as the pawn is only cleared after this method returns
	APawn* ControlledPawn = GetPawn();

	// If we have a reference to the pawn
	if (ControlledPawn)
	{
		// Unsubscribe from the pawn's OnDestroyed delegate
		ControlledPawn->OnDestroyed.RemoveDynamic(this, &ASDTAPlayerController::OnPawnDestroyed);

		// If it's a SDTA player character, unsubscribe from its delegates
		if (ASDTAPlayer* SDTAPlayer = Cast<ASDTAPlayer>(ControlledPawn))
		{
			// Unsubscribe from all pawn delegates
			SDTAPlayer->OnHealthChanged.RemoveDynamic(this, &ASDTAPlayerController::OnHealthChanged);
			SDTAPlayer->OnStaminaChanged.RemoveDynamic(this, &ASDTAPlayerController::OnStaminaChanged);
			SDTAPlayer->OnDeath.RemoveDynamic(this, &ASDTAPlayerController::OnPawnDeath);
		}
	}

	// Call the parent class's OnUnPossess method
	Super::OnUnPossess();
}

/**
 * 当Actor结束生命周期时调用，用于清理资源和委托
 * @param EndPlayReason 结束游戏的原因
 */
void ASDTAPlayerController::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	// 获取当前拥有的pawn
	APawn* ControlledPawn = GetPawn();

	// 如果我们还拥有一个pawn，确保移除所有委托绑定
	if (ControlledPawn)
	{
		// 移除OnDestroyed委托
		ControlledPawn->OnDestroyed.RemoveDynamic(this, &ASDTAPlayerController::OnPawnDestroyed);

		// 如果是SDTA玩家角色，移除所有相关委托
		if (ASDTAPlayer* SDTAPlayer = Cast<ASDTAPlayer>(ControlledPawn))
		{
			SDTAPlayer->OnHealthChanged.RemoveDynamic(this, &ASDTAPlayerController::OnHealthChanged);
			SDTAPlayer->OnStaminaChanged.RemoveDynamic(this, &ASDTAPlayerController::OnStaminaChanged);
			SDTAPlayer->OnDeath.RemoveDynamic(this, &ASDTAPlayerController::OnPawnDeath);
		}
	}

	// 调用父类的EndPlay方法
	Super::EndPlay(EndPlayReason);
}

void ASDTAPlayerController::OnPawnDestroyed(AActor* DestroyedActor)
{
	// 重置UI元素
	// 这里可以添加UI重置逻辑

	// 无论控制器是否还拥有该pawn，只要它是SDTA玩家角色，就解绑所有委托
	// 这确保了即使pawn在控制器不再拥有它后被销毁，委托也能被正确清理
	if (ASDTAPlayer* SDTAPlayer = Cast<ASDTAPlayer>(DestroyedActor))
	{
		// 解绑所有委托
		SDTAPlayer->OnHealthChanged.RemoveDynamic(this, &ASDTAPlayerController::OnHealthChanged);
		SDTAPlayer->OnStaminaChanged.RemoveDynamic(this, &ASDTAPlayerController::OnStaminaChanged);
		SDTAPlayer->OnDeath.RemoveDynamic(this, &ASDTAPlayerController::OnPawnDeath);
		
		// 确保从OnDestroyed事件中解绑，防止重复调用
		SDTAPlayer->OnDestroyed.RemoveDynamic(this, &ASDTAPlayerController::OnPawnDestroyed);
	}

	// find the player start
	TArray<AActor*> ActorList;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), ActorList);

	if (ActorList.Num() > 0)
	{
		// select a random player start
		AActor* RandomPlayerStart = ActorList[FMath::RandRange(0, ActorList.Num() - 1)];

		// spawn a character at the player start
		const FTransform SpawnTransform = RandomPlayerStart->GetActorTransform();

		if (ASDTAPlayer* RespawnedCharacter = GetWorld()->SpawnActor<ASDTAPlayer>(CharacterClass, SpawnTransform))
		{
			// possess the character
			Possess(RespawnedCharacter);
		}
	}
}

void ASDTAPlayerController::OnHealthChanged(float HealthPercent)
{
	// 更新健康值UI
	if (PlayerHUD)
	{
		ASDTAPlayer* SDTAPlayer = GetControlledSDTAPlayer();
		if (SDTAPlayer)
		{
			// 更新健康值百分比（总是更新，用于进度条）
			PlayerHUD->HealthPercent = HealthPercent;

			// 将当前值转换为整数
			int32 CurrentHealthInt = SDTAPlayer->HealthComponent ? FMath::RoundToInt(SDTAPlayer->HealthComponent->Health) : 0;
			int32 MaxHealthInt = SDTAPlayer->HealthComponent ? FMath::RoundToInt(SDTAPlayer->HealthComponent->MaxHealth) : 0;

			// 只有当整数部分变化时才更新HUD的当前值和最大值
			if (CurrentHealthInt != LastHealthInt || MaxHealthInt != LastMaxHealthInt)
			{
				PlayerHUD->CurrentHealth = CurrentHealthInt;
				PlayerHUD->MaxHealth = MaxHealthInt;

				// 更新跟踪变量
				LastHealthInt = CurrentHealthInt;
				LastMaxHealthInt = MaxHealthInt;
			}

			// BP_UpdateHealthBar会通过OnHealthPercentChanged自动调用
		}
	}
}

void ASDTAPlayerController::OnStaminaChanged(float StaminaPercent)
{
	// 更新能量值UI
	if (PlayerHUD)
	{
		ASDTAPlayer* SDTAPlayer = GetControlledSDTAPlayer();
		if (SDTAPlayer)
		{
			// 更新能量值百分比（总是更新，用于进度条）
			PlayerHUD->StaminaPercent = StaminaPercent;

			// 将当前值转换为整数
			int32 CurrentStaminaInt = SDTAPlayer->StaminaComponent ? FMath::RoundToInt(SDTAPlayer->StaminaComponent->Stamina) : 0;
			int32 MaxStaminaInt = SDTAPlayer->StaminaComponent ? FMath::RoundToInt(SDTAPlayer->StaminaComponent->MaxStamina) : 0;

			// 只有当整数部分变化时才更新HUD的当前值和最大值
			if (CurrentStaminaInt != LastStaminaInt || MaxStaminaInt != LastMaxStaminaInt)
			{
				PlayerHUD->CurrentStamina = CurrentStaminaInt;
				PlayerHUD->MaxStamina = MaxStaminaInt;

				// 更新跟踪变量
				LastStaminaInt = CurrentStaminaInt;
				LastMaxStaminaInt = MaxStaminaInt;
			}

			// BP_UpdateStaminaBar会通过OnStaminaPercentChanged自动调用
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
		ASDTAPlayer* SDTAPlayer = GetControlledSDTAPlayer();
		if (SDTAPlayer)
		{
			// 获取角色当前速度（使用Velocity的大小）
			float CurrentSpeed = SDTAPlayer->GetVelocity().Size();
			DebugUIWidget->UpdateSpeed(CurrentSpeed);
		}
	}
}

ASDTAPlayer* ASDTAPlayerController::GetControlledSDTAPlayer() const
{
	return Cast<ASDTAPlayer>(GetPawn());
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

