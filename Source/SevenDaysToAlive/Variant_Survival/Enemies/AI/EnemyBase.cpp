// 七日求生敌人基类实现
// 
// 功能：实现敌人的核心行为和状态管理
// 设计要点：
// 1. 封装敌人的核心属性和行为
// 2. 实现伤害处理和死亡机制
// 3. 集成动画系统和动画完成事件
// 4. 支持对象池的回收和复用
// 5. 提供基础AI行为框架
// 
// 主要组件：
// - 健康系统：管理生命值和伤害处理
// - 动画系统：集成死亡和受击动画
// - AI系统：提供基础的敌人行为接口
// - 对象池集成：支持敌人的回收和复用
// 
// 使用方法：
// 1. 继承AEnemyBase创建具体的敌人类
// 2. 根据需要重写或扩展相应的方法
// 3. 通过对象池管理器获取和回收敌人对象
// 4. 在蓝图中设置属性和动画蒙太奇

#include "Variant_Survival/Enemies/AI/EnemyBase.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

// 前向声明
class ASDTAGameMode;

// 包含对象池管理器和游戏模式头文件
#include "Variant_Survival/Core/Pool/SDTAPoolManager.h"
#include "Variant_Survival/Core/Game/SDTAGameMode.h"

/**
 * 构造函数
 * 
 * 功能：初始化敌人的默认属性和状态
 * 设计要点：
 * 1. 设置敌人的默认健康值、伤害值和移动速度
 * 2. 初始化敌人的状态标志
 * 3. 配置角色的基础参数
 */
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
	bIsHit = false;

	// 设置移动速度
	GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;
}

/**
 * 游戏开始时调用
 * 
 * 功能：初始化敌人的运行时状态
 * 设计要点：
 * 1. 确保敌人的生命值在游戏开始时恢复到最大值
 * 2. 可以扩展用于初始化其他运行时组件或配置
 */
void AEnemyBase::BeginPlay()
{
	Super::BeginPlay();
	
	// 初始化敌人
	Health = MaxHealth;
}

/**
 * 每帧更新时调用
 * 
 * 功能：执行敌人的实时更新逻辑
 * 设计要点：
 * 1. 只有在敌人未死亡时执行更新
 * 2. 默认实现为追击玩家，可在子类中扩展
 * 3. 处理敌人的状态更新和行为逻辑
 * 
 * @param DeltaTime 帧间隔时间，单位为秒
 */
void AEnemyBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bIsDead)
	{
		// 敌人AI逻辑
		ChasePlayer();
	}
}

/**
 * 处理敌人受到的伤害
 * 
 * 功能：处理伤害事件，更新生命值，触发死亡逻辑
 * 设计要点：
 * 1. 只有在敌人未死亡时才处理伤害
 * 2. 减少敌人的生命值
 * 3. 当生命值降至0或以下时触发死亡逻辑
 * 
 * @param DamageAmount 受到的伤害量
 * @param DamageEvent 伤害事件信息
 * @param EventInstigator 造成伤害的控制器
 * @param DamageCauser 造成伤害的物体
 * 
 * @return 实际应用的伤害量
 */
float AEnemyBase::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (bIsDead) return 0.0f;

	Health -= DamageAmount;

	if (Health <= 0.0f)
	{
		Die();
	}

	return DamageAmount;
}

/**
 * 应用伤害到敌人
 * 
 * 功能：直接对敌人应用伤害，更新状态并触发受击反馈
 * 设计要点：
 * 1. 只有在敌人未死亡时才应用伤害
 * 2. 减少敌人的生命值并触发受击反馈事件
 * 3. 播放受击动画并设置定时器监听动画完成
 * 4. 当生命值降至0或以下时触发死亡逻辑
 * 
 * @param DamageAmount 要应用的伤害量
 * @param HitResult 击中结果信息
 */
void AEnemyBase::ApplyDamage(float DamageAmount, const FHitResult& HitResult)
{
	if (bIsDead) return;

	Health -= DamageAmount;

	// 触发击中反馈事件
	BP_OnHitReceived(HitResult, DamageAmount);
	OnHitReceived.Broadcast(HitResult, DamageAmount);

	// 播放受击动画蒙太奇（不影响移动）
	if (HitMontage && !bIsHit)
	{
		bIsHit = true;
		float MontageLength = HitMontage->GetPlayLength();
		GetMesh()->GetAnimInstance()->Montage_Play(HitMontage);
		GetWorld()->GetTimerManager().SetTimer(HitAnimationTimer, this, &AEnemyBase::OnHitAnimationFinished, MontageLength, false);
	}

	if (Health <= 0.0f)
	{
		Die();
	}
}

/**
 * 敌人攻击方法
 * 
 * 功能：执行敌人的攻击逻辑和动画
 * 设计要点：
 * 1. 只有在敌人未死亡且不在攻击状态时才执行攻击
 * 2. 设置攻击状态标志
 * 3. 具体攻击逻辑由子类实现
 */
void AEnemyBase::Attack()
{
	if (bIsDead || bIsAttacking) return;

	bIsAttacking = true;
	// 攻击逻辑将在具体敌人类中实现
}

/**
 * 敌人死亡方法
 * 
 * 功能：处理敌人的死亡逻辑，播放死亡动画
 * 设计要点：
 * 1. 设置死亡状态标志
 * 2. 禁用敌人的碰撞和移动
 * 3. 清理受击动画定时器
 * 4. 播放死亡动画并设置定时器监听动画完成
 * 5. 如果没有死亡动画，则直接调用死亡动画完成回调
 * 
 * 注意：死亡动画播放完成后会触发OnDeathAnimationFinished方法进行后续处理
 */
void AEnemyBase::Die()
{
	bIsDead = true;
	
	// 死亡逻辑
	SetActorEnableCollision(false);
	GetCharacterMovement()->DisableMovement();

	// 取消受击动画定时器（如果有）
	if (GetWorld()->GetTimerManager().IsTimerActive(HitAnimationTimer))
	{
		GetWorld()->GetTimerManager().ClearTimer(HitAnimationTimer);
	}

	// 播放死亡动画蒙太奇
	if (DeathMontage)
	{
		float MontageLength = DeathMontage->GetPlayLength();
		GetMesh()->GetAnimInstance()->Montage_Play(DeathMontage);
		GetWorld()->GetTimerManager().SetTimer(DeathAnimationTimer, this, &AEnemyBase::OnDeathAnimationFinished, MontageLength, false);
	}
	else
	{
		OnDeathAnimationFinished();
	}

	// 通知游戏模式敌人死亡
	// OnEnemyDestroyed.Broadcast(this);
}

/**
 * 死亡动画完成回调
 * 
 * 功能：在死亡动画播放完成后调用，处理死亡后的逻辑
 * 设计要点：
 * 1. 触发蓝图实现的死亡动画完成事件
 * 2. 获取对象池管理器并尝试将敌人回收回对象池
 * 3. 如果对象池管理器不存在，则直接销毁敌人对象
 * 
 * 注意：该方法通过FTimerManager在死亡动画播放完成后自动调用
 */
void AEnemyBase::OnDeathAnimationFinished()
{
	// 死亡动画播放完成后的逻辑
	BP_OnDeathAnimationFinished();
	
	// 获取对象池管理器
	USDTAPoolManager* PoolManager = GetPoolManager();
	if (PoolManager)
	{
		// 将敌人对象回收回对象池
		PoolManager->ReturnObject(this);
	}
	else
	{
		// 如果对象池管理器不存在，直接销毁敌人
		Destroy();
	}
}

/**
 * 受击动画完成回调
 * 
 * 功能：在受击动画播放完成后调用，恢复敌人的正常状态
 * 设计要点：
 * 1. 重置受击状态标志，允许敌人再次播放受击动画
 * 2. 触发蓝图实现的受击动画完成事件
 * 
 * 注意：该方法通过FTimerManager在受击动画播放完成后自动调用
 */
void AEnemyBase::OnHitAnimationFinished()
{
	// 受击动画播放完成后的逻辑
	bIsHit = false;
	BP_OnHitAnimationFinished();
}

/**
 * 追逐玩家行为
 * 
 * 功能：实现敌人追逐玩家的逻辑
 * 设计要点：
 * 1. 提供基础的追逐行为框架
 * 2. 具体的追逐逻辑由子类实现
 * 3. 通常在Tick函数中调用
 */
void AEnemyBase::ChasePlayer()
{
	// 追逐玩家逻辑将在具体敌人类中实现
}

/**
 * 巡逻行为
 * 
 * 功能：实现敌人的巡逻逻辑
 * 设计要点：
 * 1. 提供基础的巡逻行为框架
 * 2. 具体的巡逻逻辑由子类实现
 * 3. 通常用于实现敌人在指定区域的巡逻行为
 */
void AEnemyBase::Patrol()
{
	// 巡逻逻辑将在具体敌人类中实现
}

/**
 * 攻击玩家行为
 * 
 * 功能：实现敌人攻击玩家的逻辑
 * 设计要点：
 * 1. 提供基础的攻击行为框架
 * 2. 具体的攻击逻辑由子类实现
 * 3. 通常在敌人接近玩家时调用
 */
void AEnemyBase::AttackPlayer()
{
	// 攻击玩家逻辑将在具体敌人类中实现
}

/**
 * 重置敌人状态
 * 
 * 功能：将敌人状态重置为初始状态，用于对象池回收后重新使用
 * 设计要点：
 * 1. 重置敌人的生命值到最大值
 * 2. 重置所有状态标志
 * 3. 恢复敌人的移动能力和移动速度
 * 4. 重置碰撞、可见性和Tick状态
 * 5. 停止所有动画蒙太奇并重置骨骼网格
 * 6. 清除所有定时器
 * 7. 可选：重置敌人位置（通常由对象池管理器在获取对象时设置）
 * 
 * 注意：该方法由对象池管理器在回收敌人对象时调用
 */
void AEnemyBase::Reset()
{
	// 重置生命值
	Health = MaxHealth;

	// 重置状态标志
	bIsDead = false;
	bIsAttacking = false;
	bIsHit = false;

	// 重置移动
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;
	GetCharacterMovement()->Velocity = FVector::ZeroVector;

	// 重置碰撞和可见性
	SetActorEnableCollision(true);
	SetActorHiddenInGame(false);
	SetActorTickEnabled(true);

	// 重置动画状态
	if (GetMesh() && GetMesh()->GetAnimInstance())
	{
		// 停止所有动画蒙太奇
		GetMesh()->GetAnimInstance()->Montage_Stop(0.1f);
		
		// 重置骨骼网格到初始状态
		GetMesh()->ResetRelativeTransform();
	}

	// 清除所有定时器
	if (GetWorld() && GetWorld()->GetTimerManager().IsTimerActive(DeathAnimationTimer))
	{
		GetWorld()->GetTimerManager().ClearTimer(DeathAnimationTimer);
	}
	if (GetWorld() && GetWorld()->GetTimerManager().IsTimerActive(HitAnimationTimer))
	{
		GetWorld()->GetTimerManager().ClearTimer(HitAnimationTimer);
	}

	// 重置敌人位置（可选，可以在获取对象时由对象池管理器设置）
	// SetActorLocation(FVector::ZeroVector);
	// SetActorRotation(FRotator::ZeroRotator);
}

/**
 * 获取对象池管理器实例
 * 
 * 功能：安全获取游戏模式中的对象池管理器实例
 * 设计要点：
 * 1. 进行安全检查，确保世界和游戏模式有效
 * 2. 通过游戏模式的公共GetPoolManager方法获取对象池管理器
 * 3. 提供安全的接口，避免直接访问私有成员
 * 
 * @return 对象池管理器实例，如果不存在则返回nullptr
 */
USDTAPoolManager* AEnemyBase::GetPoolManager() const
{
	if (!GetWorld())
	{
		return nullptr;
	}

	// 获取游戏模式
	ASDTAGameMode* GameMode = Cast<ASDTAGameMode>(GetWorld()->GetAuthGameMode());
	if (!GameMode)
	{
		return nullptr;
	}

	// 调用GameMode的公共GetPoolManager方法
	return GameMode->GetPoolManager();
}

