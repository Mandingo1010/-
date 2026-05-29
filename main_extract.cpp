#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cstdlib>
#include "feature_extractor.h"
#include "radar_types.h"

// ============================================================================
// 模块D主程序：特征提取
// ============================================================================
// 功能：读取清洗后的数据，提取三类特征并合并
//
// 输入文件：
//   data/tmp/middle2_cleaned.csv（清洗后的雷达数据，含 tmp 列标识航迹分组）
//   data/tmp/middle5_cleaned.csv（速度记录，含 tmp 列标识航迹分组）
//
// 输出文件：
//   data/tmp/feature1.csv（速度统计特征）
//   data/tmp/feature2.csv（径向动力学特征）
//   data/tmp/feature3.csv（航向角统计特征）
//   data/output/merged_cleaned.csv（合并后的最终特征）
//
// 注意：输入文件的第8列（tmp列）为实际航迹分组ID，需要用它替换 track_id
// ============================================================================

// 辅助函数：解析 double
static bool parse_dbl(const std::string& str, double& out) {
    if (str.empty()) return false;
    char* end = NULL;
    out = std::strtod(str.c_str(), &end);
    return (end != str.c_str());
}

// 读取带 tmp 列的雷达 CSV，用 tmp 列作为 track_id
static bool read_radar_with_tmp(const std::string& filepath,
                                std::vector<RadarPoint>& points,
                                std::string& error_msg) {
    std::ifstream file(filepath.c_str());
    if (!file.is_open()) {
        error_msg = "Cannot open file: " + filepath;
        return false;
    }

    std::string line;
    if (!std::getline(file, line)) {
        error_msg = "Empty file: " + filepath;
        return false;
    }

    points.clear();
    int current_tmp = -1;

    while (std::getline(file, line)) {
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);
        if (line.empty()) continue;

        std::stringstream ss(line);
        std::vector<std::string> cells;
        std::string cell;
        while (std::getline(ss, cell, ',')) {
            cells.push_back(cell);
        }
        while (cells.size() < 8) cells.push_back("");

        RadarPoint p;
        parse_dbl(cells[0], p.azimuth);
        parse_dbl(cells[1], p.range);
        parse_dbl(cells[2], p.height);
        parse_dbl(cells[3], p.radial_rate);
        parse_dbl(cells[4], p.rcs);
        parse_dbl(cells[5], p.time);

        // 使用 tmp 列（第8列，索引7）作为 track_id
        double tmp_val = 0;
        if (parse_dbl(cells[7], tmp_val)) {
            current_tmp = (int)tmp_val;
        }
        p.track_id = current_tmp;

        points.push_back(p);
    }

    return true;
}

// 读取带 tmp 列的速度记录 CSV，用 tmp 列作为 track_id
static bool read_speed_with_tmp(const std::string& filepath,
                                std::vector<SpeedRecord>& records,
                                std::string& error_msg) {
    std::ifstream file(filepath.c_str());
    if (!file.is_open()) {
        error_msg = "Cannot open file: " + filepath;
        return false;
    }

    std::string line;
    if (!std::getline(file, line)) {
        error_msg = "Empty file: " + filepath;
        return false;
    }

    records.clear();
    int current_label = 0;
    int current_tmp = -1;

    while (std::getline(file, line)) {
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);
        if (line.empty()) continue;

        std::stringstream ss(line);
        std::vector<std::string> cells;
        std::string cell;
        while (std::getline(ss, cell, ',')) {
            cells.push_back(cell);
        }
        while (cells.size() < 7) cells.push_back("");

        SpeedRecord r;
        double tmp;

        // 标签列（可能为空，继承上一个有效值）
        if (!cells[0].empty() && parse_dbl(cells[0], tmp)) {
            current_label = (int)tmp;
        }
        r.label = current_label;

        parse_dbl(cells[1], tmp); r.point1_idx = (int)tmp;
        parse_dbl(cells[2], tmp); r.point2_idx = (int)tmp;
        parse_dbl(cells[3], r.speed);
        parse_dbl(cells[4], r.time_diff);

        // 使用 tmp 列（第7列，索引6）作为 track_id
        if (parse_dbl(cells[6], tmp)) {
            current_tmp = (int)tmp;
        }
        r.track_id = current_tmp;

        records.push_back(r);
    }

    return true;
}

int main(int argc, char* argv[]) {
    std::string error_msg;

    // ---- 1. 读取清洗后的雷达数据（使用 tmp 列作为航迹分组ID） ----
    std::vector<RadarPoint> cleaned_points;
    if (!read_radar_with_tmp("data/tmp/middle2_cleaned.csv", cleaned_points, error_msg)) {
        std::cerr << "读取 middle2_cleaned.csv 失败: " << error_msg << std::endl;
        return 1;
    }
    std::cout << "读取清洗后雷达数据: " << cleaned_points.size() << " 条" << std::endl;

    // ---- 2. 读取速度记录（使用 tmp 列作为航迹分组ID） ----
    std::vector<SpeedRecord> speed_records;
    if (!read_speed_with_tmp("data/tmp/middle5_cleaned.csv", speed_records, error_msg)) {
        std::cerr << "读取 middle5_cleaned.csv 失败: " << error_msg << std::endl;
        return 1;
    }
    std::cout << "读取速度记录: " << speed_records.size() << " 条" << std::endl;

    // ---- 3. 创建 FeatureExtractor 并提取三类特征 ----
    FeatureExtractor extractor;

    std::vector<SpeedFeature> f1 = extractor.extract_speed_features(speed_records);
    std::cout << "速度特征数: " << f1.size() << std::endl;

    std::vector<RadialFeature> f2 = extractor.extract_radial_features(cleaned_points);
    std::cout << "径向特征数: " << f2.size() << std::endl;

    std::vector<HeadingFeature> f3 = extractor.extract_heading_features(cleaned_points);
    std::cout << "航向特征数: " << f3.size() << std::endl;

    // ---- 4. 合并特征并去除 NaN ----
    std::vector<MergedFeature> merged = FeatureExtractor::merge_features(f1, f2, f3);
    std::cout << "合并后特征数: " << merged.size() << std::endl;

    std::vector<MergedFeature> final_result = FeatureExtractor::drop_na(merged);
    std::cout << "去除NaN后: " << final_result.size() << std::endl;

    // ---- 5. 保存结果到 CSV ----
    if (!write_speed_feature_csv("data/tmp/feature1.csv", f1, error_msg)) {
        std::cerr << "写入 feature1.csv 失败: " << error_msg << std::endl;
        return 1;
    }

    if (!write_radial_feature_csv("data/tmp/feature2.csv", f2, error_msg)) {
        std::cerr << "写入 feature2.csv 失败: " << error_msg << std::endl;
        return 1;
    }

    if (!write_heading_feature_csv("data/tmp/feature3.csv", f3, error_msg)) {
        std::cerr << "写入 feature3.csv 失败: " << error_msg << std::endl;
        return 1;
    }

    if (!write_merged_feature_csv("data/output/merged_cleaned.csv", final_result, error_msg)) {
        std::cerr << "写入 merged_cleaned.csv 失败: " << error_msg << std::endl;
        return 1;
    }

    std::cout << "Feature extraction completed." << std::endl;
    return 0;
}
