// 七日求生普通敌人类
// 实现通用的敌人AI行为逻辑

#pragma once

#include "CoreMinimal.h"
#include "Variant_SDTA/Enemies/AI/EnemyBase.h"
#include "CommonEnemy.generated.h"

/**
 * 普通敌人类，实现通用的AI行为逻辑
 * 具体敌人类型（如僵尸、感染者等）可以继承此类或直接创建蓝图
 */
UCLASS()
class SEVENDAYSTOALIVE_API ACommonEnemy : public AEnemyBase
{
	GENERATED_BODY()

public:
	ACommonEnemy();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

public:
	// 敌人AI行为配置
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Config")
	float ChaseRange; // 追逐范围（cm）

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Config")
	float AttackRange; // 攻击范围（cm）

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Config")
	float AttackCooldown; // 攻击冷却时间（秒）

	// 动画变量
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation")
	bool IsMoving;  // 是否正在移动

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation")
	bool IsAttacking;  // 是否正在攻击

	// 实现基类的AI行为
	virtual void ChasePlayer() override;
	virtual void AttackPlayer() override;

	// 获取玩家角色
	class ASDTAPlayerBase* GetPlayerCharacter();

	// 检查玩家是否在范围内
	bool IsPlayerInRange(float Range);

private:
	// 内部状态
	FTimerHandle AttackCooldownTimer;
	bool bCanAttack;

	// 玩家引用
	UPROPERTY()
	class ASDTAPlayerBase* PlayerRef;

	// 设置攻击冷却
	void SetAttackCooldown();

	// 攻击冷却结束回调
	void OnAttackCooldownFinished();
};
