#ifndef PIPELINE_ORCHESTRATOR_H
#define PIPELINE_ORCHESTRATOR_H

#include "radar_types.h"
#include <vector>
#include <string>

// ============================================================================
// 模块E：主控流水线
// ============================================================================
// 功能：编排整个数据处理流程，将前四个模块串联起来
//
// 三阶段流水线：
//   Stage 1: 数据增强 -> 统计合并 -> 数据清洗 -> 速度计算
//   Stage 2: 特征提取（速度、径向、航向）
//   Stage 3: 特征合并 -> 去除NaN
//
// 输入：原始雷达数据文件路径
// 输出：最终特征文件路径
// ============================================================================

class PipelineOrchestrator {
public:
    PipelineOrchestrator();
    ~PipelineOrchestrator();

    // 运行完整流水线
    // input_files: 原始输入文件路径列表，如 {"data/input/label0.csv", "data/input/label1.csv"}
    // output_file: 最终输出文件路径，如 "data/output/merged_cleaned.csv"
    // 返回: 成功返回 true，失败返回 false
    bool run(const std::vector<std::string>& input_files,
             const std::string& output_file);

    // 获取各阶段统计信息
    int get_num_tracks() const;           // 原始航迹数
    int get_num_augmented() const;        // 增强后数据点数
    int get_num_cleaned() const;          // 清洗后数据点数
    int get_num_speed_records() const;    // 速度记录数
    int get_num_features() const;         // 最终特征数

    // 获取错误信息
    std::string get_error_msg() const;

private:
    std::string error_msg_;
    int num_tracks_;
    int num_augmented_;
    int num_cleaned_;
    int num_speed_records_;
    int num_features_;
};

#endif
