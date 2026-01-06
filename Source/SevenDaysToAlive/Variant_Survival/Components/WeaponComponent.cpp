// Fill out your copyright notice in the Description page of Project Settings.


#include "Variant_Survival/Components/WeaponComponent.h"
#include "Variant_Survival/Characters/SDTAPlayer.h"
#include "Variant_Survival/Weapons/SDTAWeapon.h"
#include "Variant_Survival/Controller/SDTAPlayerController.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "SevenDaysToAlive.h"

// Sets default values for this component's properties
UWeaponComponent::UWeaponComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// 启用组件的网络复制
	SetIsReplicated(true);

	// 设置武器附着的默认插座名称
	WeaponAttachSocketName = FName("HandGrip_R");
	UE_LOG(LogSevenDaysToAlive, Log, TEXT("[WeaponComponent] 武器附着插座已设置: %s"), *WeaponAttachSocketName.ToString());
}

// Called when the game starts
void UWeaponComponent::BeginPlay()
{
	Super::BeginPlay();

	// 获取武器持有者
	WeaponHolder = Cast<ASDTAPlayer>(GetOwner());
	if (WeaponHolder)
	{
		UE_LOG(LogSevenDaysToAlive, Log, TEXT("[WeaponComponent] 武器持有者已设置: %s"), *WeaponHolder->GetName());

		// 自动装备初始武器
		if (bEquipStartingWeaponOnSpawn && StartingWeaponClass)
		{
			UE_LOG(LogSevenDaysToAlive, Log, TEXT("[WeaponComponent] 准备自动装备初始武器: %s"), *StartingWeaponClass->GetName());
			AddWeaponClass_Implementation(StartingWeaponClass);
		}
	}
	else
	{
		UE_LOG(LogSevenDaysToAlive, Warning, TEXT("[WeaponComponent] 武器持有者无效，所有者: %s"), *GetOwner()->GetName());
	}
}

// Called every frame
void UWeaponComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 组件的每帧更新逻辑
}

// 网络复制相关
void UWeaponComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 复制当前武器
	DOREPLIFETIME(UWeaponComponent, CurrentWeapon);

	// 复制武器数组
	DOREPLIFETIME(UWeaponComponent, Weapons);
}

/** 初始化武器组件，设置持有武器的角色 */
void UWeaponComponent::InitializeWeaponHolder(ASDTAPlayer* InWeaponHolder)
{
	WeaponHolder = InWeaponHolder;
	if (WeaponHolder)
	{
		UE_LOG(LogSevenDaysToAlive, Log, TEXT("[WeaponComponent] 武器持有者已初始化: %s"), *WeaponHolder->GetName());
	}
}

/** 获取武器持有者 */
ASDTAPlayer* UWeaponComponent::GetWeaponHolder() const
{
	return WeaponHolder;
}

/** 处理开火开始输入 */
void UWeaponComponent::DoFireStart()
{
	// 确保是本地控制的角色
	if (!WeaponHolder || !WeaponHolder->IsLocallyControlled()) return;
	
	// 检查当前是否有武器且武器可以开火
	if (CurrentWeapon)
	{
		UE_LOG(LogSevenDaysToAlive, Log, TEXT("[WeaponComponent] 开火开始，武器: %s"), *CurrentWeapon->GetName());
		CurrentWeapon->StartFiring();
	}
	else
	{
		UE_LOG(LogSevenDaysToAlive, Warning, TEXT("[WeaponComponent] 开火失败: 无当前武器"));
	}
}

/** 处理开火结束输入 */
void UWeaponComponent::DoFireEnd()
{
	// 确保是本地控制的角色
	if (!WeaponHolder || !WeaponHolder->IsLocallyControlled()) return;
	
	// 检查当前是否有武器
	if (CurrentWeapon)
	{
		UE_LOG(LogSevenDaysToAlive, Log, TEXT("[WeaponComponent] 开火结束，武器: %s"), *CurrentWeapon->GetName());
		CurrentWeapon->StopFiring();
	}
}

/** 处理重新装填输入 */
void UWeaponComponent::DoReload()
{
	// 确保是本地控制的角色
	if (!WeaponHolder || !WeaponHolder->IsLocallyControlled()) return;
	
	// 检查当前是否有武器
	if (CurrentWeapon)
	{
		UE_LOG(LogSevenDaysToAlive, Log, TEXT("[WeaponComponent] 换弹，武器: %s"), *CurrentWeapon->GetName());
		CurrentWeapon->Reload();
	}
	else
	{
		UE_LOG(LogSevenDaysToAlive, Warning, TEXT("[WeaponComponent] 换弹失败: 无当前武器"));
	}
}

/** 处理武器切换输入 */
void UWeaponComponent::DoSwitchWeapon()
{
	// 确保是本地控制的角色
	if (!WeaponHolder || !WeaponHolder->IsLocallyControlled()) return;
	
	UE_LOG(LogSevenDaysToAlive, Log, TEXT("[WeaponComponent] 切换武器，当前武器数量: %d"), Weapons.Num());
	
	// 切换到下一个武器
	SwitchToNextWeapon();
}

/** 切换到下一个武器 */
void UWeaponComponent::SwitchToNextWeapon()
{
	// 确保服务器端处理武器切换逻辑，保证网络一致性
	if (WeaponHolder && WeaponHolder->IsServer())
	{
		// 如果没有武器，直接返回
		if (Weapons.Num() <= 0)
		{
			UE_LOG(LogSevenDaysToAlive, Warning, TEXT("[WeaponComponent] 武器切换失败: 无武器"));
			return;
		}
		
		// 如果只有一个武器，不需要切换
		if (Weapons.Num() == 1)
		{
			UE_LOG(LogSevenDaysToAlive, Log, TEXT("[WeaponComponent] 武器切换跳过: 只有一个武器"));
			return;
		}
		
		// 找到当前武器在列表中的索引
		int32 CurrentWeaponIndex = -1;
		if (CurrentWeapon)
		{
			CurrentWeaponIndex = Weapons.IndexOfByKey(CurrentWeapon);
		}
		
		// 计算下一个武器的索引
		int32 NextWeaponIndex = (CurrentWeaponIndex + 1) % Weapons.Num();
		
		// 如果当前没有武器或者索引无效，默认选择第一个武器
		if (CurrentWeaponIndex == -1)
		{
			NextWeaponIndex = 0;
		}
		
		UE_LOG(LogSevenDaysToAlive, Log, TEXT("[WeaponComponent] 武器切换: 当前索引=%d, 下一个索引=%d"), CurrentWeaponIndex, NextWeaponIndex);
		
		// 停用当前武器
		if (CurrentWeapon)
		{
			CurrentWeapon->DeactivateWeapon();
		}
		
		// 激活下一个武器
		ASDTAWeapon* NextWeapon = Weapons[NextWeaponIndex];
		if (NextWeapon)
		{
			NextWeapon->ActivateWeapon();
			CurrentWeapon = NextWeapon;
			UE_LOG(LogSevenDaysToAlive, Log, TEXT("[WeaponComponent] 武器已切换到: %s"), *NextWeapon->GetName());
		}
	}
}



/** 在武器持有者的物品栏中查找指定类型的武器 */
ASDTAWeapon* UWeaponComponent::FindWeaponOfType(TSubclassOf<ASDTAWeapon> WeaponClass) const
{
	if (!WeaponClass) return nullptr;

	for (ASDTAWeapon* Weapon : Weapons)
	{
		if (Weapon && Weapon->GetClass() == WeaponClass)
		{
			return Weapon;
		}
	}

	return nullptr;
}

/** 获取当前武器 */
ASDTAWeapon* UWeaponComponent::GetCurrentWeapon() const
{
	return CurrentWeapon;
}

/** 获取所有武器 */
TArray<ASDTAWeapon*> UWeaponComponent::GetWeapons() const
{
	return Weapons;
}

/** 清理武器资源 */
void UWeaponComponent::CleanupWeapons()
{
	if (WeaponHolder && WeaponHolder->IsServer())
	{
		// 先清空当前武器指针
		if (CurrentWeapon)
		{
			CurrentWeapon = nullptr;
			UE_LOG(LogSevenDaysToAlive, Log, TEXT("[WeaponComponent] 当前武器指针已清空"));
		}
		
		// 清理武器数组中的所有武器
		for (int32 i = Weapons.Num() - 1; i >= 0; i--)
		{
			ASDTAWeapon* Weapon = Weapons[i];
			if (Weapon)
			{
				// 确保武器有效且未被标记为 pending kill
				if (!Weapon->IsPendingKillPending())
				{
					Weapon->Destroy();
					UE_LOG(LogSevenDaysToAlive, Log, TEXT("[WeaponComponent] 武器已销毁: %s"), *Weapon->GetName());
				}
			}
		}
		// 清空武器数组
		Weapons.Empty();
		UE_LOG(LogSevenDaysToAlive, Log, TEXT("[WeaponComponent] 武器数组已清空，数量: %d"), Weapons.Num());
	}
}

//~Begin ISDTAWeaponHolder Interface Implementation

/** 附加武器网格到角色身上 */
void UWeaponComponent::AttachWeaponMeshes(ASDTAWeapon* Weapon)
{
	UE_LOG(LogSevenDaysToAlive, Log, TEXT("[SDTAWeaponHolder] 附加武器网格: %s"), Weapon ? *Weapon->GetName() : TEXT("无效"));

	if (!Weapon || !WeaponHolder) return;
	
	const FAttachmentTransformRules AttachmentRule(EAttachmentRule::SnapToTarget, false);

	// 将武器Actor附加到角色上
	Weapon->AttachToActor(WeaponHolder, AttachmentRule);

	// 附加第一人称武器网格到角色的第一人称Mesh组件
	USkeletalMeshComponent* FPWeaponMesh = Weapon->GetFirstPersonMesh();
	if (FPWeaponMesh && WeaponHolder->GetFirstPersonMesh())
	{
		FPWeaponMesh->AttachToComponent(WeaponHolder->GetFirstPersonMesh(), AttachmentRule, WeaponAttachSocketName);
		UE_LOG(LogSevenDaysToAlive, Log, TEXT("[SDTAWeaponHolder] 第一人称武器网格已附加到插座: %s"), *WeaponAttachSocketName.ToString());
	}
	
	// 附加第三人称武器网格到角色的Mesh组件
	USkeletalMeshComponent* TPWeaponMesh = Weapon->GetThirdPersonMesh();
	if (TPWeaponMesh)
	{
		TPWeaponMesh->AttachToComponent(WeaponHolder->GetMesh(), AttachmentRule, WeaponAttachSocketName);
		UE_LOG(LogSevenDaysToAlive, Log, TEXT("[SDTAWeaponHolder] 第三人称武器网格已附加到插座: %s"), *WeaponAttachSocketName.ToString());
	}
}

/** 播放武器射击动画 */
void UWeaponComponent::PlayFiringMontage(UAnimMontage* Montage)
{
	UE_LOG(LogSevenDaysToAlive, Log, TEXT("[SDTAWeaponHolder] 播放射击动画: %s"), Montage ? *Montage->GetName() : TEXT("无效"));

	if (!Montage || !WeaponHolder) return;
	
	// 播放射击动画 - 在第一人称网格上播放，因为武器是第一人称视角的
	USkeletalMeshComponent* FirstPersonMeshComp = WeaponHolder->GetFirstPersonMesh();
	if (FirstPersonMeshComp)
	{
		UAnimInstance* AnimInstance = FirstPersonMeshComp->GetAnimInstance();
		if (AnimInstance)
		{
			// 设置动画播放参数并播放
			float PlayRate = 1.0f;
			float BlendInTime = 0.1f;
			float BlendOutTime = 0.1f;
			
			// 停止当前可能播放的其他蒙太奇
			AnimInstance->Montage_Stop(BlendOutTime);
			
			// 播放新的蒙太奇
			AnimInstance->Montage_Play(Montage, PlayRate, EMontagePlayReturnType::Duration, BlendInTime);
		}
		else
		{
			UE_LOG(LogSevenDaysToAlive, Warning, TEXT("[SDTAWeaponHolder] 第一人称网格的动画实例无效"));
		}
	}
	else
	{
		UE_LOG(LogSevenDaysToAlive, Warning, TEXT("[SDTAWeaponHolder] 第一人称网格无效"));
	}
}

/** 应用武器后坐力到角色 */
void UWeaponComponent::AddWeaponRecoil(float Recoil)
{
	UE_LOG(LogSevenDaysToAlive, Log, TEXT("[SDTAWeaponHolder] 应用后坐力: %.2f"), Recoil);

	if (!WeaponHolder) return;

	// 仅在本地角色上应用后坐力
	if (!WeaponHolder->IsLocallyControlled()) return;
	
	// 获取控制器
	APlayerController* PC = Cast<APlayerController>(WeaponHolder->GetController());
	if (!PC) return;
	
	// 应用后坐力到控制器的旋转
	PC->AddPitchInput(-Recoil);
}

/** 更新武器HUD显示当前弹药数量 */
void UWeaponComponent::UpdateWeaponHUD(int32 CurrentAmmo, int32 MagazineSize)
{
	UE_LOG(LogSevenDaysToAlive, Log, TEXT("[SDTAWeaponHolder] 更新HUD弹药: %d/%d"), CurrentAmmo, MagazineSize);

	if (!WeaponHolder) return;

	// 在客户端上更新HUD
	if (WeaponHolder->IsLocallyControlled())
	{
		// 获取玩家控制器并更新武器计数器UI
		if (APlayerController* PlayerController = Cast<APlayerController>(WeaponHolder->GetController()))
		{
			// 调用控制器的UI更新方法
			if (ASDTAPlayerController* SDTAController = Cast<ASDTAPlayerController>(PlayerController))
			{
				SDTAController->UpdateWeaponCounterUI(CurrentAmmo, MagazineSize);
			}
		}
	}
}

/** 计算并返回武器的瞄准目标位置 */
FVector UWeaponComponent::GetWeaponTargetLocation()
{
	if (!WeaponHolder) 
	{
		UE_LOG(LogSevenDaysToAlive, Warning, TEXT("[SDTAWeaponHolder] 无有效控制器，返回零向量"));
		return FVector::ZeroVector;
	}
	
	// 获取控制器
	AController* LocalController = WeaponHolder->GetController();
	if (!LocalController) 
	{
		UE_LOG(LogSevenDaysToAlive, Warning, TEXT("[SDTAWeaponHolder] 无有效控制器，返回零向量"));
		return FVector::ZeroVector;
	}
	
	// 对于玩家控制器，使用鼠标位置进行射线检测
	APlayerController* PC = Cast<APlayerController>(LocalController);
	if (PC)
	{
		FHitResult HitResult;
		if (PC->GetHitResultUnderCursor(ECC_Visibility, false, HitResult))
		{
			UE_LOG(LogSevenDaysToAlive, Log, TEXT("[SDTAWeaponHolder] 鼠标瞄准位置: %s"), *HitResult.Location.ToString());
			return HitResult.Location;
		}
	}
	
	// 默认返回角色前方一定距离的位置
	FVector DefaultTarget = WeaponHolder->GetActorLocation() + WeaponHolder->GetActorForwardVector() * 1000.0f;
	UE_LOG(LogSevenDaysToAlive, Log, TEXT("[SDTAWeaponHolder] 使用默认瞄准位置: %s"), *DefaultTarget.ToString());
	return DefaultTarget;
}

/** 给玩家添加指定类型的武器 */
void UWeaponComponent::AddWeaponClass_Implementation(TSubclassOf<ASDTAWeapon> WeaponClass)
{
	UE_LOG(LogSevenDaysToAlive, Log, TEXT("[SDTAWeaponHolder] 尝试添加武器: %s"), WeaponClass ? *WeaponClass->GetName() : TEXT("无效"));

	if (!WeaponClass || !WeaponHolder) return;
	
	// 检查玩家是否已经拥有这种武器
	if (FindWeaponOfType(WeaponClass))
	{
		UE_LOG(LogSevenDaysToAlive, Warning, TEXT("[SDTAWeaponHolder] 已拥有武器: %s，跳过添加"), *WeaponClass->GetName());
		// 已经拥有这种武器，可以选择增加弹药或替换
		return;
	}
	
	// 仅在服务器上创建武器
	if (WeaponHolder->IsServer())
	{
		// 创建武器实例
		ASDTAWeapon* NewWeapon = GetWorld()->SpawnActor<ASDTAWeapon>(WeaponClass, WeaponHolder->GetActorLocation(), WeaponHolder->GetActorRotation());
		if (NewWeapon)
		{
			// 设置武器所有者
			NewWeapon->SetOwner(WeaponHolder);
			
			// 初始化武器所有者（解决时序问题）
			NewWeapon->InitializeWeaponOwner();
			
			// 添加到武器列表
			Weapons.Add(NewWeapon);
			
			UE_LOG(LogSevenDaysToAlive, Log, TEXT("[SDTAWeaponHolder] 武器已创建并添加到列表: %s，当前武器数量: %d"), *NewWeapon->GetName(), Weapons.Num());
			
			// 将武器网格附加到角色身上
			AttachWeaponMeshes(NewWeapon);
			
			// 如果是第一个武器，自动装备
			if (Weapons.Num() == 1)
			{
				NewWeapon->ActivateWeapon();
				CurrentWeapon = NewWeapon;
				UE_LOG(LogSevenDaysToAlive, Log, TEXT("[SDTAWeaponHolder] 第一个武器已自动装备: %s"), *NewWeapon->GetName());
			}
		}
		else
		{
			UE_LOG(LogSevenDaysToAlive, Error, TEXT("[SDTAWeaponHolder] 武器创建失败: %s"), *WeaponClass->GetName());
		}
	}
}

/** 激活武器 */
void UWeaponComponent::OnWeaponActivated_Implementation(ASDTAWeapon* Weapon)
{
	UE_LOG(LogSevenDaysToAlive, Log, TEXT("[SDTAWeaponHolder] 激活武器: %s"), Weapon ? *Weapon->GetName() : TEXT("无效"));

	if (!Weapon || !WeaponHolder) return;
	
	// 仅在服务器上处理激活逻辑
	if (WeaponHolder->IsServer())
	{
		// 如果有当前武器，先停用它
		if (CurrentWeapon && CurrentWeapon != Weapon)
		{
			UE_LOG(LogSevenDaysToAlive, Log, TEXT("[SDTAWeaponHolder] 停用当前武器: %s"), *CurrentWeapon->GetName());
			CurrentWeapon->DeactivateWeapon();
		}
		
		// 设置当前武器
		CurrentWeapon = Weapon;
		UE_LOG(LogSevenDaysToAlive, Log, TEXT("[SDTAWeaponHolder] 当前武器已设置为: %s"), *Weapon->GetName());
	}
	
	// 设置角色网格的AnimInstance类
	USkeletalMeshComponent* FirstPersonCharacterMesh = WeaponHolder->GetFirstPersonMesh();
	if (FirstPersonCharacterMesh)
	{
		FirstPersonCharacterMesh->SetAnimInstanceClass(Weapon->GetFirstPersonAnimInstanceClass());
		UE_LOG(LogSevenDaysToAlive, Log, TEXT("[SDTAWeaponHolder] 第一人称动画实例已设置: %s"), Weapon->GetFirstPersonAnimInstanceClass() ? *Weapon->GetFirstPersonAnimInstanceClass()->GetName() : TEXT("无"));
	}
}

/** 停用武器 */
void UWeaponComponent::OnWeaponDeactivated_Implementation(ASDTAWeapon* Weapon)
{
	UE_LOG(LogSevenDaysToAlive, Log, TEXT("[SDTAWeaponHolder] 停用武器: %s"), Weapon ? *Weapon->GetName() : TEXT("无效"));

	// 武器停用的逻辑可以在这里添加
}

//~End ISDTAWeaponHolder Interface Implementation

/** 开始开火 */
void UWeaponComponent::StartFiring()
{
	DoFireStart();
}

/** 停止开火 */
void UWeaponComponent::StopFiring()
{
	DoFireEnd();
}

/** 重新装填 */
void UWeaponComponent::Reload()
{
	DoReload();
}
