#include "pipeline_orchestrator.h"
#include "data_augment.h"
#include "statistics_merge.h"
#include "data_preprocessor.h"
#include "feature_extractor.h"
#include <iostream>
#include <ctime>

// ============================================================================
// 学生任务：实现 PipelineOrchestrator 类的所有成员函数
// ============================================================================
// 162510418
// 需要实现的函数：
//   1. PipelineOrchestrator() - 构造函数
//   2. ~PipelineOrchestrator() - 析构函数
//   3. run() - 运行完整流水线
//   4. get_num_tracks() - 获取原始航迹数
//   5. get_num_augmented() - 获取增强后数据点数
//   6. get_num_cleaned() - 获取清洗后数据点数
//   7. get_num_speed_records() - 获取速度记录数
//   8. get_num_features() - 获取最终特征数
//   9. get_error_msg() - 获取错误信息
//
// run() 流水线编排提示：
//
//   Stage 1 - 数据增强：
//     1. 读取每个输入文件（read_radar_csv）
//     2. 创建 DataAugmentor，设置参数
//     3. 对每个文件调用 augment()
//     4. 保存增强结果到CSV
//
//   Stage 2 - 统计合并：
//     1. 创建 StatisticsMerger
//     2. 调用 merge() 合并所有增强数据
//     3. 调用 generate_report() 输出统计
//     4. 保存合并结果
//
//   Stage 3 - 数据清洗与速度计算：
//     1. 创建 DataPreprocessor
//     2. 调用 process() 清洗数据
//     3. 调用 compute_speed_records() 计算速度
//     4. 保存清洗结果和速度记录
//
//   Stage 4 - 特征提取：
//     1. 创建 FeatureExtractor
//     2. 调用 extract_speed_features()
//     3. 调用 extract_radial_features()
//     4. 调用 extract_heading_features()
//     5. 保存三类特征
//
//   Stage 5 - 特征合并：
//     1. 调用 FeatureExtractor::merge_features()
//     2. 调用 FeatureExtractor::drop_na()
//     3. 保存最终结果
//
// ============================================================================

PipelineOrchestrator::PipelineOrchestrator()
    : num_tracks_(0), num_augmented_(0), num_cleaned_(0),
      num_speed_records_(0), num_features_(0) {
}

PipelineOrchestrator::~PipelineOrchestrator() {
}

bool PipelineOrchestrator::run(const std::vector<std::string>& input_files,
                               const std::string& output_file) {
    std::clock_t start = std::clock();

    // ========== Stage 1 - 数据增强 + 合并 + 清洗 + 速度计算 ==========
    std::cout << ">>> [Stage 1] Data augmentation, merge, cleaning and speed calculation..." << std::endl;

    // 读取所有输入文件并进行数据增强
    std::vector<std::vector<RadarPoint> > all_augmented_data;
    int total_original_count = 0;

    for (size_t i = 0; i < input_files.size(); ++i) {
        std::vector<RadarPoint> input_points;
        if (!read_radar_csv(input_files[i], input_points, error_msg_)) {
            std::cerr << "Error reading file: " << input_files[i] << std::endl;
            return false;
        }
        total_original_count += input_points.size();
        std::cout << "  Read " << input_files[i] << ": " << input_points.size() << " points" << std::endl;

        // 创建增强器并进行数据增强
        DataAugmentor augmentor;
        if (input_files[i].find("label0") != std::string::npos) {
            augmentor.set_params({1, 2}, {5, 7, 11});
        } else {
            augmentor.set_params({1, 2, 3}, {5, 7, 11, 13, 17});
        }
        std::vector<std::vector<RadarPoint> > augmented = augmentor.augment(input_points);
        all_augmented_data.insert(all_augmented_data.end(),
                                   augmented.begin(), augmented.end());
        num_tracks_ = augmentor.get_num_tracks();
        std::cout << "  Augmented: " << augmentor.get_num_windows() << " windows" << std::endl;
    }

    // Stage 2 - 统计合并
    StatisticsMerger merger;
    std::vector<RadarPoint> merged_data = merger.merge(all_augmented_data);
    merger.generate_report(total_original_count);
    num_augmented_ = merger.get_augmented_count();
    std::cout << "  Merged augmented data: " << merged_data.size() << " points" << std::endl;

    if (!write_radar_csv("data_full/tmp/train_data_all.csv", merged_data, error_msg_)) {
        std::cerr << "Error writing merged data: " << error_msg_ << std::endl;
        return false;
    }

    // Stage 3 - 数据清洗与速度计算
    DataPreprocessor preprocessor;
    std::vector<RadarPoint> cleaned_data = preprocessor.process(merged_data);
    num_cleaned_ = cleaned_data.size();
    std::cout << "  Cleaned data: " << cleaned_data.size() << " points" << std::endl;

    if (!write_radar_csv("data_full/tmp/middle2_cleaned.csv", cleaned_data, error_msg_)) {
        std::cerr << "Error writing cleaned data: " << error_msg_ << std::endl;
        return false;
    }

    std::vector<SpeedRecord> speed_records = preprocessor.compute_speed_records(cleaned_data);
    num_speed_records_ = speed_records.size();
    std::cout << "  Speed records: " << speed_records.size() << " records" << std::endl;

    if (!write_speed_csv("data_full/tmp/middle5_cleaned.csv", speed_records, error_msg_)) {
        std::cerr << "Error writing speed records: " << error_msg_ << std::endl;
        return false;
    }

    // ========== Stage 4 - 特征提取 ==========
    std::cout << ">>> [Stage 2] Feature extraction..." << std::endl;

    FeatureExtractor extractor;

    std::vector<SpeedFeature> speed_features = extractor.extract_speed_features(speed_records);
    std::cout << "  Speed features: " << speed_features.size() << " records" << std::endl;
    if (!write_speed_feature_csv("data_full/tmp/feature1.csv", speed_features, error_msg_)) {
        std::cerr << "Error writing speed features: " << error_msg_ << std::endl;
        return false;
    }

    std::vector<RadialFeature> radial_features = extractor.extract_radial_features(cleaned_data);
    std::cout << "  Radial features: " << radial_features.size() << " records" << std::endl;
    if (!write_radial_feature_csv("data_full/tmp/feature2.csv", radial_features, error_msg_)) {
        std::cerr << "Error writing radial features: " << error_msg_ << std::endl;
        return false;
    }

    std::vector<HeadingFeature> heading_features = extractor.extract_heading_features(cleaned_data);
    std::cout << "  Heading features: " << heading_features.size() << " records" << std::endl;
    if (!write_heading_feature_csv("data_full/tmp/feature3.csv", heading_features, error_msg_)) {
        std::cerr << "Error writing heading features: " << error_msg_ << std::endl;
        return false;
    }

    // ========== Stage 5 - 特征合并和去NaN ==========
    std::cout << ">>> [Stage 3] Feature merging and cleaning..." << std::endl;

    std::vector<MergedFeature> merged_features =
        FeatureExtractor::merge_features(speed_features, radial_features, heading_features);
    std::cout << "  Merged features: " << merged_features.size() << " records" << std::endl;

    std::vector<MergedFeature> final_features = FeatureExtractor::drop_na(merged_features);
    num_features_ = final_features.size();
    std::cout << "  Final features (after dropna): " << final_features.size() << " records" << std::endl;

    if (!write_merged_feature_csv(output_file, final_features, error_msg_)) {
        std::cerr << "Error writing final features: " << error_msg_ << std::endl;
        return false;
    }

    // 输出总耗时
    double elapsed = (std::clock() - start) / (double)CLOCKS_PER_SEC;
    std::cout << "Total time: " << elapsed << " seconds" << std::endl;

    return true;
}

int PipelineOrchestrator::get_num_tracks() const {
    return num_tracks_;
}

int PipelineOrchestrator::get_num_augmented() const {
    return num_augmented_;
}

int PipelineOrchestrator::get_num_cleaned() const {
    return num_cleaned_;
}

int PipelineOrchestrator::get_num_speed_records() const {
    return num_speed_records_;
}

int PipelineOrchestrator::get_num_features() const {
    return num_features_;
}

std::string PipelineOrchestrator::get_error_msg() const {
    return error_msg_;
}
