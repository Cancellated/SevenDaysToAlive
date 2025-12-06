// 七日求生敌人基类实现

#include "Variant_Survival/Core/AI/EnemyBase.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

AEnemyBase::AEnemyBase()
{
	// 设置默认属性
	PrimaryActorTick.bCanEverTick = true;

	Health = 100.0f;
	MaxHealth = 100.0f;
	Damage = 10.0f;
	MoveSpeed = 300.0f;

	bIsDead = false;
	bIsAttacking = false;

	// 设置移动速度
	GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;
}

void AEnemyBase::BeginPlay()
{
	Super::BeginPlay();
	
	// 初始化敌人
	Health = MaxHealth;
}

void AEnemyBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bIsDead)
	{
		// 敌人AI逻辑
		ChasePlayer();
	}
}

void AEnemyBase::TakeDamage(float DamageAmount)
{
	if (bIsDead) return;

	Health -= DamageAmount;

	if (Health <= 0.0f)
	{
		Die();
	}
}

void AEnemyBase::Attack()
{
	if (bIsDead || bIsAttacking) return;

	bIsAttacking = true;
	// 攻击逻辑将在具体敌人类中实现
}

void AEnemyBase::Die()
{
	bIsDead = true;
	
	// 死亡逻辑
	SetActorEnableCollision(false);
	GetCharacterMovement()->DisableMovement();

	// 通知游戏模式敌人死亡
	// OnEnemyDestroyed.Broadcast(this);
}

void AEnemyBase::ChasePlayer()
{
	// 追逐玩家逻辑将在具体敌人类中实现
}

void AEnemyBase::Patrol()
{
	// 巡逻逻辑将在具体敌人类中实现
}

void AEnemyBase::AttackPlayer()
{
	// 攻击玩家逻辑将在具体敌人类中实现
}

