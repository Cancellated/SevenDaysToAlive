// Fill out your copyright notice in the Description page of Project Settings.


#include "Variant_Survival/Upgrade/WorkStation.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/DecalComponent.h"
#include "TimerManager.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/GameModeBase.h"

// 包含必要的头文件
#include "Variant_Survival/Core/Game/SDTAGameMode.h"
#include "Variant_Survival/Core/Game/DayNight/SDTADayNightManager.h"

// Sets default values
AWorkStation::AWorkStation()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// 创建工作台网格
	WorkbenchMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WorkbenchMesh"));
	RootComponent = WorkbenchMesh;

	// 创建交互球体
	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	InteractionSphere->SetupAttachment(RootComponent);
	InteractionSphere->SetSphereRadius(200.f);
	InteractionSphere->SetGenerateOverlapEvents(true);
	InteractionSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

	// 绑定重叠事件
	InteractionSphere->OnComponentBeginOverlap.AddDynamic(this, &AWorkStation::OnBeginOverlap);
	InteractionSphere->OnComponentEndOverlap.AddDynamic(this, &AWorkStation::OnEndOverlap);

	// 设置默认状态
	bIsInteractable = false;

	// 设置网络复制
	SetReplicates(true);
	SetReplicateMovement(true);
}

// Called when the game starts or when spawned
void AWorkStation::BeginPlay()
{
	Super::BeginPlay();
	
	// 初始设置为不可见，等待昼夜系统激活
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	SetActorTickEnabled(false);

	// 获取昼夜管理器并订阅事件
	UWorld* World = GetWorld();
	if (World)
	{
		// 获取GameMode
		AGameModeBase* GameMode = World->GetAuthGameMode();
		if (GameMode)
		{
			// 将GameMode转换为ASDTAGameMode
			ASDTAGameMode* SDTAGameMode = Cast<ASDTAGameMode>(GameMode);
			if (SDTAGameMode)
			{
				// 获取昼夜管理器
				USDTADayNightManager* DayNightManager = SDTAGameMode->GetDayNightManager();
				if (DayNightManager)
				{
					// 订阅昼夜状态变化事件
					DayNightManager->OnDayNightStateChanged.AddDynamic(this, &AWorkStation::OnDayNightStateChanged);
					
					// 初始化当前状态
					bool bIsNight = DayNightManager->IsNight();
					SetDayNightState(!bIsNight); // 注意：这里取反，因为事件参数是bIsNowNight
				}
			}
		}
	}
}

// Called when the game ends or when destroyed
void AWorkStation::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	// 取消订阅事件
	UWorld* World = GetWorld();
	if (World)
	{
		AGameModeBase* GameMode = World->GetAuthGameMode();
		if (GameMode)
		{
			ASDTAGameMode* SDTAGameMode = Cast<ASDTAGameMode>(GameMode);
			if (SDTAGameMode)
			{
				USDTADayNightManager* DayNightManager = SDTAGameMode->GetDayNightManager();
				if (DayNightManager)
				{
					DayNightManager->OnDayNightStateChanged.RemoveDynamic(this, &AWorkStation::OnDayNightStateChanged);
				}
			}
		}
	}
	
	Super::EndPlay(EndPlayReason);
}

// 昼夜状态变化回调
void AWorkStation::OnDayNightStateChanged(bool bIsNowNight)
{
	// 注意：bIsNowNight为true表示夜晚，false表示白天
	SetDayNightState(!bIsNowNight);
}

// Called every frame
void AWorkStation::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 可以添加一些视觉效果更新
	if (bIsInteractable)
	{
		// 例如：闪烁效果或旋转动画
	}
}

/**
 * 交互方法，由玩家触发
 * @param InteractingPawn 交互的 pawn
 */
void AWorkStation::Interact(APawn* InteractingPawn)
{
	if (!InteractingPawn || !bIsInteractable)
	{
		return;
	}

	// 获取玩家控制器
	APlayerController* PC = Cast<APlayerController>(InteractingPawn->GetController());
	if (!PC)
	{
		return;
	}

	// 检查是否有升级UI类
	if (UpgradeUIClass)
	{
		// 创建并显示升级UI
		UUserWidget* UpgradeUI = CreateWidget<UUserWidget>(PC, UpgradeUIClass);
		if (UpgradeUI)
		{
			UpgradeUI->AddToViewport();
			// 播放交互音效
			UGameplayStatics::PlaySoundAtLocation(GetWorld(), nullptr, GetActorLocation());
		}
	}
}

/**
 * 设置昼夜状态
 * @param bIsDay 是否为白天
 */
void AWorkStation::SetDayNightState(bool bIsDay)
{
	if (bIsDay)
	{
		// 白天：显示并启用交互
		SetActorHiddenInGame(false);
		SetActorEnableCollision(true);
		SetActorTickEnabled(true);
		bIsInteractable = true;
		
		// 启用碰撞组件
		if (WorkbenchMesh)
		{
			WorkbenchMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		}
		
		if (InteractionSphere)
		{
			InteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		}
	}
	else
	{
		// 夜晚：隐藏并禁用交互
		SetActorHiddenInGame(true);
		SetActorEnableCollision(false);
		SetActorTickEnabled(false);
		bIsInteractable = false;
		
		// 禁用碰撞组件
		if (WorkbenchMesh)
		{
			WorkbenchMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
		
		if (InteractionSphere)
		{
			InteractionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
}

/**
 * 设置对象池管理器
 * @param InPoolManager 对象池管理器
 */
void AWorkStation::SetPoolManager(USDTAPoolManager* InPoolManager)
{
	PoolManager = InPoolManager;
}

/**
 * 重置工作台状态
 * 用于对象池回收时重置状态
 */
void AWorkStation::Reset()
{
	// 重置位置和旋转
	SetActorLocation(FVector::ZeroVector);
	SetActorRotation(FRotator::ZeroRotator);
	
	// 重置状态
	bIsInteractable = false;
	
	// 重置组件状态
	if (WorkbenchMesh)
	{
		WorkbenchMesh->SetVisibility(true);
		WorkbenchMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	
	if (InteractionSphere)
	{
		InteractionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	
	// 隐藏工作台
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	SetActorTickEnabled(false);
}

/**
 * 获取对象池管理器实例
 * @return 对象池管理器实例，如果不存在则返回nullptr
 */
USDTAPoolManager* AWorkStation::GetPoolManager() const
{
	return PoolManager;
}

/**
 * 处理玩家进入交互范围
 */
void AWorkStation::OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!bIsInteractable)
	{
		return;
	}

	// 检查是否为玩家
	APawn* PlayerPawn = Cast<APawn>(OtherActor);
	if (PlayerPawn && PlayerPawn->IsPlayerControlled())
	{
		// 显示交互提示
		// 例如：创建一个交互提示UI或粒子效果
	}
}

/**
 * 处理玩家离开交互范围
 */
void AWorkStation::OnEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	// 检查是否为玩家
	APawn* PlayerPawn = Cast<APawn>(OtherActor);
	if (PlayerPawn && PlayerPawn->IsPlayerControlled())
	{
		// 隐藏交互提示
	}
}

