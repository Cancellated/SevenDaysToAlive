// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Containers/Array.h"
#include "Containers/Map.h"
#include "Templates/Function.h"
#include "Net/UnrealNetwork.h"
#include "SDTAPoolManager.generated.h"

/**
 * 通用对象池管理器，支持多类型对象的池化管理
 */
UCLASS(Blueprintable, BlueprintType)
class SEVENDAYSTOALIVE_API USDTAPoolManager : public UObject
{
	GENERATED_BODY()

public:
	// 构造函数
	USDTAPoolManager();

	/**
	 * 初始化对象池管理器
	 * @param InWorld 世界上下文
	 */
	UFUNCTION(BlueprintCallable, Category = "对象池")
	void Initialize(UWorld* InWorld);

	/**
	 * 为特定类型的对象初始化对象池
	 * @param ObjectClass 要池化的对象类
	 * @param InitialSize 初始池大小
	 * @param MaxSize 最大池大小
	 */
	UFUNCTION(BlueprintCallable, Category = "对象池")
	void InitPoolForClass(TSubclassOf<UObject> ObjectClass, int32 InitialSize, int32 MaxSize = -1);

	/**
	 * 从对象池中获取指定类型的对象
	 * @param ObjectClass 要获取的对象类
	 * @return 获取到的对象，如果池为空且达到最大大小则返回nullptr
	 */
	UFUNCTION(BlueprintCallable, Category = "对象池")
	UObject* GetObject(TSubclassOf<UObject> ObjectClass);

	/**
	 * 将对象回收回对象池
	 * @param Object 要回收的对象
	 * @return 是否成功回收
	 */
	UFUNCTION(BlueprintCallable, Category = "对象池")
	bool ReturnObject(UObject* Object);

	/**
	 * 清除指定类型的对象池
	 * @param ObjectClass 要清除的对象类型
	 */
	UFUNCTION(BlueprintCallable, Category = "对象池")
	void ClearPool(TSubclassOf<UObject> ObjectClass);

	/**
	 * 清除所有对象池
	 */
	UFUNCTION(BlueprintCallable, Category = "对象池")
	void ClearAllPools();

	/**
	 * 获取指定类型对象池的信息
	 * @param ObjectClass 对象类型
	 * @param OutPooledCount 池化对象数量
	 * @param OutActiveCount 活跃对象数量
	 */
	UFUNCTION(BlueprintCallable, Category = "对象池")
	void GetPoolInfo(TSubclassOf<UObject> ObjectClass, int32& OutPooledCount, int32& OutActiveCount) const;

protected:
	/**
	 * 对象池配置结构体
	 */
	struct FPoolConfig
	{
		TSubclassOf<UObject> ObjectClass; // 对象类型
		int32 MaxSize; // 最大池大小，-1表示无限制
		TArray<UObject*> PooledObjects; // 池化对象数组
		TArray<UObject*> ActiveObjects; // 活跃对象数组

		// 构造函数
		FPoolConfig() : MaxSize(-1) {}
		FPoolConfig(TSubclassOf<UObject> InClass, int32 InMaxSize) : ObjectClass(InClass), MaxSize(InMaxSize) {}
	};

	/**
	 * 创建新的对象实例
	 * @param ObjectClass 对象类
	 * @return 新创建的对象
	 */
	UObject* CreateNewObject(TSubclassOf<UObject> ObjectClass);

	/**
	 * 重置对象状态
	 * @param Object 要重置的对象
	 */
	void ResetObject(UObject* Object);

protected:
	/** 世界上下文 */
	UPROPERTY()
	UWorld* World;

	/** 对象池映射，按对象类分类 */
	TMap<UClass*, FPoolConfig> ObjectPools;

	/** 活跃对象到对象类的映射，用于快速查找对象所属的池 */
	TMap<UObject*, UClass*> ActiveObjectToClassMap;
};

// C++模板方法，用于更方便地使用对象池

/**
 * 从对象池中获取指定类型的对象（模板版本）
 * @param PoolManager 对象池管理器
 * @param ObjectClass 要获取的对象类
 * @return 获取到的对象，如果池为空且达到最大大小则返回nullptr
 */
template<typename T>
T* SDTAGetPooledObject(USDTAPoolManager* PoolManager, TSubclassOf<T> ObjectClass)
{
	if (!PoolManager || !ObjectClass)
	{
		return nullptr;
	}

	return Cast<T>(PoolManager->GetObject(ObjectClass));
}

/**
 * 将对象回收回对象池（模板版本）
 * @param PoolManager 对象池管理器
 * @param Object 要回收的对象
 * @return 是否成功回收
 */
template<typename T>
bool SDTAreturnPooledObject(USDTAPoolManager* PoolManager, T* Object)
{
	if (!PoolManager || !Object)
	{
		return false;
	}

	return PoolManager->ReturnObject(Object);
}
