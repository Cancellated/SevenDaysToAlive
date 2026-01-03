// 七日求生普通敌人类实现

#include "Variant_Survival/Core/AI/CommonEnemy.h"
#include "Variant_Survival/Characters/SDTAPlayer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "AIController.h"

ACommonEnemy::ACommonEnemy()
{
	// 设置默认AI配置
	ChaseRange = 2000.0f; // 追逐范围20米
	AttackRange = 100.0f; // 攻击范围1米
	AttackCooldown = 2.0f; // 攻击冷却2秒
	PatrolRange = 500.0f; // 巡逻范围5米

	bCanAttack = true;
	PlayerRef = nullptr;
}

void ACommonEnemy::BeginPlay()
{
	Super::BeginPlay();

	// 记录初始位置
	InitialLocation = GetActorLocation();

	// 获取玩家引用
	PlayerRef = GetPlayerCharacter();

	// 选择初始巡逻目标
	SelectNewPatrolTarget();
}

void ACommonEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 每帧更新玩家引用
	if (!PlayerRef)
	{
		PlayerRef = GetPlayerCharacter();
	}

	// AI行为逻辑
	if (PlayerRef && !bIsDead)
	{
		// 检查玩家是否在追逐范围内
		if (IsPlayerInRange(ChaseRange))
		{
			// 检查是否在攻击范围内
			if (IsPlayerInRange(AttackRange))
			{
				// 攻击玩家
				AttackPlayer();
			}
			else
			{
				// 追逐玩家
				ChasePlayer();
			}
		}
		else
		{
			// 巡逻
			Patrol();
		}
	}
}

ASDTAPlayer* ACommonEnemy::GetPlayerCharacter()
{
	// 获取玩家角色
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (PlayerPawn)
	{
		return Cast<ASDTAPlayer>(PlayerPawn);
	}
	return nullptr;
}

bool ACommonEnemy::IsPlayerInRange(float Range)
{
	if (!PlayerRef)
	{
		return false;
	}

	// 计算与玩家的距离
	float Distance = FVector::Dist(GetActorLocation(), PlayerRef->GetActorLocation());
	return Distance <= Range;
}

void ACommonEnemy::ChasePlayer()
{
	if (!PlayerRef || bIsDead)
	{
		return;
	}

	// 获取AI控制器
	AAIController* AIController = Cast<AAIController>(GetController());
	if (AIController)
	{
		// 使用AI Move To让敌人移动到玩家位置
		AIController->MoveToLocation(PlayerRef->GetActorLocation(), -1.0f, true, true, false, true, 0, true);
	}
}

void ACommonEnemy::Patrol()
{
	if (bIsDead)
	{
		return;
	}

	// 获取AI控制器
	AAIController* AIController = Cast<AAIController>(GetController());
	if (AIController)
	{
		// 移动到巡逻目标
		AIController->MoveToLocation(PatrolTargetLocation, -1.0f, true, true, false, true, 0, true);

		// 检查是否到达巡逻目标
		float DistanceToTarget = FVector::Dist(GetActorLocation(), PatrolTargetLocation);
		if (DistanceToTarget < 50.0f)
		{
			// 选择新的巡逻目标
			SelectNewPatrolTarget();
		}
	}
}

void ACommonEnemy::AttackPlayer()
{
	if (!PlayerRef || bIsDead || !bCanAttack)
	{
		return;
	}

	// 执行攻击
	Attack();

	// 设置攻击冷却
	SetAttackCooldown();
}

void ACommonEnemy::SetAttackCooldown()
{
	bCanAttack = false;

	// 设置定时器
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(
			AttackCooldownTimer,
			this,
			&ACommonEnemy::OnAttackCooldownFinished,
			AttackCooldown,
			false
		);
	}
}

void ACommonEnemy::OnAttackCooldownFinished()
{
	bCanAttack = true;
}

void ACommonEnemy::SelectNewPatrolTarget()
{
	// 在初始位置周围随机选择一个巡逻目标
	FVector RandomOffset = FMath::VRand() * FMath::FRandRange(0.0f, PatrolRange);
	RandomOffset.Z = 0.0f; // 保持高度不变

	PatrolTargetLocation = InitialLocation + RandomOffset;
}
