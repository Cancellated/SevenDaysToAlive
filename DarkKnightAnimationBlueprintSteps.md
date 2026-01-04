# 为Dark_Knight_Male创建动画蓝图步骤

## 1. 创建动画蓝图

1. 打开Unreal Engine编辑器
2. 在内容浏览器中导航到你想要保存动画蓝图的位置（建议：`Content/Blueprints/Animations/Enemies/`）
3. 右键点击空白处，选择`动画` → `动画蓝图`
4. 在弹出的对话框中：
   - 选择`Skeletal Mesh Animation Blueprint`
   - 点击`选择`
5. 在"选择父类"对话框中，选择`AnimInstance`，然后点击`选择`
6. 在"选择骨架"对话框中：
   - 导航到`Content/Assets/Dark_Knight/Dark_Knight_Male/`路径
   - 选择Dark_Knight_Male的骨架资产（通常命名为`SK_DarkKnight`或类似名称）
   - 点击`选择`
7. 给动画蓝图命名（例如：`ABP_DarkKnight`）
8. 点击`创建动画蓝图`

## 2. 配置动画蓝图

### 设置基础输入变量
1. 双击打开刚刚创建的动画蓝图
2. 在动画蓝图编辑器中，点击左侧的"我的蓝图"面板
3. 点击"变量"标签，然后点击"添加变量"
4. 创建以下必要的输入变量：
   - `IsMoving` (布尔型)：控制移动/静止动画
   - `MoveSpeed` (浮点型)：控制行走/奔跑的速度切换
   - `IsAttacking` (布尔型)：控制攻击动画
   - `IsTakingDamage` (布尔型)：控制受击动画
   - `IsDead` (布尔型)：控制死亡动画
   - `AttackIndex` (整型)：控制攻击动画变体

### 创建动画状态机
1. 在动画蓝图编辑器中，点击"图表"选项卡
2. 右键点击空白处，选择`状态机` → `添加状态机`
3. 给状态机命名（例如：`MainStateMachine`）
4. 双击状态机进入状态机编辑器
5. 创建以下状态：
   - `Idle`：闲置状态
   - `Locomotion`：移动状态（包含行走和奔跑）
   - `Attack`：攻击状态
   - `Hit`：受击状态
   - `Death`：死亡状态

## 3. 连接动画序列到状态

### 配置Idle状态
1. 双击`Idle`状态进入其内部
2. 右键点击空白处，选择`添加动画序列`
3. 导航到`Content/Assets/Dark_Knight/Dark_Knight_Male/Animations/`路径
4. 选择`Anim_DKM_Idle`动画序列
5. 连接`入口`节点到`Anim_DKM_Idle`节点，再连接到`出口`节点

### 配置Locomotion状态
1. 双击`Locomotion`状态进入其内部
2. 创建以下子状态：
   - `Walk`
   - `Run`
3. 为每个子状态添加对应的动画序列：
   - `Walk`：连接`Anim_DKM_Walk_Fwd`
   - `Run`：连接`Anim_DKM_Run_Fwd`
4. 设置状态转换条件：
   - `Walk` → `Run`：当`MoveSpeed > 300`时
   - `Run` → `Walk`：当`MoveSpeed <= 300`时

### 配置Attack状态
1. 双击`Attack`状态进入其内部
2. 创建攻击状态机或使用选择器来处理不同的攻击动画：
   - 添加`Anim_DKM_Attack_01`、`Anim_DKM_Attack_02`、`Anim_DKM_Attack_03`
3. 使用`AttackIndex`变量来选择播放哪个攻击动画
4. 确保攻击动画播放完成后自动退出攻击状态

### 配置Hit状态
1. 双击`Hit`状态进入其内部
2. 添加受击动画序列（例如：`Anim_DKM_Hit_Fwd`）
3. 确保受击动画播放完成后自动返回到之前的状态

### 配置Death状态
1. 双击`Death`状态进入其内部
2. 添加死亡动画序列（`Anim_DKM_Death`）
3. 死亡状态应该没有退出转换，因为角色死亡后不需要再切换到其他状态

## 4. 设置状态转换条件

回到主状态机，为各个状态之间设置转换条件：

1. `Idle` ↔ `Locomotion`：
   - 当`IsMoving`为`true`时，从`Idle`转换到`Locomotion`
   - 当`IsMoving`为`false`时，从`Locomotion`转换到`Idle`

2. 所有状态 → `Attack`：
   - 当`IsAttacking`为`true`且`IsDead`为`false`时

3. 所有状态 → `Hit`：
   - 当`IsTakingDamage`为`true`且`IsDead`为`false`时

4. 所有状态 → `Death`：
   - 当`IsDead`为`true`时

## 5. 连接动画蓝图到敌人蓝图

1. 打开之前创建的敌人蓝图（例如：`BP_DarkKnightEnemy`）
2. 在蓝图编辑器中，点击左侧的"组件"面板
3. 选择`Mesh`组件
4. 在右侧的"细节"面板中，找到`动画模式`属性
5. 设置为`使用动画蓝图`
6. 点击`动画蓝图`属性旁边的下拉菜单
7. 选择刚刚创建的动画蓝图（`ABP_DarkKnight`）

## 6. 在敌人蓝图中控制动画状态

1. 在敌人蓝图编辑器中，点击"图表"选项卡
2. 找到`Tick`事件或创建一个新的自定义事件
3. 根据敌人的状态更新动画蓝图的输入变量：
   - `IsMoving`：当敌人的移动速度大于0时设置为`true`
   - `MoveSpeed`：设置为敌人的实际移动速度
   - `IsAttacking`：当敌人正在攻击时设置为`true`
   - `IsTakingDamage`：当敌人受到伤害时设置为`true`
   - `IsDead`：当敌人死亡时设置为`true`

## 7. 测试动画蓝图

1. 保存所有更改
2. 在内容浏览器中找到敌人蓝图（`BP_DarkKnightEnemy`）
3. 拖拽它到场景中
4. 点击`运行`按钮进入游戏模式
5. 测试各种动画状态：
   - 静止时的闲置动画
   - 移动时的行走/奔跑动画
   - 攻击时的攻击动画
   - 受到伤害时的受击动画
   - 死亡时的死亡动画

## 8. 优化和扩展（可选）

### 添加转向动画
1. 在Locomotion状态中添加转向动画（`Anim_DKM_Walk_Left`、`Anim_DKM_Walk_Right`等）
2. 使用角色的移动方向来选择正确的转向动画

### 添加混合空间
1. 为行走/奔跑创建2D混合空间，以支持不同方向的移动
2. 为攻击创建1D混合空间，以支持流畅的攻击动画过渡

### 添加动画蒙太奇
1. 创建攻击动画蒙太奇来支持连击系统
2. 创建受击动画蒙太奇来支持不同方向的受击效果

## 注意事项

- 确保所有动画序列的骨架都与Dark_Knight_Male的骨架匹配
- 调整移动速度的阈值（例如300）以匹配实际的游戏速度
- 为状态转换添加适当的过渡时间，以确保动画流畅
- 测试所有动画状态的转换，确保没有意外的行为
- 在敌人蓝图中正确设置动画蓝图的输入变量，以确保动画与游戏逻辑同步