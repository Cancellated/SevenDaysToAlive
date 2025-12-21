// 在项目设置的描述页面填写您的版权声明。

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/DataTable.h"
#include "Engine/StaticMesh.h"
#include "SDTAPickup.generated.h"

class USphereComponent;
class UPrimitiveComponent;
class ASDTAWeapon;
class ASDTAPlayer;

/**
 *  保存武器拾取类型的信息
 */
USTRUCT(BlueprintType)
struct FSDTAWeaponTableRow : public FTableRowBase
{
	GENERATED_BODY()

	/** 拾取物上显示的网格 */
	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<UStaticMesh> StaticMesh;

	/** 拾取时获得的武器类 */
	UPROPERTY(EditAnywhere)
	TSubclassOf<ASDTAWeapon> WeaponToSpawn;
};

/**
 *  简单的生存游戏武器拾取物
 */
UCLASS(abstract)
class SEVENDAYSTOALIVE_API ASDTAPickup : public AActor
{
	GENERATED_BODY()

	/** 碰撞球体 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USphereComponent* SphereCollision;

	/** 武器拾取物网格。其网格资源从武器数据表中设置 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* Mesh;
	
protected:

	/** 关于拾取的武器类型和该拾取物视觉效果的数据 */
	UPROPERTY(EditAnywhere, Category="Pickup")
	FDataTableRowHandle WeaponType;

	/** 拾取时获得的武器类型。从武器数据表中设置。 */
	TSubclassOf<ASDTAWeapon> WeaponClass;
	
	/** 此拾取物重生前的等待时间 */
	UPROPERTY(EditAnywhere, Category="Pickup", meta = (ClampMin = 0, ClampMax = 120, Units = "s"))
	float RespawnTime = 4.0f;

	/** 拾取物重生计时器 */
	FTimerHandle RespawnTimer;

public:	

	/** 构造函数 */
	ASDTAPickup();

protected:

	/** 原生构建脚本 */
	virtual void OnConstruction(const FTransform& Transform) override;

	/** 游戏初始化 */
	virtual void BeginPlay() override;

	/** 游戏清理 */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 处理碰撞重叠 */
	UFUNCTION()
	virtual void OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

protected:

	/** 当该拾取物需要重生时调用 */
	void RespawnPickup();

	/** 将控制传递给蓝图以动画化拾取物重生。应通过调用FinishRespawn结束 */
	UFUNCTION(BlueprintImplementableEvent, Category="Pickup", meta = (DisplayName = "OnRespawn"))
	void BP_OnRespawn();

	/** 重生后启用此拾取物 */
	UFUNCTION(BlueprintCallable, Category="Pickup")
	void FinishRespawn();
};