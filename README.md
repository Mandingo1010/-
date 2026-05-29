# 微型无人机识别雷达数据处理

## 1. 项目背景

### 1.1 应用场景

随着低空经济的发展，微型无人机（UAV）在物流、巡检、航拍等领域广泛应用，但同时也带来了"黑飞"等安全隐患。传统的雷达目标识别主要依赖操作员经验判断，效率低且容易出错。本项目基于**雷达航迹特征**，通过数据处理和特征工程，提取可用于机器学习分类的关键特征，实现对微型无人机的自动识别。

### 1.2 项目来源

本项目源自一个真实的科研课题：将原本用 Python 编写的雷达数据处理代码移植为 **C++11** 版本。Python 版本虽然开发快速，但在处理大规模雷达数据时性能不足。C++ 版本的目标是：

- **零第三方依赖**：仅使用 C++11 标准库（STL、`<cmath>`、`<fstream>` 等）
- **面向对象设计**：每个模块封装为类，提供清晰的接口
- **教学导向**：代码结构清晰，适合作为 C++ 程序设计课程设计项目

### 1.3 数据说明

雷达硬件采集的原始数据包含以下 7 个字段：

| 字段名 | 单位 | 说明 |
|--------|------|------|
| 目标方位角 | ° | 雷达测得的目标方位角 |
| 目标斜距 | m | 雷达与目标的直线距离 |
| 相对高度 | m | 目标相对于雷达的高度 |
| 径向速率 | m/s | 目标沿雷达视线方向的速度分量 |
| RCS | m² | 雷达散射截面积，反映目标反射特性 |
| 测量时间 | s | 数据采集时间戳 |
| 航迹序号 | - | 同一目标的连续测量点序列标识 |

**数据特点**：
- 每条航迹由多个连续测量点组成，同一条航迹中除第一个点外，`航迹序号` 列为空
- 数据包含两类目标：`label0.csv`（类别 0）和 `label1.csv`（类别 1）
- 原始数据可能存在缺失值（NaN）或异常值（inf）

**数据目录说明：**

项目提供两套数据，方便开发和测试：

| 目录 | 路径 | 说明 |
|------|------|------|
| `data/` | `data/input/` | **默认使用的简化数据**（约2-4条航迹，用于开发和调试） |
| `data_full/` | `data_full/input/` | **完整数据**（约10000+条航迹，用于最终测试和验证） |

**简化数据（`data/` 目录）：**
- `data/input/label0.csv`：类别0，约2条航迹，197行
- `data/input/label1.csv`：类别1，约2条航迹，142行
- 数据量小，编译运行速度快，便于调试和验证算法正确性

**完整数据（`data_full/` 目录）：**
- `data_full/input/label0.csv`：类别0，约11456条航迹
- `data_full/input/label1.csv`：类别1，约180条航迹
- 数据量大，用于最终性能测试和结果验证

**使用建议：**
1. 开发阶段：使用 `data/` 目录下的简化数据
2. 调试完成后：将路径改为 `data_full/` 进行最终测试
3. 所有模块的默认代码框架中使用的是 `data/` 路径

### 1.4 学习指导

#### 学习Git

本项目需要使用Git协作工具与开源协作平台Gitee完成，下面是一些推荐的教程
- Git+Github+Gitee全套教程：[Git全套教程，git技术大全（GitHub、Gitee码云、GitLab）](https://www.bilibili.com/video/BV1vy4y1s7k6/?share_source=copy_web&vd_source=0570627fe4db8d027abb3de2ed1e49c0)
- 单独的Git教程（视频版）：[【GeekHour】一小时Git教程】](https://www.bilibili.com/video/BV1HM411377j/?share_source=copy_web&vd_source=0570627fe4db8d027abb3de2ed1e49c0)。
- 单独的Git教程（文本版）：https://missing-semester-cn.github.io/2020/version-control/

#### 使用AtomGit协作指导

- 多位组员之间使用Git进行协作开发，依托AtomGit在线仓库进行项目管理。
- 【组长操作】
    + 申请AtomGit账户 (https://atomgit.com/)。
    + 在AtomGit上Fork本仓库到自己的账户下，将自己账户下的仓库作为小组的核心在线开发仓库。
    + 将组员的账户添加为自己仓库的协作者，并设置为开发者权限。
    + 各小组同学通过组长的代码仓库进行协作开发。
    + 安排小组成员互相审核代码。
- 【组员操作】
    + 申请AtomGit账户 (https://atomgit.com/)。
    + 向组长提供自己的AtomGit账户信息。
    + 将组长的仓库Git clone到本地，在本地进行开发并推送。

---

## 2. 项目架构

### 2.1 整体流程

``` {text}
┌─────────────────────────────────────────────────────────────────┐
│                         数据流图                                 │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  Stage 1: 数据增强                                               │
│  ┌─────────────┐    ┌─────────────────────┐                     │
│  │ label*.csv  │───>│  DataAugmentor      │──> aug_*.csv        │
│  └─────────────┘    │  (模块A)            │                     │
│                     └─────────────────────┘                     │
│                                                                 │
│  Stage 2: 统计合并                                               │
│  ┌─────────────────┐    ┌─────────────────────┐                 │
│  │ aug_*.csv       │───>│  StatisticsMerger   │──> train_data   │
│  └─────────────────┘    │  (模块B)            │    _all.csv      │
│                         └─────────────────────┘                 │
│                                                                 │
│  Stage 3: 数据清洗与速度计算                                      │
│  ┌─────────────────┐    ┌─────────────────────┐                 │
│  │ train_data_all  │───>│  DataPreprocessor   │──> cleaned.csv │
│  │     .csv        │    │  (模块C)            │──> speed.csv   │
│  └─────────────────┘    └─────────────────────┘                 │
│                                                                 │
│  Stage 4: 特征提取                                               │
│  ┌─────────────────┐    ┌─────────────────────┐                 │
│  │ cleaned.csv     │───>│  FeatureExtractor   │──> feature*.csv│
│  │ speed.csv       │───>│  (模块D)            │                 │
│  └─────────────────┘    └─────────────────────┘                 │
│                                                                 │
│  Stage 5: 特征合并                                               │
│  ┌─────────────────┐    ┌─────────────────────┐                 │
│  │ feature*.csv    │───>│  PipelineOrchestrator│──> merged_    │
│  └─────────────────┘    │  (模块E)            │    cleaned.csv │
│                         └─────────────────────┘                 │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 2.2 内存数据结构

项目使用以下核心数据结构（定义在 `radar_types.h` 中）：

```cpp
// 原始雷达测量点
struct RadarPoint {
    double azimuth;      // 目标方位角 (°)
    double range;        // 目标斜距 (m)
    double height;       // 相对高度 (m)
    double radial_rate;  // 径向速率 (m/s)
    double rcs;          // RCS
    double time;         // 测量时间 (s)
    int track_id;        // 航迹序号
};

// 速度记录（相邻两点计算得到）
struct SpeedRecord {
    int label;           // 类别标签
    int point1_idx;      // 点1索引
    int point2_idx;      // 点2索引
    double speed;        // 3D速度 (m/s)
    double time_diff;    // 时间差 (s)
    int track_id;        // 航迹ID
};

// 速度统计特征
struct SpeedFeature {
    int track_id;
    double mean_speed;         // 加权平均速度
    double std_speed;          // 加权标准差
    double oscillation_freq;   // 速度震荡频率
    double max_speed;          // 最大速度
    double min_speed;          // 最小速度
    int label;
};

// 径向动力学特征
struct RadialFeature {
    int track_id;
    double max_accel;          // 最大径向加速度
    double max_rate;           // 最大径向速率
    double min_rate;           // 最小径向速率
    double abs_max_accel;      // abs最大径向加速度
    double abs_max_rate;       // abs最大径向速率
    double abs_min_rate;       // abs最小径向速率
    double mean_rcs;           // 平均RCS
    int label;
    double mean_height;        // 相对高度均值
    double std_height;         // 相对高度标准差
};

// 航向角统计特征
struct HeadingFeature {
    int track_id;
    double mean_heading;       // 航向角均值 (弧度)
    double std_heading;        // 航向角标准差
    double oscillation_freq;   // 航向震荡频率
};

// 合并后的特征（最终输出）
struct MergedFeature {
    int track_id;
    double mean_heading;       // 航向角均值
    double std_heading;        // 航向角标准差
    double heading_osc_freq;   // 航向震荡频率
    double mean_speed;         // 平均速度
    double std_speed;          // 速度标准差
    double speed_osc_freq;     // 速度震荡频率
    double max_speed;          // 最大速度
    double min_speed;          // 最小速度
    int label;
    double max_accel;          // 最大径向加速度
    double max_rate;           // 最大径向速率
    double min_rate;           // 最小径向速率
    double abs_max_accel;      // abs最大径向加速度
    double abs_max_rate;       // abs最大径向速率
    double abs_min_rate;       // abs最小径向速率
    double mean_rcs;           // 平均RCS
    double mean_height;        // 相对高度均值
    double std_height;         // 相对高度标准差
};
```

---

## 3. 模块详细说明

整个项目分为5个模块，每个模块负责一个阶段的数据处理。
- 可以根据小组人员数量，将不同模块的开发任务分配给组员。
- 建议模块5（特征合并）由组长负责完成。

### 3.1 模块A：数据增强（DataAugmentor）

**负责人**：开发者 A

**类定义**：`DataAugmentor`（`data_augment.h` / `data_augment.cpp`）

**功能**：对原始雷达航迹数据进行滑动窗口增强，生成多组训练样本。

**核心算法：滑动窗口增强**

**什么是滑动窗口？**

滑动窗口是一种从一个序列中截取固定长度子序列的方法。想象你有一串珠子（数据点），你用一个固定大小的框（窗口）沿着这串珠子滑动，每次截取框内的珠子作为一个新的样本。

**具体步骤：**

假设有一条航迹包含 10 个数据点（编号 0~9），参数为 `stride=2, x=3`：

```
原始航迹: [P0] [P1] [P2] [P3] [P4] [P5] [P6] [P7] [P8] [P9]
```

**Step 1: 按航迹序号分组**

输入数据是按时间顺序排列的所有雷达点。`track_id` 相同的点属于同一条航迹。当 `track_id` 发生变化时，说明进入了一条新的航迹。

例如：
```
点0: track_id=0  <-- 航迹0开始
点1: track_id=0
点2: track_id=0
...
点8: track_id=1  <-- 航迹1开始
点9: track_id=1
...
```

**Step 2: 对每个 (stride, x) 组合处理每条航迹**

- `stride`（步长）：窗口内相邻两个点之间的间隔
- `x`（窗口大小）：每个窗口包含的点数

**Step 3: 滑动窗口截取**

窗口起始位置从 0 开始，每次向后移动 3 个位置（步进=3）：

```
窗口0: 取点 [0, 0+2, 0+4] = [P0, P2, P4]  (start=0)
窗口1: 取点 [3, 3+2, 3+4] = [P3, P5, P7]  (start=3)
窗口2: 取点 [6, 6+2, 6+4] = [P6, P8, P10] -- P10不存在，舍弃
```

窗口内点的索引计算公式：
```
对于第 k 个点（k = 0, 1, ..., x-1）:
    idx = start + k * stride
```

**Step 4: 过滤条件**

仅当航迹长度 `>= x * stride` 时才处理。例如：
- 航迹长度=10, x=3, stride=2: 10 >= 6，可以处理
- 航迹长度=5, x=3, stride=2: 5 < 6，跳过这条航迹

**Step 5: 航迹号处理**

每个窗口截取出来后：
- 窗口内第一个点保留原 `track_id`
- 其余点的 `track_id` 设为 -1（表示属于同一条航迹，但不是航迹起点）

例如窗口 `[P0, P2, P4]` 的输出：
```
P0: track_id = 0  (保留原航迹号)
P2: track_id = -1 (清空)
P4: track_id = -1 (清空)
```

**完整示例：**

假设一条航迹有 8 个点（编号 0~7），参数 `stride=2, x=3`：

```
原始航迹: [0] [1] [2] [3] [4] [5] [6] [7]

检查: 航迹长度 8 >= x * stride = 3 * 2 = 6，满足条件

窗口起始位置: start = 0, 3 (每次+3，直到 start + (x-1)*stride < 长度)
  start=0: 取点 0, 2, 4  -> 窗口0: [P0, P2, P4]
  start=3: 取点 3, 5, 7  -> 窗口1: [P3, P5, P7]
  start=6: 取点 6, 8, 10 -> 8不存在，停止

最终输出2个窗口（2条新的"航迹"片段）
```

**多参数组合：**

对 `label0.csv`，参数为 `strides={1,2}, x_values={5,7,11}`，共 2×3=6 种组合：
- (stride=1, x=5), (stride=1, x=7), (stride=1, x=11)
- (stride=2, x=5), (stride=2, x=7), (stride=2, x=11)

每种组合独立处理所有航迹，生成独立的输出文件。

**为什么要数据增强？**

原始雷达数据通常数量有限。通过滑动窗口，可以从一条长航迹中生成多个短片段作为训练样本，增加训练数据的多样性，提高机器学习模型的泛化能力。

---

**输入输出文件说明**

**输入文件：**

| 文件名 | 路径 | 说明 |
|--------|------|------|
| `label0.csv` | `data/input/label0.csv`（简化版）或 `data_full/input/label0.csv`（完整版） | 类别0原始数据 |
| `label1.csv` | `data/input/label1.csv`（简化版）或 `data_full/input/label1.csv`（完整版） | 类别1原始数据 |

**默认使用 `data/` 目录下的简化数据（约2-4条航迹），便于开发和调试。**
**开发完成后，可切换到 `data_full/` 目录下的完整数据（约10000+条航迹）进行最终测试。**

**输出文件：**

**重要：每种 (stride, x) 参数组合生成一个独立的输出文件。**

对 `label0.csv`，参数 `strides={1,2}, x_values={5,7,11}`，生成 **6 个文件**：
- `data/output/aug_label0_stride1_x5.csv`
- `data/output/aug_label0_stride1_x7.csv`
- `data/output/aug_label0_stride1_x11.csv`
- `data/output/aug_label0_stride2_x5.csv`
- `data/output/aug_label0_stride2_x7.csv`
- `data/output/aug_label0_stride2_x11.csv`

对 `label1.csv`，参数 `strides={1,2,3}, x_values={5,7,11,13,17}`，生成 **15 个文件**：
- `data/output/aug_label1_stride1_x5.csv` 到 `aug_label1_stride1_x17.csv`（5个）
- `data/output/aug_label1_stride2_x5.csv` 到 `aug_label1_stride2_x17.csv`（5个）
- `data/output/aug_label1_stride3_x5.csv` 到 `aug_label1_stride3_x17.csv`（5个）

**共 21 个增强输出文件。**

**输出文件命名规则：**
```
data/output/aug_{label_prefix}_stride{s}_x{x}.csv

其中：
  {label_prefix} = label0 或 label1
  {s} = stride 值
  {x} = x 值
```

**输入文件格式**（CSV）：

文件包含 7 列，第一行为表头，后续每行为一个雷达测量点。

```csv
目标方位角(°),目标斜距(m),相对高度(m),径向速率(m/s),RCS,测量时间(s),航迹序号
210.811157226562,9274,81,-18,0.004,0,0
211.09130859375,9346,85,-18,0.004,0.000999999996565748,
210.87158203125,9381,78,-18,0.042,1.98500000000058,
210.641479492187,9416,77,-18,0.004,2.98099999999476,
210.411376953125,9450,79,-18,0.042,3.98600000000442,
...
```

**格式说明：**
- 第1列：目标方位角（度），浮点数
- 第2列：目标斜距（米），浮点数
- 第3列：相对高度（米），浮点数
- 第4列：径向速率（米/秒），浮点数
- 第5列：RCS（雷达散射截面积），浮点数
- 第6列：测量时间（秒），浮点数
- 第7列：航迹序号，整数。同一条航迹中只有第一个点有值，其余为空

**输出文件格式**（CSV）：与输入格式完全相同，但数据经过滑动窗口截取。

**输出文件示例**（`aug_label0_stride1_x5.csv`）：

```csv
目标方位角(°),目标斜距(m),相对高度(m),径向速率(m/s),RCS,测量时间(s),航迹序号
210.811157226562,9274,81,-18,0.004,0,0
211.09130859375,9346,85,-18,0.004,0.000999999996565748,
210.87158203125,9381,78,-18,0.042,1.98500000000058,
210.641479492187,9416,77,-18,0.004,2.98099999999476,
210.411376953125,9450,79,-18,0.042,3.98600000000442,
210.811157226562,9274,81,-18,0.004,0,0
211.09130859375,9346,85,-18,0.004,0.000999999996565748,
...
```

注意：输出文件中，每个窗口的第一个点保留原航迹号，其余点航迹序号为空（与输入格式一致）。
```cpp
class DataAugmentor {
public:
    void set_params(const std::vector<int>& strides,
                    const std::vector<int>& x_values);
    std::vector<std::vector<RadarPoint>> augment(
        const std::vector<RadarPoint>& input);
    int get_num_tracks() const;
    int get_num_windows() const;
};
```

**输入**：`std::vector<RadarPoint>`（原始雷达数据，已按航迹分组）

**输出**：`std::vector<std::vector<RadarPoint>>`（增强后的多组数据，每组是一个航迹片段）

**参数配置**：
- `label0.csv`：strides `{1, 2}`，x_values `{5, 7, 11}`
- `label1.csv`：strides `{1, 2, 3}`，x_values `{5, 7, 11, 13, 17}`

**输入文件格式**（CSV）：
```csv
目标方位角(°),目标斜距(m),相对高度(m),径向速率(m/s),RCS,测量时间(s),航迹序号
210.811157226562,9274,81,-18,0.004,0,0
211.09130859375,9346,85,-18,0.004,0.000999999996565748,
...
```

**输出文件格式**（CSV）：与输入格式相同，但数据经过滑动窗口截取

---

### 3.2 模块B：统计合并（StatisticsMerger）

**负责人**：开发者 B

**类定义**：`StatisticsMerger`（`statistics_merge.h` / `statistics_merge.cpp`）

**功能**：将所有增强后的数据文件合并为一个完整的训练数据集，并生成数据统计报告。

**核心算法：统计合并**

**Step 1: 合并所有增强数据**

将所有增强后的数据组按顺序拼接为一个大的数据序列。

**示例：**

```
增强数据组1（来自 aug_label0_stride1_x5.csv）:
  [P0, P1, P2, P3, P4]  <- 窗口0
  [P5, P6, P7, P8, P9]  <- 窗口1

增强数据组2（来自 aug_label0_stride1_x7.csv）:
  [Q0, Q1, Q2, Q3, Q4, Q5, Q6]  <- 窗口0

合并结果:
  [P0, P1, P2, P3, P4, P5, P6, P7, P8, P9, Q0, Q1, Q2, Q3, Q4, Q5, Q6]
```

**Step 2: 统计原始数据行数**

读取原始输入文件（label0.csv 和 label1.csv），统计总行数（不包括表头）。

**Step 3: 统计增强数据行数**

合并后的数据总行数。

**Step 4: 统计重复行**

**什么是重复行？**

两行数据的所有字段值完全相同（包括 track_id, azimuth, range, height, radial_rate, rcs, time）。

**如何检测？**

将每行数据转换为一个字符串（各字段用逗号连接），使用 `std::set<std::string>` 检测重复：

```cpp
std::set<std::string> seen;
int duplicates = 0;

对每行数据:
  line_str = "azimuth,range,height,rate,rcs,time,track_id"
  如果 line_str 已在 seen 中:
    duplicates++
  否则:
    seen.insert(line_str)
```

**示例：**

```
行0: "210.8,9274,81,-18,0.004,0,0"
行1: "211.1,9346,85,-18,0.004,0.001,0"
行2: "210.8,9274,81,-18,0.004,0,0"  <- 与行0完全相同！

seen 初始: {}
处理行0: seen = {"210.8,9274,81,-18,0.004,0,0"}
处理行1: seen = {"210.8,9274,81,-18,0.004,0,0", "211.1,9346,85,-18,0.004,0.001,0"}
处理行2: 已在 seen 中，duplicates = 1
```

**Step 5: 计算统计指标**

```
重复行占比 = duplicates / augmented_count * 100%
唯一行数 = augmented_count - duplicates
```

**Step 6: 输出报告**

```
--- Data Statistics Report ---
Original data: 12345 rows
Augmented data: 67890 rows
Duplicate rows: 1234 (1.82%)
Unique rows: 66656
```

---

**输入输出文件说明**

**输入文件：**

模块B需要读取模块A生成的所有增强文件，以及原始输入文件用于统计对比。

| 文件名 | 路径 | 说明 |
|--------|------|------|
| `aug_label0_stride*.csv` | `data/output/aug_label0_stride*.csv`（简化版）或 `data_full/output/aug_label0_stride*.csv`（完整版） | 类别0增强数据（6个文件） |
| `aug_label1_stride*.csv` | `data/output/aug_label1_stride*.csv`（简化版）或 `data_full/output/aug_label1_stride*.csv`（完整版） | 类别1增强数据（15个文件） |
| `label0.csv` | `data/input/label0.csv`（简化版）或 `data_full/input/label0.csv`（完整版） | 类别0原始数据（用于统计） |
| `label1.csv` | `data/input/label1.csv`（简化版）或 `data_full/input/label1.csv`（完整版） | 类别1原始数据（用于统计） |

**输出文件：**

| 文件名 | 路径 | 说明 |
|--------|------|------|
| `train_data_all.csv` | `data/tmp/train_data_all.csv`（简化版）或 `data_full/tmp/train_data_all.csv`（完整版） | 合并后的完整训练数据 |

**输入文件格式**（与模块A输出相同）：

```csv
目标方位角(°),目标斜距(m),相对高度(m),径向速率(m/s),RCS,测量时间(s),航迹序号
210.811157226562,9274,81,-18,0.004,0,0
211.09130859375,9346,85,-18,0.004,0.000999999996565748,
...
```

**输出文件格式**（`train_data_all.csv`）：

与输入格式相同，但包含所有增强文件的合并结果。

```csv
目标方位角(°),目标斜距(m),相对高度(m),径向速率(m/s),RCS,测量时间(s),航迹序号
210.811157226562,9274,81,-18,0.004,0,0
211.09130859375,9346,85,-18,0.004,0.000999999996565748,
210.87158203125,9381,78,-18,0.042,1.98500000000058,
...
```

文件较大（完整数据约 200MB），包含所有增强数据的拼接结果。
```cpp
class StatisticsMerger {
public:
    std::vector<RadarPoint> merge(
        const std::vector<std::vector<RadarPoint>>& inputs);
    void generate_report(int orig_count) const;
    int get_augmented_count() const;
    int get_duplicate_count() const;
    int get_unique_count() const;
};
```

**输入**：`std::vector<std::vector<RadarPoint>>`（多组增强数据）

**输出**：`std::vector<RadarPoint>`（合并后的训练数据）

**控制台输出示例**：
```
--- Data Statistics Report ---
Original data: 12345 rows
Augmented data: 67890 rows
Duplicate rows: 1234 (1.82%)
Unique rows: 66656
```

---

### 3.3 模块C：数据清洗与速度计算（DataPreprocessor）

**负责人**：开发者 C

**类定义**：`DataPreprocessor`（`data_preprocessor.h` / `data_preprocessor.cpp`）

**功能**：对合并后的训练数据进行清洗（去除异常值），并计算相邻点之间的 3D 速度。

**核心算法：数据清洗**

**Step 1: 按航迹分组**

输入数据是按时间顺序排列的雷达点序列。`track_id` 相同的连续点属于同一条航迹。当遇到一个新的 `track_id`（与上一个点的 `track_id` 不同），说明进入了一条新的航迹。

例如：
```
输入序列: [P0(tid=0), P1(tid=0), P2(tid=0), P3(tid=1), P4(tid=1), ...]

分组结果:
  航迹0: [P0, P1, P2]
  航迹1: [P3, P4, ...]
```

**Step 2: 每组按时间排序**

虽然输入数据通常已经按时间排序，但为了确保正确性，需要对每组内的点按 `time` 字段升序排序。

排序前：
```
航迹0: [P0(time=2.0), P1(time=0.0), P2(time=1.0)]
```

排序后：
```
航迹0: [P1(time=0.0), P2(time=1.0), P0(time=2.0)]
```

**Step 3: 异常值过滤**

遍历每组内的每个点，检查以下两个字段：
- `radial_rate`（径向速率）
- `time`（测量时间）

如果任一字段为 **NaN**（Not a Number，非数字）或 **inf**（无穷大），则剔除该点。

**如何判断 NaN 和 inf？**

```cpp
// 判断 NaN: NaN 的特性是自己不等于自己
bool is_nan = (val != val);

// 判断 inf: 与无穷大比较
bool is_inf = (val == std::numeric_limits<double>::infinity()) ||
              (val == -std::numeric_limits<double>::infinity());
```

**Step 4: 标签前移（重点！）**

这是最容易出错的地方。在雷达数据中，每条航迹的第一个点包含有效的 `track_id`，其余点的 `track_id` 为 -1。

当被剔除的点恰好是航迹的第一个点（包含有效 `track_id`）时，需要将它的 `track_id` **前移**给下一个有效点，以保证航迹的完整性。

**示例：**

```
原始航迹0:
  点0: track_id=0, radial_rate=NaN, time=0.0  <-- 异常，需要剔除
  点1: track_id=-1, radial_rate=10.0, time=1.0
  点2: track_id=-1, radial_rate=12.0, time=2.0

处理过程:
  1. 点0是异常值，需要剔除
  2. 但点0包含 track_id=0，需要前移给点1
  3. 点1的 track_id 从 -1 变为 0

清洗后:
  点0(原点1): track_id=0, radial_rate=10.0, time=1.0
  点1(原点2): track_id=-1, radial_rate=12.0, time=2.0
```

**如果没有标签前移：**

```
错误结果:
  点0(原点1): track_id=-1  <-- 航迹0失去了标识！
  点1(原点2): track_id=-1
```

这样航迹0就会丢失，后续处理会出错。

**完整示例：**

```
原始数据:
  航迹0:
    P0: tid=0, rate=NaN,  time=0.0  <-- 异常，但有tid
    P1: tid=-1, rate=10.0, time=1.0
    P2: tid=-1, rate=inf,  time=2.0  <-- 异常
    P3: tid=-1, rate=12.0, time=3.0

处理过程:
  1. P0异常，剔除，tid=0前移给P1
  2. P1变为 tid=0
  3. P2异常，剔除，tid=-1无需前移
  4. P3保持 tid=-1

清洗后:
  航迹0:
    P0(原P1): tid=0, rate=10.0, time=1.0
    P1(原P3): tid=-1, rate=12.0, time=3.0
```

---

**核心算法：3D 速度计算**

**背景知识：雷达坐标系**

雷达测量的是球坐标（方位角、斜距、高度），需要转换为直角坐标（x, y, z）才能计算真实的 3D 速度。

```
雷达坐标系:
  - 方位角(azimuth): 从正北方向顺时针旋转的角度（度）
  - 斜距(range): 雷达到目标的直线距离
  - 高度(height): 目标相对于雷达的高度

转换公式:
  1. 将方位角从度转换为弧度:
     azimuth_rad = azimuth * PI / 180.0

  2. 计算水平投影距离（地面上雷达到目标正下方的距离）:
     r = sqrt(range^2 - height^2)
     解释: 根据勾股定理，range是斜边，height是一条直角边，r是另一条直角边

  3. 转换为直角坐标:
     x = r * sin(azimuth_rad)   // 东西方向分量
     y = r * cos(azimuth_rad)   // 南北方向分量
     z = height                  // 垂直方向分量
```

**为什么需要转换？**

径向速率（radial_rate）只是目标速度在雷达视线方向的分量。例如：
- 如果目标绕雷达做圆周运动，径向速率可能为 0，但目标实际速度不为 0
- 3D 速度通过计算相邻时刻目标位置的变化，能更准确地反映真实运动状态

**速度计算步骤：**

给定两个雷达点 P1 和 P2：

```
Step 1: 分别将 P1 和 P2 转换为直角坐标
  P1: (x1, y1, z1)
  P2: (x2, y2, z2)

Step 2: 计算两点间的欧氏距离
  dx = x2 - x1
  dy = y2 - y1
  dz = z2 - z1
  d = sqrt(dx^2 + dy^2 + dz^2)

Step 3: 计算时间差
  dt = P2.time - P1.time

Step 4: 计算速度
  speed = d / dt

Step 5: 处理特殊情况
  如果 dt == 0:
    返回 inf（无穷大），避免除以零
```

**完整公式：**

```cpp
double calculate_speed(const RadarPoint& p1, const RadarPoint& p2) {
    // 度转弧度
    double az1_rad = p1.azimuth * PI / 180.0;
    double az2_rad = p2.azimuth * PI / 180.0;

    // 水平投影
    double r1 = sqrt(p1.range * p1.range - p1.height * p1.height);
    double r2 = sqrt(p2.range * p2.range - p2.height * p2.height);

    // 转直角坐标
    double x1 = r1 * sin(az1_rad);
    double y1 = r1 * cos(az1_rad);
    double z1 = p1.height;

    double x2 = r2 * sin(az2_rad);
    double y2 = r2 * cos(az2_rad);
    double z2 = p2.height;

    // 距离和时间差
    double d = sqrt((x2-x1)^2 + (y2-y1)^2 + (z2-z1)^2);
    double dt = p2.time - p1.time;

    // 计算速度
    if (dt == 0) return inf;
    return d / dt;
}
```

**数值示例：**

```
P1: azimuth=0°, range=100m, height=0m, time=0s
P2: azimuth=0°, range=110m, height=0m, time=1s

计算:
  r1 = sqrt(100^2 - 0^2) = 100
  r2 = sqrt(110^2 - 0^2) = 110

  x1 = 100 * sin(0) = 0
  y1 = 100 * cos(0) = 100
  z1 = 0

  x2 = 110 * sin(0) = 0
  y2 = 110 * cos(0) = 110
  z2 = 0

  d = sqrt((0-0)^2 + (110-100)^2 + (0-0)^2) = sqrt(0 + 100 + 0) = 10
  dt = 1 - 0 = 1

  speed = 10 / 1 = 10 m/s
```

---

**核心算法：速度记录计算**

**Step 1: 遍历相邻点**

对清洗后的数据，依次取相邻的两个点 (P[i], P[i+1]) 计算速度。

**Step 2: 跳过跨航迹计算**

关键判断：如果当前点 `track_id == -1` 且下一点 `track_id != -1`，说明下一点是新航迹的起点，两点不属于同一航迹，不能计算速度。

```
数据序列:
  [P0(tid=0), P1(tid=-1), P2(tid=-1), P3(tid=1), P4(tid=-1)]

相邻点对:
  (P0, P1): tid0=0, tid1=-1  --> 同航迹，计算速度
  (P1, P2): tid1=-1, tid2=-1 --> 同航迹，计算速度
  (P2, P3): tid2=-1, tid3=1  --> P2是航迹0末尾，P3是航迹1开头，跳过！
  (P3, P4): tid3=1, tid4=-1  --> 同航迹，计算速度
```

**Step 3: 构建 SpeedRecord**

对每个有效的相邻点对：
```cpp
SpeedRecord record;
record.label = current_track_id;    // 当前航迹ID作为标签
record.point1_idx = i;              // 第一个点的索引
record.point2_idx = i + 1;          // 第二个点的索引
record.speed = calculate_speed(P[i], P[i+1]);  // 3D速度
record.time_diff = P[i+1].time - P[i].time;    // 时间差
record.track_id = current_track_id; // 航迹ID
```

**Step 4: 异常值过滤**

剔除 `speed` 或 `time_diff` 为 NaN/inf 的记录（与数据清洗类似）。

**输出格式：**

```csv
标签,点1索引,点2索引,速度(m/s),time,航迹ID
0,0,1,10.5,1.0,0
0,1,2,11.2,1.0,0
1,3,4,5.8,2.0,1
...
```

---

**输入输出文件说明**

**输入文件：**

| 文件名 | 路径 | 说明 |
|--------|------|------|
| `train_data_all.csv` | `data/tmp/train_data_all.csv`（简化版）或 `data_full/tmp/train_data_all.csv`（完整版） | 模块B合并后的训练数据 |

**输出文件：**

| 文件名 | 路径 | 说明 |
|--------|------|------|
| `middle2_cleaned.csv` | `data/tmp/middle2_cleaned.csv`（简化版）或 `data_full/tmp/middle2_cleaned.csv`（完整版） | 清洗后的雷达数据 |
| `middle5_cleaned.csv` | `data/tmp/middle5_cleaned.csv`（简化版）或 `data_full/tmp/middle5_cleaned.csv`（完整版） | 速度计算结果 |

**输入文件格式**（`train_data_all.csv`）：

与模块A输出格式相同，7列CSV：

```csv
目标方位角(°),目标斜距(m),相对高度(m),径向速率(m/s),RCS,测量时间(s),航迹序号
210.811157226562,9274,81,-18,0.004,0,0
211.09130859375,9346,85,-18,0.004,0.000999999996565748,
210.87158203125,9381,78,-18,0.042,1.98500000000058,
...
```

**输出文件1格式**（`middle2_cleaned.csv`，清洗后的雷达数据）：

与输入格式相同，但已去除异常值并排序：

```csv
目标方位角(°),目标斜距(m),相对高度(m),径向速率(m/s),RCS,测量时间(s),航迹序号
210.811157226562,9274,81,-18,0.004,0,0
211.09130859375,9346,85,-18,0.004,0.000999999996565748,
210.87158203125,9381,78,-18,0.042,1.98500000000058,
...
```

**输出文件2格式**（`middle5_cleaned.csv`，速度记录）：

6列CSV，每行代表相邻两点的速度计算结果：

```csv
标签,点1索引,点2索引,速度(m/s),time,航迹ID
0,0,1,15.2345678901234,0.000999999996565748,0
0,1,2,14.8765432109876,1.98400000000384,0
0,2,3,15.1234567890123,0.99599999999418,0
1,150,151,3.45678901234567,1.97999999999593,1
1,151,152,3.56789012345678,2.01500000000306,1
...
```

**格式说明（速度记录）：**
- 第1列：标签（即航迹ID），整数
- 第2列：点1索引（在原始数据中的位置），整数
- 第3列：点2索引（在原始数据中的位置），整数
- 第4列：3D速度（米/秒），浮点数
- 第5列：时间差（秒），浮点数
- 第6列：航迹ID，整数

**接口**：
```cpp
class DataPreprocessor {
public:
    std::vector<RadarPoint> process(const std::vector<RadarPoint>& input);
    static double calculate_speed(const RadarPoint& p1, const RadarPoint& p2);
    std::vector<SpeedRecord> compute_speed_records(
        const std::vector<RadarPoint>& input);
    int get_input_count() const;
    int get_output_count() const;
    int get_removed_count() const;
};
```

**输入**：`std::vector<RadarPoint>`（原始雷达数据）

**输出**：
- `process()` → `std::vector<RadarPoint>`（清洗后的数据）
- `compute_speed_records()` → `std::vector<SpeedRecord>`（速度记录）

**速度记录输出格式**（CSV）：
```csv
标签,点1索引,点2索引,速度(m/s),time,航迹ID
1,0,1,1.83129,1.98,1
...
```

---

### 3.4 模块D：特征提取（FeatureExtractor）

**负责人**：开发者 D

**类定义**：`FeatureExtractor`（`feature_extractor.h` / `feature_extractor.cpp`）

**功能**：从清洗后的数据中提取三类统计特征，用于后续的机器学习分类。

**核心算法：速度统计特征提取**

**输入**：`std::vector<SpeedRecord>`（速度记录列表）

**Step 1: 按航迹分组**

根据 `track_id` 将速度记录分组。`track_id` 相同的记录属于同一航迹。

```
输入:
  [R0(tid=0, speed=10.0, time=1.0),
   R1(tid=0, speed=12.0, time=1.5),
   R2(tid=0, speed=11.0, time=2.0),
   R3(tid=1, speed=5.0, time=1.0),
   R4(tid=1, speed=6.0, time=2.0)]

分组:
  航迹0: [R0, R1, R2]
  航迹1: [R3, R4]
```

**Step 2: 对每组计算统计特征**

假设某航迹有 N 条速度记录，每条记录包含 `speed` 和 `time_diff`。

**(1) 加权平均速度**

以 `time_diff` 为权重，计算加权平均：

```
总位移 = sum(speed[i] * time_diff[i])  对所有 i
总时间 = sum(time_diff[i])  对所有 i
加权平均速度 = 总位移 / 总时间
```

**为什么加权？**

不同速度记录对应的时间间隔可能不同。例如：
- 记录A: speed=10m/s, time_diff=1s  --> 位移=10m
- 记录B: speed=12m/s, time_diff=2s  --> 位移=24m

简单平均: (10 + 12) / 2 = 11 m/s
加权平均: (10*1 + 12*2) / (1+2) = 34/3 = 11.33 m/s

加权平均更准确，因为记录B持续了更长时间。

**(2) 加权标准差**

```
方差 = sum(time_diff[i] * (speed[i] - 平均速度)^2) / 总时间
标准差 = sqrt(方差)
```

**示例：**

```
速度记录: [(10, 1.0), (12, 2.0), (11, 1.5)]
  (speed, time_diff)

总位移 = 10*1.0 + 12*2.0 + 11*1.5 = 10 + 24 + 16.5 = 50.5
总时间 = 1.0 + 2.0 + 1.5 = 4.5
平均速度 = 50.5 / 4.5 = 11.222...

方差 = [1.0*(10-11.222)^2 + 2.0*(12-11.222)^2 + 1.5*(11-11.222)^2] / 4.5
     = [1.0*1.494 + 2.0*0.605 + 1.5*0.049] / 4.5
     = [1.494 + 1.210 + 0.074] / 4.5
     = 2.778 / 4.5
     = 0.617

标准差 = sqrt(0.617) = 0.786
```

**(3) 速度震荡频率**

统计相邻速度变化剧烈的频率：

```
震荡次数 = count(|speed[i+1] - speed[i]| > 1.0)  对所有相邻对
震荡频率 = 震荡次数 / (N - 1)
```

**示例：**

```
速度序列: [10.0, 12.5, 11.0, 15.0]

相邻差值:
  |12.5 - 10.0| = 2.5 > 1.0  --> 震荡+1
  |11.0 - 12.5| = 1.5 > 1.0  --> 震荡+1
  |15.0 - 11.0| = 4.0 > 1.0  --> 震荡+1

震荡次数 = 3
N = 4
震荡频率 = 3 / (4-1) = 3/3 = 1.0
```

**(4) 最大/最小速度**

直接取该航迹所有速度记录中的最大值和最小值。

**输出结构：**

对每个航迹，输出一个 `SpeedFeature`：
```cpp
struct SpeedFeature {
    int track_id;              // 航迹ID
    double mean_speed;         // 加权平均速度
    double std_speed;          // 加权标准差
    double oscillation_freq;   // 速度震荡频率
    double max_speed;          // 最大速度
    double min_speed;          // 最小速度
    int label;                 // 类别标签（从track_id获取）
};
```

---

**核心算法：径向动力学特征提取**

**输入**：`std::vector<RadarPoint>`（清洗后的雷达数据）

**Step 1: 按航迹分组**

根据 `track_id` 分组。

**Step 2: 对每组计算统计特征**

假设某航迹有 N 个雷达点，每个点包含 `radial_rate`, `time`, `rcs`, `height`。

**(1) 最大径向加速度**

径向加速度 = 径向速率的变化 / 时间变化

```
accel[i] = (rate[i+1] - rate[i]) / (time[i+1] - time[i])

最大径向加速度 = accel[i] 中绝对值最大的那个（保留符号）
```

**示例：**

```
时间:    [0.0, 1.0, 2.0, 3.0]
径向速率: [5.0, 7.0, 4.0, 6.0]

accel[0] = (7.0 - 5.0) / (1.0 - 0.0) = 2.0 / 1.0 = 2.0
accel[1] = (4.0 - 7.0) / (2.0 - 1.0) = -3.0 / 1.0 = -3.0
accel[2] = (6.0 - 4.0) / (3.0 - 2.0) = 2.0 / 1.0 = 2.0

绝对值: [2.0, 3.0, 2.0]
最大绝对值是 3.0，对应 accel[1] = -3.0

最大径向加速度 = -3.0
```

**(2) 最大/最小径向速率**

按绝对值比较，但保留原始符号：

```
最大径向速率 = rate[i] 中 |rate[i]| 最大的那个（保留符号）
最小径向速率 = rate[i] 中 |rate[i]| 最小的那个（保留符号）
```

**示例：**

```
径向速率: [-5.0, 3.0, -8.0, 2.0]

绝对值: [5.0, 3.0, 8.0, 2.0]
最大绝对值 = 8.0，对应 -8.0  --> 最大径向速率 = -8.0
最小绝对值 = 2.0，对应 2.0   --> 最小径向速率 = 2.0
```

**(3) abs 版本**

```
abs最大径向加速度 = |最大径向加速度|
abs最大径向速率 = |最大径向速率|
abs最小径向速率 = |最小径向速率|
```

**(4) 平均 RCS**

```
平均RCS = sum(rcs[i]) / N
```

**(5) 相对高度均值和标准差**

```
高度均值 = sum(height[i]) / N

方差 = sum((height[i] - 高度均值)^2) / N   <-- 注意除以N，不是N-1
高度标准差 = sqrt(方差)
```

**示例：**

```
高度: [80.0, 82.0, 78.0, 80.0]
N = 4

高度均值 = (80 + 82 + 78 + 80) / 4 = 320 / 4 = 80.0

方差 = [(80-80)^2 + (82-80)^2 + (78-80)^2 + (80-80)^2] / 4
     = [0 + 4 + 4 + 0] / 4
     = 8 / 4
     = 2.0

高度标准差 = sqrt(2.0) = 1.414
```

**输出结构：**

```cpp
struct RadialFeature {
    int track_id;
    double max_accel;          // 最大径向加速度
    double max_rate;           // 最大径向速率
    double min_rate;           // 最小径向速率
    double abs_max_accel;      // abs最大径向加速度
    double abs_max_rate;       // abs最大径向速率
    double abs_min_rate;       // abs最小径向速率
    double mean_rcs;           // 平均RCS
    int label;                 // 类别标签
    double mean_height;        // 相对高度均值
    double std_height;         // 相对高度标准差
};
```

---

**核心算法：航向角统计特征提取**

**输入**：`std::vector<RadarPoint>`（清洗后的雷达数据）

**背景知识：圆周统计**

方位角是角度（0°~360°），具有周期性：0° 和 360° 是同一个方向。普通的算术平均不适用，需要使用**圆周统计**。

**Step 1: 度转弧度**

```
heading_rad = azimuth * PI / 180.0
```

**Step 2: 圆周均值**

将角度视为单位圆上的点，计算这些点的平均位置：

```
对每个角度 heading[i]:
  sin_sum += sin(heading[i])
  cos_sum += cos(heading[i])

mean_sin = sin_sum / N
mean_cos = cos_sum / N

圆周均值 = atan2(mean_sin, mean_cos)
```

**为什么用 atan2？**

`atan2(y, x)` 返回从 x 轴到点 (x, y) 的角度，能正确处理所有象限。

**示例：**

```
方位角: [10°, 20°, 30°]

sin(10°) = 0.1736,  sin(20°) = 0.3420,  sin(30°) = 0.5
sin_sum = 0.1736 + 0.3420 + 0.5 = 1.0156
mean_sin = 1.0156 / 3 = 0.3385

cos(10°) = 0.9848,  cos(20°) = 0.9397,  cos(30°) = 0.8660
cos_sum = 0.9848 + 0.9397 + 0.8660 = 2.7905
mean_cos = 2.7905 / 3 = 0.9302

圆周均值 = atan2(0.3385, 0.9302) = 0.349 弧度 ≈ 20°
```

**Step 3: 圆周标准差**

```
R = sqrt(mean_cos^2 + mean_sin^2)

圆周标准差 = -2 * ln(R)    (ln 是自然对数)
```

**特殊情况：**
- 如果 R > 1（浮点误差导致），标准差设为 0
- 如果 R = 1（所有角度相同），标准差 = 0
- 如果 R = 0（角度均匀分布），标准差 = inf

**示例：**

```
mean_cos = 0.9302, mean_sin = 0.3385

R = sqrt(0.9302^2 + 0.3385^2)
  = sqrt(0.8653 + 0.1146)
  = sqrt(0.9799)
  = 0.9899

圆周标准差 = -2 * ln(0.9899)
           = -2 * (-0.0101)
           = 0.0202
```

**Step 4: 航向震荡频率**

统计航向角变化的方向震荡频率。

**(a) 计算相邻方位角差值**

```
diff[i] = heading[i+1] - heading[i]    (i = 0, 1, ..., N-2)
```

**(b) 构建 O 数组（方向数组）**

根据差值的大小判断方向变化：

```
阈值 gamma = 0.2（弧度，约 11.5°）

对每个 diff[i]:
  如果 diff[i] > gamma:      O[i] = +1  (正向变化)
  如果 diff[i] < -gamma:     O[i] = -1  (负向变化)
  否则:                      O[i] = 0   (无显著变化)
```

**(c) 统计震荡模式**

遍历 O 数组，统计两种震荡模式：

**模式1：连续符号变化**
```
条件: O[i-1] + O[i] == 0 且 O[i-1] != O[i]
解释: O[i-1] = +1 且 O[i] = -1，或 O[i-1] = -1 且 O[i] = +1
      即方向从正变负或从负变正

示例:
  O = [+1, -1, 0, +1]
  i=1: O[0]=+1, O[1]=-1, +1 + (-1) = 0, 且 +1 != -1  --> 震荡+1
  i=2: O[1]=-1, O[2]=0, -1 + 0 = -1 != 0  --> 不震荡
  i=3: O[2]=0, O[3]=+1, 0 + 1 = 1 != 0  --> 不震荡
```

**模式2：间隔符号变化**
```
条件: O[i-1] + O[i+1] == 0 且 O[i-1] != O[i+1] 且 O[i] == 0
解释: 中间有一个0，但两边符号相反
      例如: [+1, 0, -1] 或 [-1, 0, +1]

示例:
  O = [+1, 0, -1, +1]
  i=1: O[0]=+1, O[1]=0, O[2]=-1
       O[0] + O[2] = +1 + (-1) = 0
       O[0] != O[2] (+1 != -1)
       O[1] == 0
       --> 震荡+1
```

**(d) 计算频率**

```
震荡频率 = 震荡次数 / (N - 2)
```

**为什么除以 N-2？**

因为需要至少 3 个点才能判断震荡模式（需要 O[i-1], O[i], O[i+1]）。

**完整示例：**

```
方位角（弧度）: [0.1, 0.3, 0.5, 0.4, 0.2, 0.3]
N = 6

Step 1: 已经是弧度，无需转换

Step 2: 计算差值
  diff[0] = 0.3 - 0.1 = 0.2
  diff[1] = 0.5 - 0.3 = 0.2
  diff[2] = 0.4 - 0.5 = -0.1
  diff[3] = 0.2 - 0.4 = -0.2
  diff[4] = 0.3 - 0.2 = 0.1

Step 3: 构建 O 数组 (gamma = 0.2)
  diff[0] = 0.2:  |0.2| <= 0.2  --> O[0] = 0
  diff[1] = 0.2:  |0.2| <= 0.2  --> O[1] = 0
  diff[2] = -0.1: |-0.1| < 0.2  --> O[2] = 0
  diff[3] = -0.2: |-0.2| <= 0.2 --> O[3] = 0
  diff[4] = 0.1:  |0.1| < 0.2   --> O[4] = 0

  O = [0, 0, 0, 0, 0]
  无震荡，频率 = 0

另一个例子:
方位角: [0.1, 0.5, 0.2, 0.6, 0.3]

diff: [0.4, -0.3, 0.4, -0.3]
O:    [+1, -1, +1, -1]    (gamma=0.2)

震荡统计:
  i=1: O[0]=+1, O[1]=-1, +1+(-1)=0, +1!=-1 --> 震荡+1
  i=2: O[1]=-1, O[2]=+1, -1+(+1)=0, -1!=+1 --> 震荡+1
  i=3: O[2]=+1, O[3]=-1, +1+(-1)=0, +1!=-1 --> 震荡+1

震荡次数 = 3
N = 5
震荡频率 = 3 / (5-2) = 3/3 = 1.0
```

**输出结构：**

```cpp
struct HeadingFeature {
    int track_id;
    double mean_heading;       // 航向角均值（弧度）
    double std_heading;        // 航向角标准差
    double oscillation_freq;   // 航向震荡频率
};
```

---

**核心算法：特征合并**

**目标**：将三类特征（SpeedFeature, RadialFeature, HeadingFeature）合并为一个完整的特征表。

**Step 1: 收集所有唯一的 track_id**

从三类特征中收集所有出现过的 `track_id`。

**Step 2: 对每个 track_id 构建 MergedFeature**

```cpp
对每个 track_id:
  在 SpeedFeature 中查找该 track_id 的记录
  在 RadialFeature 中查找该 track_id 的记录
  在 HeadingFeature 中查找该 track_id 的记录

  构建 MergedFeature:
    - 如果某类特征存在，复制对应字段
    - 如果某类特征不存在，对应字段设为 NaN
```

**示例：**

```
SpeedFeature:   [{tid=0, mean_speed=10.0}, {tid=1, mean_speed=5.0}]
RadialFeature:  [{tid=0, max_accel=2.0}, {tid=2, max_accel=1.0}]
HeadingFeature: [{tid=1, mean_heading=0.5}]

track_id 集合: {0, 1, 2}

MergedFeature[0] (tid=0):
  从 SpeedFeature: mean_speed=10.0
  从 RadialFeature: max_accel=2.0
  从 HeadingFeature: 不存在 --> mean_heading=NaN

MergedFeature[1] (tid=1):
  从 SpeedFeature: mean_speed=5.0
  从 RadialFeature: 不存在 --> max_accel=NaN
  从 HeadingFeature: mean_heading=0.5

MergedFeature[2] (tid=2):
  从 SpeedFeature: 不存在 --> mean_speed=NaN
  从 RadialFeature: max_accel=1.0
  从 HeadingFeature: 不存在 --> mean_heading=NaN
```

**Step 3: 去除 NaN（dropna）**

遍历所有 `MergedFeature`，如果**任何字段**为 NaN，则剔除该记录。

上例中：
- MergedFeature[0]: 有 NaN（mean_heading）--> 剔除
- MergedFeature[1]: 有 NaN（max_accel）--> 剔除
- MergedFeature[2]: 有 NaN（mean_speed, mean_heading）--> 剔除

最终结果为空！这说明三类特征的 track_id 不完全匹配。

**实际场景中**，如果数据完整，三类特征应该覆盖相同的 track_id，合并后不会有 NaN。

**输出结构：**

```cpp
struct MergedFeature {
    int track_id;
    double mean_heading;       // 航向角均值
    double std_heading;        // 航向角标准差
    double heading_osc_freq;   // 航向震荡频率
    double mean_speed;         // 平均速度
    double std_speed;          // 速度标准差
    double speed_osc_freq;     // 速度震荡频率
    double max_speed;          // 最大速度
    double min_speed;          // 最小速度
    int label;
    double max_accel;          // 最大径向加速度
    double max_rate;           // 最大径向速率
    double min_rate;           // 最小径向速率
    double abs_max_accel;      // abs最大径向加速度
    double abs_max_rate;       // abs最大径向速率
    double abs_min_rate;       // abs最小径向速率
    double mean_rcs;           // 平均RCS
    double mean_height;        // 相对高度均值
    double std_height;         // 相对高度标准差
};
```

---

**输入输出文件说明**

**输入文件：**

| 文件名 | 路径 | 说明 |
|--------|------|------|
| `middle5_cleaned.csv` | `data/tmp/middle5_cleaned.csv`（简化版）或 `data_full/tmp/middle5_cleaned.csv`（完整版） | 模块C生成的速度记录 |
| `middle2_cleaned.csv` | `data/tmp/middle2_cleaned.csv`（简化版）或 `data_full/tmp/middle2_cleaned.csv`（完整版） | 模块C生成的清洗后雷达数据 |

**输出文件：**

| 文件名 | 路径 | 说明 |
|--------|------|------|
| `feature1.csv` | `data/tmp/feature1.csv`（简化版）或 `data_full/tmp/feature1.csv`（完整版） | 速度统计特征 |
| `feature2.csv` | `data/tmp/feature2.csv`（简化版）或 `data_full/tmp/feature2.csv`（完整版） | 径向动力学特征 |
| `feature3.csv` | `data/tmp/feature3.csv`（简化版）或 `data_full/tmp/feature3.csv`（完整版） | 航向角统计特征 |
| `merged_cleaned.csv` | `data/output/merged_cleaned.csv`（简化版）或 `data_full/output/merged_cleaned.csv`（完整版） | 合并并去除NaN后的最终特征 |

**输入文件1格式**（`middle5_cleaned.csv`，速度记录）：

```csv
标签,点1索引,点2索引,速度(m/s),time,航迹ID
0,0,1,15.2345678901234,0.000999999996565748,0
0,1,2,14.8765432109876,1.98400000000384,0
0,2,3,15.1234567890123,0.99599999999418,0
...
```

**输入文件2格式**（`middle2_cleaned.csv`，清洗后的雷达数据）：

```csv
目标方位角(°),目标斜距(m),相对高度(m),径向速率(m/s),RCS,测量时间(s),航迹序号
210.811157226562,9274,81,-18,0.004,0,0
211.09130859375,9346,85,-18,0.004,0.000999999996565748,
...
```

**输出文件1格式**（`feature1.csv`，速度统计特征）：

```csv
航迹ID,平均速度,速度标准差,速度震荡频率,最大速度,最小速度,label
0,15.0123456789012,0.234567890123456,0.333333333333333,15.2345678901234,14.8765432109876,0
1,3.45678901234567,0.123456789012345,0.5,3.56789012345678,3.34567890123456,1
2,8.90123456789012,0.567890123456789,0.25,9.01234567890123,8.78901234567890,0
...
```

**输出文件2格式**（`feature2.csv`，径向动力学特征）：

```csv
航迹ID,最大径向加速度,最大径向速率,最小径向速率,abs最大径向加速度,abs最大径向速率,abs最小径向速率,平均RCS,label,相对高度均值,相对高度标准差
0,2.5,18.0,-15.0,2.5,18.0,15.0,0.025,0,80.5,2.345
1,0.8,13.0,-12.0,0.8,13.0,12.0,0.015,1,55.0,1.234
2,1.5,20.0,-18.0,1.5,20.0,18.0,0.030,0,75.0,3.456
...
```

**输出文件3格式**（`feature3.csv`，航向角统计特征）：

```csv
航迹ID,航向角均值,航向角标准差,heading_oscillation_frequency
0,0.349065850398866,0.0202020202020202,0.0
1,2.79252680319093,0.0151515151515152,0.333333333333333
2,1.0471975511966,0.0101010101010101,0.5
...
```

**输出文件4格式**（`merged_cleaned.csv`，最终合并特征）：

```csv
航迹ID,航向角均值,航向角标准差,heading_oscillation_frequency,平均速度,速度标准差,速度震荡频率,最大速度,最小速度,label,最大径向加速度,最大径向速率,最小径向速率,abs最大径向加速度,abs最大径向速率,abs最小径向速率,平均RCS,相对高度均值,相对高度标准差
0,0.349065850398866,0.0202020202020202,0.0,15.0123456789012,0.234567890123456,0.333333333333333,15.2345678901234,14.8765432109876,0,2.5,18.0,-15.0,2.5,18.0,15.0,0.025,80.5,2.345
1,2.79252680319093,0.0151515151515152,0.333333333333333,3.45678901234567,0.123456789012345,0.5,3.56789012345678,3.34567890123456,1,0.8,13.0,-12.0,0.8,13.0,12.0,0.015,55.0,1.234
...
```

**格式说明（最终合并特征）：**
- 第1列：航迹ID，整数
- 第2列：航向角均值（弧度），浮点数
- 第3列：航向角标准差，浮点数
- 第4列：航向震荡频率，浮点数
- 第5列：平均速度（米/秒），浮点数
- 第6列：速度标准差，浮点数
- 第7列：速度震荡频率，浮点数
- 第8列：最大速度，浮点数
- 第9列：最小速度，浮点数
- 第10列：label（类别标签），整数
- 第11列：最大径向加速度，浮点数
- 第12列：最大径向速率，浮点数
- 第13列：最小径向速率，浮点数
- 第14列：abs最大径向加速度，浮点数
- 第15列：abs最大径向速率，浮点数
- 第16列：abs最小径向速率，浮点数
- 第17列：平均RCS，浮点数
- 第18列：相对高度均值，浮点数
- 第19列：相对高度标准差，浮点数

共 **19 列**，无任何空值（已去除NaN）。

**接口**：
```cpp
class FeatureExtractor {
public:
    std::vector<SpeedFeature> extract_speed_features(
        const std::vector<SpeedRecord>& input);
    std::vector<RadialFeature> extract_radial_features(
        const std::vector<RadarPoint>& input);
    std::vector<HeadingFeature> extract_heading_features(
        const std::vector<RadarPoint>& input);
    static std::vector<MergedFeature> merge_features(
        const std::vector<SpeedFeature>& speed_features,
        const std::vector<RadialFeature>& radial_features,
        const std::vector<HeadingFeature>& heading_features);
    static std::vector<MergedFeature> drop_na(
        const std::vector<MergedFeature>& input);
};
```

---

### 3.5 模块E：主控流水线（PipelineOrchestrator）

**负责人**：开发者 E

**类定义**：`PipelineOrchestrator`（`pipeline_orchestrator.h` / `pipeline_orchestrator.cpp`）

**功能**：编排整个数据处理流程，将前四个模块串联起来，形成完整的数据处理流水线。

**三阶段编排**：

| 阶段 | 调用函数 | 输入文件 | 输出文件 |
|------|---------|---------|---------|
| Stage 1 | `DataAugmentor::augment()` + `StatisticsMerger::merge()` + `DataPreprocessor::process()` + `DataPreprocessor::compute_speed_records()` | `data/input/label0.csv`, `data/input/label1.csv` | `data/tmp/middle2_cleaned.csv`, `data/tmp/middle5_cleaned.csv` |
| Stage 2 | `FeatureExtractor::extract_*_features()` | `data/tmp/middle2_cleaned.csv`, `data/tmp/middle5_cleaned.csv` | `data/tmp/feature1.csv`, `data/tmp/feature2.csv`, `data/tmp/feature3.csv` |
| Stage 3 | `FeatureExtractor::merge_features()` + `FeatureExtractor::drop_na()` | `data/tmp/feature*.csv` | `data/output/merged_cleaned.csv` |

**输入输出文件说明**

**输入文件：**

| 文件名 | 路径 | 说明 |
|--------|------|------|
| `label0.csv` | `data/input/label0.csv`（简化版）或 `data_full/input/label0.csv`（完整版） | 类别0原始数据 |
| `label1.csv` | `data/input/label1.csv`（简化版）或 `data_full/input/label1.csv`（完整版） | 类别1原始数据 |

**输出文件：**

| 文件名 | 路径 | 说明 |
|--------|------|------|
| `merged_cleaned.csv` | `data/output/merged_cleaned.csv`（简化版）或 `data_full/output/merged_cleaned.csv`（完整版） | 最终合并特征（19列） |

**中间文件（模块E内部生成和使用）：**

| 文件名 | 路径 | 生成阶段 | 说明 |
|--------|------|---------|------|
| `aug_label*.csv` | `data/output/aug_*.csv` 或 `data_full/output/aug_*.csv` | Stage 1 | 数据增强结果（21个文件） |
| `train_data_all.csv` | `data/tmp/train_data_all.csv` 或 `data_full/tmp/train_data_all.csv` | Stage 1 | 合并后的训练数据 |
| `middle2_cleaned.csv` | `data/tmp/middle2_cleaned.csv` 或 `data_full/tmp/middle2_cleaned.csv` | Stage 1 | 清洗后的雷达数据 |
| `middle5_cleaned.csv` | `data/tmp/middle5_cleaned.csv` 或 `data_full/tmp/middle5_cleaned.csv` | Stage 1 | 速度记录 |
| `feature1.csv` | `data/tmp/feature1.csv` 或 `data_full/tmp/feature1.csv` | Stage 2 | 速度统计特征 |
| `feature2.csv` | `data/tmp/feature2.csv` 或 `data_full/tmp/feature2.csv` | Stage 2 | 径向动力学特征 |
| `feature3.csv` | `data/tmp/feature3.csv` 或 `data_full/tmp/feature3.csv` | Stage 2 | 航向角统计特征 |

**接口**：
```cpp
class PipelineOrchestrator {
public:
    bool run(const std::vector<std::string>& input_files,
             const std::string& output_file);
    int get_num_tracks() const;
    int get_num_augmented() const;
    int get_num_cleaned() const;
    int get_num_speed_records() const;
    int get_num_features() const;
    std::string get_error_msg() const;
};
```

---

## 4. 公共基础设施

### 4.1 CSV 读写工具（已实现）

以下函数已实现，定义在 `radar_types.h` / `radar_types.cpp` 中：

```cpp
// 读取雷达数据CSV
bool read_radar_csv(const std::string& filepath,
                    std::vector<RadarPoint>& points,
                    std::string& error_msg);

// 写入雷达数据CSV
bool write_radar_csv(const std::string& filepath,
                     const std::vector<RadarPoint>& points,
                     std::string& error_msg);

// 读取速度记录CSV
bool read_speed_csv(const std::string& filepath,
                    std::vector<SpeedRecord>& records,
                    std::string& error_msg);

// 写入速度记录CSV
bool write_speed_csv(const std::string& filepath,
                     const std::vector<SpeedRecord>& records,
                     std::string& error_msg);

// 写入各类特征CSV
bool write_speed_feature_csv(const std::string& filepath,
                             const std::vector<SpeedFeature>& features,
                             std::string& error_msg);
bool write_radial_feature_csv(const std::string& filepath,
                              const std::vector<RadialFeature>& features,
                              std::string& error_msg);
bool write_heading_feature_csv(const std::string& filepath,
                               const std::vector<HeadingFeature>& features,
                               std::string& error_msg);
bool write_merged_feature_csv(const std::string& filepath,
                              const std::vector<MergedFeature>& features,
                              std::string& error_msg);
```

**注意事项**：
- 兼容 Windows 换行符（`\r\n`）
- 缺失列自动补空字符串
- `read_radar_csv` 会自动处理航迹序号：空字符串继承上一个有效值

---

## 5. 项目编译与运行

本项目使用CMake来管理多源代码文件的编译，使用VS Code作为IDE进行开发。

请搜索VS Code的CMake插件的使用方法。

### 5.1 环境要求

建议使用的是课程组提供的VS Code开发环境（开发环境使用手册： https://365.kdocs.cn/l/cujPsRwDnuE6），已经具备了编译环境，无需额外配置。

- CMake 3.10 或更高版本
- 支持 C++11 的编译器（g++ 4.8+、clang++ 3.3+、MSVC 2015+）

### 5.2 编译步骤

关于CMake的使用方法，可以自行搜索参考资料。项目已经提供了编写完整的CMakeLists.txt文件。

#### 5.2.1 在VS Code中的方法

请自行搜索VS Code中的CMake插件的使用方法。

#### 5.2.2 命令行运行

```bash
# 在项目根目录下运行下面的命令，创建build目录
cmake -B build -S .
# 进行代码编译
cmake --build build 
```

### 5.3 运行程序

编译完成后，`build/` 目录下会生成以下可执行文件：

| 可执行文件 | 功能 | 运行命令 |
|-----------|------|---------|
| `augment` | 数据增强 | `./augment` |
| `statistics` | 统计合并 | `./statistics` |
| `preprocess` | 数据清洗与速度计算 | `./preprocess` |
| `extract` | 特征提取 | `./extract` |
| `pipeline` | 完整流水线 | `./pipeline` |

### 5.4 完整运行流程

```bash
# 方式1：分步运行（适合调试各模块）
cd build
./augment
./statistics
./preprocess
./extract

# 方式2：直接运行完整流水线
./pipeline
```

### 5.5 验证结果

运行完成后，检查 `data/output/merged_cleaned.csv` 是否生成，并与参考输出对比。

---

## 6. 测试数据

### 6.1 输入数据（`data/input/`）

- `label0.csv`：类别 0 原始数据（约 11456 条航迹）
- `label1.csv`：类别 1 原始数据（约 180 条航迹）

为便于快速测试和理解，默认提供的 `data/` 目录下是简化版数据：

- `data/input/label0.csv`：类别 0 前 2 条航迹（197 行）
- `data/input/label1.csv`：类别 1 前 2 条航迹（142 行）
- `data/tmp/train_data_all.csv`：合并后的小规模训练数据（339 行）
- `data/output/merged_cleaned.csv`：参考输出格式示例（4 行）

**使用简化数据的好处**：
- 编译运行速度快，便于调试
- 数据量小，便于手动验证计算结果
- 有助于理解各模块的输入输出格式

### 6.2 完整数据（`data_full/`）

用于最终测试和性能验证的完整数据集。【完整数据集内容过大，仓库中默认不提供，可联系助教获取】。

- `data_full/input/label0.csv`：类别 0 原始数据（约 11456 条航迹）
- `data_full/input/label1.csv`：类别 1 原始数据（约 180 条航迹）
- `data_full/tmp/`：中等规模参考中间结果
- `data_full/output/merged_cleaned.csv`：完整参考输出

**测试建议**：
1. 先用 `data/` 目录下的简化数据测试各模块
2. 确认结果正确后，将代码中的路径从 `data/` 改为 `data_full/`，使用完整数据进行最终测试

---

## 7. 分工协作指南

### 7.1 模块分工建议

可以根据小组人数，确定每位同学负责哪些模块。建议由小组组长完成“主控流水线”的模块。

| 开发者 | 负责模块 | 核心任务 | 需要实现的文件 |
|--------|---------|---------|--------------|
| A | 数据增强 | 实现滑动窗口算法 | `data_augment.h`, `data_augment.cpp`, `main_augment.cpp` |
| B | 统计合并 | 实现多文件合并和统计报告 | `statistics_merge.h`, `statistics_merge.cpp`, `main_statistics.cpp` |
| C | 数据清洗与速度计算 | 实现数据清洗、3D 速度计算 | `data_preprocessor.h`, `data_preprocessor.cpp`, `main_preprocess.cpp` |
| D | 特征提取 | 实现三类统计特征的计算和合并 | `feature_extractor.h`, `feature_extractor.cpp`, `main_extract.cpp` |
| E | 主控流水线 | 编排整个流程，集成测试 | `pipeline_orchestrator.h`, `pipeline_orchestrator.cpp`, `main_pipeline.cpp` |

### 7.2 协作流程

1. **阶段1**：所有人共同理解项目需求和数据格式
2. **阶段2**：各自实现分配模块的核心算法（类成员函数）
3. **阶段3**：使用 `data` 下的测试数据进行单元测试
4. **阶段4**：开发者 E 集成所有模块，运行完整流水线
5. **阶段5**：对比输出结果，调试修复问题

---

## 8. Git 与 Gitee 协作流程

### 8.1 Git 基础命令

```bash
# 克隆仓库
git clone <仓库地址>

# 查看状态
git status

# 添加文件到暂存区
git add <文件名>
git add .                    # 添加所有修改

# 提交更改
git commit -m "描述信息"

# 推送到远程
git push origin main

# 拉取最新代码
git pull origin main

# 查看提交历史
git log --oneline

# 创建分支
git checkout -b feature-xxx

# 切换分支
git checkout main

# 合并分支
git merge feature-xxx
```

### 8.2 Gitee 协作流程

[Gitee](https://gitee.com) 是国内常用的代码托管平台，提供与 GitHub 类似的功能。

**创建仓库**：
1. 登录 Gitee，点击右上角 "+" 创建新仓库
2. 填写仓库名称和描述，选择 "开源" 或 "私有"
3. 创建完成后，复制仓库地址（HTTPS 或 SSH）


**推荐的提交信息规范**（Conventional Commits）：

```
feat: 新增功能
fix: 修复 bug
docs: 文档更新
style: 代码格式调整（不影响功能）
refactor: 代码重构
test: 添加测试
chore: 构建过程或辅助工具的变动
```

### 8.3 解决合并冲突

当多人修改同一文件时，可能产生冲突：

```bash
# 拉取代码时提示冲突
git pull origin main

# 查看冲突文件
git status

# 手动编辑冲突文件，保留需要的代码
# 冲突标记格式：
# <<<<<<< HEAD
# 你的代码
# =======
# 别人的代码
# >>>>>>> branch-name

# 解决后添加并提交
git add <冲突文件>
git commit -m "fix: resolve merge conflict"
git push origin main
```

---

## 9. 文件目录结构

```
微型无人机识别雷达数据处理/
├── CMakeLists.txt              # CMake 构建配置
├── README.md                   # 本文件
├── src/                        # 源代码
│   ├── radar_types.h           # 公共数据结构（已提供）
│   ├── radar_types.cpp         # CSV读写实现（已提供）
│   ├── data_augment.h          # 模块A：数据增强类定义
│   ├── data_augment.cpp        # 模块A：数据增强实现（学生完成）
│   ├── statistics_merge.h      # 模块B：统计合并类定义
│   ├── statistics_merge.cpp    # 模块B：统计合并实现（学生完成）
│   ├── data_preprocessor.h     # 模块C：数据清洗类定义
│   ├── data_preprocessor.cpp   # 模块C：数据清洗实现（学生完成）
│   ├── feature_extractor.h     # 模块D：特征提取类定义
│   ├── feature_extractor.cpp   # 模块D：特征提取实现（学生完成）
│   ├── pipeline_orchestrator.h # 模块E：流水线类定义
│   ├── pipeline_orchestrator.cpp # 模块E：流水线实现（学生完成）
│   ├── main_augment.cpp        # 模块A主程序（学生完成）
│   ├── main_statistics.cpp     # 模块B主程序（学生完成）
│   ├── main_preprocess.cpp     # 模块C主程序（学生完成）
│   ├── main_extract.cpp        # 模块D主程序（学生完成）
│   └── main_pipeline.cpp       # 模块E主程序（学生完成）
├── data/                       # 简化数据（默认使用，用于开发和调试）
│   ├── input/                  # 输入数据（约2-4条航迹）
│   │   ├── label0.csv
│   │   └── label1.csv
│   ├── tmp/                    # 中间处理文件
│   │   └── train_data_all.csv
│   └── output/                 # 最终输出
│       └── merged_cleaned.csv
├── data_full/                  # 完整数据（用于最终测试和验证）
│   ├── input/                  # 完整输入数据（约10000+条航迹）
│   │   ├── label0.csv
│   │   └── label1.csv
│   ├── tmp/                    # 参考中间结果
│   └── output/                 # 参考最终结果
│       └── merged_cleaned.csv
└── docs/                       # 文档目录
```

---

## 10. 评分标准

评分过程会考虑以下因素：

1. 代码正确性：程序能正确编译运行，输出结果与参考结果是否一致或接近）
2. 代码质量：代码结构清晰，注释充分，命名规范，合理使用类）
3. Git协作记录：
    - 是否能使用Gitee平台完成开发
    - 在仓库的Git提交历史记录中是否有参与协作的记录
    - Git提交记录中实际完成的代码工作量
4. 课程设计报告与现场答辩呈现

---

## 11. 常见问题

**Q1: 为什么使用结构体数组而不是 `vector<vector<string>>`？**

A: 结构体数组类型安全、访问高效，且更符合面向对象的设计思想。每个字段有明确的类型和含义。

**Q2: 如何处理 NaN 和 inf？**

A: 使用 `val != val` 判断 NaN，使用 `std::numeric_limits<double>::infinity()` 判断 inf。

**Q3: 为什么速度计算使用 3D 欧氏距离而不是直接用径向速率？**

A: 径向速率只是速度在雷达视线方向的分量，3D 速度能更准确地反映目标的真实运动状态。

**Q4: 特征提取中的 "加权" 是什么意思？**

A: 以时间间隔作为权重，因为不同采样间隔的测量值可信度不同。

**Q6: 如何按 track_id 分组？**

A: 遍历数据序列，当 `track_id` 发生变化时（或遇到 `track_id != -1` 时），开始一个新的组。例如：

```cpp
std::vector<std::vector<RadarPoint>> groups;
std::vector<RadarPoint> current_group;
int current_tid = -999;

for (const auto& p : points) {
    if (p.track_id != -1 && p.track_id != current_tid) {
        // 新的航迹开始
        if (!current_group.empty()) {
            groups.push_back(current_group);
        }
        current_group.clear();
        current_tid = p.track_id;
    }
    current_group.push_back(p);
}
if (!current_group.empty()) {
    groups.push_back(current_group);
}
```

**Q7: 为什么航迹中大部分点的 track_id 是 -1？**

A: 在原始 CSV 中，同一条航迹只有第一个点有航迹序号，其余为空。`read_radar_csv` 会将第一个点的航迹号赋给整组，但为了标识航迹起点，第一个点保留原始 `track_id`，其余点设为 -1。这样可以通过 `track_id != -1` 判断航迹起点。

**Q8: 加权平均和普通平均有什么区别？**

A: 普通平均将所有值一视同仁，加权平均考虑每个值的权重（这里是时间间隔）。如果时间间隔不同，加权平均更准确。例如：
- 速度 10m/s 持续 1 秒，速度 20m/s 持续 9 秒
- 普通平均: (10 + 20) / 2 = 15 m/s
- 加权平均: (10*1 + 20*9) / 10 = 19 m/s
- 加权平均更接近真实情况，因为高速持续了更长时间。

---

## 12. 参考资料

- [CMake 官方文档](https://cmake.org/documentation/)
- [C++ Reference](https://en.cppreference.com/)
- [Gitee 帮助中心](https://gitee.com/help)
- [Conventional Commits 规范](https://www.conventionalcommits.org/)
