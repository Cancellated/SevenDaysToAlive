// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Variant_Survival/Core/Pool/SDTAPoolManager.h"
#include "SDTABullet.generated.h"

/**
 * 子弹类
 */
UCLASS()
class SEVENDAYSTOALIVE_API ASDTABullet : public AActor
{
    GENERATED_BODY()

public:
    /** 构造函数 */
    ASDTABullet();

    /** 静态方法：初始化子弹对象池 */
    static void InitializeBulletPool(USDTAPoolManager* InPoolManager, TSubclassOf<ASDTABullet> InBulletClass, int32 InitialSize = 10, int32 MaxSize = 50);

    /** 静态方法：从对象池获取子弹 */
    static ASDTABullet* GetBullet();

    /** 静态方法：回收子弹到对象池 */
    static void ReturnBullet(ASDTABullet* Bullet);

protected:
    /** 子弹速度 */
    UPROPERTY(EditAnywhere, Category = "Bullet", meta = (ClampMin = 100))
    float BulletSpeed;

    /** 子弹伤害 */
    float Damage;

    /** 子弹射程 */
    float Range;

    /** 子弹移动距离 */
    float TravelDistance;

    /** 子弹碰撞组件 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    class USphereComponent* CollisionComponent;

    /** 子弹运动组件 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    class UProjectileMovementComponent* MovementComponent;

    /** 子弹网格组件 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    class UStaticMeshComponent* MeshComponent;

protected:
    /** 游戏开始时调用 */
    virtual void BeginPlay() override;

    /** 每帧调用 */
    virtual void Tick(float DeltaTime) override;

    /** 处理碰撞 */
    UFUNCTION()
    void OnCollision(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Hit);

public:
    /** 设置子弹伤害 */
    void SetDamage(float NewDamage) { Damage = NewDamage; }

    /** 设置子弹射程 */
    void SetRange(float NewRange) { Range = NewRange; }

    /** 重置子弹状态 */
    void ResetBullet();

    /** 激活子弹 */
    void ActivateBullet(const FVector& Location, const FRotator& Rotation);

private:
    /** 静态对象池管理器 */
    static USDTAPoolManager* PoolManager;

    /** 静态子弹类 */
    static TSubclassOf<ASDTABullet> BulletClass;
};
