// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Variant_Survival/Core/Pool/SDTAPoolManager.h"
#include "WorkStation.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UUserWidget;
class USDTAPoolManager;

/**
 * 工作台类，用于白天升级交互
 * 
 * 核心功能：
 * 1. 白天出现，夜晚消失
 * 2. 玩家交互触发升级UI
 * 3. 支持对象池管理
 * 4. 提供交互反馈和视觉效果
 */
UCLASS()
class SEVENDAYSTOALIVE_API AWorkStation : public AActor
{
	GENERATED_BODY()
	
public:
	// Sets default values for this actor's properties
	AWorkStation();

	/**
	 * 交互方法，由玩家触发
	 * @param InteractingPawn 交互的 pawn
	 */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void Interact(APawn* InteractingPawn);

	/**
	 * 设置昼夜状态
	 * @param bIsDay 是否为白天
	 */
	UFUNCTION(BlueprintCallable, Category = "DayNight")
	void SetDayNightState(bool bIsDay);

	/**
	 * 设置对象池管理器
	 * @param InPoolManager 对象池管理器
	 */
	UFUNCTION(BlueprintCallable, Category = "Pooling")
	void SetPoolManager(USDTAPoolManager* InPoolManager);

	/**
	 * 重置工作台状态
	 * 用于对象池回收时重置状态
	 */
	UFUNCTION(BlueprintCallable, Category = "Pooling")
	void Reset();

	/**
	 * 获取对象池管理器实例
	 * @return 对象池管理器实例，如果不存在则返回nullptr
	 */
	UFUNCTION(BlueprintCallable, Category = "Pooling")
	USDTAPoolManager* GetPoolManager() const;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called when the game ends or when destroyed
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	/** 升级UI类 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UUserWidget> UpgradeUIClass;

protected:
	/** 工作台网格组件 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* WorkbenchMesh;

	/** 交互检测球体组件 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* InteractionSphere;

	/** 是否可交互 */
	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsInteractable;

	/** 对象池管理器引用 */
	UPROPERTY()
	USDTAPoolManager* PoolManager;

private:
	/** 处理玩家进入交互范围 */
	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** 处理玩家离开交互范围 */
	UFUNCTION()
	void OnEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	/** 昼夜状态变化回调 */
	UFUNCTION()
	void OnDayNightStateChanged(bool bIsNowNight);

};
