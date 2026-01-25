// Fill out your copyright notice in the Description page of Project Settings.

#include "Variant_Survival/Weapons/SDTABullet.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Variant_Survival/Components/HealthComponent.h"
#include "Variant_Survival/Core/Pool/SDTAPoolManager.h"
#include "SevenDaysToAlive.h"

// 初始化静态成员变量
USDTAPoolManager* ASDTABullet::PoolManager = nullptr;
TSubclassOf<ASDTABullet> ASDTABullet::BulletClass = nullptr;

// 设置默认值
ASDTABullet::ASDTABullet()
{
    // 设置此 actor 为每帧调用 Tick()
    PrimaryActorTick.bCanEverTick = true;

    // 创建碰撞组件
    CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
    CollisionComponent->InitSphereRadius(5.0f);
    CollisionComponent->SetCollisionProfileName(TEXT("Projectile"));
    CollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &ASDTABullet::OnCollision);
    RootComponent = CollisionComponent;

    // 创建运动组件
    MovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("MovementComponent"));
    MovementComponent->SetUpdatedComponent(CollisionComponent);
    MovementComponent->InitialSpeed = 3000.0f;
    MovementComponent->MaxSpeed = 3000.0f;
    MovementComponent->bRotationFollowsVelocity = true;
    MovementComponent->bShouldBounce = false;

    // 创建网格组件
    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
    MeshComponent->SetupAttachment(RootComponent);

    // 默认值
    BulletSpeed = 3000.0f;
    Damage = 10.0f;
    Range = 10000.0f;
    TravelDistance = 0.0f;
}

// Called when the game starts or when spawned
void ASDTABullet::BeginPlay()
{
    Super::BeginPlay();

    // 设置运动组件速度
    if (MovementComponent)
    {
        MovementComponent->InitialSpeed = BulletSpeed;
        MovementComponent->MaxSpeed = BulletSpeed;
    }
}

// Called every frame
void ASDTABullet::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 计算移动距离
    if (MovementComponent)
    {
        TravelDistance += MovementComponent->Velocity.Size() * DeltaTime;

        // 检查是否超出射程
        if (TravelDistance >= Range)
        {
            ReturnBullet(this);
        }
    }
}

/** 处理碰撞 */
void ASDTABullet::OnCollision(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Hit)
{
    // 忽略自身和所有者
    if (OtherActor == this || OtherActor == GetOwner())
    {
        return;
    }

    // 检查是否有健康组件
    if (UHealthComponent* HealthComp = OtherActor->FindComponentByClass<UHealthComponent>())
    {
        // 应用伤害
        HealthComp->RemoveHealth(Damage);
        UE_LOG(LogTemp, Log, TEXT("子弹命中: %s, 伤害: %.2f"), *OtherActor->GetName(), Damage);
    }

    // 回收子弹到对象池
    ReturnBullet(this);
}

/** 静态方法：初始化子弹对象池 */
void ASDTABullet::InitializeBulletPool(USDTAPoolManager* InPoolManager, TSubclassOf<ASDTABullet> InBulletClass, int32 InitialSize, int32 MaxSize)
{
    PoolManager = InPoolManager;
    BulletClass = InBulletClass;

    if (PoolManager && BulletClass)
    {
        PoolManager->InitPoolForClass(BulletClass, InitialSize, MaxSize);
        UE_LOG(LogTemp, Log, TEXT("子弹对象池初始化成功，初始大小: %d, 最大大小: %d"), InitialSize, MaxSize);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("子弹对象池初始化失败，对象池管理器或子弹类未设置"));
    }
}

/** 静态方法：从对象池获取子弹 */
ASDTABullet* ASDTABullet::GetBullet()
{
    if (!PoolManager || !BulletClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("无法从对象池获取子弹，对象池管理器或子弹类未初始化"));
        return nullptr;
    }

    return SDTAGetPooledObject<ASDTABullet>(PoolManager, BulletClass);
}

/** 静态方法：回收子弹到对象池 */
void ASDTABullet::ReturnBullet(ASDTABullet* Bullet)
{
    if (Bullet && PoolManager)
    {
        // 重置子弹状态
        Bullet->ResetBullet();
        
        // 回收子弹到对象池
        SDTAreturnPooledObject(PoolManager, Bullet);
    }
}

/** 重置子弹状态 */
void ASDTABullet::ResetBullet()
{
    // 重置子弹状态
    TravelDistance = 0.0f;
    Damage = 10.0f;
    Range = 10000.0f;

    // 停止移动
    if (MovementComponent)
    {
        MovementComponent->StopMovementImmediately();
    }

    // 禁用碰撞
    if (CollisionComponent)
    {
        CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    // 隐藏子弹
    SetActorHiddenInGame(true);
    SetActorTickEnabled(false);
}

/** 激活子弹 */
void ASDTABullet::ActivateBullet(const FVector& Location, const FRotator& Rotation)
{
    // 设置子弹位置和旋转
    SetActorLocation(Location);
    SetActorRotation(Rotation);

    // 重置状态
    TravelDistance = 0.0f;

    // 启用碰撞
    if (CollisionComponent)
    {
        CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    }

    // 显示子弹
    SetActorHiddenInGame(false);
    SetActorTickEnabled(true);

    // 开始移动
    if (MovementComponent)
    {
        MovementComponent->Velocity = Rotation.Vector() * BulletSpeed;
    }
}
