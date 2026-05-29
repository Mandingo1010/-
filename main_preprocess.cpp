#include "data_preprocessor.h"
#include "radar_types.h"
#include <iostream>
#include <string>
#include <vector>

// ============================================================================
// 模块C主程序：数据清洗与速度计算
// ============================================================================
// 功能：读取合并后的训练数据，执行清洗和速度计算
//
// 输入文件：
//   data/tmp/train_data_all.csv（模块B的输出）
//
// 输出文件：
//   data/tmp/middle2_cleaned.csv（清洗后的数据）
//   data/tmp/middle5_cleaned.csv（速度记录）
// ============================================================================

int main(int argc, char *argv[]) {
  std::string error_msg;

  // 1. 读取输入数据
  std::cout << "正在读取输入数据..." << std::endl;
  std::vector<RadarPoint> points;
  if (!read_radar_csv("data/tmp/train_data_all.csv", points, error_msg)) {
    std::cerr << "读取输入文件失败: " << error_msg << std::endl;
    return 1;
  }
  std::cout << "读取了 " << points.size() << " 个数据点" << std::endl;

  // 2. 创建预处理器并执行数据清洗
  std::cout << "正在进行数据清洗..." << std::endl;
  DataPreprocessor preprocessor;
  std::vector<RadarPoint> cleaned = preprocessor.process(points);

  // 3. 计算速度记录
  std::cout << "正在计算速度记录..." << std::endl;
  std::vector<SpeedRecord> speeds = preprocessor.compute_speed_records(cleaned);

  // 4. 保存清洗后的数据
  if (!write_radar_csv("data/tmp/middle2_cleaned.csv", cleaned, error_msg)) {
    std::cerr << "保存清洗数据失败: " << error_msg << std::endl;
    return 1;
  }
  std::cout << "清洗后数据已保存到 data/tmp/middle2_cleaned.csv" << std::endl;

  // 5. 保存速度记录
  if (!write_speed_csv("data/tmp/middle5_cleaned.csv", speeds, error_msg)) {
    std::cerr << "保存速度记录失败: " << error_msg << std::endl;
    return 1;
  }
  std::cout << "速度记录已保存到 data/tmp/middle5_cleaned.csv" << std::endl;

  // 6. 输出统计信息
  std::cout << std::endl;
  std::cout << "========== 统计信息 ==========" << std::endl;
  std::cout << "输入点数: " << preprocessor.get_input_count() << std::endl;
  std::cout << "清洗后点数: " << preprocessor.get_output_count() << std::endl;
  std::cout << "剔除点数: " << preprocessor.get_removed_count() << std::endl;
  std::cout << "速度记录数: " << speeds.size() << std::endl;

  return 0;
}
