#ifndef STATISTICS_MERGE_H
#define STATISTICS_MERGE_H

#include "radar_types.h"
#include <vector>
#include <string>

// ============================================================================
// 模块B：统计合并
// ============================================================================
// 功能：将多个增强后的数据文件合并为一个训练集，并生成统计报告
//
// 算法说明：
//   1. 合并：将所有增强数据按顺序拼接
//   2. 统计：
//      - 原始数据总行数
//      - 增强数据总行数
//      - 重复行数量及占比
//      - 唯一行数量
//
// 输入：多个 std::vector<RadarPoint>（增强后的数据组）
// 输出：std::vector<RadarPoint>（合并后的训练数据）
// ============================================================================

class StatisticsMerger {
public:
    StatisticsMerger();
    ~StatisticsMerger();

    // 合并多个数据组
    // inputs: 多组增强数据
    // 返回: 合并后的单一数据序列
    std::vector<RadarPoint> merge(
        const std::vector<std::vector<RadarPoint> >& inputs);

    // 生成统计报告
    // orig_count: 原始数据行数
    // 输出到控制台：
    //   --- Data Statistics Report ---
    //   Original data: X rows
    //   Augmented data: Y rows
    //   Duplicate rows: Z (P%)
    //   Unique rows: U
    void generate_report(int orig_count) const;

    // 获取统计值
    int get_augmented_count() const;   // 增强数据总行数
    int get_duplicate_count() const;   // 重复行数
    int get_unique_count() const;      // 唯一行数

private:
    int augmented_count_;
    int duplicate_count_;
    int unique_count_;
};

#endif
