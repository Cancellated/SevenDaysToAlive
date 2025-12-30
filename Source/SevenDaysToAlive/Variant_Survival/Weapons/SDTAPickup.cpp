// 在项目设置的描述页面填写您的版权声明。


#include "Variant_Survival/Weapons/SDTAPickup.h"
#include "SevenDaysToAlive.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Variant_Survival/Weapons/SDTAWeaponHolder.h"
#include "Variant_Survival/Weapons/SDTAWeapon.h"
#include "Engine/World.h"
#include "TimerManager.h"

ASDTAPickup::ASDTAPickup()
{
 	PrimaryActorTick.bCanEverTick = true;

	// 创建根组件
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	// 创建碰撞球体
	SphereCollision = CreateDefaultSubobject<USphereComponent>(TEXT("Sphere Collision"));
	SphereCollision->SetupAttachment(RootComponent);

	SphereCollision->SetRelativeLocation(FVector(0.0f, 0.0f, 84.0f));
	SphereCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SphereCollision->SetCollisionObjectType(ECC_WorldStatic);
	SphereCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	SphereCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	SphereCollision->bFillCollisionUnderneathForNavmesh = true;

	// 订阅球体上的碰撞重叠事件
	SphereCollision->OnComponentBeginOverlap.AddDynamic(this, &ASDTAPickup::OnOverlap);

	// 创建网格组件
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(SphereCollision);

	Mesh->SetCollisionProfileName(FName("NoCollision"));
}

void ASDTAPickup::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (FSDTAWeaponTableRow* WeaponData = WeaponType.GetRow<FSDTAWeaponTableRow>(FString()))
	{
		// 设置网格
		Mesh->SetStaticMesh(WeaponData->StaticMesh.LoadSynchronous());
	}
}

void ASDTAPickup::BeginPlay()
{
	Super::BeginPlay();

	if (FSDTAWeaponTableRow* WeaponData = WeaponType.GetRow<FSDTAWeaponTableRow>(FString()))
	{
		// 复制武器类
		WeaponClass = WeaponData->WeaponToSpawn;
	}
}

void ASDTAPickup::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	UE_LOG(LogSevenDaysToAlive, Log, TEXT("[SDTAPickup] %s - EndPlay，原因: %d"), *GetName(), (int32)EndPlayReason);

	// 清除重生计时器
	GetWorld()->GetTimerManager().ClearTimer(RespawnTimer);
}

void ASDTAPickup::OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// 我们是否与武器持有者发生了碰撞？
	if (ISDTAWeaponHolder* WeaponHolder = Cast<ISDTAWeaponHolder>(OtherActor))
	{
		WeaponHolder->AddWeaponClass(WeaponClass);

		// 隐藏这个拾取物
		SetActorHiddenInGame(true);

		// 禁用碰撞
		SetActorEnableCollision(false);

		// 禁用Tick
		SetActorTickEnabled(false);

		// 安排重生
		GetWorld()->GetTimerManager().SetTimer(RespawnTimer, this, &ASDTAPickup::RespawnPickup, RespawnTime, false);
	}
}

void ASDTAPickup::RespawnPickup()
{
	UE_LOG(LogSevenDaysToAlive, Log, TEXT("[SDTAPickup] %s - 重生开始"), *GetName());

	// 显示这个拾取物
	SetActorHiddenInGame(false);

	// 调用蓝图处理函数
	BP_OnRespawn();
}

void ASDTAPickup::FinishRespawn()
{
	UE_LOG(LogSevenDaysToAlive, Log, TEXT("[SDTAPickup] %s - 重生完成，重新启用碰撞和Tick"), *GetName());

	// 启用碰撞
	SetActorEnableCollision(true);

	// 启用Tick
	SetActorTickEnabled(true);
}