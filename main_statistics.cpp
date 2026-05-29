#include <iostream>
#include <vector>
#include <string>
#include "statistics_merge.h"
#include "radar_types.h"
#include "data_augment.h"

// ============================================================================
// 模块B主程序：统计合并
// ============================================================================
// 功能：读取多个增强后的数据文件，合并并生成统计报告
//
// 输入文件：
//   data/output/aug_label*_stride*_x*.csv（模块A的输出）
//
// 输出文件：
//   data/tmp/train_data_all.csv
//
// 学生任务：
//   1. 读取所有增强文件（使用 read_radar_csv）
//   2. 创建 StatisticsMerger 对象
//   3. 调用 merge() 合并数据
//   4. 调用 generate_report() 输出统计报告
//   5. 使用 write_radar_csv() 保存合并结果
// ============================================================================

int main(int argc, char* argv[]) {
    std::string error_msg;

    // TODO: 定义增强文件列表
    // 提示: 根据模块A生成的文件名，构建文件路径列表
    // 例如:
    //   std::vector<std::string> aug_files = {
    //       "data/output/aug_label0_stride1_x5.csv",
    //       ...
    //   };

    // TODO: 读取所有增强文件
    // std::vector<std::vector<RadarPoint> > all_aug_data;
    // for each file:
    //   std::vector<RadarPoint> points;
    //   read_radar_csv(file, points, error_msg);
    //   all_aug_data.push_back(points);

    // TODO: 创建 StatisticsMerger 并合并数据
    // StatisticsMerger merger;
    // std::vector<RadarPoint> merged = merger.merge(all_aug_data);

    // TODO: 读取原始数据并统计行数
    // int orig_count = ...;

    // TODO: 生成统计报告
    // merger.generate_report(orig_count);

    // TODO: 保存合并结果
    // write_radar_csv("data/tmp/train_data_all.csv", merged, error_msg);

    std::cout << "Statistics and merge completed." << std::endl;
    return 0;
}
