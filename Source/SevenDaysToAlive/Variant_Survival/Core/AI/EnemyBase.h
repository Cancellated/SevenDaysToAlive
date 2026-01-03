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
public:
	// 敌人行为
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void ApplyDamage(float DamageAmount, const FHitResult& HitResult);

	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void Attack();

	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void Die();

	// 击中反馈事件
	UFUNCTION(BlueprintImplementableEvent, Category = "Enemy", meta = (DisplayName = "击中反馈"))
	void BP_OnHitReceived(const FHitResult& HitResult, float DamageAmount);

	// 击中反馈委托
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHitReceived, const FHitResult&, HitResult, float, DamageAmount);
	UPROPERTY(BlueprintAssignable, Category = "Enemy")
	FOnHitReceived OnHitReceived;

	// 敌人AI
	virtual void ChasePlayer();
	virtual void Patrol();
	virtual void AttackPlayer();

protected:
	// 内部状态
	bool bIsDead;
	bool bIsAttacking;
};
