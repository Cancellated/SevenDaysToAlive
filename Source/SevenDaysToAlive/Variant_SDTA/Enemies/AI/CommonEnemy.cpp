// 七日求生普通敌人类实现

#include "Variant_SDTA/Enemies/AI/CommonEnemy.h"
#include "Variant_SDTA/Characters/SDTAPlayerBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "AIController.h"

ACommonEnemy::ACommonEnemy()
{
	PrimaryActorTick.bCanEverTick = true;

	// 设置AI控制器类
	AIControllerClass = AAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// 设置默认AI配置
	ChaseRange = 2000.0f; // 追逐范围20米
	AttackRange = 100.0f; // 攻击范围1米
	AttackCooldown = 2.0f; // 攻击冷却2秒

	// 初始化动画变量
	IsMoving = false;
	IsAttacking = false;

	bCanAttack = true;
	PlayerRef = nullptr;
	
	// 确保CharacterMovement组件可用
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = 300.0f;
	}
}

void ACommonEnemy::BeginPlay()
{
	Super::BeginPlay();

	// 设置移动速度
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = 300.0f;
		UE_LOG(LogTemp, Log, TEXT("[敌人] 初始化: 移动速度设置为 300"));
	}

	// 获取玩家引用
	PlayerRef = GetPlayerCharacter();
	UE_LOG(LogTemp, Log, TEXT("[敌人] 初始化: 玩家引用获取 %s"), PlayerRef ? TEXT("成功") : TEXT("失败"));
	UE_LOG(LogTemp, Log, TEXT("[敌人] 初始化完成: ID=%d, 位置=%s"), GetUniqueID(), *GetActorLocation().ToString());
}

void ACommonEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 检测是否正在移动
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (Movement)
	{
		IsMoving = Movement->Velocity.Size2D() > 1.0f;
	}

	// 每帧更新玩家引用
	if (!PlayerRef)
	{
		PlayerRef = GetPlayerCharacter();
		if (PlayerRef)
		{
			UE_LOG(LogTemp, Log, TEXT("[敌人] 状态变化: ID=%d 玩家引用获取成功，位置=%s"), GetUniqueID(), *PlayerRef->GetActorLocation().ToString());
		}
	}

	// AI行为逻辑：直接追逐攻击玩家，不再巡逻
	if (PlayerRef && !bIsDead)
	{
		// 检查是否在攻击范围内
		if (IsPlayerInRange(AttackRange))
		{
			// 攻击玩家
			UE_LOG(LogTemp, Log, TEXT("[敌人] 状态变化: ID=%d 进入攻击范围，距离玩家=%.1fcm"), GetUniqueID(), FVector::Dist(GetActorLocation(), PlayerRef->GetActorLocation()));
			AttackPlayer();
		}
		else if (IsPlayerInRange(ChaseRange))
		{
			// 追逐玩家
			UE_LOG(LogTemp, Log, TEXT("[敌人] 状态变化: ID=%d 进入追逐范围，距离玩家=%.1fcm"), GetUniqueID(), FVector::Dist(GetActorLocation(), PlayerRef->GetActorLocation()));
			ChasePlayer();
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("[敌人] 状态变化: ID=%d 玩家超出范围，距离玩家=%.1fcm"), GetUniqueID(), FVector::Dist(GetActorLocation(), PlayerRef->GetActorLocation()));
		}
	}
}

ASDTAPlayerBase* ACommonEnemy::GetPlayerCharacter()
{
	// 获取玩家角色
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (PlayerPawn)
	{
		return Cast<ASDTAPlayerBase>(PlayerPawn);
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
		UE_LOG(LogTemp, Log, TEXT("[敌人] 追逐失败: ID=%d 玩家引用无效或已死亡"), GetUniqueID());
		return;
	}

	// 获取AI控制器
	AAIController* AIController = Cast<AAIController>(GetController());
	if (AIController)
	{
		// 使用AI Move To让敌人移动到玩家位置
		AIController->MoveToLocation(PlayerRef->GetActorLocation(), -1.0f, true, true, false, true, 0, true);
		UE_LOG(LogTemp, Log, TEXT("[敌人] 追逐: ID=%d 使用AI控制器追逐玩家，目标位置=%s"), GetUniqueID(), *PlayerRef->GetActorLocation().ToString());
	}
	else
	{
		// 直接调用Character的AddMovementInput方法
		FVector Direction = (PlayerRef->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
		AddMovementInput(Direction);
		UE_LOG(LogTemp, Log, TEXT("[敌人] 追逐: ID=%d 直接移动追逐玩家，方向=%s"), GetUniqueID(), *Direction.ToString());
	}
}

void ACommonEnemy::AttackPlayer()
{
	if (!PlayerRef || bIsDead || !bCanAttack)
	{
		UE_LOG(LogTemp, Log, TEXT("[敌人] 攻击失败: ID=%d 条件不满足 (玩家=%s, 存活=%d, 可攻击=%d)"), GetUniqueID(), PlayerRef ? TEXT("有效") : TEXT("无效"), !bIsDead, bCanAttack);
		return;
	}

	// 执行攻击
	IsAttacking = true;
	Attack();
	UE_LOG(LogTemp, Log, TEXT("[敌人] 攻击: ID=%d 向玩家发起攻击"), GetUniqueID());

	// 设置攻击冷却
	SetAttackCooldown();
}

void ACommonEnemy::SetAttackCooldown()
{
	bCanAttack = false;
	UE_LOG(LogTemp, Log, TEXT("[敌人] 攻击冷却: ID=%d 开始冷却，持续时间=%.1f秒"), GetUniqueID(), AttackCooldown);

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
	IsAttacking = false;
	UE_LOG(LogTemp, Log, TEXT("[敌人] 攻击冷却: ID=%d 冷却结束，可再次攻击"), GetUniqueID());
}
