#ifndef DATA_AUGMENT_H
#define DATA_AUGMENT_H

#include "radar_types.h"
#include <vector>
#include <string>

// ============================================================================
// 模块A：数据增强
// ============================================================================
// 功能：对原始雷达航迹数据进行滑动窗口增强，生成多组训练样本
//
// 算法说明：
//   1. 将输入数据按航迹序号分组
//   2. 对每个 (stride, x) 参数组合：
//      - stride: 采样步长，控制窗口内点的间隔
//      - x: 窗口大小，即每个样本包含的点数
//   3. 在每个航迹组内，使用步进为3的滑动窗口截取子序列
//   4. 仅当航迹长度 >= x * stride 时才处理
//   5. 窗口内第一行保留原航迹号，其余行航迹号设为-1
//
// 输入：std::vector<RadarPoint>（原始雷达数据）
// 输出：std::vector<std::vector<RadarPoint>>（增强后的多组数据）
// ============================================================================

class DataAugmentor {
public:
    DataAugmentor();
    ~DataAugmentor();

    // 设置增强参数
    // strides: 采样步长列表，如 {1, 2}
    // x_values: 窗口大小列表，如 {5, 7, 11}
    void set_params(const std::vector<int>& strides,
                    const std::vector<int>& x_values);

    // 执行数据增强
    // input: 原始雷达数据点序列
    // 返回: 增强后的数据组，每组是一个RadarPoint向量
    // 注意: 输入数据已按航迹分组（track_id标识不同航迹）
    std::vector<std::vector<RadarPoint> > augment(
        const std::vector<RadarPoint>& input);

    // 获取增强统计信息
    int get_num_tracks() const;       // 原始航迹数
    int get_num_windows() const;      // 生成的窗口总数

private:
    std::vector<int> strides_;
    std::vector<int> x_values_;
    int num_tracks_;
    int num_windows_;
};

#endif
