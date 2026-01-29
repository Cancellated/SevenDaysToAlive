// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SDTAWeapon.h"
#include "SDTARifle.generated.h"

/**
 * 步枪武器子类
 * 用于验证武器系统的模型显示功能
 */
UCLASS()
class SEVENDAYSTOALIVE_API ASDTAWeaponRifle : public ASDTAWeapon
{
	GENERATED_BODY()
	
public:
	// Sets default values for this actor's properties
	ASDTAWeaponRifle();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// 实现开火方法，添加步枪特有的开火逻辑
	virtual void Fire();

	// 实现装填方法，添加步枪特有的装填逻辑
	virtual void Reload();

protected:
	// 步枪特有的属性
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rifle")
	float RifleRecoilMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rifle")
	float RifleFireRate;

	// 步枪开火音效
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rifle")
	class USoundBase* FireSound;

	// 步枪装填音效
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rifle")
	class USoundBase* ReloadSound;

	// 步枪开火粒子效果
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rifle")
	class UParticleSystem* MuzzleFlash;

private:
	// 内部方法
	void PlayFireEffects();
	void PlayReloadEffects();
};