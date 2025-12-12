// Copyright Epic Games, Inc. All Rights Reserved.


#include "SevenDaysToAlivePlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "SevenDaysToAliveCameraManager.h"
#include "Blueprint/UserWidget.h"
#include "SevenDaysToAlive.h"
#include "Widgets/Input/SVirtualJoystick.h"

ASevenDaysToAlivePlayerController::ASevenDaysToAlivePlayerController()
{
	// set the player camera manager class
	PlayerCameraManagerClass = ASevenDaysToAliveCameraManager::StaticClass();
}

void ASevenDaysToAlivePlayerController::BeginPlay()
{
	Super::BeginPlay();

	
	// only spawn touch controls on local player controllers
	// 仅在本地玩家控制器上生成触摸控件
	if (ShouldUseTouchControls() && IsLocalPlayerController())
	{
		// spawn the mobile controls widget
		// 生成移动控件小部件
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			// 将移动控件添加到玩家屏幕
			MobileControlsWidget->AddToPlayerScreen(0);

		} else {

			UE_LOG(LogSevenDaysToAlive, Error, TEXT("Could not spawn mobile controls widget."));

		}

	}
}

void ASevenDaysToAlivePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// only add IMCs for local player controllers
	// 仅为本地玩家控制器添加输入映射上下文
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Context
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			// 仅在不使用移动触摸输入时添加这些IMC
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

bool ASevenDaysToAlivePlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	// 是否在移动平台上？是否强制使用触摸控件
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}
