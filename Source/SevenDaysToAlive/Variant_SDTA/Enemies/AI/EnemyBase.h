/**
 * 七日求生敌人基类
 * 所有敌人类的基础类，实现敌人的核心功能和行为
 * 
 * 核心功能：
 * 1. 敌人基础属性管理（生命值、伤害值、移动速度等）
 * 2. 伤害处理和死亡机制
 * 3. 动画系统集成（死亡动画、受击动画及完成事件）
 * 4. 基础AI行为（追击玩家、巡逻、攻击）
 * 5. 对象池支持，实现敌人的高效回收和复用
 * 
 * 设计要点：
 * - 采用继承结构，支持多种敌人类型的扩展
 * - 使用UE5内置FTimerManager处理动画完成事件
 * - 提供蓝图可调用的方法，支持蓝图扩展
 * - 实现对象池兼容的Reset方法，支持敌人的高效复用
 * - 完善的事件系统，支持动画事件和状态变化通知
 * 
 * 主要组件：
 * - 健康系统：管理生命值和伤害处理
 * - 动画系统：集成死亡和受击动画，支持动画完成事件
 * - AI系统：实现基础的敌人行为逻辑
 * - 对象池集成：支持敌人的创建、获取、回收和复用
 * 
 * 使用方法：
 * 1. 创建敌人类时继承AEnemyBase
 * 2. 根据敌人类型重写或扩展相应的方法
 * 3. 在蓝图中设置属性和动画蒙太奇
 * 4. 通过对象池管理器获取和回收敌人对象
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Animation/AnimMontage.h"
#include "TimerManager.h"

// 前向声明
class USDTAPoolManager;

#include "EnemyBase.generated.h"

/**
 * 敌人基类，所有敌人类都继承自此
 */
UCLASS()
class SEVENDAYSTOALIVE_API AEnemyBase : public ACharacter
{
	GENERATED_BODY()

/**
	 * 构造函数
	 * 
	 * 功能：初始化敌人的默认属性和组件
	 * 设计要点：设置默认的敌人属性值，确保安全初始状态
	 */
public:
	AEnemyBase();

/**
	 * 游戏开始时调用
	 * 
	 * 功能：初始化敌人的运行时状态和属性
	 * 设计要点：设置初始生命值、移动速度等属性，注册必要的事件
	 */
protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	// 敌人属性
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Enemy")
	float Health;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Enemy")
	float MaxHealth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Enemy")
	float Damage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Enemy")
	float MoveSpeed;

	/**
	 * 敌人死亡事件委托
	 * 
	 * 功能：当敌人死亡时触发，用于通知GameMode处理死亡逻辑
	 * 设计要点：使用动态多播委托，支持多订阅者监听
	 */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEnemyDestroyedDelegate, class AEnemyBase*, DestroyedEnemy);

	UPROPERTY(BlueprintAssignable, Category = "Enemy|Events")
	FEnemyDestroyedDelegate OnEnemyDestroyed;
/**
	 * 处理敌人受到的伤害
	 * 
	 * 功能：处理伤害事件，更新生命值，触发受击动画和死亡逻辑
	 * 设计要点：重写UE5的TakeDamage方法，实现自定义伤害处理
	 * 
	 * @param DamageAmount 受到的伤害量
	 * @param DamageEvent 伤害事件信息
	 * @param EventInstigator 造成伤害的控制器
	 * @param DamageCauser 造成伤害的物体
	 * 
	 * @return 实际应用的伤害量
	 */
public:
	// 敌人行为
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	/**
	 * 应用伤害到敌人
	 * 
	 * 功能：直接对敌人应用伤害，更新状态并触发受击反馈
	 * 设计要点：提供蓝图可调用的伤害应用方法
	 * 
	 * @param DamageAmount 要应用的伤害量
	 * @param HitResult 击中结果信息
	 */
	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void ApplyDamage(float DamageAmount, const FHitResult& HitResult);

	/**
	 * 敌人攻击方法
	 * 
	 * 功能：执行敌人的攻击逻辑和动画
	 * 设计要点：触发攻击动画，处理攻击伤害和目标检测
	 */
	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void Attack();

	/**
	 * 敌人死亡方法
	 * 
	 * 功能：处理敌人的死亡逻辑，播放死亡动画
	 * 设计要点：设置死亡状态，播放死亡动画，并通过定时器监听动画完成事件
	 */
	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void Die();

	// 击中反馈事件
	UFUNCTION(BlueprintImplementableEvent, Category = "Enemy", meta = (DisplayName = "击中反馈"))
	void BP_OnHitReceived(const FHitResult& HitResult, float DamageAmount);

	// 击中反馈委托
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHitReceived, const FHitResult&, HitResult, float, DamageAmount);
	UPROPERTY(BlueprintAssignable, Category = "Enemy")
	FOnHitReceived OnHitReceived;
public:
	// 敌人AI
	/**
	 * 追击玩家行为
	 * 
	 * 功能：控制敌人追击玩家的逻辑
	 * 设计要点：实现路径跟随、追击范围判断等逻辑
	 */
	virtual void ChasePlayer();

	/**
	 * 巡逻行为
	 * 
	 * 功能：控制敌人的巡逻逻辑
	 * 设计要点：实现巡逻路径、随机移动等逻辑
	 */
	virtual void Patrol();

	/**
	 * 攻击玩家行为
	 * 
	 * 功能：控制敌人攻击玩家的逻辑
	 * 设计要点：实现攻击范围检测、伤害应用等逻辑
	 */
	virtual void AttackPlayer();

protected:
	// 内部状态
	bool bIsDead; // 敌人是否已死亡
	bool bIsAttacking; // 敌人是否正在攻击
	bool bIsHit; // 是否正在播放受击动画

	// 死亡动画定时器
	FTimerHandle DeathAnimationTimer; // 用于监听死亡动画完成事件的定时器
	// 受击动画定时器
	FTimerHandle HitAnimationTimer; // 用于监听受击动画完成事件的定时器

	/**
	 * 死亡动画完成回调
	 * 
	 * 功能：在死亡动画播放完成后调用
	 * 设计要点：处理死亡动画后的逻辑，如回收敌人到对象池
	 */
	UFUNCTION()
	void OnDeathAnimationFinished();

	/**
	 * 受击动画完成回调
	 * 
	 * 功能：在受击动画播放完成后调用
	 * 设计要点：恢复敌人的正常状态和行为
	 */
	UFUNCTION()
	void OnHitAnimationFinished();

public:
	// 死亡动画蒙太奇
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Animation")
	UAnimMontage* DeathMontage;

	// 受击动画蒙太奇
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Animation")
	UAnimMontage* HitMontage;

	// 死亡动画完成事件
	UFUNCTION(BlueprintImplementableEvent, Category = "Enemy|Animation", meta = (DisplayName = "死亡动画完成"))
	void BP_OnDeathAnimationFinished();
public:
	// 受击动画完成事件
	UFUNCTION(BlueprintImplementableEvent, Category = "Enemy|Animation", meta = (DisplayName = "受击动画完成"))
	void BP_OnHitAnimationFinished();

	/**
	 * 重置敌人状态，用于对象池回收后重新使用
	 */
	UFUNCTION(BlueprintCallable, Category = "Enemy|Pool")
	virtual void Reset();

protected:
	/**
	 * 获取对象池管理器实例
	 * @return 对象池管理器实例，如果不存在则返回nullptr
	 */
	USDTAPoolManager* GetPoolManager() const;

};
