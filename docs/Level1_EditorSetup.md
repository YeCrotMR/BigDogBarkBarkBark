# 关卡1 零基础配置教程（UE4）

你现在图里基本只有树、房子、地面；**玩法物体（车道、资源点、鸡圈）还没放**，所以感觉「没有功能」。按下面做即可。

---

## 0. 开始前：先编译 C++

1. 关掉正在 Play 的游戏（如果在运行）
2. 顶部工具栏点 **Compile**（或用 Visual Studio / Cursor 编过 `BigDogBarkBarkBarkEditor`）
3. 等编译成功（无红色报错）再往下做  
   > 没编过最新 C++ 时，放置列表里可能看不到 `RTSLaneSpline` 等类。

---

## 方案 A：最快试玩（自动生成玩法）

代码里有自动装配：地图里**没有任何车道**时，点 Play 会在 **PlayerStart 附近**自动生成：

- 2 条战斗道  
- 1 个粮草点  
- 1 个鸡圈  
- 波次管理器  

### 步骤

1. 打开 `Content/Scene/level_1`
2. 右侧 **World Settings** → **GameMode Override** = `RTSGameMode`（必须）
3. 展开 GameMode 下方的 **Selected GameMode**，确认：
   - Player Controller Class = `RTSPlayerController`
   - Default Pawn Class = `RTSCameraPawn`
   - HUD Class = `RTSHUD`
4. 把 **Player Start** 拖到草地一端（玩法会生成在它附近）
5. **完全关掉**正在运行的 Play，再点绿色 **Play**（必要时重启编辑器以加载新 DLL）

### Play 后应出现

- 左上角**黄色/白色文字 HUD**（饲料、波次、按键说明）——不是按钮式 UMG  
- 屏幕顶部短暂提示 `RTS ready...`  
- 按 **2** 应刷出绿色球体（鸡的占位模型）  
- 约 10 秒后出现红色球体敌人  

### 操作

| 键 | 作用 |
|----|------|
| A / D | 镜头左右移 |
| 滚轮 | 缩放 |
| 1 / 2 / 3 / 4 | 招募 兔 / 鸡 / 羊 / 猪 |
| Q / E | 选车道 0 / 1 |
| 左键点自己的单位 + T | 切换 采集 / 战斗 |

若按了 1～4 完全没反应：按 **\`**（波浪键）打开控制台，输入：

```
AddFodderCmd 100
```

再招募。

---

## 方案 B：手动放到你的草地上（推荐长期用）

自动生成在**世界坐标 (0,0,0) 附近**。若你的草地不在原点，玩法会「漂移」到别处。这时对齐草地手动放。

### B1. 打开放置菜单

1. 顶部点 **Place Actors**（或左上 Modes → Place）
2. 搜索栏输入：`RTS`

应能看到（C++ 类）：

- `RTSLaneSpline` — 战斗道  
- `RTSResourceNode` — 粮草点  
- `RTSBaseBuilding` — 鸡圈  
- `RTSWaveManager` — 波次（可选）  

搜不到 → 回去做第 0 步编译，或重启编辑器。

### B2. 放 2 条战斗道

1. 拖一个 `RTSLaneSpline` 到草地  
2. World Outliner 里选中它 → Details：  
   - `Lane Index` = **0**  
3. 再放第二条，`Lane Index` = **1**  
4. 两条路大致平行，沿你草地的长方向摆

#### 调 Spline 形状（很重要）

1. 选中 `RTSLaneSpline`  
2. 点组件 **Spline**（或视口里点控制点）  
3. 默认有两个点：  
   - **起点（靠近基地 / PlayerStart / 鸡圈一侧）**  
   - **终点（敌人进攻来的另一端）**  
4. 拖控制点，让线贴着你的草地路

方向规则（记清楚）：

```
鸡圈 / PlayerStart  ———→  敌人出生（敌端）
     起点 distance=0           终点 distance=最大
```

### B3. 放 1 个粮草点（在其中一条道上）

1. 拖入 `RTSResourceNode`  
2. Details：  
   - `Owner Lane` → 选 **Lane Index=0** 那条道  
   - `Spline Distance` → 例如 **900**（越大越靠敌端）  
3. 刷新/移动一下，会自动吸附到道上（绿/灰小方块标记）

### B4. 放鸡圈

1. 拖入 `RTSBaseBuilding`，放在**起点附近**（PlayerStart 旁边）  
2. Details：  
   - ✅ `Is Core Building`  
   - `Max Health` = **500**

### B5. 波次（可选）

- 不放也可以：运行时会自动建 `RTSWaveManager`  
- 若要放：拖一个到关卡即可，`Lanes` 可留空

### B6. 保存并试玩

1. **Ctrl+S** 保存关卡  
2. **Play**  
3. 用 1～4 出兵，等敌人出现

---

## 对照你截图里的 Outliner

你现在大概只有：

- `Zhuangshi`（装饰）  
- `Floor` / `Player Start` / 灯光天空  

**还没有** `RTSLaneSpline`、`RTSResourceNode`、`RTSBaseBuilding` —— 所以「没功能」是正常的。  
按方案 A 先 Play 出自动物体，或按方案 B 往草地上放这三类。

---

## 常见问题

| 现象 | 原因 / 处理 |
|------|-------------|
| Place 里搜不到 RTS | 未编译成功，或需重启编辑器 |
| Play 后一片空、没 HUD | GameMode 不是 `RTSGameMode`；或用了旧 DLL，重新 Compile |
| 有 HUD但看不到单位 | 相机离出生点太远 → 调 PlayerStart；或用 A/D 移动找 |
| 单位在很远的白地板上 | 自动生成在原点 → 改用方案 B 对齐草地 |
| 只有胶囊没有动物模型 | 正常：皮肤要用蓝图挂 Mesh，玩法可先不管外观 |
| 敌我全是灰色、按 T 没反应 | 需重新 Compile 并重启编辑器。友军=绿球、敌军=红方块；招募后会自动选中，直接按 T 切换采集/战斗（未选中时 T 会选最近的友军） |

---

## 验收清单

- [ ] Play 后左上有饲料数字  
- [ ] 按 2 能出鸡（扣饲料）  
- [ ] 约 10 秒后出现敌方单位  
- [ ] 敌方会往鸡圈方向走并攻击  
- [ ] 鸡圈血量打完会失败提示  

做到以上，关卡1 最小功能就配好了。
