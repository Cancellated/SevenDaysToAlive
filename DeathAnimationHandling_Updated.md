# 动画蒙太奇处理指南（死亡动画和受击动画）

## 实现概述

我已经修改了`EnemyBase`类，添加了使用动画蒙太奇处理死亡动画和受击动画的支持。这种方法比在状态机中处理这些动画具有更多优势，包括：

- **立即中断**：可以立即中断当前播放的任何动画
- **更好的控制**：支持精确的动画时间和事件触发
- **灵活性**：可以轻松切换不同的动画变体
- **事件支持**：可以在动画的特定时间点触发声音、粒子效果等
- **瞬时响应**：适用于需要立即反馈的受击和死亡动画

## 已实现的C++修改

### 1. EnemyBase.h 修改

- 添加了`DeathMontage`属性（死亡动画蒙太奇）
- 添加了`HitMontage`属性（受击动画蒙太奇）
- 添加了死亡动画完成事件`BP_OnDeathAnimationFinished`
- 添加了受击动画完成事件`BP_OnHitAnimationFinished`
- 添加了内部状态标志`bIsHit`和`bIsDead`
- 添加了定时器和回调函数来处理动画完成

### 2. EnemyBase.cpp 修改

- 更新了`Die()`方法，实现了死亡动画蒙太奇的播放逻辑
- 更新了`ApplyDamage()`方法，实现了受击动画蒙太奇的播放逻辑
- 添加了`OnDeathAnimationFinished()`回调函数，处理死亡动画完成后的逻辑
- 添加了`OnHitAnimationFinished()`回调函数，处理受击动画完成后的逻辑
- 添加了受击动画和死亡动画之间的冲突处理

## 蓝图配置步骤

### 1. 编译C++代码

首先，需要在UE编辑器中编译修改后的C++代码：

1. 打开UE编辑器
2. 点击顶部菜单的`工具` -> `编译SevenDaysToAlive`
3. 等待编译完成

### 2. 配置敌人蓝图

在敌人蓝图（如`BP_DarkKnightEnemy`）中配置动画蒙太奇：

1. 在UE编辑器中打开敌人蓝图
2. 在`组件`面板中选择敌人的Mesh组件
3. 在`细节`面板中找到`动画`部分
4. 确保已正确设置`动画蓝图`

### 3. 设置死亡动画蒙太奇

1. 在蓝图的`类默认值`面板中
2. 找到`Enemy|Animation`分类
3. 点击`DeathMontage`属性旁边的选择按钮
4. 从资源浏览器中选择死亡动画蒙太奇（如`Content/Anims/DKM_Death_Montage.uasset`）

### 4. 设置受击动画蒙太奇

1. 在蓝图的`类默认值`面板中
2. 找到`Enemy|Animation`分类
3. 点击`HitMontage`属性旁边的选择按钮
4. 从资源浏览器中选择受击动画蒙太奇（如`Content/Characters/Mannequins/Anims/HitReact/MM_HitReact_Front_Lgt_01.uasset`）

### 5. 处理死亡动画完成事件

1. 在蓝图的`事件图表`中
2. 右键点击空白处，搜索并添加`死亡动画完成`事件
3. 在这个事件节点后，可以添加以下逻辑：
   - 销毁敌人Actor
   - 生成物品或特效
   - 播放音效
   - 更新游戏统计数据

### 6. 处理受击动画完成事件

1. 在蓝图的`事件图表`中
2. 右键点击空白处，搜索并添加`受击动画完成`事件
3. 在这个事件节点后，可以添加以下逻辑：
   - 恢复攻击状态
   - 播放后续音效
   - 触发特殊效果

## 测试动画效果

### 测试受击动画

1. 将敌人蓝图拖放到场景中
2. 点击`播放`按钮运行游戏
3. 攻击敌人（不致命伤害）
4. 观察敌人播放受击动画
5. 确认敌人在受击后仍能继续移动和执行其他动作

### 测试死亡动画

1. 将敌人蓝图拖放到场景中
2. 点击`播放`按钮运行游戏
3. 攻击敌人使其生命值降至0
4. 观察敌人播放死亡动画
5. 确认敌人在死亡后停止移动和响应

## 高级配置

### 使用多个动画变体

#### 死亡动画变体

1. 在动画编辑器中打开死亡动画蒙太奇
2. 添加多个动画片段作为不同的死亡变体
3. 在C++代码中，可以随机选择变体播放：

```cpp
if (DeathMontage)
{
    // 随机选择一个死亡动画片段
    int32 SectionIndex = FMath::RandRange(0, DeathMontage->CompositeSections.Num() - 1);
    FName SectionName = DeathMontage->CompositeSections[SectionIndex].SectionName;
    
    // 播放指定片段
    float MontageLength = DeathMontage->GetSectionLength(SectionName);
    GetMesh()->GetAnimInstance()->Montage_Play(DeathMontage);
    GetMesh()->GetAnimInstance()->Montage_JumpToSection(SectionName, DeathMontage);
    GetWorld()->GetTimerManager().SetTimer(DeathAnimationTimer, this, &AEnemyBase::OnDeathAnimationFinished, MontageLength, false);
}
```

#### 受击动画变体

1. 在动画编辑器中打开受击动画蒙太奇
2. 添加多个动画片段作为不同方向的受击变体（前、后、左、右）
3. 在C++代码中，可以根据攻击方向选择不同的变体：

```cpp
// 根据HitResult的方向计算受击方向
FVector HitDirection = HitResult.ImpactNormal.GetSafeNormal();
FVector Forward = GetActorForwardVector();
FVector Right = GetActorRightVector();

float ForwardDot = FVector::DotProduct(HitDirection, Forward);
float RightDot = FVector::DotProduct(HitDirection, Right);

FName SectionName;
if (FMath::Abs(ForwardDot) > FMath::Abs(RightDot))
{
    if (ForwardDot > 0)
    {
        SectionName = TEXT("FrontHit");
    }
    else
    {
        SectionName = TEXT("BackHit");
    }
}
else
{
    if (RightDot > 0)
    {
        SectionName = TEXT("RightHit");
    }
    else
    {
        SectionName = TEXT("LeftHit");
    }
}

// 播放对应方向的受击动画
if (HitMontage->HasSection(SectionName))
{
    float MontageLength = HitMontage->GetSectionLength(SectionName);
    GetMesh()->GetAnimInstance()->Montage_Play(HitMontage);
    GetMesh()->GetAnimInstance()->Montage_JumpToSection(SectionName, HitMontage);
    GetWorld()->GetTimerManager().SetTimer(HitAnimationTimer, this, &AEnemyBase::OnHitAnimationFinished, MontageLength, false);
}
```

### 添加动画通知

1. 在动画编辑器中打开动画蒙太奇
2. 点击时间轴上的时间点
3. 添加`动画通知`（如`PlaySound`或`SpawnParticleEffect`）
4. 配置通知参数
5. 在动画蓝图中添加对应的通知事件处理逻辑

## 受击动画与状态机的配合

受击动画蒙太奇与常规的状态机动画（如走路、跑步、攻击）可以很好地配合：

1. **优先级**：受击动画会自动中断当前播放的状态机动画
2. **恢复**：受击动画播放完成后，角色会自动恢复到之前的状态机动画
3. **移动**：受击动画不会阻止角色移动，敌人可以在受击后继续追逐或巡逻
4. **攻击**：敌人在受击时会暂时停止攻击，但受击动画结束后可以继续攻击

## 结论

使用动画蒙太奇处理受击和死亡动画是一种更灵活、更强大的方法，可以提供更好的视觉效果和控制能力。通过以上配置，您可以轻松地为敌人添加高质量的受击和死亡动画效果。