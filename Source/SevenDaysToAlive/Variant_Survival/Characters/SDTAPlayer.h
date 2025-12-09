// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "SDTAPlayer.generated.h"

UCLASS()
class SEVENDAYSTOALIVE_API ASDTAPlayer : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ASDTAPlayer();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// 网络复制相关
	// 重写GetLifetimeReplicatedProps方法来设置需要复制的属性
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 示例：需要在网络上复制的健康值
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Character Stats")
	float Health;

	// 示例：需要在网络上复制的能量值
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Character Stats")
	float Stamina;

	// RPC方法示例
	// 服务器端执行的方法（客户端调用）
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SetHealth(float NewHealth);
	void Server_SetHealth_Implementation(float NewHealth);
	bool Server_SetHealth_Validate(float NewHealth);

	// 客户端执行的方法（服务器调用）
	UFUNCTION(Client, Reliable)
	void Client_UpdateHUD(float NewHealth, float NewStamina);
	void Client_UpdateHUD_Implementation(float NewHealth, float NewStamina);

	// 所有客户端执行的方法（服务器调用）
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlaySound(USoundBase* SoundToPlay);
	void Multicast_PlaySound_Implementation(USoundBase* SoundToPlay);

	// 网络角色检查辅助方法
	bool IsLocallyControlled() const;
	bool IsServer() const;
};
