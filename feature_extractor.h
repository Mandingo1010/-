#ifndef FEATURE_EXTRACTOR_H
#define FEATURE_EXTRACTOR_H

#include "radar_types.h"
#include <vector>
#include <string>

// ============================================================================
// 模块D：特征提取
// ============================================================================
// 功能：从清洗后的雷达数据中提取三类统计特征
//
// 特征1 - 速度统计特征（基于 SpeedRecord）：
//   - 加权平均速度: total_displacement / total_time
//   - 加权标准差: sqrt(sum(time * (speed - mean)^2) / sum(time))
//   - 速度震荡频率: count(|diff(speed)| > 1.0) / (N-1)
//   - 最大/最小速度
//
// 特征2 - 径向动力学特征（基于 RadarPoint）：
//   - 最大径向加速度: (rate[i+1] - rate[i]) / dt
//   - 最大/最小径向速率（按绝对值比较，保留符号）
//   - 平均RCS
//   - 相对高度均值和标准差
//
// 特征3 - 航向角统计特征（基于 RadarPoint）：
//   - 圆周均值: atan2(mean(sin), mean(cos))
//   - 圆周标准差: -2 * log(R), R = sqrt(mean_cos^2 + mean_sin^2)
//   - 航向震荡频率: 统计相邻差值的符号变化模式
//
// 输入：
//   extract_speed_features() -> std::vector<SpeedRecord>
//   extract_radial_features() -> std::vector<RadarPoint>
//   extract_heading_features() -> std::vector<RadarPoint>
// 输出：
//   对应的 Feature 结构体向量
// ============================================================================

class FeatureExtractor {
public:
    FeatureExtractor();
    ~FeatureExtractor();

    // 提取速度统计特征
    // input: 速度记录列表（已按航迹分组，track_id标识不同航迹）
    // 返回: SpeedFeature 列表
    std::vector<SpeedFeature> extract_speed_features(
        const std::vector<SpeedRecord>& input);

    // 提取径向动力学特征
    // input: 清洗后的雷达数据（已按航迹分组）
    // 返回: RadialFeature 列表
    std::vector<RadialFeature> extract_radial_features(
        const std::vector<RadarPoint>& input);

    // 提取航向角统计特征
    // input: 清洗后的雷达数据（已按航迹分组）
    // 返回: HeadingFeature 列表
    std::vector<HeadingFeature> extract_heading_features(
        const std::vector<RadarPoint>& input);

    // 合并特征
    // 以 track_id 为键，将三类特征合并为 MergedFeature
    // 如果某个 track_id 在某类特征中缺失，对应字段设为 NaN
    static std::vector<MergedFeature> merge_features(
        const std::vector<SpeedFeature>& speed_features,
        const std::vector<RadialFeature>& radial_features,
        const std::vector<HeadingFeature>& heading_features);

    // 去除包含 NaN 的特征记录（dropna）
    static std::vector<MergedFeature> drop_na(
        const std::vector<MergedFeature>& input);
};

#endif
