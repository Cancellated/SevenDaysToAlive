// 七日求生敌人基类
// 所有敌人类的基础类

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EnemyBase.generated.h"

/**
 * 敌人基类，所有敌人类都继承自此
 */
UCLASS()
class SEVENDAYSTOALIVE_API AEnemyBase : public ACharacter
{
	GENERATED_BODY()

public:
	AEnemyBase();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

public:
	// 敌人属性
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	float Health;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	float MaxHealth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	float Damage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	float MoveSpeed;

	// 敌人行为
	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void TakeDamage(float DamageAmount);

	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void Attack();

	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void Die();

	// 敌人AI
	virtual void ChasePlayer();
	virtual void Patrol();
	virtual void AttackPlayer();

private:
	// 内部状态
	bool bIsDead;
	bool bIsAttacking;

};
