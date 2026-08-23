# Plotopia

UE5 第三人称动作游戏 Demo（个人项目）

> **引擎版本**：Unreal Engine 5.7 ｜ **语言**：C++ + 蓝图协同 ｜ **开发周期**：约 2 个月

一个由建筑学背景开发者独立完成的 UE5 动作游戏 Demo，涵盖完整战斗系统（GAS）、敌人 AI、服务器权威背包系统与 PCG 程序化生成。

## 功能特性

### 🗡️ 战斗系统（GAS / GameplayAbilitySystem）
- 按 UE 社区标准 GAS 架构范式实现：ASC 挂载 PlayerState（角色重生不丢数据）、AttributeSet 管理属性、GE 驱动数值结算、GC 播放表现反馈、AbilityTask 串联技能流程
- 自定义 `GAS_AttributeSet`：Health / Mana 属性，含网络复制、OnRep 回调与 `PostGameplayEffectExecute` 数值修正（Clamp / 死亡判定）
- 自定义 AbilityTask：`GAS_AttributeChangeTask`（属性变化监听）、`GAS_WaitGameplayEvent`（GameplayEvent 等待），用于技能流程驱动
- 玩家技能：普攻三连（GA_Primary / GA_Secondary / GA_Tertiary）、范围攻击（消耗蓝条 + 冷却 GameplayEffect）、投射物攻击
- GameplayCue：受击特效（GC_BurstImpact / GC_HitReact）、摄像机震动、四方向受击动画、死亡流程
- 集中式 GameplayTags 管理（`GASTags`），技能、事件、状态全用 Tag 驱动

### 🤖 敌人 AI
- 近战 + 远程两类敌人（BP_Enemy_Melee / BP_Enemy_Ranged）
- 基于 Ability + AITask_MoveTo 的"索敌 → 追击 → 攻击"循环：随机攻击延迟、到达判定（AcceptanceRadius）、转向目标
- GameplayTags 事件驱动：攻击结束（EndAttack）→ 重新索敌，形成完整行为循环
- 受击 / 死亡 / 击退状态，投射物敌人有独立的弹道攻击

### 🎒 背包系统（服务器权威 + 复制）
- 36 格背包 + 9 格快捷栏，DataTable（`DT_Items`）驱动物品数据
- 服务器权威架构：客户端操作自动路由到服务器（Add / Remove / Swap / Move 全套可靠 RPC），属性复制 + 完整状态推送（`ApplyReplicatedState`）双通道保证 UI 同步
- UMG 拖拽交互（物品交换 / 移动）、滚轮切换快捷栏、快捷栏选择状态
- 死亡时背包物品随机散落掉落并清空；自定义 `ItemTrace` 碰撞通道实现世界物品拾取
- 版本号 + 缓存机制（`ItemCountCache` / `EmptySlotCache`）优化 UI 刷新性能

### 🌲 PCG 程序化生成
- 使用 UE PCG 框架程序化生成场景植被与道路（`PCG_RoadBuild` 等生成图）
- 结合 Stylized_Spruce_Forest 资产搭建场景，探索程序化美术工作流

### 🖥️ UI 与输入
- UMG：HUD（血条 / 蓝条）、背包 / 快捷栏界面、主菜单
- WidgetComponent 实现角色头顶血条
- Enhanced Input 分类管理：移动 / 技能 / 物品 / 菜单操作

### ⚙️ 工程化实践
- Steam 联机子系统（AdvancedSessions / AdvancedSteamSessions）配置
- 编辑器 Commandlet 诊断工具：`DumpBpGraphsCommandlet`（导出蓝图图表）、`CleanupBpPrintsCommandlet`（清理调试打印）
- 蓝图工具函数库 `GAS_BlueprintLibrary`（含 Actor Tag 查询等通用工具）
- 项目命名空间（`/Game/GAS/`）与重定向配置（CoreRedirects）管理

## 项目结构

```
Source/Plotopia/
├── Public/
│   ├── AbilitySystem/          # GAS 核心（ASC / AttributeSet / Abilities / AbilityTasks）
│   ├── Characters/             # 玩家 / 敌人角色
│   ├── GameObjects/            # 投射物等游戏对象
│   ├── GameplayTags/           # 集中式 Tag 定义
│   ├── Items/                  # 背包系统（组件 / 世界物品 / 数据）
│   ├── Notifies/               # 动画通知（近战攻击判定）
│   ├── Player/                 # PlayerController / PlayerState
│   ├── UI/                     # HUD / 背包 / 快捷栏 Widget
│   └── Utils/                  # 蓝图工具库
Content/GAS/
├── AbilitySystem/              # 技能 / GameplayEffect / GameplayCue 蓝图资产
├── Characters/                 # 角色蓝图与动画
├── Items/                      # 物品蓝图与 DataTable
├── Maps/                       # GASMap（主场景）/ Startup / kaifang
├── PCG/                        # PCG 生成图资产
└── UI/                         # 界面蓝图
```

## 运行方式

1. 安装 **Unreal Engine 5.7**（Epic Games Launcher）
2. 右键 `Plotopia.uproject` → **Generate Visual Studio project files**（或直接双击打开）
3. 首次打开会自动编译 C++ 模块（约 10–20 分钟）
4. 打开关卡 `Content/GAS/Maps/GASMap` 即可运行

> 需要启用插件：GameplayAbilities、PCG、CommonUI（已在 `.uproject` 中配置）

## 技术要点备忘

- **为什么 ASC 放 PlayerState**：角色重生时 PlayerState 不销毁，技能状态/属性得以保留；且 ASC 的复制需要稳定的属主
- **背包为什么用 RPC 推送而非属性复制**：避免数组属性复制的时序问题，保证客户端 UI 状态一致
- **GAS 网络执行策略**：Ability 区分 `ServerOnly` / `ClientPredicted`，敌人 AI 能力走服务器权威

## 致谢与说明

- GAS 战斗框架基于业界主流的 GameplayAbilitySystem 架构范式实现（参考官方文档与社区最佳实践）
- 背包系统在 AI 辅助下开发，代码经逐行理解与调试
- 使用资产：Paragon Boris / Paragon Minions（Epic 免费）、Stylized_Spruce_Forest（Epic 免费）

## 路线图

- [ ] 上传 Demo 演示视频（B 站）
- [ ] 客户端预测（Ability Prediction）优化战斗手感
- [ ] 自研自定义 PCG 节点（C++ 扩展）
- [ ] 补充关卡设计与更多敌人类型

---

*作者：周尊 ｜ 2026 ｜ 建筑学背景转游戏开发*
