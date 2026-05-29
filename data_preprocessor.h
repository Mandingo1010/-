#ifndef DATA_PREPROCESSOR_H
#define DATA_PREPROCESSOR_H

#include "radar_types.h"
#include <vector>
#include <string>

// ============================================================================
// 模块C：数据清洗与速度计算
// ============================================================================
// 功能：
//   1. 数据清洗：按航迹分组、排序、去除异常值
//   2. 3D速度计算：计算相邻雷达点之间的三维速度
//
// 算法说明：
//   process():
//     1. 按 track_id 分组
//     2. 每组按 time 升序排序
//     3. 剔除 radial_rate 或 time 为 NaN/inf 的点
//     4. 如果被剔除的点包含 track_id，将其前移给下一个有效点
//
//   calculate_speed():
//     将雷达球坐标(方位角、斜距、高度)转换为直角坐标，计算欧氏距离：
//       r = sqrt(range^2 - height^2)
//       x = r * sin(azimuth_rad)
//       y = r * cos(azimuth_rad)
//       z = height
//       speed = sqrt((x2-x1)^2 + (y2-y1)^2 + (z2-z1)^2) / (t2 - t1)
//
//   compute_speed_records():
//     1. 遍历相邻点计算速度
//     2. 跳过跨航迹的计算（当前点无track_id且下一点有track_id）
//     3. 剔除 speed 或 time_diff 为 NaN/inf 的记录
//
// 输入：std::vector<RadarPoint>（原始雷达数据）
// 输出：
//   process() -> std::vector<RadarPoint>（清洗后的数据）
//   compute_speed_records() -> std::vector<SpeedRecord>（速度记录）
// ============================================================================

class DataPreprocessor {
public:
    DataPreprocessor();
    ~DataPreprocessor();

    // 数据清洗
    // input: 原始雷达数据
    // 返回: 清洗后的数据（已分组排序，去除异常值）
    std::vector<RadarPoint> process(const std::vector<RadarPoint>& input);

    // 计算两个雷达点之间的3D速度
    static double calculate_speed(const RadarPoint& p1, const RadarPoint& p2);

    // 计算所有相邻点的速度记录
    // input: 清洗后的雷达数据
    // 返回: 速度记录列表
    std::vector<SpeedRecord> compute_speed_records(
        const std::vector<RadarPoint>& input);

    // 获取清洗统计信息
    int get_input_count() const;      // 输入数据点数
    int get_output_count() const;     // 清洗后数据点数
    int get_removed_count() const;    // 剔除的数据点数

private:
    int input_count_;
    int output_count_;
    int removed_count_;
};

#endif
