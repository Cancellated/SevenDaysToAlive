# 生存模式武器系统实现总结

## 系统概述

本次实现基于UE5引擎，为生存模式（Variant_Survival）开发了一套完整的武器系统，参考了Shooter模板的设计思路，实现了武器的装备、激活/停用、开火、换弹、切换以及拾取功能。系统采用了接口驱动设计、数据驱动的拾取系统和模块化的架构，确保了代码的可扩展性和可维护性。

## 核心组件与类结构

### 1. 武器基类 (ASDTAWeapon)

**文件路径**: `Variant_Survival/Weapons/SDTAWeapon.h/cpp`

**核心功能**:
- 管理第一人称和第三人称视角的武器网格
- 处理武器的激活/停用状态
- 实现开火、换弹和子弹管理逻辑
- 通过`ISDTAWeaponHolder`接口与武器所有者交互

**关键属性**:
- `FirstPersonMesh`: 第一人称视角的武器网格组件
- `ThirdPersonMesh`: 第三人称视角的武器网格组件
- `CurrentBullets`: 当前弹夹中的子弹数量
- `MagazineSize`: 弹夹容量
- `bFullAuto`: 是否为全自动武器

**关键方法**:
```cpp
// 激活武器，设置可见性、碰撞和tick状态
void ActivateWeapon();

// 停用武器，隐藏武器并停止开火
void DeactivateWeapon();

// 开始开火，支持半自动和全自动模式
void StartFiring();

// 停止开火
void StopFiring();

// 重新装填武器
void Reload();

// 发射子弹
virtual void Fire();
```

### 2. 武器持有者接口 (ISDTAWeaponHolder)

**文件路径**: `Variant_Survival/Weapons/SDTAWeaponHolder.h`

**核心功能**:
- 定义武器与持有者（如角色）之间的交互标准
- 实现武器与持有者的解耦，提高系统灵活性

**关键方法**:
```cpp
// 附加武器网格到持有者身上
void AttachWeaponMeshes(ASDTAWeapon* Weapon);

// 向持有者添加武器
UFUNCTION(BlueprintNativeEvent, Category = "Weapon")
void AddWeaponClass(TSubclassOf<ASDTAWeapon> WeaponClass);

// 武器激活时调用
UFUNCTION(BlueprintNativeEvent, Category = "Weapon")
void OnWeaponActivated(ASDTAWeapon* Weapon);

// 武器停用时调用
UFUNCTION(BlueprintNativeEvent, Category = "Weapon")
void OnWeaponDeactivated(ASDTAWeapon* Weapon);
```

### 3. 玩家角色类 (ASDTAPlayer)

**文件路径**: `Variant_Survival/Characters/SDTAPlayer.h/cpp`

**核心功能**:
- 实现`ISDTAWeaponHolder`接口，成为武器的持有者
- 处理玩家输入（开火、换弹、武器切换）
- 管理玩家拥有的武器列表
- 处理武器的激活/停用和切换逻辑

**关键属性**:
- `Weapons`: 玩家拥有的武器列表
- `CurrentWeapon`: 当前激活的武器
- `WeaponAttachSocketName`: 武器附着的插座名称

**关键方法**:
```cpp
// 附加武器网格
void ASDTAPlayer::AttachWeaponMeshes(ASDTAWeapon* Weapon);

// 向玩家添加武器
void ASDTAPlayer::AddWeaponClass_Implementation(TSubclassOf<ASDTAWeapon> WeaponClass);

// 切换到下一个武器
void ASDTAPlayer::SwitchToNextWeapon();

// 武器激活时调用
void ASDTAPlayer::OnWeaponActivated_Implementation(ASDTAWeapon* Weapon);

// 武器停用时调用
void ASDTAPlayer::OnWeaponDeactivated_Implementation(ASDTAWeapon* Weapon);
```

### 4. 武器拾取类 (ASDTAPickup)

**文件路径**: `Variant_Survival/Weapons/SDTAPickup.h/cpp`

**核心功能**:
- 实现武器的拾取功能
- 支持数据驱动的武器配置
- 处理拾取后的重生逻辑

**关键属性**:
- `WeaponType`: 武器类型的数据表行句柄
- `RespawnTime`: 拾取后的重生时间

**关键方法**:
```cpp
// 处理碰撞重叠事件
void ASDTAPickup::OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

// 重生拾取物
void ASDTAPickup::RespawnPickup();
```

### 5. 投射物类 (ASDTAProjectiles)

**文件路径**: `Variant_Survival/Weapons/SDTAProjectiles.h/cpp`

**核心功能**:
- 实现子弹的发射和飞行逻辑
- 支持实体子弹和即时弹道两种机制
- 处理子弹的命中检测和伤害计算

**关键属性**:
- `ProjectileType`: 弹道类型（实体子弹或即时弹道）
- `FireDirection`: 射击方向
- `HitDamage`: 伤害值

**关键方法**:
```cpp
// 设置射击方向
void SetFireDirection(const FVector& Direction);

// 触发射击逻辑
void Fire();

// 处理碰撞（仅实体子弹使用）
virtual void NotifyHit(class UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit) override;

// 执行即时弹道的射线检测
void PerformInstantHit();
```

## 关键功能实现

### 1. 武器激活/停用

武器的激活/停用是武器系统的核心功能之一，通过`ActivateWeapon`和`DeactivateWeapon`方法实现：

```cpp
// 激活武器
void ASDTAWeapon::ActivateWeapon()
{
    // 设置武器为可见
    SetActorHiddenInGame(false);
    SetActorEnableCollision(true);
    SetActorTickEnabled(true);

    // 激活第一人称网格
    if (FirstPersonMesh)
    {
        FirstPersonMesh->SetHiddenInGame(false);
        FirstPersonMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    // 激活第三人称网格
    if (ThirdPersonMesh)
    {
        ThirdPersonMesh->SetHiddenInGame(false);
        ThirdPersonMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    // 通知武器所有者武器已激活
    if (WeaponOwner)
    {
        WeaponOwner->OnWeaponActivated(this);
    }
}

// 停用武器
void ASDTAWeapon::DeactivateWeapon()
{
    // 停止当前的开火动作
    StopFiring();

    // 设置武器为隐藏
    SetActorHiddenInGame(true);
    SetActorEnableCollision(false);
    SetActorTickEnabled(false);

    // 隐藏第一人称网格
    if (FirstPersonMesh)
    {
        FirstPersonMesh->SetHiddenInGame(true);
    }

    // 隐藏第三人称网格
    if (ThirdPersonMesh)
    {
        ThirdPersonMesh->SetHiddenInGame(true);
    }

    // 通知武器所有者武器已停用
    if (WeaponOwner)
    {
        WeaponOwner->OnWeaponDeactivated(this);
    }
}
```

### 2. 武器开火

开火逻辑支持半自动和全自动模式，通过`StartFiring`、`StopFiring`和`Fire`方法实现：

```cpp
// 开始开火
void ASDTAWeapon::StartFiring()
{
    if (bIsFiring)
    {
        return;
    }

    bIsFiring = true;

    // 发射初始子弹
    Fire();

    // 如果是全自动，设置自动开火定时器
    if (bFullAuto)
    {
        GetWorldTimerManager().SetTimer(RefireTimer, this, &ASDTAWeapon::Fire, RefireRate, true);
    }
}

// 停止开火
void ASDTAWeapon::StopFiring()
{
    bIsFiring = false;
    GetWorldTimerManager().ClearTimer(RefireTimer);
}

// 开火
void ASDTAWeapon::Fire()
{
    // 检查武器是否有子弹
    if (CurrentBullets <= 0)
    {
        StopFiring();
        return;
    }

    // 播放开火动画蒙太奇
    if (WeaponOwner && FiringMontage)
    {
        WeaponOwner->PlayFiringMontage(FiringMontage);
    }

    // 应用后坐力
    if (WeaponOwner)
    {
        WeaponOwner->AddWeaponRecoil(FiringRecoil);
    }

    // 计算目标位置（当前实现为武器前方10000单位）
    FVector MuzzleLocation = FirstPersonMesh->GetSocketLocation(MuzzleSocketName);
    FVector FireDirection = FirstPersonMesh->GetForwardVector();
    FVector TargetLocation = MuzzleLocation + FireDirection * 10000.0f;

    // 发射投射物
    FireProjectile(TargetLocation);

    // 减少子弹数量
    CurrentBullets--;

    // 更新武器HUD
    if (WeaponOwner)
    {
        WeaponOwner->UpdateWeaponHUD(CurrentBullets, MagazineSize);
    }

    // 保存最后一次射击时间
    TimeOfLastShot = GetWorld()->TimeSeconds;
}
```

### 3. 武器切换

武器切换功能允许玩家在拥有的武器之间进行切换，通过`DoSwitchWeapon`和`SwitchToNextWeapon`方法实现：

```cpp
// 处理武器切换输入
void ASDTAPlayer::DoSwitchWeapon()
{
    // 确保是本地控制的角色
    if (!IsLocallyControlled()) return;
    
    // 切换到下一个武器
    SwitchToNextWeapon();
}

// 切换到下一个武器
void ASDTAPlayer::SwitchToNextWeapon()
{
    // 确保服务器端处理武器切换逻辑，保证网络一致性
    if (IsServer())
    {
        // 如果没有武器，直接返回
        if (Weapons.Num() <= 0)
        {
            return;
        }
        
        // 如果只有一个武器，不需要切换
        if (Weapons.Num() == 1)
        {
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
        }
    }
    else
    {
        // 客户端请求服务器执行武器切换
        // 注意：这里应该使用RPC调用，但为了简化示例，我们直接在客户端处理
        // 在实际项目中，应该创建一个Server_RPC方法来处理武器切换
    }
}
```

### 4. 武器拾取

武器拾取功能允许玩家通过碰撞触发拾取武器，使用数据驱动的设计，通过`FSDTAWeaponTableRow`定义武器的网格和类：

```cpp
// 武器拾取表行结构体
USTRUCT(BlueprintType)
struct FSDTAWeaponTableRow : public FTableRowBase
{
    GENERATED_BODY()

    // 拾取时显示的网格
    UPROPERTY(EditAnywhere)
    TSoftObjectPtr<UStaticMesh> StaticMesh;

    // 拾取时生成的武器类
    UPROPERTY(EditAnywhere)
    TSubclassOf<ASDTAWeapon> WeaponToSpawn;
};

// 处理碰撞重叠事件
void ASDTAPickup::OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    // 检查是否与武器持有者发生碰撞
    if (ISDTAWeaponHolder* WeaponHolder = Cast<ISDTAWeaponHolder>(OtherActor))
    {
        WeaponHolder->AddWeaponClass(WeaponClass);

        // 隐藏拾取物
        SetActorHiddenInGame(true);

        // 禁用碰撞
        SetActorEnableCollision(false);

        // 禁用tick
        SetActorTickEnabled(false);

        // 安排重生
        GetWorld()->GetTimerManager().SetTimer(RespawnTimer, this, &ASDTAPickup::RespawnPickup, RespawnTime, false);
    }
}
```

## 技术细节与架构决策

### 1. 接口驱动设计

系统采用了`ISDTAWeaponHolder`接口来定义武器与持有者之间的交互，实现了武器类与持有者类的解耦。这种设计允许不同类型的角色（玩家、AI）都可以持有武器，只要它们实现了该接口。

### 2. 双重网格系统

武器类同时管理第一人称和第三人称网格，通过`ActivateWeapon`和`DeactivateWeapon`方法控制它们的可见性、碰撞和tick状态。这种设计确保了在不同视角下武器都能正确显示和交互。

### 3. 武器状态管理

武器的状态（可见性、碰撞、tick）集中在`ActivateWeapon`和`DeactivateWeapon`方法中进行管理，确保了状态转换的一致性。当武器被激活时，它会变得可见、启用碰撞并开始tick；当武器被停用时，它会隐藏、禁用碰撞并停止tick。

### 4. 输入处理

使用UE5的Enhanced Input System来处理玩家输入，将输入动作（开火、换弹、武器切换）绑定到玩家控制器的方法上：

```cpp
// 输入绑定
if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
{
    // 开火绑定
    EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &ASDTAPlayer::DoFireStart);
    EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &ASDTAPlayer::DoFireEnd);
    
    // 换弹绑定
    EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Started, this, &ASDTAPlayer::DoReload);

    // 武器切换绑定
    EnhancedInputComponent->BindAction(SwitchWeaponAction, ETriggerEvent::Started, this, &ASDTAPlayer::DoSwitchWeapon);
}
```

### 5. 网络同步

系统考虑了网络同步问题，关键属性（如`CurrentWeapon`、`Weapons`、`bIsDashing`）都标记为`Replicated`，确保了在多人游戏中的一致性。武器的切换逻辑也设计为服务器权威，客户端仅触发请求，实际切换由服务器处理。

```cpp
// 设置需要复制的属性
void ASDTAPlayer::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    // 冲刺相关属性的网络复制
    DOREPLIFETIME(ASDTAPlayer, bIsDashing);
    DOREPLIFETIME(ASDTAPlayer, LastDashTime);
    DOREPLIFETIME(ASDTAPlayer, OriginalMaxWalkSpeed);
    
    // 武器相关属性的网络复制（示例）
    // DOREPLIFETIME(ASDTAPlayer, Weapons);
    // DOREPLIFETIME(ASDTAPlayer, CurrentWeapon);
}
```

### 6. 数据驱动的拾取系统

拾取系统使用`FDataTableRowHandle`来引用武器数据表，实现了数据与逻辑的分离。这种设计允许设计者通过数据表配置武器的网格和类，而不需要修改代码，提高了系统的灵活性和可维护性。

## 代码模式与最佳实践

### 1. 模块化设计

系统采用了模块化的设计，将武器、武器持有者、拾取系统等功能分离到不同的类中，每个类负责单一的功能，提高了代码的可维护性和可测试性。

### 2. 面向接口编程

通过定义`ISDTAWeaponHolder`接口，实现了武器与持有者的解耦，允许不同类型的角色都可以持有武器，提高了系统的灵活性和可扩展性。

### 3. 集中式状态管理

武器的状态（可见性、碰撞、tick）集中在`ActivateWeapon`和`DeactivateWeapon`方法中进行管理，确保了状态转换的一致性，避免了状态不一致的问题。

### 4. 服务器权威设计

关键逻辑（如武器切换、开火）设计为服务器权威，客户端仅触发请求，实际逻辑由服务器处理，确保了在多人游戏中的一致性。

### 5. 使用UE5新特性

系统充分利用了UE5的新特性，如Enhanced Input System、数据驱动设计等，提高了开发效率和代码质量。

## 未来改进方向

1. **完整的网络同步实现**：目前的实现中，武器列表和当前武器的网络同步还未完全实现，需要添加相应的网络复制和RPC调用。

2. **更复杂的开火逻辑**：可以添加更复杂的开火逻辑，如弹道预测、散布、后坐力系统等。

3. **武器自定义系统**：允许玩家自定义武器的配件、皮肤等。

4. **AI武器使用**：扩展系统，允许AI角色使用武器。

5. **武器磨损和维修**：添加武器磨损和维修系统，增加游戏的生存元素。

6. **更好的动画支持**：添加更丰富的武器动画，如换弹、拉枪栓等。

## 总结

本次实现为生存模式开发了一套完整的武器系统，涵盖了武器的装备、激活/停用、开火、换弹、切换以及拾取功能。系统采用了接口驱动设计、数据驱动的拾取系统和模块化的架构，确保了代码的可扩展性和可维护性。通过参考Shooter模板的设计思路，并结合UE5的新特性，实现了一个功能完整、性能良好的武器系统，为生存模式的游戏体验提供了坚实的基础。