// Fill out your copyright notice in the Description page of Project Settings.

#include "USDTAWeaponManagerComponent.h"
#include "Net/UnrealNetwork.h"
#include "EnhancedInputComponent.h"

// Sets default values for this component's properties
USDTAWeaponManagerComponent::USDTAWeaponManagerComponent()
{
	// 设置此组件在游戏开始时被初始化
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);

	// 默认值设置
	FirstPersonWeaponSocket = FName("HandGrip_R");
	ThirdPersonWeaponSocket = FName("HandGrip_R");
	MaxAimDistance = 10000.0f;
	bIsFiring = false;
	bIsReloading = false;
	bIsSwitchingWeapon = false;
}

// 服务器端添加武器类，客户端请求时调用，加载时将数据表格配置传递给武器实例
void USDTAWeaponManagerComponent::AddWeaponClass(TSubclassOf<ASDTAWeapon> WeaponClass)
{
	if (GetOwnerRole() < ROLE_Authority)
	{
		ServerAddWeaponClass(WeaponClass);
		return;
	}

	if (!WeaponClass)
	{
		return;
	}

	// 生成武器实例
	ASDTAWeapon* NewWeapon = GetWorld()->SpawnActor<ASDTAWeapon>(WeaponClass);
	if (NewWeapon)
	{
		// 从武器类默认对象获取数据表格配置
	if (ASDTAWeapon* DefaultWeapon = WeaponClass->GetDefaultObject<ASDTAWeapon>())
	{
		// 获取默认武器的数据表格句柄
		FDataTableRowHandle DefaultWeaponData = DefaultWeapon->GetWeaponData();
		
		// 如果默认对象有数据表格配置，则加载数据
		if (DefaultWeaponData.DataTable)
		{
			NewWeapon->SetWeaponData(DefaultWeaponData);
			UE_LOG(LogTemp, Log, TEXT("设置武器数据表格成功"));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("默认武器对象没有数据表格配置"));
		}
	}
		
		// 设置武器的持有者和所有权
		NewWeapon->SetWeaponOwner(GetOwner());
		NewWeapon->SetOwner(GetOwner());

		// 将武器附加到玩家身上
		AttachWeaponMeshes(NewWeapon);

		// 添加到武器列表
		OwnedWeapons.Add(NewWeapon);

		// 如果是第一个武器，自动装备并激活
		if (CurrentWeapon == nullptr)
		{
			CurrentWeapon = NewWeapon;
			NewWeapon->ActivateWeapon();
			
			// 切换动画蓝图
			SwitchWeaponAnimInstance(CurrentWeapon);
		}
	}
}

void USDTAWeaponManagerComponent::SwitchToNextWeapon()
{
	if (GetOwnerRole() < ROLE_Authority)
	{
		ServerSwitchToNextWeapon();
		return;
	}

	if (OwnedWeapons.Num() <= 1)
	{
		return;
	}

	int32 CurrentIndex = OwnedWeapons.Find(CurrentWeapon);
	int32 NextIndex = (CurrentIndex + 1) % OwnedWeapons.Num();
	
	InternalSwitchWeapon(OwnedWeapons[NextIndex]);
}

void USDTAWeaponManagerComponent::SwitchToPreviousWeapon()
{
	if (GetOwnerRole() < ROLE_Authority)
	{
		ServerSwitchToPreviousWeapon();
		return;
	}

	if (OwnedWeapons.Num() <= 1)
	{
		return;
	}

	int32 CurrentIndex = OwnedWeapons.Find(CurrentWeapon);
	int32 PreviousIndex = (CurrentIndex - 1 + OwnedWeapons.Num()) % OwnedWeapons.Num();
	
	InternalSwitchWeapon(OwnedWeapons[PreviousIndex]);
}

void USDTAWeaponManagerComponent::SwitchToWeapon(int32 WeaponIndex)
{
	if (GetOwnerRole() < ROLE_Authority)
	{
		ServerSwitchToWeapon(WeaponIndex);
		return;
	}

	if (!OwnedWeapons.IsValidIndex(WeaponIndex))
	{
		return;
	}

	InternalSwitchWeapon(OwnedWeapons[WeaponIndex]);
}

void USDTAWeaponManagerComponent::StartFire()
{
	if (GetOwnerRole() < ROLE_Authority)
	{
		ServerStartFire();
		return;
	}

	if (CurrentWeapon && !bIsFiring && !bIsReloading && !bIsSwitchingWeapon)
	{
		bIsFiring = true;
		CurrentWeapon->StartFiring();
	}
}

void USDTAWeaponManagerComponent::StopFire()
{
	if (GetOwnerRole() < ROLE_Authority)
	{
		ServerStopFire();
		return;
	}

	if (CurrentWeapon && bIsFiring)
	{
		bIsFiring = false;
		CurrentWeapon->StopFiring();
	}
}

void USDTAWeaponManagerComponent::Reload()
{
	if (GetOwnerRole() < ROLE_Authority)
	{
		ServerReload();
		return;
	}

	if (CurrentWeapon && !bIsReloading && !bIsSwitchingWeapon)
	{
		bIsReloading = true;
		CurrentWeapon->Reload();
		// 这里应该有一个装填完成的回调来重置bIsReloading
	}
}

void USDTAWeaponManagerComponent::SetupInputBindings(UEnhancedInputComponent* InputComponent)
{
	// 输入绑定将在具体的输入配置中实现
	// 这里只是占位实现
}

void USDTAWeaponManagerComponent::EquipInitialWeapon()
{
	if (!InitialWeaponClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("未设置初始武器类"));
		return;
	}

	// 添加初始武器
	AddWeaponClass(InitialWeaponClass);
	
	UE_LOG(LogTemp, Log, TEXT("装备初始武器: %s"), *InitialWeaponClass->GetName());
}

void USDTAWeaponManagerComponent::AttachWeaponMeshes(ASDTAWeapon* Weapon)
{
	if (!Weapon || !GetOwner())
	{
		return;
	}

	AActor* OwnerActor = Cast<AActor>(GetOwner());
	if (!OwnerActor)
	{
		return;
	}

	// 将武器actor附加到角色actor上
	FAttachmentTransformRules AttachmentRule(EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::KeepWorld, false);
	Weapon->AttachToActor(OwnerActor, AttachmentRule);

	// 将武器的第一人称和第三人称网格组件附加到角色的第一人称和第三人称网格组件上
	ACharacter* Character = Cast<ACharacter>(OwnerActor);
	if (Character)
	{
		if (Weapon->GetFirstPersonMesh())
		{
			// 获取角色的第一人称网格组件
			USkeletalMeshComponent* FirstPersonMesh = nullptr;
			ASDTAPlayerBase* PlayerBase = Cast<ASDTAPlayerBase>(Character);
			if (PlayerBase && PlayerBase->GetFirstPersonMesh())
			{
				FirstPersonMesh = PlayerBase->GetFirstPersonMesh();
			}
			
			if (FirstPersonMesh)
			{
				Weapon->GetFirstPersonMesh()->AttachToComponent(FirstPersonMesh, AttachmentRule, FirstPersonWeaponSocket);
			}
			else
			{
				// 如果没有找到第一人称网格，附加到根组件
				Weapon->GetFirstPersonMesh()->AttachToComponent(OwnerActor->GetRootComponent(), AttachmentRule, FirstPersonWeaponSocket);
			}
		}
		
		if (Weapon->GetThirdPersonMesh())
		{
			// 获取角色的第三人称网格组件（使用ACharacter的GetMesh方法）
			USkeletalMeshComponent* ThirdPersonMesh = Character->GetMesh();
			
			if (ThirdPersonMesh)
			{
				Weapon->GetThirdPersonMesh()->AttachToComponent(ThirdPersonMesh, AttachmentRule, ThirdPersonWeaponSocket);
			}
			else
			{
				// 如果没有找到第三人称网格，附加到根组件
				Weapon->GetThirdPersonMesh()->AttachToComponent(OwnerActor->GetRootComponent(), AttachmentRule, ThirdPersonWeaponSocket);
			}
		}
	}
	
	UE_LOG(LogTemp, Log, TEXT("武器网格挂载完成: %s"), *Weapon->GetName());
}

void USDTAWeaponManagerComponent::SwitchWeaponAnimInstance(ASDTAWeapon* Weapon)
{
	if (!Weapon || !GetOwner())
	{
		return;
	}

	// 获取角色的动画实例
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character)
	{
		return;
	}

	// 获取武器的动画实例类
	TSubclassOf<UAnimInstance> FirstPersonAnimClass = Weapon->GetFirstPersonAnimInstanceClass();
	TSubclassOf<UAnimInstance> ThirdPersonAnimClass = Weapon->GetThirdPersonAnimInstanceClass();
	
	// 调试信息：输出动画实例类信息
	UE_LOG(LogTemp, Log, TEXT("武器动画实例类 - 第一人称: %s, 第三人称: %s"), 
		FirstPersonAnimClass ? *FirstPersonAnimClass->GetName() : TEXT("None"), 
		ThirdPersonAnimClass ? *ThirdPersonAnimClass->GetName() : TEXT("None"));

	// 获取角色的第一人称和第三人称网格组件
	USkeletalMeshComponent* FirstPersonMesh = nullptr;
	USkeletalMeshComponent* ThirdPersonMesh = nullptr;
	
	// 获取第一人称网格组件
	ASDTAPlayerBase* PlayerBase = Cast<ASDTAPlayerBase>(Character);
	if (PlayerBase && PlayerBase->GetFirstPersonMesh())
	{
		FirstPersonMesh = PlayerBase->GetFirstPersonMesh();
	}
	
	// 获取第三人称网格组件（使用ACharacter的GetMesh方法）
	ThirdPersonMesh = Character->GetMesh();

	// 切换第一人称动画蓝图
	if (FirstPersonMesh && FirstPersonAnimClass)
	{
		FirstPersonMesh->SetAnimInstanceClass(FirstPersonAnimClass);
		UE_LOG(LogTemp, Log, TEXT("切换第一人称动画蓝图: %s"), *FirstPersonAnimClass->GetName());
	}

	// 切换第三人称动画蓝图
	if (ThirdPersonMesh && ThirdPersonAnimClass)
	{
		ThirdPersonMesh->SetAnimInstanceClass(ThirdPersonAnimClass);
		UE_LOG(LogTemp, Log, TEXT("切换第三人称动画蓝图: %s"), *ThirdPersonAnimClass->GetName());
	}

	// 广播动画实例类变化委托
	ASDTAPlayerBase* PlayerBaseForDelegate = Cast<ASDTAPlayerBase>(Character);
	if (PlayerBaseForDelegate)
	{
		PlayerBaseForDelegate->OnWeaponAnimInstanceChanged.Broadcast(FirstPersonAnimClass, ThirdPersonAnimClass);
	}
}

void USDTAWeaponManagerComponent::RestoreDefaultAnimInstance()
{
	if (!GetOwner())
	{
		return;
	}

	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character)
	{
		return;
	}

	// 获取角色的第一人称和第三人称网格组件
	USkeletalMeshComponent* FirstPersonMesh = nullptr;
	USkeletalMeshComponent* ThirdPersonMesh = nullptr;
	
	// 获取第一人称网格组件
	ASDTAPlayerBase* PlayerBase = Cast<ASDTAPlayerBase>(Character);
	if (PlayerBase && PlayerBase->GetFirstPersonMesh())
	{
		FirstPersonMesh = PlayerBase->GetFirstPersonMesh();
	}
	
	// 获取第三人称网格组件（使用ACharacter的GetMesh方法）
	ThirdPersonMesh = Character->GetMesh();

	// 恢复默认动画蓝图（设置为nullptr将使用网格的默认动画蓝图）
	if (FirstPersonMesh)
	{
		FirstPersonMesh->SetAnimInstanceClass(nullptr);
		UE_LOG(LogTemp, Log, TEXT("恢复第一人称默认动画蓝图"));
	}

	if (ThirdPersonMesh)
	{
		ThirdPersonMesh->SetAnimInstanceClass(nullptr);
		UE_LOG(LogTemp, Log, TEXT("恢复第三人称默认动画蓝图"));
	}

	// 广播动画实例类变化委托
	ASDTAPlayerBase* PlayerBaseForDelegate = Cast<ASDTAPlayerBase>(Character);
	if (PlayerBaseForDelegate)
	{
		PlayerBaseForDelegate->OnWeaponAnimInstanceChanged.Broadcast(nullptr, nullptr);
	}
}

void USDTAWeaponManagerComponent::ServerAddWeaponClass_Implementation(TSubclassOf<ASDTAWeapon> WeaponClass)
{
	AddWeaponClass(WeaponClass);
}

bool USDTAWeaponManagerComponent::ServerAddWeaponClass_Validate(TSubclassOf<ASDTAWeapon> WeaponClass)
{
	return true;
}

void USDTAWeaponManagerComponent::ServerSwitchToNextWeapon_Implementation()
{
	SwitchToNextWeapon();
}

bool USDTAWeaponManagerComponent::ServerSwitchToNextWeapon_Validate()
{
	return true;
}

void USDTAWeaponManagerComponent::ServerSwitchToPreviousWeapon_Implementation()
{
	SwitchToPreviousWeapon();
}

bool USDTAWeaponManagerComponent::ServerSwitchToPreviousWeapon_Validate()
{
	return true;
}

void USDTAWeaponManagerComponent::ServerSwitchToWeapon_Implementation(int32 WeaponIndex)
{
	SwitchToWeapon(WeaponIndex);
}

bool USDTAWeaponManagerComponent::ServerSwitchToWeapon_Validate(int32 WeaponIndex)
{
	return WeaponIndex >= 0;
}

void USDTAWeaponManagerComponent::ServerStartFire_Implementation()
{
	StartFire();
}

bool USDTAWeaponManagerComponent::ServerStartFire_Validate()
{
	return true;
}

void USDTAWeaponManagerComponent::ServerStopFire_Implementation()
{
	StopFire();
}

bool USDTAWeaponManagerComponent::ServerStopFire_Validate()
{
	return true;
}

void USDTAWeaponManagerComponent::ServerReload_Implementation()
{
	Reload();
}

bool USDTAWeaponManagerComponent::ServerReload_Validate()
{
	return true;
}

void USDTAWeaponManagerComponent::BeginPlay()
{
	Super::BeginPlay();

	// 组件开始播放时的初始化逻辑
}

void USDTAWeaponManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 同步关键属性
	DOREPLIFETIME(USDTAWeaponManagerComponent, OwnedWeapons);
	DOREPLIFETIME(USDTAWeaponManagerComponent, CurrentWeapon);
	DOREPLIFETIME(USDTAWeaponManagerComponent, bIsFiring);
	DOREPLIFETIME(USDTAWeaponManagerComponent, bIsReloading);
	DOREPLIFETIME(USDTAWeaponManagerComponent, bIsSwitchingWeapon);
}

void USDTAWeaponManagerComponent::InternalSwitchWeapon(ASDTAWeapon* NewWeapon)
{
	if (!NewWeapon || NewWeapon == CurrentWeapon)
	{
		return;
	}

	// 设置切换武器状态
	bIsSwitchingWeapon = true;

	// 停用当前武器
	if (CurrentWeapon)
	{
		CurrentWeapon->DeactivateWeapon();
		
		// 通知角色武器停用
		if (ISDTAWeaponHolder* WeaponHolder = Cast<ISDTAWeaponHolder>(GetOwner()))
		{
			WeaponHolder->OnWeaponDeactivated(CurrentWeapon);
		}
	}

	// 激活新武器
	CurrentWeapon = NewWeapon;
	CurrentWeapon->ActivateWeapon();
	
	// 切换动画蓝图
	SwitchWeaponAnimInstance(CurrentWeapon);
	
	// 通知角色武器激活
	if (ISDTAWeaponHolder* WeaponHolder = Cast<ISDTAWeaponHolder>(GetOwner()))
	{
		WeaponHolder->OnWeaponActivated(CurrentWeapon);
	}

	// 重置切换武器状态
	bIsSwitchingWeapon = false;
}

void USDTAWeaponManagerComponent::HandleFireInput()
{
	// 输入处理将在具体的输入配置中实现
}

void USDTAWeaponManagerComponent::HandleReloadInput()
{
	// 输入处理将在具体的输入配置中实现
}

void USDTAWeaponManagerComponent::HandleSwitchWeaponInput()
{
	// 输入处理将在具体的输入配置中实现
}