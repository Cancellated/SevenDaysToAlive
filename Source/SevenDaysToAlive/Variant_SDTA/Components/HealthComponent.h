// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Net/UnrealNetwork.h" // 添加网络相关头文件
#include "HealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealthChangedComponent, float, HealthPercent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHealthLowWarningComponent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeathComponent);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SEVENDAYSTOALIVE_API UHealthComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    // Sets default values for this component's properties
    UHealthComponent();

protected:
    // Called when the game starts
    virtual void BeginPlay() override;

public:
    // Called every frame
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // 角色统计属性
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Replicated, Category = "Health System")
    float MaxHealth;

    // 当前健康值
    UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Health, Category = "Health System")
    float Health;

    // 健康值变化委托
    UPROPERTY(BlueprintAssignable, Category = "Health System")
    FOnHealthChangedComponent OnHealthChanged;

    // 低健康值警告委托
    UPROPERTY(BlueprintAssignable, Category = "Health System")
    FOnHealthLowWarningComponent OnHealthLowWarning;

    // 死亡委托
    UPROPERTY(BlueprintAssignable, Category = "Health System")
    FOnDeathComponent OnDeath;

    // 设置健康值
    UFUNCTION(BlueprintCallable, Category = "Health System")
    void SetHealth(float NewHealth);

    // 增加健康值
    UFUNCTION(BlueprintCallable, Category = "Health System")
    void AddHealth(float HealthToAdd);

    // 减少健康值
    UFUNCTION(BlueprintCallable, Category = "Health System")
    void RemoveHealth(float HealthToRemove);

    // 检查是否死亡
    UFUNCTION(BlueprintCallable, Category = "Health System")
    bool IsDead() const;

    // 网络复制相关
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // 当Health属性在网络上复制时调用
    UFUNCTION()
    void OnRep_Health();
};
