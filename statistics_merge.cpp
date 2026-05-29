#include "statistics_merge.h"
#include <iostream>
#include <set>
//覃zj
// ============================================================================
// 学生任务：实现 StatisticsMerger 类的所有成员函数
// ============================================================================
//
// 需要实现的函数：
//   1. StatisticsMerger() - 构造函数
//   2. ~StatisticsMerger() - 析构函数
//   3. merge() - 合并多个数据组
//   4. generate_report() - 生成统计报告
//   5. get_augmented_count() - 获取增强数据总行数
//   6. get_duplicate_count() - 获取重复行数
//   7. get_unique_count() - 获取唯一行数
//
// 核心算法提示（merge函数）：
//   步骤1: 将所有输入数据组按顺序拼接到一个vector中
//         提示: 遍历inputs，将每组数据push_back到结果中
//
//   步骤2: 统计总行数
//
//   步骤3: 统计重复行数
//         提示: 将每行数据转换为字符串表示（如各字段用逗号连接），
//               使用 std::set<std::string> 检测重复
//
//   步骤4: 计算唯一行数 = 总行数 - 重复行数
//
// ============================================================================

StatisticsMerger::StatisticsMerger()
    : augmented_count_(0), duplicate_count_(0), unique_count_(0) {
    // TODO: 构造函数实现
}

StatisticsMerger::~StatisticsMerger() {
    // TODO: 析构函数实现
}

std::vector<RadarPoint> StatisticsMerger::merge(
    const std::vector<std::vector<RadarPoint> >& inputs) {
    std::vector<RadarPoint> result;

    // 步骤1: 将所有输入数据拼接
    for (size_t i = 0; i < inputs.size(); ++i) {
        for (size_t j = 0; j < inputs[i].size(); ++j) {
            result.push_back(inputs[i][j]);
        }
    }

    // 步骤2: 统计增强数据总行数
    augmented_count_ = result.size();

    // 步骤3: 统计重复行
    std::set<std::string> unique_rows;
    duplicate_count_ = 0;
    
    for (size_t i = 0; i < result.size(); ++i) {
        // 将每行转换为字符串表示
        char row_str[256];
        std::snprintf(row_str, sizeof(row_str), 
                     "%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%d,%d",
                     result[i].azimuth, result[i].range, result[i].height,
                     result[i].radial_rate, result[i].rcs, result[i].time,
                     result[i].track_id, result[i].label);
        
        std::string row_key(row_str);
        
        // 如果集合中已有这行，则是重复
        if (unique_rows.find(row_key) != unique_rows.end()) {
            duplicate_count_++;
        } else {
            unique_rows.insert(row_key);
        }
    }

    // 步骤4: 计算唯一行数
    unique_count_ = unique_rows.size();

    return result;
}

void StatisticsMerger::generate_report(int orig_count) const {
    std::cout << "--- Data Statistics Report ---" << std::endl;
    std::cout << "Original data: " << orig_count << " rows" << std::endl;
    std::cout << "Augmented data: " << augmented_count_ << " rows" << std::endl;
    
    double dup_percent = 0.0;
    if (augmented_count_ > 0) {
        dup_percent = (double)duplicate_count_ / augmented_count_ * 100.0;
    }
    std::cout << "Duplicate rows: " << duplicate_count_ << " (" << dup_percent << "%)" << std::endl;
    std::cout << "Unique rows: " << unique_count_ << std::endl;
}

int StatisticsMerger::get_augmented_count() const {
    return augmented_count_;
}

int StatisticsMerger::get_duplicate_count() const {
    return duplicate_count_;
}

int StatisticsMerger::get_unique_count() const {
    return unique_count_;
}
