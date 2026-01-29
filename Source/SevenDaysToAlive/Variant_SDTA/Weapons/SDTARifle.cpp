// Fill out your copyright notice in the Description page of Project Settings.

#include "SDTARifle.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundBase.h"

// Sets default values
ASDTAWeaponRifle::ASDTAWeaponRifle()
{
	// 设置此actor在每一帧调用Tick()。如果不需要，可以关闭以提高性能
	PrimaryActorTick.bCanEverTick = true;

	// 设置步枪特有属性的默认值
	RifleRecoilMultiplier = 1.5f;
	RifleFireRate = 0.1f; // 每秒10发

	// 设置默认的武器数据
	WeaponData.DataTable = nullptr;
	WeaponData.RowName = FName("Rifle");
}

// Called when the game starts or when spawned
void ASDTAWeaponRifle::BeginPlay()
{
	Super::BeginPlay();
	
	// 初始化步枪特有逻辑
	UE_LOG(LogTemp, Log, TEXT("步枪武器初始化完成"));
}

// Called every frame
void ASDTAWeaponRifle::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 添加步枪特有的每帧逻辑
}

// 重写开火方法，实现步枪特有的开火逻辑
void ASDTAWeaponRifle::Fire()
{
	// 调用父类的开火逻辑
	Super::Fire();

	// 添加步枪特有的开火逻辑
	UE_LOG(LogTemp, Log, TEXT("步枪开火: 当前弹药 %d/%d"), CurrentAmmo, WeaponDataRow.MagazineSize);

	// 播放开火效果
	PlayFireEffects();

	// 应用步枪特有的后坐力
	if (WeaponOwner)
	{
		ISDTAWeaponHolder::Execute_AddWeaponRecoil(WeaponOwner.GetObject(), Recoil * RifleRecoilMultiplier);
	}
}

// 重写装填方法
void ASDTAWeaponRifle::Reload()
{
	// 调用父类的装填逻辑
	Super::Reload();

	// 添加步枪特有的装填逻辑
	UE_LOG(LogTemp, Log, TEXT("步枪装填弹药"));

	// 播放装填效果
	PlayReloadEffects();

	// 更新弹药数量
	CurrentAmmo = WeaponDataRow.MagazineSize;

	// 更新HUD
	if (WeaponOwner)
	{
		ISDTAWeaponHolder::Execute_UpdateWeaponHUD(WeaponOwner.GetObject(), CurrentAmmo, WeaponDataRow.MagazineSize);
	}
}

// 播放开火效果
void ASDTAWeaponRifle::PlayFireEffects()
{
	// 播放开火音效
	if (FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
	}

	// 播放枪口闪光粒子效果
	if (MuzzleFlash && FirstPersonMesh)
	{
		UGameplayStatics::SpawnEmitterAttached(
			MuzzleFlash,
			FirstPersonMesh,
			MuzzleSocketName,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget
		);
	}
}

// 播放装填效果
void ASDTAWeaponRifle::PlayReloadEffects()
{
	// 播放装填音效
	if (ReloadSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ReloadSound, GetActorLocation());
	}
}