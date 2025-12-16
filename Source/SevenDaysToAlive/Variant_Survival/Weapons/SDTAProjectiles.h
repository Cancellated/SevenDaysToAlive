// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Variant_Survival/Core/Pool/SDTAPoolManager.h"
#include "SDTAProjectiles.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class ACharacter;
class UPrimitiveComponent;
class USDTAPoolManager;

/** 弹道类型枚举 */
UENUM(BlueprintType)
enum class ESDTAProjectileType : uint8
{
	EntityProjectile	UMETA(DisplayName = "实体子弹"),
	InstantProjectile	UMETA(DisplayName = "即时弹道")
};

/**
 * 子弹基类，支持实体子弹和即时弹道两种机制
 */
UCLASS(abstract)
class SEVENDAYSTOALIVE_API ASDTAProjectiles : public AActor
{
	GENERATED_BODY()
	
	/** 提供碰撞检测的组件 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="组件", meta = (AllowPrivateAccess = "true"))
	USphereComponent* CollisionComponent;

	/** 处理子弹运动的组件 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="组件", meta = (AllowPrivateAccess = "true"))
	UProjectileMovementComponent* ProjectileMovement;

protected:
	/** 弹道类型 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="弹道", meta = (AllowPrivateAccess = "true"))
	ESDTAProjectileType ProjectileType = ESDTAProjectileType::EntityProjectile;

	/** 射击方向 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="弹道", meta = (AllowPrivateAccess = "true"))
	FVector FireDirection;

	/** 射击距离（仅即时弹道使用） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="弹道|即时弹道", meta = (AllowPrivateAccess = "true", ClampMin = 0, ClampMax = 100000, Units = "cm"))
	float FireDistance = 5000.0f;

	/** 子弹速度（仅实体子弹使用） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="弹道|实体子弹", meta = (AllowPrivateAccess = "true", ClampMin = 0, ClampMax = 10000, Units = "cm/s"))
	float ProjectileSpeed = 3000.0f;

	/** 子弹是否弹跳（仅实体子弹使用） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="弹道|实体子弹", meta = (AllowPrivateAccess = "true"))
	bool bShouldBounce = false;

	/** AI感知噪音大小 */
	UPROPERTY(EditAnywhere, Category="子弹|噪音", meta = (ClampMin = 0, ClampMax = 100))
	float NoiseLoudness = 3.0f;

	/** AI感知噪音范围 */
	UPROPERTY(EditAnywhere, Category="子弹|噪音", meta = (ClampMin = 0, ClampMax = 100000, Units = "cm"))
	float NoiseRange = 3000.0f;

	/** AI感知噪音标签 */
	UPROPERTY(EditAnywhere, Category="噪音")
	FName NoiseTag = FName("Projectile");

	/** 物理冲击力 */
	UPROPERTY(EditAnywhere, Category="子弹|命中", meta = (ClampMin = 0, ClampMax = 50000))
	float PhysicsForce = 100.0f;

	/** 伤害值 */
	UPROPERTY(EditAnywhere, Category="子弹|命中", meta = (ClampMin = 0, ClampMax = 100))
	float HitDamage = 25.0f;

	/** 伤害类型 */
	UPROPERTY(EditAnywhere, Category="子弹|命中")
	TSubclassOf<UDamageType> HitDamageType;

	/** 是否可以伤害发射者 */
	UPROPERTY(EditAnywhere, Category="子弹|命中")
	bool bDamageOwner = false;

	/** 是否爆炸命中（仅实体子弹使用） */
	UPROPERTY(EditAnywhere, Category="子弹|爆炸", meta = (EditCondition = "ProjectileType == ESDTAProjectileType::EntityProjectile"))
	bool bExplodeOnHit = false;

	/** 爆炸半径（仅实体子弹使用） */
	UPROPERTY(EditAnywhere, Category="子弹|爆炸", meta = (EditCondition = "ProjectileType == ESDTAProjectileType::EntityProjectile", ClampMin = 0, ClampMax = 5000, Units = "cm"))
	float ExplosionRadius = 500.0f;	

	/** 是否已命中目标 */
	bool bHit = false;

	/** 命中后延迟销毁时间 */
	UPROPERTY(EditAnywhere, Category="子弹|销毁", meta = (ClampMin = 0, ClampMax = 10, Units = "s"))
	float DeferredDestructionTime = 5.0f;

	/** 延迟销毁定时器 */
	FTimerHandle DestructionTimer;

	/** 对象池管理器引用 */
	UPROPERTY()
	USDTAPoolManager* PoolManager;

public:	
	// Sets default values for this actor's properties
	ASDTAProjectiles();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called when the game ends or when destroyed
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	/** 设置射击方向 */
	UFUNCTION(BlueprintCallable, Category="子弹")
	void SetFireDirection(const FVector& Direction);

	/** 触发射击逻辑 */
	UFUNCTION(BlueprintCallable, Category="子弹")
	void Fire();

	/** 设置对象池管理器 */
	UFUNCTION(BlueprintCallable, Category="子弹")
	void SetPoolManager(USDTAPoolManager* InPoolManager);

protected:
	/** 处理碰撞（仅实体子弹使用） */
	virtual void NotifyHit(class UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit) override;

	/** 执行即时弹道的射线检测 */
	void PerformInstantHit();

	/** 查找爆炸半径内的演员并伤害他们 */
	void ExplosionCheck(const FVector& ExplosionCenter);

	/** 处理子弹命中 */
	void ProcessHit(AActor* HitActor, UPrimitiveComponent* HitComp, const FVector& HitLocation, const FVector& HitDirection);

	/** 子弹命中的蓝图事件 */
	UFUNCTION(BlueprintImplementableEvent, Category="子弹", meta = (DisplayName = "子弹命中"))
	void BP_OnProjectileHit(const FHitResult& Hit);

	/** 即时弹道命中的蓝图事件 */
	UFUNCTION(BlueprintImplementableEvent, Category="子弹", meta = (DisplayName = "即时弹道命中"))
	void BP_OnInstantHit(const FHitResult& Hit);

	/** 延迟销毁子弹 */
	void OnDeferredDestruction();

};