#include <iostream>
#include <vector>
#include <string>
#include "pipeline_orchestrator.h"

// ============================================================================
// 模块E主程序：完整流水线
// ============================================================================
// 功能：运行完整的数据处理流水线
//
// 输入文件：
//   data/input/label0.csv
//   data/input/label1.csv
//
// 输出文件：
//   data/output/merged_cleaned.csv
//
// 学生任务：
//   1. 创建 PipelineOrchestrator 对象
//   2. 调用 run() 运行流水线
//   3. 输出各阶段统计信息
// ============================================================================

int main(int argc, char* argv[]) {
    // 创建 PipelineOrchestrator
    PipelineOrchestrator pipeline;

    // 定义输入文件列表
    std::vector<std::string> input_files = {
        "data_full/input/label0.csv",
        "data_full/input/label1.csv"
    };

    // 运行流水线
    if (!pipeline.run(input_files, "data_full/output/merged_cleaned.csv")) {
        std::cerr << "Error: " << pipeline.get_error_msg() << std::endl;
        return 1;
    }

    // 输出统计信息
    std::cout << "原始航迹数: " << pipeline.get_num_tracks() << std::endl;
    std::cout << "增强后数据: " << pipeline.get_num_augmented() << std::endl;
    std::cout << "清洗后数据: " << pipeline.get_num_cleaned() << std::endl;
    std::cout << "速度记录: " << pipeline.get_num_speed_records() << std::endl;
    std::cout << "最终特征: " << pipeline.get_num_features() << std::endl;

    std::cout << "Pipeline completed." << std::endl;
    return 0;
}
