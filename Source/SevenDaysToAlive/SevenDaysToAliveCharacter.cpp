// Copyright Epic Games, Inc. All Rights Reserved.

#include "SevenDaysToAliveCharacter.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "SevenDaysToAlive.h"

ASevenDaysToAliveCharacter::ASevenDaysToAliveCharacter()
{
	// Set size for collision capsule
	// 碰撞胶囊体大小设置
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);
	
	// Create the first person mesh that will be viewed only by this character's owner
	// 第一人称模型设置
	FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("First Person Mesh"));

	FirstPersonMesh->SetupAttachment(GetMesh());
	FirstPersonMesh->SetOnlyOwnerSee(true);
	FirstPersonMesh->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;
	FirstPersonMesh->SetCollisionProfileName(FName("NoCollision"));

	// Create the Camera Component	
	// 第一人称相机设置
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("First Person Camera"));
	FirstPersonCameraComponent->SetupAttachment(FirstPersonMesh, FName("head"));
	FirstPersonCameraComponent->SetRelativeLocationAndRotation(FVector(-2.8f, 5.89f, 0.0f), FRotator(0.0f, 90.0f, -90.0f));
	FirstPersonCameraComponent->bUsePawnControlRotation = true;
	FirstPersonCameraComponent->bEnableFirstPersonFieldOfView = true;
	FirstPersonCameraComponent->bEnableFirstPersonScale = true;
	FirstPersonCameraComponent->FirstPersonFieldOfView = 70.0f;
	FirstPersonCameraComponent->FirstPersonScale = 0.6f;

	// configure the character components
	// 角色组件配置
	GetMesh()->SetOwnerNoSee(true);
	GetMesh()->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::WorldSpaceRepresentation;

	GetCapsuleComponent()->SetCapsuleSize(34.0f, 96.0f);

	// Configure character movement
	// 角色移动配置
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;
	GetCharacterMovement()->AirControl = 0.5f;
}

void ASevenDaysToAliveCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{	
	// Set up action bindings
	// 输入绑定配置
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Jumping
		// 跳跃绑定
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ASevenDaysToAliveCharacter::DoJumpStart);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ASevenDaysToAliveCharacter::DoJumpEnd);

		// Moving
		// 移动绑定
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ASevenDaysToAliveCharacter::MoveInput);

		// Looking/Aiming
		// 瞄准绑定
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ASevenDaysToAliveCharacter::LookInput);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &ASevenDaysToAliveCharacter::LookInput);
	}
	else
	{
		UE_LOG(LogSevenDaysToAlive, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}


void ASevenDaysToAliveCharacter::MoveInput(const FInputActionValue& Value)
{
	// get the Vector2D move axis
	// 移动轴值获取
	FVector2D MovementVector = Value.Get<FVector2D>();

	// pass the axis values to the move input
	// 移动输入处理
	DoMove(MovementVector.X, MovementVector.Y);

}

void ASevenDaysToAliveCharacter::LookInput(const FInputActionValue& Value)
{
	// get the Vector2D look axis
	// 瞄准轴值获取
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// pass the axis values to the aim input
	// 瞄准输入处理
	DoAim(LookAxisVector.X, LookAxisVector.Y);

}

void ASevenDaysToAliveCharacter::DoAim(float Yaw, float Pitch)
{
	if (GetController())
	{
		// pass the rotation inputs to the controller
		// 传递旋转输入到控制器
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void ASevenDaysToAliveCharacter::DoMove(float Right, float Forward)
{
	if (GetController())
	{
		// pass the move inputs to the controller
		// 传递移动输入到控制器
		AddMovementInput(GetActorRightVector(), Right);
		AddMovementInput(GetActorForwardVector(), Forward);
	}
}

void ASevenDaysToAliveCharacter::DoJumpStart()
{
	// pass Jump to the character
	// 传递跳跃输入到角色
	Jump();
}

void ASevenDaysToAliveCharacter::DoJumpEnd()
{
	// pass StopJumping to the character
	// 传递停止跳跃输入到角色
	StopJumping();
}
