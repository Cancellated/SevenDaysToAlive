// Fill out your copyright notice in the Description page of Project Settings.

#include "SDTAWeaponManager.h"
#include "Core/Game/SDTAPlayerState.h"
#include "SDTAWeapon.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

#define LOCTEXT_NAMESPACE "SDTAWeaponManager"

// 构造函数
USDTAWeaponManager::USDTAWeaponManager()
	: PlayerState(nullptr)
	, World(nullptr)
	, WeaponDataTable(nullptr)
	, CurrentWeaponName(NAME_None)
	, bIsFiring(false)
	, LastFireTime(0.0f)
	, Recoil(0.0f)
	, RecoilDecayRate(5.0f)
	, CurrentWeaponActor(nullptr)
{
	// 设置默认值
}

// 初始化武器管理器
void USDTAWeaponManager::Initialize(ASDTAPlayerState* InPlayerState)
{
	PlayerState = InPlayerState;
	World = PlayerState->GetWorld();

	// 加载武器数据
	LoadWeaponData();

	// 设置初始状态
	ResetState();

	UE_LOG(LogTemp, Log, TEXT("武器管理器初始化完成"));
}

// 更新武器管理器状态
void USDTAWeaponManager::Tick(float DeltaTime)
{
	// 更新冷却时间
	UpdateCooldown(DeltaTime);

	// 更新后坐力衰减
	UpdateRecoilDecay(DeltaTime);

	// 处理持续开火
	if (bIsFiring && CanFire() && IsFullAuto())
	{
		ProcessFire();
	}
}

// 获取当前武器
FName USDTAWeaponManager::GetCurrentWeapon() const
{
	return CurrentWeaponName;
}

// 切换武器
void USDTAWeaponManager::SwitchWeapon(const FName& WeaponName)
{
	if (CurrentWeaponName == WeaponName)
	{
		return;
	}

	if (FindWeaponData(WeaponName))
	{
		if (HasAuthority())
		{
			// 服务器端直接处理
			CurrentWeaponName = WeaponName;
			// 更新武器Actor
			CurrentWeaponActor = nullptr; // 重置当前武器Actor引用
			// 更新UI
			UpdateWeaponUI();
			// 触发武器切换委托
			OnCurrentWeaponChanged.Broadcast(CurrentWeaponName);
		}
		else
		{
			// 客户端请求服务器切换
			ServerSwitchWeapon(WeaponName);
		}
	}
}

// 添加武器到库存
void USDTAWeaponManager::AddWeapon(const FName& WeaponName, int32 InitialAmmo, int32 ReserveAmmo)
{
	if (HasAuthority())
	{
		// 检查武器是否已存在
		if (FindWeaponData(WeaponName))
		{
			return;
		}

		// 服务器端直接处理
		FWeaponInventoryData WeaponData;
		WeaponData.WeaponName = WeaponName;
		
		// 尝试从数据表格加载武器数据
		const FSDTAWeaponTableRow* WeaponTableRow = nullptr;
		if (WeaponDataTable)
		{
			WeaponTableRow = WeaponDataTable->FindRow<FSDTAWeaponTableRow>(WeaponName, TEXT("USDTAWeaponManager::AddWeapon"), false);
			if (WeaponTableRow)
			{
				// 加载武器数据
				WeaponData.WeaponData = *WeaponTableRow;
				UE_LOG(LogTemp, Log, TEXT("成功加载武器数据: %s"), *WeaponTableRow->WeaponName);
				
				// 如果初始弹药未指定，使用弹匣容量
				if (InitialAmmo <= 0)
				{
					WeaponData.CurrentAmmo = WeaponTableRow->MagazineSize;
				}
				else
				{
					WeaponData.CurrentAmmo = InitialAmmo;
				}
				
				// 确保初始弹药不超过弹匣容量
				WeaponData.CurrentAmmo = FMath::Min(WeaponData.CurrentAmmo, WeaponTableRow->MagazineSize);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("无法从数据表格加载武器数据: %s"), *WeaponName.ToString());
				// 使用默认值
				WeaponData.CurrentAmmo = InitialAmmo;
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("未设置武器数据表格，无法加载武器数据: %s"), *WeaponName.ToString());
			// 使用默认值
			WeaponData.CurrentAmmo = InitialAmmo;
		}
		
		// 设置后备弹药
		WeaponData.ReserveAmmo = ReserveAmmo;

		WeaponInventory.Add(WeaponData);

		// 如果是第一个武器，自动装备
		if (CurrentWeaponName == NAME_None)
		{
			CurrentWeaponName = WeaponName;
		}
	}
	else
	{
		// 客户端请求服务器添加
		ServerAddWeapon(WeaponName, InitialAmmo, ReserveAmmo);
	}
}

// 移除武器从库存
void USDTAWeaponManager::RemoveWeapon(const FName& WeaponName)
{
	if (HasAuthority())
	{
		// 服务器端直接处理：手动移除匹配的武器
		for (int32 i = WeaponInventory.Num() - 1; i >= 0; --i)
		{
			if (WeaponInventory[i].WeaponName == WeaponName)
			{
				WeaponInventory.RemoveAt(i);
			}
		}

		// 如果移除的是当前武器，切换到第一个可用武器或清空
		if (CurrentWeaponName == WeaponName)
		{
			if (WeaponInventory.Num() > 0)
			{
				CurrentWeaponName = WeaponInventory[0].WeaponName;
			}
			else
			{
				CurrentWeaponName = NAME_None;
			}
			// 更新武器Actor
			CurrentWeaponActor = nullptr; // 重置当前武器Actor引用
		}
	}
	else
	{
		// 客户端请求服务器移除
		ServerRemoveWeapon(WeaponName);
	}
}

// 辅助函数：通过武器名称查找武器数据
FWeaponInventoryData* USDTAWeaponManager::FindWeaponData(const FName& WeaponName)
{
	for (FWeaponInventoryData& WeaponData : WeaponInventory)
	{
		if (WeaponData.WeaponName == WeaponName)
		{
			return &WeaponData;
		}
	}
	return nullptr;
}

// 辅助函数：通过武器名称查找武器数据（const版本）
const FWeaponInventoryData* USDTAWeaponManager::FindWeaponData(const FName& WeaponName) const
{
	for (const FWeaponInventoryData& WeaponData : WeaponInventory)
	{
		if (WeaponData.WeaponName == WeaponName)
		{
			return &WeaponData;
		}
	}
	return nullptr;
}

// 获取武器库存
TArray<FWeaponInventoryData> USDTAWeaponManager::GetWeaponInventory() const
{
	return WeaponInventory;
}

// 开始开火
void USDTAWeaponManager::StartFiring()
{
	if (!bIsFiring)
	{
		bIsFiring = true;

		if (HasAuthority())
		{
			// 服务器端直接处理
			ProcessFire();
			// 触发开火状态变更委托
			OnWeaponFireStateChanged.Broadcast(bIsFiring);
		}
		else
		{
			// 客户端请求服务器开始开火
			ServerStartFire();
		}
	}
}

// 停止开火
void USDTAWeaponManager::StopFiring()
{
	if (bIsFiring)
	{
		bIsFiring = false;

		if (HasAuthority())
		{
			// 服务器端直接处理
			// 触发开火状态变更委托
			OnWeaponFireStateChanged.Broadcast(bIsFiring);
		}
		else
		{
			// 客户端请求服务器停止开火
			ServerStopFire();
		}
	}
}

// 单次开火
void USDTAWeaponManager::FireOnce()
{
	if (CanFire() && !bIsFiring)
	{
		if (HasAuthority())
		{
			// 服务器端直接处理
			ProcessFire();
		}
		else
		{
			// 客户端请求服务器单次开火
			ServerFireOnce();
		}
	}
}

// 检查是否可以开火
bool USDTAWeaponManager::CanFire() const
{
	// 如果没有当前武器，不能开火
	if (CurrentWeaponName == NAME_None)
	{
		return false;
	}

	// 查找当前武器数据
	const FWeaponInventoryData* WeaponData = FindWeaponData(CurrentWeaponName);
	if (!WeaponData)
	{
		return false;
	}

	// 检查弹药
	if (WeaponData->CurrentAmmo <= 0)
	{
		return false;
	}

	// 检查冷却时间
	float FireRate = GetCurrentFireRate();
	float CooldownTime = 1.0f / FireRate;
	float CurrentTime = World ? World->GetTimeSeconds() : 0.0f;
	
	if (CurrentTime - LastFireTime < CooldownTime)
	{
		return false;
	}

	return true;
}

// 检查是否正在开火
bool USDTAWeaponManager::IsFiring() const
{
	return bIsFiring;
}

// 重新装填弹药
void USDTAWeaponManager::ReloadCurrentWeapon()
{
	if (CurrentWeaponName == NAME_None)
	{
		return;
	}

	if (HasAuthority())
	{
		// 服务器端直接处理
		FWeaponInventoryData* WeaponData = FindWeaponData(CurrentWeaponName);
		if (WeaponData)
		{
			int32 MagazineSize = GetCurrentMagazineSize();
			int32 AmmoNeeded = MagazineSize - WeaponData->CurrentAmmo;

			if (AmmoNeeded > 0 && WeaponData->ReserveAmmo > 0)
			{
				int32 AmmoToLoad = FMath::Min(AmmoNeeded, WeaponData->ReserveAmmo);
				WeaponData->CurrentAmmo += AmmoToLoad;
				WeaponData->ReserveAmmo -= AmmoToLoad;
				// 更新UI
				UpdateWeaponUI();
			}
		}
	}
	else
	{
		// 客户端请求服务器重新装填
		ServerReload();
	}
}

// 检查弹药是否充足
bool USDTAWeaponManager::HasEnoughAmmo(const FName& WeaponName, int32 RequiredAmmo) const
{
	FName CheckWeaponName = WeaponName;
	if (CheckWeaponName == NAME_None)
	{
		CheckWeaponName = CurrentWeaponName;
	}

	if (CheckWeaponName != NAME_None)
	{
		const FWeaponInventoryData* WeaponData = FindWeaponData(CheckWeaponName);
		if (WeaponData)
		{
			return WeaponData->CurrentAmmo >= RequiredAmmo;
		}
	}

	return false;
}

// 获取当前武器的当前弹药数量
int32 USDTAWeaponManager::GetCurrentWeaponAmmo() const
{
	const FWeaponInventoryData* WeaponData = FindWeaponData(CurrentWeaponName);
	if (WeaponData)
	{
		return WeaponData->CurrentAmmo;
	}
	return 0;
}

// 获取当前武器的弹匣容量
int32 USDTAWeaponManager::GetCurrentWeaponMagazineSize() const
{
	const FWeaponInventoryData* WeaponData = FindWeaponData(CurrentWeaponName);
	if (WeaponData)
	{
		return WeaponData->WeaponData.MagazineSize;
	}
	return 0;
}

// 获取当前武器的伤害
float USDTAWeaponManager::GetCurrentWeaponDamage() const
{
	const FWeaponInventoryData* WeaponData = FindWeaponData(CurrentWeaponName);
	if (WeaponData)
	{
		return WeaponData->WeaponData.Damage;
	}
	return 0.0f;
}

// 获取当前武器的射程
float USDTAWeaponManager::GetCurrentWeaponRange() const
{
	const FWeaponInventoryData* WeaponData = FindWeaponData(CurrentWeaponName);
	if (WeaponData)
	{
		return WeaponData->WeaponData.Range;
	}
	return 0.0f;
}

// 获取当前武器的名称
FString USDTAWeaponManager::GetCurrentWeaponName() const
{
	const FWeaponInventoryData* WeaponData = FindWeaponData(CurrentWeaponName);
	if (WeaponData)
	{
		return WeaponData->WeaponData.WeaponName;
	}
	return TEXT("Unknown Weapon");
}

// 获取当前武器的数据行
bool USDTAWeaponManager::GetCurrentWeaponDataRow(FSDTAWeaponTableRow& OutWeaponDataRow) const
{
	const FWeaponInventoryData* WeaponData = FindWeaponData(CurrentWeaponName);
	if (WeaponData)
	{
		OutWeaponDataRow = WeaponData->WeaponData;
		return true;
	}
	return false;
}

// 处理开火逻辑
void USDTAWeaponManager::ProcessFire()
{
	if (!CanFire())
	{
		return;
	}

	// 消耗弹药
	FWeaponInventoryData* WeaponData = FindWeaponData(CurrentWeaponName);
	if (WeaponData)
	{
		WeaponData->CurrentAmmo--;
		// 更新UI
		UpdateWeaponUI();
	}

	// 更新开火时间
	if (World)
	{
		LastFireTime = World->GetTimeSeconds();
	}

	// 增加后坐力
	Recoil = FMath::Clamp(Recoil + 1.0f, 0.0f, 10.0f);

	// 更新武器Actor
	// TODO: 实现武器Actor的开火逻辑调用
	if (CurrentWeaponActor)
	{
		CurrentWeaponActor->Fire();
	}

	UE_LOG(LogTemp, Log, TEXT("武器管理器处理开火: %s, 剩余弹药: %d"), 
		*CurrentWeaponName.ToString(), 
		WeaponData ? WeaponData->CurrentAmmo : 0);
}

// 更新冷却时间
void USDTAWeaponManager::UpdateCooldown(float DeltaTime)
{
	// 冷却时间在CanFire中计算，不需要在这里单独更新
}

// 更新后坐力衰减
void USDTAWeaponManager::UpdateRecoilDecay(float DeltaTime)
{
	if (Recoil > 0.0f)
	{
		Recoil = FMath::Max(Recoil - (RecoilDecayRate * DeltaTime), 0.0f);
	}
}

// 设置武器数据表格
void USDTAWeaponManager::SetWeaponDataTable(UDataTable* InWeaponDataTable)
{
	WeaponDataTable = InWeaponDataTable;
	UE_LOG(LogTemp, Log, TEXT("武器数据表格已设置"));
}

// 获取武器数据表格
UDataTable* USDTAWeaponManager::GetWeaponDataTable() const
{
	return WeaponDataTable;
}

// 通过武器名称获取武器数据
bool USDTAWeaponManager::GetWeaponDataByName(const FName& WeaponName, FSDTAWeaponTableRow& OutWeaponData) const
{
	if (WeaponDataTable)
	{
		const FSDTAWeaponTableRow* WeaponRow = WeaponDataTable->FindRow<FSDTAWeaponTableRow>(WeaponName, TEXT("USDTAWeaponManager::GetWeaponDataByName"), false);
		if (WeaponRow)
		{
			OutWeaponData = *WeaponRow;
			return true;
		}
	}
	return false;
}

// 加载武器数据
void USDTAWeaponManager::LoadWeaponData()
{
	// 目前这个方法可以用于加载默认武器或从其他来源获取武器数据
	// 实际的武器数据加载在AddWeapon方法中进行
	UE_LOG(LogTemp, Log, TEXT("武器数据加载方法调用"));
}

// 重置武器状态
void USDTAWeaponManager::ResetState()
{
	// 重置开火状态
	bIsFiring = false;
	LastFireTime = 0.0f;
	Recoil = 0.0f;

	// 重置武器Actor引用
	CurrentWeaponActor = nullptr;
}

// 检查是否是全自动武器
bool USDTAWeaponManager::IsFullAuto() const
{
	if (CurrentWeaponName != NAME_None)
	{
		const FWeaponInventoryData* WeaponData = FindWeaponData(CurrentWeaponName);
		if (WeaponData)
		{
			return WeaponData->WeaponData.bFullAuto;
		}
	}

	return false;
}

// 当前武器的射速
float USDTAWeaponManager::GetCurrentFireRate() const
{
	if (CurrentWeaponName != NAME_None)
	{
		const FWeaponInventoryData* WeaponData = FindWeaponData(CurrentWeaponName);
		if (WeaponData)
		{
			return WeaponData->WeaponData.FireRate;
		}
	}

	return 1.0f; // 默认射速
}

// 当前武器的弹匣容量
int32 USDTAWeaponManager::GetCurrentMagazineSize() const
{
	if (CurrentWeaponName != NAME_None)
	{
		const FWeaponInventoryData* WeaponData = FindWeaponData(CurrentWeaponName);
		if (WeaponData)
		{
			return WeaponData->WeaponData.MagazineSize;
		}
	}

	return 30; // 默认弹匣容量
}

// 服务器端开始开火
void USDTAWeaponManager::ServerStartFire_Implementation()
{
	StartFiring();
}

bool USDTAWeaponManager::ServerStartFire_Validate()
{
	return true;
}

// 服务器端停止开火
void USDTAWeaponManager::ServerStopFire_Implementation()
{
	StopFiring();
}

bool USDTAWeaponManager::ServerStopFire_Validate()
{
	return true;
}

// 服务器端单次开火
void USDTAWeaponManager::ServerFireOnce_Implementation()
{
	FireOnce();
}

bool USDTAWeaponManager::ServerFireOnce_Validate()
{
	return true;
}

// 服务器端重新装填
void USDTAWeaponManager::ServerReload_Implementation()
{
	ReloadCurrentWeapon();
}

bool USDTAWeaponManager::ServerReload_Validate()
{
	return true;
}

// 服务器端切换武器
void USDTAWeaponManager::ServerSwitchWeapon_Implementation(const FName& WeaponName)
{
	SwitchWeapon(WeaponName);
}

bool USDTAWeaponManager::ServerSwitchWeapon_Validate(const FName& WeaponName)
{
	return WeaponName != NAME_None;
}

// 服务器端添加武器
void USDTAWeaponManager::ServerAddWeapon_Implementation(const FName& WeaponName, int32 InitialAmmo, int32 ReserveAmmo)
{
	AddWeapon(WeaponName, InitialAmmo, ReserveAmmo);
}

bool USDTAWeaponManager::ServerAddWeapon_Validate(const FName& WeaponName, int32 InitialAmmo, int32 ReserveAmmo)
{
	return WeaponName != NAME_None && InitialAmmo >= 0 && ReserveAmmo >= 0;
}

// 服务器端移除武器
void USDTAWeaponManager::ServerRemoveWeapon_Implementation(const FName& WeaponName)
{
	RemoveWeapon(WeaponName);
}

bool USDTAWeaponManager::ServerRemoveWeapon_Validate(const FName& WeaponName)
{
	return WeaponName != NAME_None;
}

// 当前武器变更的RepNotify
void USDTAWeaponManager::OnRep_CurrentWeapon()
{
	UE_LOG(LogTemp, Log, TEXT("武器变更RepNotify: %s"), *CurrentWeaponName.ToString());

	// 更新武器Actor
	CurrentWeaponActor = nullptr; // 重置当前武器Actor引用
	
	// 更新UI
	UpdateWeaponUI();
	
	// 触发武器切换委托
	OnCurrentWeaponChanged.Broadcast(CurrentWeaponName);
}

// 开火状态变更的RepNotify
void USDTAWeaponManager::OnRep_IsFiring()
{
	UE_LOG(LogTemp, Log, TEXT("开火状态变更RepNotify: %s"), bIsFiring ? TEXT("是") : TEXT("否"));
	
	// 触发开火状态变更委托
	OnWeaponFireStateChanged.Broadcast(bIsFiring);
}

// 网络同步
void USDTAWeaponManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 同步当前武器
	DOREPLIFETIME(USDTAWeaponManager, CurrentWeaponName);

	// 同步武器库存
	DOREPLIFETIME(USDTAWeaponManager, WeaponInventory);

	// 同步开火状态
	DOREPLIFETIME(USDTAWeaponManager, bIsFiring);
}

// 实现HasAuthority方法
bool USDTAWeaponManager::HasAuthority() const
{
	if (PlayerState)
	{
		return PlayerState->HasAuthority();
	}
	return false;
}

// 实现IsLocalPlayer方法
bool USDTAWeaponManager::IsLocalPlayer() const
{
	if (PlayerState)
	{
		// 获取PlayerController
		APlayerController* PlayerController = PlayerState->GetOwner<APlayerController>();
		if (PlayerController)
		{
			// 检查是否是本地玩家控制器
			return PlayerController->IsLocalPlayerController();
		}
	}
	return false;
}

// 获取武器持有者接口
TScriptInterface<ISDTAWeaponHolder> USDTAWeaponManager::GetWeaponHolder() const
{
	if (PlayerState && PlayerState->GetPawn())
	{
		APawn* Pawn = PlayerState->GetPawn();
		if (Pawn->Implements<USDTAWeaponHolder>())
		{
			return TScriptInterface<ISDTAWeaponHolder>(Pawn);
		}
	}
	return TScriptInterface<ISDTAWeaponHolder>();
}

// 更新武器UI
void USDTAWeaponManager::UpdateWeaponUI()
{
	// 获取当前武器数据
	const FWeaponInventoryData* WeaponData = FindWeaponData(CurrentWeaponName);
	if (!WeaponData)
	{
		return;
	}

	// 获取武器持有者接口
	TScriptInterface<ISDTAWeaponHolder> WeaponHolder = GetWeaponHolder();
	if (WeaponHolder)
	{
		// 使用接口更新UI
		ISDTAWeaponHolder::Execute_UpdateWeaponHUD(WeaponHolder.GetObject(), 
			WeaponData->CurrentAmmo, 
			WeaponData->WeaponData.MagazineSize);
	}

	// 触发委托
	OnWeaponAmmoChanged.Broadcast(WeaponData->CurrentAmmo, WeaponData->WeaponData.MagazineSize);
}

#undef LOCTEXT_NAMESPACE