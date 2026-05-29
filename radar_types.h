#ifndef RADAR_TYPES_H
#define RADAR_TYPES_H

#include <string>
#include <vector>

// ============================================================================
// 基础数据结构
// ============================================================================

// 原始雷达测量点
struct RadarPoint {
  double azimuth;     // 目标方位角 (°)
  double range;       // 目标斜距 (m)
  double height;      // 相对高度 (m)
  double radial_rate; // 径向速率 (m/s)
  double rcs;         // 雷达散射截面积
  double time;        // 测量时间 (s)
  int track_id;       // 航迹序号
  int label;          // 类别标签（目标类型）
};

// 速度记录（相邻两点计算得到）
struct SpeedRecord {
  int label;        // 类别标签
  int point1_idx;   // 点1索引
  int point2_idx;   // 点2索引
  double speed;     // 3D速度 (m/s)
  double time_diff; // 时间差 (s)
  int track_id;     // 航迹ID
};

// 速度统计特征（feature1_calc 输出）
struct SpeedFeature {
  int track_id;            // 航迹ID
  double mean_speed;       // 加权平均速度
  double std_speed;        // 加权标准差
  double oscillation_freq; // 速度震荡频率
  double max_speed;        // 最大速度
  double min_speed;        // 最小速度
  int label;               // 类别标签
};

// 径向动力学特征（feature2_calc 输出）
struct RadialFeature {
  int track_id;         // 航迹ID
  double max_accel;     // 最大径向加速度
  double max_rate;      // 最大径向速率
  double min_rate;      // 最小径向速率
  double abs_max_accel; // abs最大径向加速度
  double abs_max_rate;  // abs最大径向速率
  double abs_min_rate;  // abs最小径向速率
  double mean_rcs;      // 平均RCS
  int label;            // 类别标签
  double mean_height;   // 相对高度均值
  double std_height;    // 相对高度标准差
};

// 航向角统计特征（feature3_calc 输出）
struct HeadingFeature {
  int track_id;            // 航迹ID
  double mean_heading;     // 航向角均值 (弧度)
  double std_heading;      // 航向角标准差
  double oscillation_freq; // 航向震荡频率
};

// 合并后的特征（最终输出）
struct MergedFeature {
  int track_id;            // 航迹ID
  double mean_heading;     // 航向角均值
  double std_heading;      // 航向角标准差
  double heading_osc_freq; // 航向震荡频率
  double mean_speed;       // 平均速度
  double std_speed;        // 速度标准差
  double speed_osc_freq;   // 速度震荡频率
  double max_speed;        // 最大速度
  double min_speed;        // 最小速度
  int label;               // 类别标签
  double max_accel;        // 最大径向加速度
  double max_rate;         // 最大径向速率
  double min_rate;         // 最小径向速率
  double abs_max_accel;    // abs最大径向加速度
  double abs_max_rate;     // abs最大径向速率
  double abs_min_rate;     // abs最小径向速率
  double mean_rcs;         // 平均RCS
  double mean_height;      // 相对高度均值
  double std_height;       // 相对高度标准差
};

// ============================================================================
// CSV 工具函数（已实现，可直接使用）
// ============================================================================

// 从CSV文件读取雷达数据
// 输入格式:
// 目标方位角(°),目标斜距(m),相对高度(m),径向速率(m/s),RCS,测量时间(s),航迹序号
// 返回: 成功返回true，失败返回false
bool read_radar_csv(const std::string &filepath,
                    std::vector<RadarPoint> &points, std::string &error_msg);

// 将雷达数据写入CSV文件
// 输出格式:
// 目标方位角(°),目标斜距(m),相对高度(m),径向速率(m/s),RCS,测量时间(s),航迹序号
bool write_radar_csv(const std::string &filepath,
                     const std::vector<RadarPoint> &points,
                     std::string &error_msg);

// 从CSV文件读取速度记录
// 输入格式: 标签,点1索引,点2索引,速度(m/s),time,航迹ID
bool read_speed_csv(const std::string &filepath,
                    std::vector<SpeedRecord> &records, std::string &error_msg);

// 将速度记录写入CSV文件
// 输出格式: 标签,点1索引,点2索引,速度(m/s),time,航迹ID
bool write_speed_csv(const std::string &filepath,
                     const std::vector<SpeedRecord> &records,
                     std::string &error_msg);

// 将SpeedFeature写入CSV
// 输出格式: 航迹ID,平均速度,速度标准差,速度震荡频率,最大速度,最小速度,label
bool write_speed_feature_csv(const std::string &filepath,
                             const std::vector<SpeedFeature> &features,
                             std::string &error_msg);

// 将RadialFeature写入CSV
// 输出格式: 航迹ID,最大径向加速度,最大径向速率,最小径向速率,abs最大径向加速度,
//           abs最大径向速率,abs最小径向速率,平均RCS,label,相对高度均值,相对高度标准差
bool write_radial_feature_csv(const std::string &filepath,
                              const std::vector<RadialFeature> &features,
                              std::string &error_msg);

// 将HeadingFeature写入CSV
// 输出格式: 航迹ID,航向角均值,航向角标准差,heading_oscillation_frequency
bool write_heading_feature_csv(const std::string &filepath,
                               const std::vector<HeadingFeature> &features,
                               std::string &error_msg);

// 将MergedFeature写入CSV
// 输出格式:
// 航迹ID,航向角均值,航向角标准差,heading_oscillation_frequency,平均速度,
//           速度标准差,速度震荡频率,最大速度,最小速度,label,最大径向加速度,最大径向速率,
//           最小径向速率,abs最大径向加速度,abs最大径向速率,abs最小径向速率,平均RCS,相对高度均值,相对高度标准差
bool write_merged_feature_csv(const std::string &filepath,
                              const std::vector<MergedFeature> &features,
                              std::string &error_msg);

#endif
