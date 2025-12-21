// Fill out your copyright notice in the Description page of Project Settings.


#include "Variant_Survival/Weapons/SDTAProjectiles.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"

// Sets default values
ASDTAProjectiles::ASDTAProjectiles()
{
 	// 默认启用Tick，即时弹道在BeginPlay中会禁用
	PrimaryActorTick.bCanEverTick = true;

	// 创建碰撞组件并设置为根组件
	RootComponent = CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("碰撞组件"));

	// 配置碰撞组件：设置半径、启用碰撞、设置碰撞响应
	CollisionComponent->SetSphereRadius(16.0f);
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Block);
	CollisionComponent->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;

	// 创建子弹运动组件
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("运动组件"));

	// 配置运动组件：初始速度、最大速度、是否弹跳
	ProjectileMovement->InitialSpeed = ProjectileSpeed;
	ProjectileMovement->MaxSpeed = ProjectileSpeed;
	ProjectileMovement->bShouldBounce = bShouldBounce;

	// 设置默认伤害类型
	HitDamageType = UDamageType::StaticClass();

	// 初始化对象池管理器指针
	PoolManager = nullptr;
}

// Called when the game starts or when spawned
void ASDTAProjectiles::BeginPlay()
{
	Super::BeginPlay();
	
	// 忽略发射者，避免子弹立即击中自己
	CollisionComponent->IgnoreActorWhenMoving(GetInstigator(), true);

	// 根据弹道类型进行初始化
	if (ProjectileType == ESDTAProjectileType::EntityProjectile)
	{
		// 实体子弹：启用运动组件并设置初始速度
		ProjectileMovement->SetVelocityInLocalSpace(FireDirection * ProjectileSpeed);
	}
	else
	{
		// 即时弹道：禁用碰撞、运动组件和Tick，提高性能
		CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ProjectileMovement->SetActive(false);
		PrimaryActorTick.SetTickFunctionEnable(false);
	}
}

// Called when the game ends or when destroyed
void ASDTAProjectiles::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// 清除延迟销毁定时器，避免内存泄漏
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(DestructionTimer);
	}
}

// Called every frame
void ASDTAProjectiles::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

/** 设置射击方向 */
void ASDTAProjectiles::SetFireDirection(const FVector& Direction)
{
	FireDirection = Direction.GetSafeNormal();
}

/** 设置对象池管理器 */
void ASDTAProjectiles::SetPoolManager(USDTAPoolManager* InPoolManager)
{
	PoolManager = InPoolManager;
}

/** 触发射击逻辑 */
void ASDTAProjectiles::Fire()
{
	if (ProjectileType == ESDTAProjectileType::InstantProjectile)
	{
		// 即时弹道：立即执行命中检测
		PerformInstantHit();
	}
	// 实体子弹：已经在BeginPlay中设置了速度，无需额外处理
}

/** 处理碰撞（仅实体子弹使用） */
void ASDTAProjectiles::NotifyHit(class UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit)
{
	// 忽略如果已经命中过其他物体，避免重复处理
	if (bHit)
	{
		return;
	}

	bHit = true;

	// 禁用子弹的碰撞，防止二次碰撞
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 生成AI感知噪音，让敌人能听到射击声
	MakeNoise(NoiseLoudness, GetInstigator(), GetActorLocation(), NoiseRange, NoiseTag);

	if (bExplodeOnHit)
	{
		// 应用爆炸伤害，以子弹为中心
		ExplosionCheck(GetActorLocation());
	}
	else
	{
		// 单体命中子弹。处理碰撞的演员
		ProcessHit(Other, OtherComp, Hit.ImpactPoint, -Hit.ImpactNormal);
	}

	// 调用蓝图事件，允许在蓝图中添加额外的命中逻辑
	BP_OnProjectileHit(Hit);

	// 检查是否需要延迟销毁子弹，用于特效显示等
	if (DeferredDestructionTime > 0.0f)
	{
		GetWorld()->GetTimerManager().SetTimer(DestructionTimer, this, &ASDTAProjectiles::OnDeferredDestruction, DeferredDestructionTime, false);
	}
	else
	{
		// 立即销毁子弹
		OnDeferredDestruction();
	}
}

/** 执行即时弹道的射线检测 */
void ASDTAProjectiles::PerformInstantHit()
{
	FHitResult HitResult;
	FVector StartLocation = GetActorLocation();
	FVector EndLocation = StartLocation + FireDirection * FireDistance;

	// 执行射线检测，查找命中的物体
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);        // 检测角色
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic); // 检测动态物体
	ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);  // 检测静态物体
	ObjectParams.AddObjectTypesToQuery(ECC_PhysicsBody);  // 检测物理物体

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);  // 忽略子弹本身
	if (!bDamageOwner)
	{
		QueryParams.AddIgnoredActor(GetInstigator());  // 忽略发射者
	}

	bool bLineHit = GetWorld()->LineTraceSingleByObjectType(HitResult, StartLocation, EndLocation, ObjectParams, QueryParams);

	// 绘制调试线（仅在开发模式下显示），用于调试射线检测
	DrawDebugLine(GetWorld(), StartLocation, bLineHit ? HitResult.ImpactPoint : EndLocation, FColor::Red, false, 2.0f, 0, 2.0f);

	// 生成AI感知噪音，让敌人能听到射击声
	MakeNoise(NoiseLoudness, GetInstigator(), StartLocation, NoiseRange, NoiseTag);

	if (bHit)
	{
		// 处理命中的物体
		ProcessHit(HitResult.GetActor(), HitResult.GetComponent(), HitResult.ImpactPoint, FireDirection);
	}

	// 调用蓝图事件，允许在蓝图中添加额外的命中逻辑
	BP_OnInstantHit(HitResult);

	// 即时弹道：命中后立即销毁，因为不需要物理运动
	OnDeferredDestruction();
}

/** 查找爆炸半径内的演员并伤害他们 */
void ASDTAProjectiles::ExplosionCheck(const FVector& ExplosionCenter)
{
	// 执行球体重叠检查，查找附近的演员
	TArray<FOverlapResult> Overlaps;

	FCollisionShape OverlapShape;
	OverlapShape.SetSphere(ExplosionRadius);

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);        // 检测角色
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic); // 检测动态物体
	ObjectParams.AddObjectTypesToQuery(ECC_PhysicsBody);  // 检测物理物体

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);  // 忽略子弹本身
	if (!bDamageOwner)
	{
		QueryParams.AddIgnoredActor(GetInstigator());  // 忽略发射者
	}

	GetWorld()->OverlapMultiByObjectType(Overlaps, ExplosionCenter, FQuat::Identity, ObjectParams, OverlapShape, QueryParams);

	TArray<AActor*> DamagedActors;

	// 处理重叠结果，确保每个演员只被伤害一次
	for (const FOverlapResult& CurrentOverlap : Overlaps)
	{
		// 重叠可能会为每个重叠的组件返回相同的演员
		// 通过将其添加到受损列表来确保我们只伤害每个演员一次
		if (DamagedActors.Find(CurrentOverlap.GetActor()) == INDEX_NONE)
		{
			DamagedActors.Add(CurrentOverlap.GetActor());

			// 计算爆炸方向
			const FVector& ExplosionDir = CurrentOverlap.GetActor()->GetActorLocation() - GetActorLocation();

			// 推动和/或伤害重叠的演员
			ProcessHit(CurrentOverlap.GetActor(), CurrentOverlap.GetComponent(), GetActorLocation(), ExplosionDir.GetSafeNormal());
		}
	}
}

/** 处理子弹命中 */
void ASDTAProjectiles::ProcessHit(AActor* HitActor, UPrimitiveComponent* HitComp, const FVector& HitLocation, const FVector& HitDirection)
{
	// 安全检查：确保命中的演员有效
	if (!HitActor || HitActor == this)
	{
		return;
	}

	// 检查是否命中了角色
	if (ACharacter* HitCharacter = Cast<ACharacter>(HitActor))
	{
		// 忽略发射者，除非设置了允许伤害自己
		if (HitCharacter != GetOwner() || bDamageOwner)
		{
			// 安全检查：确保伤害类型有效
			if (HitDamageType && GetInstigator() && GetInstigator()->GetController())
			{
				// 应用伤害
				UGameplayStatics::ApplyDamage(HitCharacter, HitDamage, GetInstigator()->GetController(), this, HitDamageType);
			}
		}
	}

	// 检查是否命中了物理对象
	if (HitComp && HitComp->IsSimulatingPhysics())
	{
		// 给物体施加物理冲量，产生击退效果
		HitComp->AddImpulseAtLocation(HitDirection * PhysicsForce, HitLocation);
	}
}

/** 延迟销毁子弹 */
void ASDTAProjectiles::OnDeferredDestruction()
{
	// 如果有对象池管理器，返回对象池，否则直接销毁
	if (PoolManager)
	{
		PoolManager->ReturnObject(this);
	}
	else
	{
		// 销毁这个Actor，释放资源
		Destroy();
	}
}