#include <iostream>
#include <vector>
#include "data_augment.h"
#include "radar_types.h"

// ============================================================================
// 模块A主程序：数据增强
// ============================================================================
// 功能：读取原始雷达数据，执行数据增强，输出增强后的CSV文件
//
// 输入文件：
//   data/small/label0_small.csv 或 data/input/label0.csv
//   data/small/label1_small.csv 或 data/input/label1.csv
//
// 输出文件：
//   data/output/aug_label0_stride{s}_x{x}.csv
//   data/output/aug_label1_stride{s}_x{x}.csv
//
// 学生任务：
//   1. 使用 read_radar_csv() 读取输入数据
//   2. 创建 DataAugmentor 对象，设置参数
//   3. 调用 augment() 获取增强结果
//   4. 使用 write_radar_csv() 将每组结果写入单独的文件
//   5. 输出统计信息
// ============================================================================

int main(int argc, char* argv[]) {
    std::string error_msg;

    // TODO: 确定输入文件路径
    // 提示: 如果命令行有参数，使用参数指定的路径；否则使用默认路径
    // 例如: ./augment data/small/label0_small.csv data/output/aug_label0

    // TODO: 读取原始雷达数据
    // std::vector<RadarPoint> points;
    // if (!read_radar_csv(input_file, points, error_msg)) { ... }

    // TODO: 创建 DataAugmentor 对象并设置参数
    // DataAugmentor augmentor;
    // std::vector<int> strides = {1, 2};
    // std::vector<int> x_values = {5, 7, 11};
    // augmentor.set_params(strides, x_values);

    // TODO: 执行数据增强
    // std::vector<std::vector<RadarPoint> > result = augmentor.augment(points);

    // TODO: 将每组增强结果写入单独的CSV文件
    // 文件名格式: {output_prefix}_stride{s}_x{x}.csv

    // TODO: 输出统计信息
    // std::cout << "原始航迹数: " << augmentor.get_num_tracks() << std::endl;
    // std::cout << "生成窗口数: " << augmentor.get_num_windows() << std::endl;

    std::cout << "Data augmentation completed." << std::endl;
    return 0;
}
