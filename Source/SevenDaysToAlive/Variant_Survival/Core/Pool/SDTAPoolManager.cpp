// Fill out your copyright notice in the Description page of Project Settings.

/**
 * SDTAPoolManager.cpp - 对象池管理器实现文件
 * 
 * 核心实现：
 * 1. 对象池的初始化和配置管理
 * 2. 多类型对象的获取和回收机制
 * 3. 服务器端权威管理的实现
 * 4. 对象状态的重置和生命周期管理
 * 
 * 实现细节：
 * - 采用双数组设计：PooledObjects（池化对象）和ActiveObjects（活跃对象）
 * - 使用UClass作为键的映射表，支持多类型对象管理
 * - 自动处理Actor类型对象的生成和隐藏
 * - 支持网络环境下的安全操作
 * 
 * 设计要点：
 * - 服务器端权威：仅在服务器上启用对象池功能
 * - 延迟初始化：仅在需要时创建对象
 * - 自动重置：回收对象时自动重置状态
 * - 安全检查：完善的空指针和有效性检查
 * 
 * 关键方法：
 * - Initialize：初始化对象池管理器，设置世界上下文
 * - GetObject：从池中获取对象，或在需要时创建新对象
 * - ReturnObject：将对象回收回池中，重置状态
 */

#include "SDTAPoolManager.h"
#include "Engine/World.h"
#include "UObject/UObjectGlobals.h"
#include "Templates/Casts.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"

// 包含敌人基类头文件，用于敌人对象的特殊处理
#include "Variant_Survival/Enemies/AI/EnemyBase.h"

// 包含工作台头文件，用于工作台对象的特殊处理
#include "Variant_Survival/Upgrade/WorkStation.h"


/**
 * USDTAPoolManager构造函数
 * 
 * 功能：初始化对象池管理器的成员变量
 * 设计要点：采用初始化列表设置World为nullptr，确保安全初始状态
 */
USDTAPoolManager::USDTAPoolManager()
	: World(nullptr)
{
}

/**
 * 初始化对象池管理器
 * 
 * 功能：设置世界上下文并根据网络模式决定是否启用对象池功能
 * 设计要点：仅在服务器模式下启用对象池，确保网络一致性
 * 
 * @param InWorld 游戏世界上下文指针，用于对象创建和管理
 */
void USDTAPoolManager::Initialize(UWorld* InWorld)
{
	World = InWorld;
	
	// 检查是否在服务器上运行
	if (World && World->GetNetMode() != NM_DedicatedServer && World->GetNetMode() != NM_ListenServer)
	{
		// 如果不在服务器上，禁用对象池功能
		World = nullptr;
	}
}

/**
 * 为特定类初始化对象池
 * 
 * 功能：创建并配置特定类型对象的对象池
 * 设计要点：支持预创建对象、设置最大容量限制，避免重复初始化
 * 
 * @param ObjectClass 要池化的对象类型
 * @param InitialSize 对象池初始大小，预创建的对象数量
 * @param MaxSize 对象池最大大小，-1表示无限制
 */
void USDTAPoolManager::InitPoolForClass(TSubclassOf<UObject> ObjectClass, int32 InitialSize, int32 MaxSize)
{
	if (!World || !ObjectClass || InitialSize < 0)
	{
		return;
	}

	UClass* Class = ObjectClass.Get();

	// 检查是否已经存在该类型的对象池
	if (ObjectPools.Contains(Class))
	{
		return;
	}

	// 创建新的对象池配置
	FPoolConfig PoolConfig(ObjectClass, MaxSize);

	// 预创建初始数量的对象
	for (int32 i = 0; i < InitialSize; ++i)
	{
		UObject* NewObject = CreateNewObject(ObjectClass);
		if (NewObject)
		{
			PoolConfig.PooledObjects.Add(NewObject);
		}
	}

	// 将新的对象池添加到映射中
	ObjectPools.Add(Class, PoolConfig);
}

/**
 * 从对象池中获取对象
 * 
 * 功能：优先从池中获取对象，如池为空且未达最大容量则创建新对象
 * 设计要点：自动管理对象状态、处理Actor类型特殊逻辑
 * 
 * @param ObjectClass 要获取的对象类型
 * @return 返回获取或创建的对象指针，如失败则返回nullptr
 */
UObject* USDTAPoolManager::GetObject(TSubclassOf<UObject> ObjectClass)
{
	if (!ObjectClass || !World)
	{
		return nullptr;
	}

	UClass* Class = ObjectClass.Get();

	// 检查是否存在该类型的对象池
	FPoolConfig* PoolConfig = ObjectPools.Find(Class);
	if (!PoolConfig)
	{
		// 如果不存在，创建一个默认配置的对象池
		InitPoolForClass(ObjectClass, 0, -1);
		PoolConfig = ObjectPools.Find(Class);
		if (!PoolConfig)
		{
			return nullptr;
		}
	}

	UObject* Object = nullptr;

	// 从池中获取对象
	if (PoolConfig->PooledObjects.Num() > 0)
	{
		Object = PoolConfig->PooledObjects.Last();
		PoolConfig->PooledObjects.Pop();
	}
	// 如果池为空且未达到最大大小，则创建新对象
	else if (PoolConfig->MaxSize == -1 || PoolConfig->ActiveObjects.Num() < PoolConfig->MaxSize)
	{
		Object = CreateNewObject(ObjectClass);
	}

	// 如果成功获取或创建了对象
	if (Object)
	{
		// 将对象标记为活跃
		PoolConfig->ActiveObjects.Add(Object);
		ActiveObjectToClassMap.Add(Object, Class);

		// 重置对象状态
		ResetObject(Object);

		// 如果是Actor类型，将其生成到世界中
		AActor* Actor = Cast<AActor>(Object);
		if (Actor && !Actor->IsActorInitialized())
		{
			World->SpawnActorDeferred<AActor>(Actor->GetClass(), FTransform::Identity);
			Actor->FinishSpawning(FTransform::Identity);
			Actor->SetActorHiddenInGame(false);
			Actor->SetActorEnableCollision(true);
			Actor->SetActorTickEnabled(true);
		}
	}

	return Object;
}

/**
 * 将对象回收到对象池中
 * 
 * 功能：将活跃对象从世界中移除并重置状态，放回池中待复用
 * 设计要点：完善的安全检查、自动重置对象状态、优化渲染和物理性能
 * 
 * @param Object 要回收的对象指针
 * @return 回收成功返回true，失败返回false
 */
bool USDTAPoolManager::ReturnObject(UObject* Object)
{
	if (!World || !Object)
	{
		return false;
	}

	// 查找对象所属的类
	UClass** ClassPtr = ActiveObjectToClassMap.Find(Object);
	if (!ClassPtr)
	{
		return false;
	}

	UClass* Class = *ClassPtr;

	// 查找对应的对象池
	FPoolConfig* PoolConfig = ObjectPools.Find(Class);
	if (!PoolConfig)
	{
		return false;
	}

	// 从活跃对象数组中移除
	PoolConfig->ActiveObjects.Remove(Object);
	ActiveObjectToClassMap.Remove(Object);

	// 将对象放回池中
	PoolConfig->PooledObjects.Add(Object);

	// 如果是Actor类型，将其从世界中隐藏
	AActor* Actor = Cast<AActor>(Object);
	if (Actor)
	{
		Actor->SetActorHiddenInGame(true);
		Actor->SetActorEnableCollision(false);
		Actor->SetActorTickEnabled(false);
		
		// 保持网络复制属性正确设置
		Actor->SetReplicates(true);
		Actor->SetReplicateMovement(true);
		
		Actor->SetActorLocation(FVector::ZeroVector);
		Actor->SetActorRotation(FRotator::ZeroRotator);
	}

	return true;
}

/**
 * 清除特定类型的对象池
 * 
 * 功能：销毁指定类型的所有池化对象和活跃对象，并移除对象池配置
 * 设计要点：完整的资源清理，避免内存泄漏
 * 
 * @param ObjectClass 要清除的对象池类型
 */
void USDTAPoolManager::ClearPool(TSubclassOf<UObject> ObjectClass)
{
	if (!ObjectClass)
	{
		return;
	}

	UClass* Class = ObjectClass.Get();

	// 查找对应的对象池
	FPoolConfig* PoolConfig = ObjectPools.Find(Class);
	if (!PoolConfig)
	{
		return;
	}

	// 销毁所有池化对象
	for (UObject* Object : PoolConfig->PooledObjects)
	{
		if (Object)
		{
			Object->ConditionalBeginDestroy();
		}
	}

	// 销毁所有活跃对象
	for (UObject* Object : PoolConfig->ActiveObjects)
	{
		if (Object)
		{
			ActiveObjectToClassMap.Remove(Object);
			Object->ConditionalBeginDestroy();
		}
	}

	// 清除对象池
	PoolConfig->PooledObjects.Empty();
	PoolConfig->ActiveObjects.Empty();
	ObjectPools.Remove(Class);
}

/**
 * 清除所有对象池
 * 
 * 功能：销毁所有类型的对象池及其包含的所有对象
 * 设计要点：批量清理资源，适用于游戏重置或卸载场景
 */
void USDTAPoolManager::ClearAllPools()
{
	// 遍历所有对象池
	for (auto& PoolPair : ObjectPools)
	{
		FPoolConfig& PoolConfig = PoolPair.Value;

		// 销毁所有池化对象
		for (UObject* Object : PoolConfig.PooledObjects)
		{
			if (Object)
			{
				Object->ConditionalBeginDestroy();
			}
		}

		// 销毁所有活跃对象
		for (UObject* Object : PoolConfig.ActiveObjects)
		{
			if (Object)
			{
				ActiveObjectToClassMap.Remove(Object);
				Object->ConditionalBeginDestroy();
			}
		}
	}

	// 清空所有映射
	ObjectPools.Empty();
	ActiveObjectToClassMap.Empty();
}

/**
 * 获取对象池信息
 * 
 * 功能：返回指定类型对象池的池化对象数和活跃对象数
 * 设计要点：使用输出参数返回信息，不修改对象池状态
 * 
 * @param ObjectClass 要查询的对象池类型
 * @param OutPooledCount 输出参数：池化对象数量
 * @param OutActiveCount 输出参数：活跃对象数量
 */
void USDTAPoolManager::GetPoolInfo(TSubclassOf<UObject> ObjectClass, int32& OutPooledCount, int32& OutActiveCount) const
{
	OutPooledCount = 0;
	OutActiveCount = 0;

	if (!ObjectClass)
	{
		return;
	}

	UClass* Class = ObjectClass.Get();

	// 查找对应的对象池
	const FPoolConfig* PoolConfig = ObjectPools.Find(Class);
	if (PoolConfig)
	{
		OutPooledCount = PoolConfig->PooledObjects.Num();
		OutActiveCount = PoolConfig->ActiveObjects.Num();
	}
}

/**
 * 创建新对象
 * 
 * 功能：根据对象类型创建新实例，支持Actor和普通UObject类型
 * 设计要点：区别处理不同类型对象，设置合适的网络和渲染属性
 * 
 * @param ObjectClass 要创建的对象类型
 * @return 返回创建的对象指针，如失败则返回nullptr
 */
UObject* USDTAPoolManager::CreateNewObject(TSubclassOf<UObject> ObjectClass)
{
	if (!ObjectClass || !World)
	{
		return nullptr;
	}

	UObject* NewObject = nullptr;

	// 根据对象类型创建实例
	if (ObjectClass->IsChildOf<AActor>())
	{
		// 创建Actor类型的对象
		NewObject = World->SpawnActorDeferred<AActor>(ObjectClass, FTransform::Identity);
		if (NewObject)
		{
			AActor* NewActor = Cast<AActor>(NewObject);
			// 设置网络复制属性
			NewActor->SetReplicates(true);
			NewActor->SetReplicateMovement(true);
			
			// 完成Actor生成
			NewActor->FinishSpawning(FTransform::Identity);
			
			// 初始隐藏状态，减少不必要的渲染开销
			NewActor->SetActorHiddenInGame(true);
			NewActor->SetActorEnableCollision(false);
			NewActor->SetActorTickEnabled(false);
		}
	}
	else
	{
		// 创建普通UObject类型的对象
		FStaticConstructObjectParameters Params(ObjectClass);

		Params.Outer = GetTransientPackage();
		Params.Name = NAME_None;
		Params.SetFlags = RF_Transient;
		NewObject = StaticConstructObject_Internal(Params);
	}

	return NewObject;
}

/**
 * 重置对象状态
 * 
 * 功能：恢复对象的初始状态，为复用做准备
 * 设计要点：支持敌人对象的特殊处理，重置物理和运动组件
 * 
 * @param Object 要重置的对象指针
 */
void USDTAPoolManager::ResetObject(UObject* Object)
{
	if (!World || !Object)
	{
		return;
	}

	// 这里可以根据需要重置对象的状态
	// 例如，对于Actor可以重置位置、旋转、速度等
	AActor* Actor = Cast<AActor>(Object);
	if (Actor)
	{
		// 检查是否是EnemyBase类型
		AEnemyBase* Enemy = Cast<AEnemyBase>(Actor);
		if (Enemy)
		{
			// 调用EnemyBase的Reset方法，确保敌人状态完全重置
			Enemy->Reset();
			return;
		}

		// 检查是否是WorkStation类型
		class AWorkStation* WorkStation = Cast<AWorkStation>(Actor);
		if (WorkStation)
		{
			// 调用WorkStation的Reset方法，确保工作台状态完全重置
			WorkStation->Reset();
			return;
		}

		// 通用Actor重置
		Actor->SetActorLocation(FVector::ZeroVector);
		Actor->SetActorRotation(FRotator::ZeroRotator);
		Actor->SetActorScale3D(FVector::OneVector);
		Actor->SetActorEnableCollision(true);
		Actor->SetActorTickEnabled(true);
		Actor->SetActorHiddenInGame(false);

		// 重置网络复制属性
		Actor->SetReplicates(true);
		Actor->SetReplicateMovement(true);

		// 重置运动组件
		UProjectileMovementComponent* ProjectileMovementComp = Actor->FindComponentByClass<UProjectileMovementComponent>();
		if (ProjectileMovementComp)
		{
			ProjectileMovementComp->Velocity = FVector::ZeroVector;
			ProjectileMovementComp->SetActive(true);
		}

		// 重置碰撞组件
		UPrimitiveComponent* CollisionComponent = Actor->FindComponentByClass<UPrimitiveComponent>();
		if (CollisionComponent)
		{
			CollisionComponent->SetPhysicsLinearVelocity(FVector::ZeroVector);
			CollisionComponent->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
			CollisionComponent->SetSimulatePhysics(false);
			CollisionComponent->SetSimulatePhysics(true);
		}
	}
}