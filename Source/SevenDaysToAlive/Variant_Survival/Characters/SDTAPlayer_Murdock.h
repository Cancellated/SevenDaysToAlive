// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Variant_Survival/Characters/SDTAPlayerBase.h"
#include "SDTAPlayer_Murdock.generated.h"

/**
 * Murdock角色类，继承自SDTAPlayerBase
 */
UCLASS()
class SEVENDAYSTOALIVE_API ASDTAPlayer_Murdock : public ASDTAPlayerBase
{
	GENERATED_BODY()

public:
	/** 构造函数 */
	ASDTAPlayer_Murdock();

protected:
	/** 游戏开始时调用 */
	virtual void BeginPlay() override;

public:
	/** 每帧调用 */
	virtual void Tick(float DeltaTime) override;
};
