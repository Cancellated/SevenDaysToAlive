#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SDTAWeaponTypes.h"
#include "SDTABullet.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class UPrimitiveComponent;

UCLASS(abstract)
class SEVENDAYSTOALIVE_API ASDTABullet : public AActor
{
	GENERATED_BODY()
	
	/** 碰撞组件 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USphereComponent* CollisionComponent;

	/** 弹丸移动组件 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UProjectileMovementComponent* ProjectileMovement;

protected:

	/** 子弹伤害 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bullet|Hit", Meta = (ClampMin = 0))
	float Damage = 20.0f;

	/** 伤害类型 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bullet|Hit")
	TSubclassOf<UDamageType> DamageType;

	/** 子弹射程 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bullet", Meta = (ClampMin = 0, ClampMax = 100000, Units = "cm"))
	float Range = 10000.0f;

	/** 子弹速度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bullet", Meta = (ClampMin = 100, ClampMax = 100000, Units = "cm/s"))
	float Velocity = 3000.0f;

	/** 子弹生命周期 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bullet", Meta = (ClampMin = 0.1, ClampMax = 10, Units = "s"))
	float Lifetime = 5.0f;

	/** 是否伤害所有者 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bullet|Hit")
	bool bDamageOwner = false;

	/** 是否在碰撞时爆炸 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bullet|Explosion")
	bool bExplodeOnHit = false;

	/** 爆炸半径 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bullet|Explosion", Meta = (ClampMin = 0, ClampMax = 5000, Units = "cm"))
	float ExplosionRadius = 500.0f;

	/** 物理冲击力 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bullet|Hit", Meta = (ClampMin = 0, ClampMax = 50000))
	float PhysicsForce = 100.0f;

	/** 是否已碰撞 */
	bool bHit = false;

	/** 延迟销毁时间 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bullet|Destruction", Meta = (ClampMin = 0, ClampMax = 10, Units = "s"))
	float DeferredDestructionTime = 0.1f;

	/** 销毁定时器 */
	FTimerHandle DestructionTimer;

	/** 生命周期定时器 */
	FTimerHandle LifetimeTimer;

public:

	/** 构造函数 */
	ASDTABullet();

protected:
	
	/** 游戏开始时调用 */
	virtual void BeginPlay() override;

	/** 游戏结束时调用 */
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

	/** 处理碰撞 */
	virtual void NotifyHit(class UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit) override;

public:

	/** 激活子弹 */
	UFUNCTION(BlueprintCallable, Category="Bullet")
	void ActivateBullet(const FVector& Direction);

protected:

	/** 爆炸检查 */
	void ExplosionCheck(const FVector& ExplosionCenter);

	/** 处理碰撞 */
	void ProcessHit(AActor* HitActor, UPrimitiveComponent* HitComp, const FVector& HitLocation, const FVector& HitDirection);

	/** 蓝图实现的碰撞效果 */
	UFUNCTION(BlueprintImplementableEvent, Category="Bullet", meta = (DisplayName = "On Bullet Hit"))
	void BP_OnBulletHit(const FHitResult& Hit);

	/** 延迟销毁 */
	void OnDeferredDestruction();

	/** 生命周期结束 */
	void OnLifetimeEnd();

	/** 重置子弹状态 */
	void ResetBulletState();

	/** 回收到对象池 */
	void ReturnToPool();

public:

	/** 设置子弹伤害 */
	UFUNCTION(BlueprintCallable, Category="Bullet")
	void SetBulletDamage(float NewDamage);

	/** 获取子弹伤害 */
	UFUNCTION(BlueprintPure, Category="Bullet")
	float GetBulletDamage() const;

	/** 设置子弹射程 */
	UFUNCTION(BlueprintCallable, Category="Bullet")
	void SetBulletRange(float NewRange);

	/** 获取子弹射程 */
	UFUNCTION(BlueprintPure, Category="Bullet")
	float GetBulletRange() const;

	/** 设置子弹速度 */
	UFUNCTION(BlueprintCallable, Category="Bullet")
	void SetBulletVelocity(float NewVelocity);

	/** 获取子弹速度 */
	UFUNCTION(BlueprintPure, Category="Bullet")
	float GetBulletVelocity() const;

	/** 设置子弹生命周期 */
	UFUNCTION(BlueprintCallable, Category="Bullet")
	void SetBulletLifetime(float NewLifetime);

	/** 获取子弹生命周期 */
	UFUNCTION(BlueprintPure, Category="Bullet")
	float GetBulletLifetime() const;

	/** 获取子弹伤害类型 */
	UFUNCTION(BlueprintPure, Category="Bullet")
	TSubclassOf<UDamageType> GetBulletDamageType() const;

	/** 获取是否伤害所有者 */
	UFUNCTION(BlueprintPure, Category="Bullet")
	bool IsBulletDamageOwner() const;

	/** 获取是否在碰撞时爆炸 */
	UFUNCTION(BlueprintPure, Category="Bullet")
	bool IsBulletExplodeOnHit() const;

	/** 获取爆炸半径 */
	UFUNCTION(BlueprintPure, Category="Bullet")
	float GetBulletExplosionRadius() const;

	/** 获取物理冲击力 */
	UFUNCTION(BlueprintPure, Category="Bullet")
	float GetBulletPhysicsForce() const;

	/** 获取碰撞组件 */
	UFUNCTION(BlueprintPure, Category="Bullet")
	USphereComponent* GetBulletCollisionComponent() const;

	/** 获取弹丸移动组件 */
	UFUNCTION(BlueprintPure, Category="Bullet")
	UProjectileMovementComponent* GetBulletProjectileMovement() const;
};
