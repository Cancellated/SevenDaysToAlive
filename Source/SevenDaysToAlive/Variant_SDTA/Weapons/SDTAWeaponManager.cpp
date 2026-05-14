// Fill out your copyright notice in the Description page of Project Settings.

#include "SDTAWeaponManager.h"
#include "Core/Game/SDTAPlayerState.h"
#include "SDTAWeapon.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogDiagnose, Log, All);

#define LOCTEXT_NAMESPACE "SDTAWeaponManager"

// 构造函数
USDTAWeaponManager::USDTAWeaponManager()
	: InitialWeaponName(TEXT("Rifle"))
	, InitialWeaponAmmo(30)
	, PlayerState(nullptr)
	, World(nullptr)
	, WeaponDataTable(nullptr)
	, CurrentWeaponName(NAME_None)
	, WeaponInventory()
	, bIsFiring(false)
	, LastFireTime(0.0f)
	, Recoil(0.0f)
	, RecoilDecayRate(5.0f)
	, CurrentWeaponActor(nullptr)
	, bIsReloading(false)
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

	UE_LOG(LogTemp, Log, TEXT("[WeaponManager]武器管理器初始化完成"));
}

/** 每帧更新（仅用于后坐力衰减等非开火逻辑） */
void USDTAWeaponManager::Tick(float DeltaTime)
{
	UpdateRecoilDecay(DeltaTime);
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
void USDTAWeaponManager::AddWeapon(const FName& WeaponName, int32 InitialAmmo)
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
				UE_LOG(LogTemp, Log, TEXT("[WeaponManager]成功加载武器数据: %s"), *WeaponTableRow->WeaponName);
				
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
				UE_LOG(LogTemp, Error,
					TEXT("[WeaponManager]AddWeapon: 数据表中未找到武器行 '%s'，请检查数据表配置，将使用传入的默认弹药值"),
					*WeaponName.ToString()
				);
				WeaponData.CurrentAmmo = InitialAmmo;
			}
		}
		else
		{
			UE_LOG(LogTemp, Error,
				TEXT("[WeaponManager]AddWeapon: WeaponDataTable未设置！无法加载武器 '%s' 的属性，请先配置武器数据表"),
				*WeaponName.ToString()
			);
			WeaponData.CurrentAmmo = InitialAmmo;
		}

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
		ServerAddWeapon(WeaponName, InitialAmmo);
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

/** 持续开火定时器回调 */
void USDTAWeaponManager::HandleFireRateTimer()
{
	if (!bIsFiring)
	{
		return;
	}

	if (CanFire() && IsFullAuto())
	{
		ProcessFire();
	}
}

// 开始开火
void USDTAWeaponManager::StartFiring()
{
	if (bIsFiring)
	{
		return;
	}

	bIsFiring = true;

	if (HasAuthority())
	{
		ProcessFire();

		float FireRate = GetCurrentFireRate();
		if (FireRate > 0.0f && IsFullAuto() && World)
		{
			float Interval = 1.0f / FireRate;
			World->GetTimerManager().SetTimer(
				FireRateTimerHandle,
				this,
				&USDTAWeaponManager::HandleFireRateTimer,
				Interval,
				true
			);
		}

		OnWeaponFireStateChanged.Broadcast(bIsFiring);
	}
	else
	{
		ServerStartFire();
	}
}

// 停止开火
void USDTAWeaponManager::StopFiring()
{
	if (!bIsFiring)
	{
		return;
	}

	bIsFiring = false;

	if (HasAuthority())
	{
		if (World)
		{
			World->GetTimerManager().ClearTimer(FireRateTimerHandle);
		}

		OnWeaponFireStateChanged.Broadcast(bIsFiring);
	}
	else
	{
		ServerStopFire();
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
	if (bIsReloading)
	{
		return;
	}

	if (CurrentWeaponName == NAME_None)
	{
		return;
	}

	bIsReloading = true;

	if (HasAuthority())
	{
		FWeaponInventoryData* WeaponData = FindWeaponData(CurrentWeaponName);
		if (WeaponData)
		{
			int32 MagazineSize = GetCurrentMagazineSize();
			int32 AmmoNeeded = MagazineSize - WeaponData->CurrentAmmo;

			if (AmmoNeeded > 0)
			{
				WeaponData->CurrentAmmo += AmmoNeeded;
				UpdateWeaponUI();
				UE_LOG(LogTemp, Log, TEXT("[WeaponManager]换弹完成: %s → %d/%d"),
					*CurrentWeaponName.ToString(), WeaponData->CurrentAmmo, MagazineSize);
			}
		}
	}
	else
	{
		ServerReload();
	}

	bIsReloading = false;
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
void USDTAWeaponManager::ServerAddWeapon_Implementation(const FName& WeaponName, int32 InitialAmmo)
{
	if (!WeaponDataTable)
	{
		UE_LOG(LogTemp, Error, TEXT("[WeaponManager]ServerAddWeapon: WeaponDataTable未设置"));
		return;
	}

	FSDTAWeaponTableRow* WeaponTableRow = WeaponDataTable->FindRow<FSDTAWeaponTableRow>(
		WeaponName,
		TEXT("[WeaponManager]ServerAddWeapon"),
		false
	);

	FWeaponInventoryData WeaponData;
	WeaponData.WeaponName = WeaponName;

	if (WeaponTableRow)
	{
		WeaponData.WeaponData = *WeaponTableRow;
		WeaponData.CurrentAmmo = (InitialAmmo > 0) ? FMath::Min(InitialAmmo, WeaponTableRow->MagazineSize) : WeaponTableRow->MagazineSize;
	}
	else
	{
		WeaponData.CurrentAmmo = InitialAmmo;
	}

	WeaponInventory.Add(WeaponData);

	if (CurrentWeaponName == NAME_None)
	{
		CurrentWeaponName = WeaponName;
	}

	UE_LOG(LogTemp, Log, TEXT("[WeaponManager]ServerAddWeapon成功: %s"), *WeaponName.ToString());
}

bool USDTAWeaponManager::ServerAddWeapon_Validate(const FName& WeaponName, int32 InitialAmmo)
{
	return WeaponName != NAME_None && InitialAmmo >= 0;
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
	
	OnWeaponFireStateChanged.Broadcast(bIsFiring);
}

/** 数据表复制到客户端后触发（通知角色重新初始化武器） */
void USDTAWeaponManager::OnRep_WeaponDataTable()
{
	UE_LOG(LogTemp, Log, TEXT("[WeaponManager]OnRep_WeaponDataTable: 数据表已同步到客户端: %s"),
		WeaponDataTable ? *WeaponDataTable->GetName() : TEXT("None"));

	OnDataTableReady.Broadcast();
}

/** 当前武器Actor复制到客户端后触发（挂载Mesh到本地Pawn） */
void USDTAWeaponManager::OnRep_CurrentWeaponActor()
{
	if (!CurrentWeaponActor)
	{
		return;
	}

	UE_LOG(LogDiagnose, Log, TEXT("[Rep] OnRep_CurrentWeaponActor: %s (PlayerState=%p)"),
		*CurrentWeaponActor->GetName(),
		PlayerState);

	TScriptInterface<ISDTAWeaponHolder> WeaponHolder = GetWeaponHolder();

	if (!WeaponHolder.GetObject())
	{
		UE_LOG(LogDiagnose, Warning, TEXT("[Rep] WeaponHolder为空! PlayerState=%p Pawn=%p 延迟重试..."),
			PlayerState,
			(PlayerState ? PlayerState->GetPawn() : nullptr));

		if (World)
		{
			FTimerDelegate Delegate = FTimerDelegate::CreateUObject(this, &USDTAWeaponManager::OnRep_CurrentWeaponActor);
			World->GetTimerManager().SetTimerForNextTick(Delegate);
		}
		return;
	}

	ISDTAWeaponHolder::Execute_AttachWeaponMeshes(WeaponHolder.GetObject(), CurrentWeaponActor);
	UE_LOG(LogTemp, Log, TEXT("[WeaponManager]客户端武器Mesh已挂载"));
}

// 网络同步
void USDTAWeaponManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(USDTAWeaponManager, WeaponDataTable);
	DOREPLIFETIME(USDTAWeaponManager, CurrentWeaponName);
	DOREPLIFETIME(USDTAWeaponManager, CurrentWeaponActor);
	DOREPLIFETIME(USDTAWeaponManager, WeaponInventory);
	DOREPLIFETIME(USDTAWeaponManager, bIsFiring);
}

// 检查是否有权威权限(服务器端)
bool USDTAWeaponManager::HasAuthority() const
{
	if (PlayerState)
	{
		return PlayerState->HasAuthority();
	}
	return false;
}

// 检查是否是本地玩家控制器(客户端)
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

// 装备初始武器
void USDTAWeaponManager::EquipInitialWeapon()
{
	if (InitialWeaponName != NAME_None)
	{
		if (HasAuthority())
		{
			AddWeapon(InitialWeaponName, InitialWeaponAmmo);
			UE_LOG(LogTemp, Log, TEXT("装备初始武器(数据): %s"), *InitialWeaponName.ToString());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("客户端尝试装备初始武器，但需要通过服务器RPC"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("未设置初始武器名称"));
	}
}

/**
 * 生成武器Actor实例
 * 
 * 原理说明：
 * 1. 使用UWorld::SpawnActor生成武器Actor（这是UE中创建Actor的标准方式）
 * 2. 通过ISDTAWeaponHolder接口将武器网格挂载到角色骨骼上
 * 3. 激活武器使其进入可用状态
 * 
 * 为什么用SpawnActor而不是NewObject？
 * - Actor需要在世界中有物理位置和变换，必须通过SpawnActor创建
 * - NewObject只能创建UObject，不具备Actor的物理存在性
 */
ASDTAWeapon* USDTAWeaponManager::SpawnWeaponActor(TSubclassOf<ASDTAWeapon> WeaponClass)
{
	if (!WeaponClass || !World)
	{
		UE_LOG(LogTemp, Warning, TEXT("SpawnWeaponActor失败: 武器类或World无效"));
		return nullptr;
	}

	FVector SpawnLocation = FVector::ZeroVector;
	FRotator SpawnRotation = FRotator::ZeroRotator;
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ASDTAWeapon* NewWeapon = World->SpawnActor<ASDTAWeapon>(WeaponClass, SpawnLocation, SpawnRotation, SpawnParams);
	if (!NewWeapon)
	{
		UE_LOG(LogTemp, Error, TEXT("SpawnWeaponActor失败: 无法生成武器Actor"));
		return nullptr;
	}

	APawn* OwnerPawn = PlayerState ? PlayerState->GetPawn() : nullptr;
	if (OwnerPawn)
	{
		NewWeapon->SetWeaponOwner(OwnerPawn);
		NewWeapon->SetOwner(OwnerPawn);
	}

	TScriptInterface<ISDTAWeaponHolder> WeaponHolder = GetWeaponHolder();
	if (WeaponHolder)
	{
		ISDTAWeaponHolder::Execute_AttachWeaponMeshes(WeaponHolder.GetObject(), NewWeapon);
	}

	NewWeapon->ActivateWeapon();

	UE_LOG(LogTemp, Log, TEXT("武器Actor生成成功: %s"), *NewWeapon->GetName());
	return NewWeapon;
}

/**
 * 装备初始武器并生成Actor（由调用方传入配置）
 *
 * 数据流：Character传入Class+Name → Manager查DataTable获取全部属性 → SpawnActor
 */
void USDTAWeaponManager::EquipInitialWeaponWithActor(
	TSubclassOf<ASDTAWeapon> InWeaponClass,
	const FName& InWeaponName)
{
	if (!InWeaponClass)
	{
		UE_LOG(LogTemp, Error, TEXT("[WeaponManager]EquipInitialWeaponWithActor: 武器类为空，无法装备"));
		return;
	}

	if (!WeaponDataTable)
	{
		UE_LOG(LogTemp, Error, TEXT("[WeaponManager]EquipInitialWeaponWithActor: WeaponDataTable未设置！请在GameMode或PlayerState中配置武器数据表"));
		return;
	}

	FName WeaponNameToUse = (InWeaponName != NAME_None) ? InWeaponName : InWeaponClass->GetFName();

	const FSDTAWeaponTableRow* WeaponRow = WeaponDataTable->FindRow<FSDTAWeaponTableRow>(
		WeaponNameToUse,
		TEXT("[WeaponManager]EquipInitialWeaponWithActor"),
		false
	);

	if (!WeaponRow)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[WeaponManager]EquipInitialWeaponWithActor: 数据表中未找到武器行 '%s'，请检查数据表配置"),
			*WeaponNameToUse.ToString()
		);
		return;
	}

	AddWeapon(WeaponNameToUse, WeaponRow->MagazineSize);

	OnWeaponDataReady.Broadcast(
		WeaponRow->FirstPersonAnimInstanceClass,
		WeaponRow->ThirdPersonAnimInstanceClass
	);

	if (!HasAuthority())
	{
		UE_LOG(LogTemp, Log, TEXT("[WeaponManager]EquipInitialWeaponWithActor: 客户端跳过Actor生成"));
		return;
	}

	CurrentWeaponActor = SpawnWeaponActor(InWeaponClass);
	if (CurrentWeaponActor)
	{
		CurrentWeaponActor->SetWeaponDataRow(*WeaponRow);

		UpdateWeaponUI();
		OnCurrentWeaponChanged.Broadcast(CurrentWeaponName);
		UE_LOG(LogTemp, Log, TEXT("[WeaponManager]初始武器装备完成(数据表驱动): %s"), *WeaponNameToUse.ToString());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[WeaponManager]初始武器Actor生成失败"));
	}
}

// 设置初始武器
void USDTAWeaponManager::SetInitialWeapon(const FName& WeaponName)
{
	InitialWeaponName = WeaponName;
}

// 设置初始武器弹药数量
void USDTAWeaponManager::SetInitialWeaponAmmo(int32 InitialAmmo)
{
	InitialWeaponAmmo = InitialAmmo;
}

#undef LOCTEXT_NAMESPACE