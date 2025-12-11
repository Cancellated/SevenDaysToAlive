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
			if (!SVirtualJoystick::ShouldDisplayTouchInterface())
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
		SDTAPlayer->OnHealthChanged.Broadcast(SDTAPlayer->Health / SDTAPlayer->MaxHealth);
		SDTAPlayer->OnStaminaChanged.Broadcast(SDTAPlayer->Stamina / SDTAPlayer->MaxStamina);
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
		PlayerHUD->HealthPercent = HealthPercent;
		// BP_UpdateHealthBar会通过OnHealthPercentChanged自动调用
	}
}

void ASDTAPlayerController::OnStaminaChanged(float StaminaPercent)
{
	// 更新能量值UI
	if (PlayerHUD)
	{
		PlayerHUD->StaminaPercent = StaminaPercent;
		// BP_UpdateStaminaBar会通过OnStaminaPercentChanged自动调用
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

bool ASDTAPlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}

ASDTAPlayer* ASDTAPlayerController::GetControlledSDTAPlayer() const
{
	return Cast<ASDTAPlayer>(GetPawn());
}

