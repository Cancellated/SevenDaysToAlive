// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Net/UnrealNetwork.h" // 添加网络相关头文件
#include "StaminaComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStaminaChangedComponent, float, StaminaPercent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStaminaLowWarningComponent);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SEVENDAYSTOALIVE_API UStaminaComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    // Sets default values for this component's properties
    UStaminaComponent();

protected:
    // Called when the game starts
    virtual void BeginPlay() override;

public:
    // Called every frame
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // 角色统计属性
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Replicated, Category = "Stamina System")
    float MaxStamina;

    // 当前能量值
    UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Stamina, Category = "Stamina System")
    float Stamina;

    // 能量回复相关属性
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Stamina System")
    float StaminaRegenerationRate;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Stamina System")
    float StaminaRegenerationDelay;

    UPROPERTY(BlueprintReadWrite, Replicated, Category = "Stamina System")
    bool bIsStaminaRegenerating;

    // 能量值变化委托
    UPROPERTY(BlueprintAssignable, Category = "Stamina System")
    FOnStaminaChangedComponent OnStaminaChanged;

    // 低能量值警告委托
    UPROPERTY(BlueprintAssignable, Category = "Stamina System")
    FOnStaminaLowWarningComponent OnStaminaLowWarning;

    // 设置能量值
    UFUNCTION(BlueprintCallable, Category = "Stamina System")
    void SetStamina(float NewStamina);

    // 增加能量值
    UFUNCTION(BlueprintCallable, Category = "Stamina System")
    void AddStamina(float StaminaToAdd);

    // 减少能量值
    UFUNCTION(BlueprintCallable, Category = "Stamina System")
    void RemoveStamina(float StaminaToRemove);

    // 消耗能量值
    UFUNCTION(BlueprintCallable, Category = "Stamina System")
    bool ConsumeStamina(float StaminaCost);

    // 开始能量回复
    UFUNCTION(BlueprintCallable, Category = "Stamina System")
    void StartStaminaRegeneration();

    // 停止能量回复
    UFUNCTION(BlueprintCallable, Category = "Stamina System")
    void StopStaminaRegeneration();

    // 网络复制相关
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // 当Stamina属性在网络上复制时调用
    UFUNCTION()
    void OnRep_Stamina();
};
