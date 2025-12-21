// Fill out your copyright notice in the Description page of Project Settings.

#include "SDTAPoolManager.h"
#include "Engine/World.h"
#include "UObject/UObjectGlobals.h"
#include "Templates/Casts.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"


USDTAPoolManager::USDTAPoolManager()
	: World(nullptr)
{
}

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