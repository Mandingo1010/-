#include "radar_types.h"
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <cmath>
#include <iostream>

// ============================================================================
// 辅助函数
// ============================================================================

static bool parse_double(const std::string& str, double& out) {
    if (str.empty()) return false;
    char* end = NULL;
    out = std::strtod(str.c_str(), &end);
    return (end != str.c_str());
}

static std::string format_double(double v) {
    if (std::isnan(v)) return "";
    if (std::isinf(v)) return v > 0 ? "inf" : "-inf";
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.15g", v);
    return std::string(buf);
}

// ============================================================================
// read_radar_csv
// ============================================================================
bool read_radar_csv(const std::string& filepath,
                    std::vector<RadarPoint>& points,
                    std::string& error_msg) {
    std::ifstream file(filepath.c_str());
    if (!file.is_open()) {
        error_msg = "Cannot open file: " + filepath;
        return false;
    }

    std::string line;
    // Skip header
    if (!std::getline(file, line)) {
        error_msg = "Empty file: " + filepath;
        return false;
    }

    points.clear();
    int current_track_id = -1;
    int current_label = -1;
    static int auto_track_counter = -1;

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

        RadarPoint p;
        parse_double(cells[0], p.azimuth);
        parse_double(cells[1], p.range);
        parse_double(cells[2], p.height);
        parse_double(cells[3], p.radial_rate);
        parse_double(cells[4], p.rcs);
        parse_double(cells[5], p.time);

        // Parse track_id（非空且不为 -1 时，表示新航迹，分配自增 ID）
        std::string track_str = cells[6];
        if (!track_str.empty()) {
            double tid;
            if (parse_double(track_str, tid)) {
                int val = (int)tid;
                if (val == -1) {
                    current_track_id = -1;
                } else {
                    current_label = val;
                    auto_track_counter++;
                    current_track_id = auto_track_counter;
                }
            }
        }
        p.track_id = current_track_id;
        p.label = current_label;

        points.push_back(p);
    }

    return true;
}

// ============================================================================
// write_radar_csv
// ============================================================================
bool write_radar_csv(const std::string& filepath,
                     const std::vector<RadarPoint>& points,
                     std::string& error_msg) {
    std::ofstream file(filepath.c_str());
    if (!file.is_open()) {
        error_msg = "Cannot create file: " + filepath;
        return false;
    }

    file << "目标方位角(°),目标斜距(m),相对高度(m),径向速率(m/s),RCS,测量时间(s),航迹序号\n";

    int last_track_id = -999;
    for (size_t i = 0; i < points.size(); ++i) {
        const RadarPoint& p = points[i];
        file << format_double(p.azimuth) << ","
             << format_double(p.range) << ","
             << format_double(p.height) << ","
             << format_double(p.radial_rate) << ","
             << format_double(p.rcs) << ","
             << format_double(p.time) << ",";

        if (p.track_id != last_track_id) {
            file << p.track_id;
            last_track_id = p.track_id;
        }
        file << "\n";
    }

    return true;
}

// ============================================================================
// read_speed_csv
// ============================================================================
bool read_speed_csv(const std::string& filepath,
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
        while (cells.size() < 6) cells.push_back("");

        SpeedRecord r;
        double tmp;
        parse_double(cells[0], tmp); r.label = (int)tmp;
        parse_double(cells[1], tmp); r.point1_idx = (int)tmp;
        parse_double(cells[2], tmp); r.point2_idx = (int)tmp;
        parse_double(cells[3], r.speed);
        parse_double(cells[4], r.time_diff);
        parse_double(cells[5], tmp); r.track_id = (int)tmp;

        records.push_back(r);
    }

    return true;
}

// ============================================================================
// write_speed_csv
// ============================================================================
bool write_speed_csv(const std::string& filepath,
                     const std::vector<SpeedRecord>& records,
                     std::string& error_msg) {
    std::ofstream file(filepath.c_str());
    if (!file.is_open()) {
        error_msg = "Cannot create file: " + filepath;
        return false;
    }

    file << "标签,点1索引,点2索引,速度(m/s),time,航迹ID\n";

    for (size_t i = 0; i < records.size(); ++i) {
        const SpeedRecord& r = records[i];
        file << r.label << ","
             << r.point1_idx << ","
             << r.point2_idx << ","
             << format_double(r.speed) << ","
             << format_double(r.time_diff) << ","
             << r.track_id << "\n";
    }

    return true;
}

// ============================================================================
// write_speed_feature_csv
// ============================================================================
bool write_speed_feature_csv(const std::string& filepath,
                             const std::vector<SpeedFeature>& features,
                             std::string& error_msg) {
    std::ofstream file(filepath.c_str());
    if (!file.is_open()) {
        error_msg = "Cannot create file: " + filepath;
        return false;
    }

    file << "航迹ID,平均速度,速度标准差,速度震荡频率,最大速度,最小速度,label\n";

    for (size_t i = 0; i < features.size(); ++i) {
        const SpeedFeature& f = features[i];
        file << f.track_id << ","
             << format_double(f.mean_speed) << ","
             << format_double(f.std_speed) << ","
             << format_double(f.oscillation_freq) << ","
             << format_double(f.max_speed) << ","
             << format_double(f.min_speed) << ","
             << f.label << "\n";
    }

    return true;
}

// ============================================================================
// write_radial_feature_csv
// ============================================================================
bool write_radial_feature_csv(const std::string& filepath,
                              const std::vector<RadialFeature>& features,
                              std::string& error_msg) {
    std::ofstream file(filepath.c_str());
    if (!file.is_open()) {
        error_msg = "Cannot create file: " + filepath;
        return false;
    }

    file << "航迹ID,最大径向加速度,最大径向速率,最小径向速率,abs最大径向加速度,"
         << "abs最大径向速率,abs最小径向速率,平均RCS,label,相对高度均值,相对高度标准差\n";

    for (size_t i = 0; i < features.size(); ++i) {
        const RadialFeature& f = features[i];
        file << f.track_id << ","
             << format_double(f.max_accel) << ","
             << format_double(f.max_rate) << ","
             << format_double(f.min_rate) << ","
             << format_double(f.abs_max_accel) << ","
             << format_double(f.abs_max_rate) << ","
             << format_double(f.abs_min_rate) << ","
             << format_double(f.mean_rcs) << ","
             << f.label << ","
             << format_double(f.mean_height) << ","
             << format_double(f.std_height) << "\n";
    }

    return true;
}

// ============================================================================
// write_heading_feature_csv
// ============================================================================
bool write_heading_feature_csv(const std::string& filepath,
                               const std::vector<HeadingFeature>& features,
                               std::string& error_msg) {
    std::ofstream file(filepath.c_str());
    if (!file.is_open()) {
        error_msg = "Cannot create file: " + filepath;
        return false;
    }

    file << "航迹ID,航向角均值,航向角标准差,heading_oscillation_frequency\n";

    for (size_t i = 0; i < features.size(); ++i) {
        const HeadingFeature& f = features[i];
        file << f.track_id << ","
             << format_double(f.mean_heading) << ","
             << format_double(f.std_heading) << ","
             << format_double(f.oscillation_freq) << "\n";
    }

    return true;
}

// ============================================================================
// write_merged_feature_csv
// ============================================================================
bool write_merged_feature_csv(const std::string& filepath,
                              const std::vector<MergedFeature>& features,
                              std::string& error_msg) {
    std::ofstream file(filepath.c_str());
    if (!file.is_open()) {
        error_msg = "Cannot create file: " + filepath;
        return false;
    }

    file << "航迹ID,航向角均值,航向角标准差,heading_oscillation_frequency,平均速度,"
         << "速度标准差,速度震荡频率,最大速度,最小速度,label,最大径向加速度,最大径向速率,"
         << "最小径向速率,abs最大径向加速度,abs最大径向速率,abs最小径向速率,平均RCS,相对高度均值,相对高度标准差\n";

    for (size_t i = 0; i < features.size(); ++i) {
        const MergedFeature& f = features[i];
        file << f.track_id << ","
             << format_double(f.mean_heading) << ","
             << format_double(f.std_heading) << ","
             << format_double(f.heading_osc_freq) << ","
             << format_double(f.mean_speed) << ","
             << format_double(f.std_speed) << ","
             << format_double(f.speed_osc_freq) << ","
             << format_double(f.max_speed) << ","
             << format_double(f.min_speed) << ","
             << f.label << ","
             << format_double(f.max_accel) << ","
             << format_double(f.max_rate) << ","
             << format_double(f.min_rate) << ","
             << format_double(f.abs_max_accel) << ","
             << format_double(f.abs_max_rate) << ","
             << format_double(f.abs_min_rate) << ","
             << format_double(f.mean_rcs) << ","
             << format_double(f.mean_height) << ","
             << format_double(f.std_height) << "\n";
    }

    return true;
}
