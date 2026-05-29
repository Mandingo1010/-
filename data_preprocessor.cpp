// =============================================================================
// 模块C：数据清洗与速度计算（DataPreprocessor）
// 功能：对合并后的训练数据进行清洗（去除异常值），并计算相邻点之间的3D速度
// =============================================================================

#include "data_preprocessor.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace {

const double PI = 3.14159265358979323846;

// 判断 NaN：NaN 的特性是自己不等于自己
bool is_nan(double val) { return val != val; }

// 判断 inf：与正负无穷大比较
bool is_inf(double val) {
  return val == std::numeric_limits<double>::infinity() ||
         val == -std::numeric_limits<double>::infinity();
}

// 检查点的 radial_rate 或 time 是否为 NaN/inf
bool is_point_invalid(double radial_rate, double time) {
  return is_nan(radial_rate) || is_inf(radial_rate) || is_nan(time) ||
         is_inf(time);
}

} // anonymous namespace

// =============================================================================
// 构造/析构
// =============================================================================

DataPreprocessor::DataPreprocessor()
    : input_count_(0), output_count_(0), removed_count_(0) {}

DataPreprocessor::~DataPreprocessor() {}

// =============================================================================
// process() — 数据清洗主流程
// =============================================================================
// Step 1: 按 track_id 分组（track_id 变化时开启新航迹）
// Step 2: 每组按 time 字段升序排序
// Step 3: 过滤 radial_rate 或 time 为 NaN/inf 的异常点
// Step 4: 标签前移——被剔除的首点 track_id 传递给下一个有效点
// =============================================================================

std::vector<RadarPoint>
DataPreprocessor::process(const std::vector<RadarPoint> &input) {
  input_count_ = static_cast<int>(input.size());
  output_count_ = 0;
  removed_count_ = 0;

  if (input.empty()) {
    return {};
  }

  // ---------- Step 1: 按航迹分组 ----------
  // track_id 相同的连续点属于同一条航迹。
  // 遇到新的 track_id（≠ -1）且当前组非空时，保存当前组并开始新组。
  std::vector<std::vector<RadarPoint>> tracks;
  std::vector<RadarPoint> current_track;

  for (const auto &point : input) {
    if (point.track_id != -1 && !current_track.empty()) {
      tracks.push_back(current_track);
      current_track.clear();
    }
    current_track.push_back(point);
  }
// 最后一组也要加入
  if (!current_track.empty()) {
    tracks.push_back(current_track);
  }

  std::vector<RadarPoint> cleaned_data;

  // ---------- Step 2 & 3 & 4: 逐组处理 ----------
  for (const auto &track : tracks) {

    // Step 2: 按 time 升序排序（防御性排序，确保时间单调递增）
    std::vector<RadarPoint> sorted_track = track;
    std::sort(sorted_track.begin(), sorted_track.end(),
              [](const RadarPoint &a, const RadarPoint &b) {
                return a.time < b.time;
              });

    // Step 3 & 4: 遍历排序后的点，过滤异常值并处理标签前移
    // pending_track_id: 暂存被剔除首点的 track_id，待转发给后续有效点
    int pending_track_id = -1;

    for (const auto &point : sorted_track) {

      // Step 3: 异常值过滤 —— 检查 radial_rate 和 time
      if (is_point_invalid(point.radial_rate, point.time)) {
        removed_count_++;
        // 如果被剔除的点携带有效 track_id（即它是航迹首点），暂存起来
        if (point.track_id != -1) {
          pending_track_id = point.track_id;
        }
        continue; // 跳过该点
      }

      // 该点有效，保留
      RadarPoint cleaned_point = point;

      // Step 4: 标签前移 —— 将暂存的 track_id 赋给下一个 track_id==-1 的有效点
      // 仅当 cleaned_point.track_id == -1 时才赋值，避免覆盖已有的有效 track_id
      if (pending_track_id != -1 && cleaned_point.track_id == -1) {
        cleaned_point.track_id = pending_track_id;
        pending_track_id = -1;
      }

      cleaned_data.push_back(cleaned_point);
      output_count_++;
    }
  }

  return cleaned_data;
}

// =============================================================================
// calculate_speed() — 两点间 3D 速度计算
// =============================================================================
// 雷达测量的是球坐标（方位角、斜距、高度），需要转换为直角坐标 (x, y, z)
// 才能计算真实的 3D 速度。
//
// 转换步骤：
//   1. 方位角度 → 弧度: az_rad = azimuth * PI / 180
//   2. 水平投影距离:     r = sqrt(range² - height²)   （勾股定理）
//   3. 直角坐标:         x = r * sin(az_rad)         （东西分量）
//                       y = r * cos(az_rad)         （南北分量）
//                       z = height                   （垂直分量）
//   4. 欧氏距离:         d = sqrt(dx² + dy² + dz²)
//   5. 速度:             speed = d / dt
// =============================================================================

double DataPreprocessor::calculate_speed(const RadarPoint &p1,
                                         const RadarPoint &p2) {
  // 度 → 弧度
  double az1_rad = p1.azimuth * PI / 180.0;
  double az2_rad = p2.azimuth * PI / 180.0;

  // 水平投影距离 r = sqrt(range² - height²)
  double r1_sq = p1.range * p1.range - p1.height * p1.height;
  double r2_sq = p2.range * p2.range - p2.height * p2.height;

  // 防御浮点舍入误差导致的微小负数
  if (r1_sq < 0.0) {
    r1_sq = 0.0;
  }
  if (r2_sq < 0.0) {
    r2_sq = 0.0;
  }double r1 = std::sqrt(r1_sq);
  double r2 = std::sqrt(r2_sq);

  // 球坐标 → 直角坐标
  // 方位角从正北顺时针旋转，sin 给出东西分量，cos 给出南北分量
  double x1 = r1 * std::sin(az1_rad);
  double y1 = r1 * std::cos(az1_rad);
  double z1 = p1.height;

  double x2 = r2 * std::sin(az2_rad);
  double y2 = r2 * std::cos(az2_rad);
  double z2 = p2.height;

  // 两点间欧氏距离
  double dx = x2 - x1;
  double dy = y2 - y1;
  double dz = z2 - z1;
  double distance = std::sqrt(dx * dx + dy * dy + dz * dz);

  // 时间差
  double dt = p2.time - p1.time;

  // dt == 0 时无法计算速度，返回 inf（后续会被过滤）
  if (dt == 0.0) {
    return std::numeric_limits<double>::infinity();
  }

  return distance / dt;
}

// =============================================================================
// compute_speed_records() — 批量计算速度记录
// =============================================================================
// 遍历清洗后的相邻点对 (P[i], P[i+1])，计算每对的 3D 速度。
//
// 跨航迹检测：
//   若 p1.track_id == -1 且 p2.track_id != -1，说明 p2 是新航迹起点，
//   两点分属不同航迹，跳过不计算。
//
// =============================================================================

std::vector<SpeedRecord>
DataPreprocessor::compute_speed_records(const std::vector<RadarPoint> &input) {
  std::vector<SpeedRecord> records;

  if (input.size() < 2) {
    return records;
  }

  int current_track_id = -1;

  for (size_t i = 0; i < input.size() - 1; ++i) {
    const RadarPoint &p1 = input[i];
    const RadarPoint &p2 = input[i + 1];

    // 遇到有效 track_id 时更新当前航迹
    if (p1.track_id != -1) {
      current_track_id = p1.track_id;
    }

    // 跳过跨航迹边界点对：
    // p1 无 track_id（上一条航迹尾部）且 p2 有 track_id（新航迹首点）
    if (p1.track_id == -1 && p2.track_id != -1) {
      continue;
    }

    // 计算 3D 速度与时间差
    double speed = calculate_speed(p1, p2);
    double time_diff = p2.time - p1.time;

    // 剔除异常记录：
    // - speed 或 time_diff 为 NaN/inf
    // - time_diff <= 0（时间倒退或跨滑动窗口边界）
    if (is_nan(speed) || is_inf(speed) || is_nan(time_diff) ||
        is_inf(time_diff)) {
      continue;
    }

    // 构建速度记录
    SpeedRecord record;
    record.label = p1.label; // 类别标签（目标类型）
    record.point1_idx = static_cast<int>(i);
    record.point2_idx = static_cast<int>(i + 1);
    record.speed = speed;               // 3D 速度 (m/s)
    record.time_diff = time_diff;       // 时间差 (s)
    record.track_id = current_track_id; // 航迹 ID

    records.push_back(record);
  }

  return records;
}

// =============================================================================
// 统计接口
// =============================================================================

int DataPreprocessor::get_input_count() const { return input_count_; }

int DataPreprocessor::get_output_count() const { return output_count_; }

int DataPreprocessor::get_removed_count() const { return removed_count_; }