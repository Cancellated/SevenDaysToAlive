// Fill out your copyright notice in the Description page of Project Settings.


#include "SDTABullet.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Core/Pool/SDTAPoolManager.h"

ASDTABullet::ASDTABullet()
{
	PrimaryActorTick.bCanEverTick = true;

	// 创建碰撞组件并设置为根组件
	RootComponent = CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("Collision Component"));

	CollisionComponent->SetSphereRadius(16.0f);
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Block);
	CollisionComponent->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;

	// 创建弹丸移动组件
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Projectile Movement"));

	ProjectileMovement->InitialSpeed = GetBulletVelocity();
	ProjectileMovement->MaxSpeed = GetBulletVelocity();
	ProjectileMovement->bShouldBounce = false;

	// 设置默认伤害类型
	DamageType = UDamageType::StaticClass();
}

void ASDTABullet::BeginPlay()
{
	Super::BeginPlay();
	
	// 忽略发射者
	CollisionComponent->IgnoreActorWhenMoving(GetInstigator(), true);

	// 设置生命周期定时器
	GetWorld()->GetTimerManager().SetTimer(LifetimeTimer, this, &ASDTABullet::OnLifetimeEnd, GetBulletLifetime(), false);
}

void ASDTABullet::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// 清除定时器
	GetWorld()->GetTimerManager().ClearTimer(DestructionTimer);
	GetWorld()->GetTimerManager().ClearTimer(LifetimeTimer);
}

void ASDTABullet::NotifyHit(class UPrimitiveComponent* MyComp, AActor* Other, class UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit)
{
	// 忽略已碰撞的情况
	if (bHit)
	{
		return;
	}

	bHit = true;

	// 禁用碰撞
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (IsBulletExplodeOnHit())
	{
		// 应用爆炸伤害
		ExplosionCheck(GetActorLocation());

	} else {
		// 处理单个碰撞
		ProcessHit(Other, OtherComp, Hit.ImpactPoint, -Hit.ImpactNormal);

	}

	// 传递给蓝图实现额外效果
	BP_OnBulletHit(Hit);

	// 安排延迟销毁
	if (DeferredDestructionTime > 0.0f)
	{
		GetWorld()->GetTimerManager().SetTimer(DestructionTimer, this, &ASDTABullet::OnDeferredDestruction, DeferredDestructionTime, false);

	} else {
		// 立即销毁
		Destroy();
	}
}

void ASDTABullet::ActivateBullet(const FVector& Direction)
{
	// 重置状态
	bHit = false;
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	// 清除所有定时器
	GetWorld()->GetTimerManager().ClearTimer(DestructionTimer);
	GetWorld()->GetTimerManager().ClearTimer(LifetimeTimer);

	// 重新设置生命周期定时器
	GetWorld()->GetTimerManager().SetTimer(LifetimeTimer, this, &ASDTABullet::OnLifetimeEnd, GetBulletLifetime(), false);

	// 设置弹丸移动方向
	if (ProjectileMovement)
	{
		ProjectileMovement->Velocity = Direction * GetBulletVelocity();
	}

	// 启用碰撞
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
}

void ASDTABullet::ResetBulletState()
{
	// 重置碰撞状态
	bHit = false;
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	// 清除所有定时器
	GetWorld()->GetTimerManager().ClearTimer(DestructionTimer);
	GetWorld()->GetTimerManager().ClearTimer(LifetimeTimer);

	// 重新设置生命周期定时器
	GetWorld()->GetTimerManager().SetTimer(LifetimeTimer, this, &ASDTABullet::OnLifetimeEnd, GetBulletLifetime(), false);
}

void ASDTABullet::ExplosionCheck(const FVector& ExplosionCenter)
{
	// 球体重叠检查
	TArray<FOverlapResult> Overlaps;

	FCollisionShape OverlapShape;
	OverlapShape.SetSphere(GetBulletExplosionRadius());

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectParams.AddObjectTypesToQuery(ECC_PhysicsBody);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	if (!IsBulletDamageOwner())
	{
		QueryParams.AddIgnoredActor(GetInstigator());
	}

	GetWorld()->OverlapMultiByObjectType(Overlaps, ExplosionCenter, FQuat::Identity, ObjectParams, OverlapShape, QueryParams);

	TArray<AActor*> DamagedActors;

	// 处理重叠结果
	for (const FOverlapResult& CurrentOverlap : Overlaps)
	{
		// 确保每个actor只被伤害一次
		if (DamagedActors.Find(CurrentOverlap.GetActor()) == INDEX_NONE)
		{
			DamagedActors.Add(CurrentOverlap.GetActor());

			// 应用物理力
			const FVector& ExplosionDir = CurrentOverlap.GetActor()->GetActorLocation() - GetActorLocation();

			// 处理碰撞
			ProcessHit(CurrentOverlap.GetActor(), CurrentOverlap.GetComponent(), GetActorLocation(), ExplosionDir.GetSafeNormal());
		}
		
	}
}

void ASDTABullet::ProcessHit(AActor* HitActor, UPrimitiveComponent* HitComp, const FVector& HitLocation, const FVector& HitDirection)
{
	// 检查是否命中角色
	if (ACharacter* HitCharacter = Cast<ACharacter>(HitActor))
	{
		// 忽略所有者
		if (HitCharacter != GetOwner() || IsBulletDamageOwner())
		{
			// 应用伤害
			UGameplayStatics::ApplyPointDamage(
				HitCharacter,
				GetBulletDamage(),
				HitDirection,
				FHitResult(HitCharacter, HitComp, HitLocation, HitDirection),
				GetInstigatorController(),
				this,
				GetBulletDamageType()
			);

			// 应用物理力
			if (HitComp && HitComp->IsSimulatingPhysics())
			{
				HitComp->AddImpulse(HitDirection * GetBulletPhysicsForce());
			}
		}

	} else if (HitComp && HitComp->IsSimulatingPhysics()) {
		// 应用物理力到物理对象
		HitComp->AddImpulse(HitDirection * GetBulletPhysicsForce());
	}
}

void ASDTABullet::OnDeferredDestruction()
{
	Destroy();
}

void ASDTABullet::OnLifetimeEnd()
{
	Destroy();	// 这里本应该集成对象池，但是为了能稳定运行，暂时不集成
}

void ASDTABullet::SetBulletDamage(float NewDamage)
{
	Damage = NewDamage;
}

void ASDTABullet::SetBulletRange(float NewRange)
{
	Range = NewRange;
}

void ASDTABullet::SetBulletVelocity(float NewVelocity)
{
	Velocity = NewVelocity;
	if (ProjectileMovement)
	{
		ProjectileMovement->InitialSpeed = GetBulletVelocity();
		ProjectileMovement->MaxSpeed = GetBulletVelocity();
	}
}

void ASDTABullet::SetBulletLifetime(float NewLifetime)
{
	Lifetime = NewLifetime;
}

float ASDTABullet::GetBulletDamage() const
{
	return Damage;
}

float ASDTABullet::GetBulletRange() const
{
	return Range;
}

float ASDTABullet::GetBulletVelocity() const
{
	return Velocity;
}

float ASDTABullet::GetBulletLifetime() const
{
	return Lifetime;
}

TSubclassOf<UDamageType> ASDTABullet::GetBulletDamageType() const
{
	return DamageType;
}

bool ASDTABullet::IsBulletDamageOwner() const
{
	return bDamageOwner;
}

bool ASDTABullet::IsBulletExplodeOnHit() const
{
	return bExplodeOnHit;
}

float ASDTABullet::GetBulletExplosionRadius() const
{
	return ExplosionRadius;
}

float ASDTABullet::GetBulletPhysicsForce() const
{
	return PhysicsForce;
}

USphereComponent* ASDTABullet::GetBulletCollisionComponent() const
{
	return CollisionComponent;
}

UProjectileMovementComponent* ASDTABullet::GetBulletProjectileMovement() const
{
	return ProjectileMovement;
}
