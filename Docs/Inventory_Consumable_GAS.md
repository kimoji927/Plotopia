# 背包消耗品 + GAS 使用系统

在 `Inv_ItemData`（`DT_Items`）里新增了「是否为消耗品」选项：勾选后，**在该物品所在槽位右键即可使用**，
使用逻辑走 GAS（GameplayEffect 应用在玩家 ASC 上，例如回血），效果由数据表配置，可自由更换。

---

## 1. 涉及的文件

| 文件 | 改动 |
| --- | --- |
| `Source/Plotopia/Public/Items/Data/Inv_ItemData.h` | `FInv_ItemDataRow` 新增消耗品相关字段 |
| `Source/Plotopia/Public/GameplayTags/GASTags.h` / `Private/GameplayTags/GASTags.cpp` | 新增原生标签 `GASTags.SetByCaller.Consume`、`GASTags.GASEvents.Player.ConsumeItem` |
| `Source/Plotopia/Public/Items/Components/Inv_InventoryComponent.h` / `.cpp` | 服务器权威的 `UseItemAtSlot`，应用 GE / Cue / 事件，扣除物品 |
| `Source/Plotopia/Public/UI/Inventory/Inv_ItemSlotWidget.h` / `.cpp` | 右键分流：消耗品 → 使用；非消耗品 → 原来的「数量丢弃」 |
| `Source/Plotopia/Public/UI/Inventory/Inv_InventoryWidget.h` / `.cpp` | `UseItemAtSlot` / `IsSlotConsumable`，详情面板「右键使用」提示（可选控件） |

---

## 2. 最简使用步骤

1. **编译** C++。注意：本次改动修改了 `FInv_ItemDataRow` 的**结构体布局**（新增字段），
   Live Coding 对结构体布局变更支持不好 —— 建议**关掉编辑器再编译，然后重新打开编辑器**
   （本次已用 `Build.bat PlotopiaEditor Win64 Development` 编译通过，重启编辑器即可加载新 DLL）。
2. 打开 `Content/GAS/Items/DT_Items`，选中要作为药品的行（如 `Meat`，或自己加一行 `HealthPotion`）。
   重启编辑器后，行结构会自动多出 `Inventory | Consumable` 分类下的新列（旧行取默认值）。
3. 在右侧细节面板 `Inventory | Consumable` 分类下：
   - 勾选 **Is Consumable**（= 消耗品）
   - 在 **Consume Effects** 里添加数组元素，选择 `GE_AddHealth`（工程里已有的回血效果，
     位于 `Content/GAS/AbilitySystem/GameplayEffects/GE_AddHealth`）
4. 进游戏 → 打开背包（默认 `ToggleInventory` 按键）→ 鼠标悬停该物品槽位 → **右键** → 回血，数量 -1。
5. （可选）想让详情面板显示「右键使用」提示：在 `WBP_Inventory` 的详情面板里加一个 `TextBlock`，
   命名为 **`DetailUseHintText`**（`BindWidgetOptional`，不加也不报错）。

> 旧的行不用改：新增列会自动取默认值（`bIsConsumable = false`），所以右键行为保持原样（弹出数量丢弃）。

---

## 3. 数据行字段一览（DT_Items → Inventory | Consumable）

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| **Is Consumable** | bool | 是否为消耗品（勾选后该槽位右键即可使用） |
| **Consume Effects** | GE 类数组 | 使用后应用的 GameplayEffect，可叠加多个（回血 + 回蓝 + Buff…）。**留空时用组件上的 `DefaultConsumeEffect` 兜底** |
| **Consume Effect Level** | float | 应用效果时的等级（GE 内可用等级缩放 / CurveTable） |
| **Consume Magnitude** | float | SetByCaller 数值（> 0 才写入），供 GE 里「Set by Caller」修饰符读取 |
| **Consume Magnitude Tag** | GameplayTag | SetByCaller 标签；留空 → 组件 `DefaultConsumeMagnitudeTag` → 再留空 → `GASTags.SetByCaller.Consume` |
| **Consume On Use** | bool | 使用后是否扣除物品（取消勾选 = 可无限次使用） |
| **Consume Count** | int | 每次使用扣除的数量（默认 1） |
| **Consume Event Tag** | GameplayTag | 使用成功时向玩家发送的 GameplayEvent（可选，见第 5 节） |
| **Consume Cue Tag** | GameplayTag | 使用成功时执行的 GameplayCue（可选，**必须以 `GameplayCue.` 开头**） |

背包组件（挂在 PlayerController 上，蓝图里 `BP_GAS_PlayerController` 的组件属性可见）上的兜底配置：

| 属性 | 说明 |
| --- | --- |
| `DefaultConsumeEffect` | 物品行没配 `Consume Effects` 时使用的统一效果 |
| `DefaultConsumeMagnitude` / `DefaultConsumeMagnitudeTag` | 兜底 SetByCaller 数值 / 标签 |
| `bBlockUseWhenDead` | 死亡后是否禁止使用物品（默认 true） |

---

## 4. 数值（回血量）怎么自由改

两条路，任选：

**A. 直接改 GE 里的固定数值**
改 `GE_AddHealth` 的 Modifiers → `Health` 的 Magnitude（ScalableFloat）即可。每个物品可以指向不同的 GE，
所以「小血瓶 / 大血瓶」直接做两个 GE 就行。

**B. 用 SetByCaller 由数据表驱动数值（推荐，一份 GE 复用）**
1. 复制/编辑 GE，把 Modifiers → `Health` 的 **Magnitude Calculation Type** 改成 **Set by Caller**，
   **Set by Caller Tag** 填 `GASTags.SetByCaller.Consume`（想自定义就在 C++ 里加标签，或直接用字符串标签）。
2. 在 `DT_Items` 里给每个物品填 **Consume Magnitude**（例如 30 = 回 30 点血）。
3. 想换成回蓝：改 Modifiers 的 Attribute 为 `Mana` 即可，或在 **Consume Effects** 里换/加 `GE_AddMana`。

> 运行时也可以改：任何蓝图拿到 `UInv_InventoryComponent` 后设置 `DefaultConsumeEffect` / 行数据即可，
> C++ 里没有硬编码任何具体 GE 资源。

---

## 5. 蓝图 / C++ 接口

### 背包组件 `UInv_InventoryComponent`
```cpp
bool UseItemAtSlot(int32 SlotIndex);                 // 客户端自动路由到服务器
bool CanUseItemAtSlot(int32 SlotIndex, FText& OutFailReason); // 校验 + 失败原因（UI置灰/提示）
bool IsConsumableItem(FName ItemID);                 // 读数据行 bIsConsumable
bool GetItemDataRow(FName ItemID, FInv_ItemDataRow& OutRow);  // 带缓存的行查询
UAbilitySystemComponent* GetOwnerAbilitySystemComponent() const; // 玩家ASC（挂在PlayerState上）解析
UPROPERTY(BlueprintAssignable) FOnItemUsed OnItemUsed;   // (ItemID, SlotIndex, bSuccess)
void OnItemUsedBP(FName ItemID, int32 SlotIndex);        // 蓝图钩子（组件蓝图子类可重写时使用）
```
`OnItemUsed` 成功和失败都会广播，适合接音效、飘字、错误提示。

### 背包 UI `UInv_InventoryWidget`
`UseItemAtSlot(SlotIndex)`（可用于自定义按钮）、`IsSlotConsumable(SlotIndex)`、可选控件 `DetailUseHintText`。

### 物品格子 `UInv_ItemSlotWidget`
`IsItemConsumable()`（可在 WBP 里做「可使用」角标高亮）、蓝图事件 `OnItemUsed(bool bSuccess)`。
快捷栏里的格子同样有效（右键走的是同一套逻辑）。

> 注意：消耗品一旦勾选 `Is Consumable`，**右键就固定是「使用」**，不会再弹数量丢弃框
> （避免主机/客户端表现不一致）；需要丢弃时把物品拖到丢弃区（`WBP_DropZone`）即可。

### 不用打开背包：快捷栏（底部物品栏）直接用 ⭐

游戏里鼠标是隐藏 + `GameOnly` 输入模式，所以 HUD 上的快捷栏格子收不到鼠标事件，
「使用物品」是通过**输入绑定**做的：

| 操作 | 效果 |
| --- | --- |
| 鼠标滚轮 / 数字键 `1`~`9` | 选中快捷栏槽位（高亮由 `WBP_Hotbar` 的 `HighlightSelectedSlot` 表现） |
| **鼠标右键** | 使用**当前选中槽位**里的物品（消耗品）→ 回血 + 数量 -1，全程不用开背包 |

实现位置：`AGAS_PlayerController::SetupInputComponent`
```cpp
// 默认：直接绑鼠标右键，不需要创建任何输入资源
InputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &ThisClass::OnUseItemInput);
// 数字键 1~9 → 选中快捷栏槽位
InputComponent->BindKey(EKeys::One, IE_Pressed, this, &ThisClass::OnHotbarNumberKey); ...
```

想改键 / 加手柄键：在 `BP_GAS_PlayerController` 的 `GAS|Input|Hotbar` 分类里给 **UseItemAction**
指定一个 `InputAction`（自己在 IMC 里映射按键）—— **一旦指定，鼠标右键的默认绑定就自动失效**，改走增强输入。

其它入口（蓝图/UI 都能用）：
* `AGAS_PlayerController::UseSelectedHotbarItem()` / `SelectHotbarSlotByIndex(int32)`
* `UInv_InventoryComponent::UseSelectedHotbarItem()`

保护：死亡时忽略；背包界面打开时忽略（此时鼠标右键由背包格子控件接管，不会被处理两次）。
背包里的热键栏格子（`WBP_Inventory` 的 HotbarGrid）和独立 HUD 快捷栏（`WBP_Hotbar`）用的是同一个 `WBP_ItemSlot`，
所以只要鼠标可见，直接在格子上右键也一样能用。

### GAS 事件（给能力/表现层用）
在数据行填 **Consume Event Tag** = `GASTags.GASEvents.Player.ConsumeItem`，
使用成功时服务器会向玩家 ASC 发一个 GameplayEvent，Payload 约定：

| Payload | 内容 |
| --- | --- |
| `Instigator` | 玩家控制器（使用发起者） |
| `Target` | 玩家角色 |
| `OptionalObject` | `UInv_InventoryComponent`（可读槽位/物品） |
| `EventMagnitude` | 被使用的槽位索引 |

蓝图能力里用 `Wait Gameplay Event`（标签填上面的标签）即可收到，用来播吃药动画 / 音效 / 加 Buff。
更简单的做法是直接在数据行填 **Consume Cue Tag**（如 `GameplayCue.HealthPotion`），走 GameplayCue 播特效。

---

## 6. 网络与权威性

* 使用判定与效果应用**只在服务器**执行，客户端右键只是发一个 `Server_UseItemAtSlot` 可靠 RPC
  （和现有的 `AddItem` / `SwapItems` 一致的写法）。
* 扣除物品后走原有的 `RebuildCache` + `BroadcastUpdate` + `PushStateToClient` 通道，
  客户端 UI 靠版本号轮询刷新，与现有背包逻辑完全兼容。
* 听服主机（Host）本身有权限，直接本地执行，不会绕一圈 RPC。

---

## 7. 排查

| 现象 | 原因 |
| --- | --- |
| 右键弹出「丢弃数量」而不是使用 | 数据行没有勾 **Is Consumable** |
| 右键没反应且日志说「找不到该物品的数据行」 | 物品ID和 `DT_Items` 的行名不一致（当前行名：`Stone` / `Wood` / `Meat`）；场景里的 `BP_Apple` / `BP_HealthPotion` 等拾取物也要有同名行才能命中 |
| 消耗品右键后没反应/没回血 | 看日志 `[Inv][Consume] 使用槽位 x 失败：...`，失败原因会直接打印（不是消耗品 / 找不到数据行 / 未配置使用效果 / 已死亡 / ASC未初始化） |
| 日志提示 GE 未能生效 | GE 被 BlockedTags 或免疫挡住；此时不会扣除物品 |
| 数量扣了但没回血 | GE 没配 Modifier，或 GE 是 `HasDuration` 但周期/时长设置导致数值很小 |
| 改了 `Consume Magnitude` 没反应 | GE 的 Magnitude Calculation Type 不是 **Set by Caller**，或标签不匹配 |
| GameplayCue 不播 | 标签必须以 `GameplayCue.` 开头，且需要对应的 `GameplayCueNotify` 资源 |
| 想用回血以外的效果 | 换 `Consume Effects` 里的 GE 即可（加护盾/加速/Buff 都可以），C++ 不需要改 |
