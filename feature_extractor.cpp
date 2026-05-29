#include "feature_extractor.h"
#include <cmath>
#include <limits>
#include <map>
#include <set>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================================================================
// 辅助函数：判断 NaN
// ============================================================================
static bool is_nan(double val) {
    return val != val;
}

// ============================================================================
// 辅助函数：按 track_id 对 SpeedRecord 分组
// ============================================================================
static std::map<int, std::vector<SpeedRecord> > group_speed_by_track(
    const std::vector<SpeedRecord>& records) {
    std::map<int, std::vector<SpeedRecord> > groups;
    for (size_t i = 0; i < records.size(); ++i) {
        groups[records[i].track_id].push_back(records[i]);
    }
    return groups;
}

// ============================================================================
// 辅助函数：按 track_id 对 RadarPoint 分组
// 当 track_id 发生变化（且 != -1）时开始新的航迹组
// ============================================================================
static std::vector<std::pair<int, std::vector<RadarPoint> > > group_radar_by_track(
    const std::vector<RadarPoint>& points) {
    std::vector<std::pair<int, std::vector<RadarPoint> > > groups;
    int current_tid = -999;

    for (size_t i = 0; i < points.size(); ++i) {
        if (points[i].track_id != -1 && points[i].track_id != current_tid) {
            // 新航迹开始
            current_tid = points[i].track_id;
            groups.push_back(std::make_pair(current_tid, std::vector<RadarPoint>()));
        }
        if (!groups.empty()) {
            groups.back().second.push_back(points[i]);
        }
    }
    return groups;
}

// ============================================================================
// 构造函数 / 析构函数
// ============================================================================
FeatureExtractor::FeatureExtractor() {
}

FeatureExtractor::~FeatureExtractor() {
}

// ============================================================================
// extract_speed_features: 速度统计特征提取
// ============================================================================
// 算法：
//   1. 按 track_id 分组
//   2. 对每组计算：
//      - 加权平均速度 = sum(speed * time_diff) / sum(time_diff)
//      - 加权标准差 = sqrt(sum(time_diff * (speed - mean)^2) / sum(time_diff))
//      - 速度震荡频率 = count(|speed[i+1] - speed[i]| > 1.0) / (N-1)
//      - 最大/最小速度
// ============================================================================
std::vector<SpeedFeature> FeatureExtractor::extract_speed_features(
    const std::vector<SpeedRecord>& input) {
    std::vector<SpeedFeature> result;

    // 按 track_id 分组
    std::map<int, std::vector<SpeedRecord> > groups = group_speed_by_track(input);

    for (std::map<int, std::vector<SpeedRecord> >::iterator it = groups.begin();
         it != groups.end(); ++it) {
        int tid = it->first;
        const std::vector<SpeedRecord>& records = it->second;
        size_t N = records.size();

        SpeedFeature feat;
        feat.track_id = tid;
        feat.label = records[0].label;

        if (N == 0) {
            feat.mean_speed = 0;
            feat.std_speed = 0;
            feat.oscillation_freq = 0;
            feat.max_speed = 0;
            feat.min_speed = 0;
            result.push_back(feat);
            continue;
        }

        // 计算加权平均速度
        double total_displacement = 0.0;
        double total_time = 0.0;
        double max_speed = records[0].speed;
        double min_speed = records[0].speed;

        for (size_t i = 0; i < N; ++i) {
            total_displacement += records[i].speed * records[i].time_diff;
            total_time += records[i].time_diff;
            if (records[i].speed > max_speed) max_speed = records[i].speed;
            if (records[i].speed < min_speed) min_speed = records[i].speed;
        }

        double mean_speed = 0.0;
        if (total_time != 0.0) {
            mean_speed = total_displacement / total_time;
        }

        // 计算加权标准差
        double variance_sum = 0.0;
        for (size_t i = 0; i < N; ++i) {
            double diff = records[i].speed - mean_speed;
            variance_sum += records[i].time_diff * diff * diff;
        }
        double std_speed = 0.0;
        if (total_time != 0.0) {
            double var = variance_sum / total_time;
            // 防止因浮点精度导致极小的负值
            if (var > 0.0) {
                std_speed = std::sqrt(var);
            }
        }

        // 计算速度震荡频率
        int oscillation_count = 0;
        for (size_t i = 0; i + 1 < N; ++i) {
            if (std::fabs(records[i + 1].speed - records[i].speed) > 1.0) {
                oscillation_count++;
            }
        }
        double oscillation_freq = 0.0;
        if (N > 1) {
            oscillation_freq = (double)oscillation_count / (double)(N - 1);
        }

        feat.mean_speed = mean_speed;
        feat.std_speed = std_speed;
        feat.oscillation_freq = oscillation_freq;
        feat.max_speed = max_speed;
        feat.min_speed = min_speed;

        result.push_back(feat);
    }

    return result;
}

// ============================================================================
// extract_radial_features: 径向动力学特征提取
// ============================================================================
std::vector<RadialFeature> FeatureExtractor::extract_radial_features(
    const std::vector<RadarPoint>& input) {
    std::vector<RadialFeature> result;

    // 按 track_id 分组
    std::vector<std::pair<int, std::vector<RadarPoint> > > groups =
        group_radar_by_track(input);

    for (size_t g = 0; g < groups.size(); ++g) {
        int tid = groups[g].first;
        const std::vector<RadarPoint>& pts = groups[g].second;
        size_t N = pts.size();

        RadialFeature feat;
        feat.track_id = tid;
        feat.label = pts[0].label;  // 使用类别标签而非 track_id

        if (N == 0) {
            result.push_back(feat);
            continue;
        }

        // 最大径向加速度（按绝对值比较，保留符号）
        double max_accel = 0.0;
        double max_abs_accel = 0.0;
        for (size_t i = 0; i + 1 < N; ++i) {
            double dt = pts[i + 1].time - pts[i].time;
            if (dt == 0.0) continue;
            double accel = (pts[i + 1].radial_rate - pts[i].radial_rate) / dt;
            if (std::fabs(accel) > max_abs_accel) {
                max_abs_accel = std::fabs(accel);
                max_accel = accel;
            }
        }

        // 最大/最小径向速率（按绝对值比较，保留符号）
        double max_rate = pts[0].radial_rate;
        double min_rate = pts[0].radial_rate;
        double max_abs_rate = std::fabs(pts[0].radial_rate);
        double min_abs_rate = std::fabs(pts[0].radial_rate);

        for (size_t i = 1; i < N; ++i) {
            double abs_rate = std::fabs(pts[i].radial_rate);
            if (abs_rate > max_abs_rate) {
                max_abs_rate = abs_rate;
                max_rate = pts[i].radial_rate;
            }
            if (abs_rate < min_abs_rate) {
                min_abs_rate = abs_rate;
                min_rate = pts[i].radial_rate;
            }
        }

        // 平均 RCS
        double sum_rcs = 0.0;
        for (size_t i = 0; i < N; ++i) {
            sum_rcs += pts[i].rcs;
        }
        double mean_rcs = sum_rcs / (double)N;

        // 相对高度均值和标准差（总体标准差，除以 N）
        double sum_height = 0.0;
        for (size_t i = 0; i < N; ++i) {
            sum_height += pts[i].height;
        }
        double mean_height = sum_height / (double)N;

        double var_height = 0.0;
        for (size_t i = 0; i < N; ++i) {
            double diff = pts[i].height - mean_height;
            var_height += diff * diff;
        }
        var_height /= (double)N;
        double std_height = std::sqrt(var_height);

        feat.max_accel = max_accel;
        feat.max_rate = max_rate;
        feat.min_rate = min_rate;
        feat.abs_max_accel = std::fabs(max_accel);
        feat.abs_max_rate = std::fabs(max_rate);
        feat.abs_min_rate = std::fabs(min_rate);
        feat.mean_rcs = mean_rcs;
        feat.mean_height = mean_height;
        feat.std_height = std_height;

        result.push_back(feat);
    }

    return result;
}

// ============================================================================
// extract_heading_features: 航向角统计特征提取
// ============================================================================
std::vector<HeadingFeature> FeatureExtractor::extract_heading_features(
    const std::vector<RadarPoint>& input) {
    std::vector<HeadingFeature> result;

    // 按 track_id 分组
    std::vector<std::pair<int, std::vector<RadarPoint> > > groups =
        group_radar_by_track(input);

    const double gamma = 0.2;  // 震荡阈值（弧度）

    for (size_t g = 0; g < groups.size(); ++g) {
        int tid = groups[g].first;
        const std::vector<RadarPoint>& pts = groups[g].second;
        size_t N = pts.size();

        HeadingFeature feat;
        feat.track_id = tid;

        if (N == 0) {
            feat.mean_heading = 0;
            feat.std_heading = 0;
            feat.oscillation_freq = 0;
            result.push_back(feat);
            continue;
        }

        // 将方位角转换为弧度
        std::vector<double> headings(N);
        for (size_t i = 0; i < N; ++i) {
            headings[i] = pts[i].azimuth * M_PI / 180.0;
        }

        // 圆周均值
        double sin_sum = 0.0;
        double cos_sum = 0.0;
        for (size_t i = 0; i < N; ++i) {
            sin_sum += std::sin(headings[i]);
            cos_sum += std::cos(headings[i]);
        }
        double mean_sin = sin_sum / (double)N;
        double mean_cos = cos_sum / (double)N;
        double mean_heading = std::atan2(mean_sin, mean_cos);

        // 圆周标准差
        double R = std::sqrt(mean_cos * mean_cos + mean_sin * mean_sin);
        double std_heading = 0.0;
        if (R > 1.0) {
            std_heading = 0.0;
        } else {
            std_heading = -2.0 * std::log(R);
        }

        // 航向震荡频率
        double oscillation_freq = 0.0;
        if (N >= 3) {
            // 计算相邻方位角差值
            std::vector<double> diffs(N - 1);
            for (size_t i = 0; i + 1 < N; ++i) {
                diffs[i] = headings[i + 1] - headings[i];
            }

            // 构建 O 数组
            std::vector<int> O(diffs.size());
            for (size_t i = 0; i < diffs.size(); ++i) {
                if (diffs[i] > gamma) {
                    O[i] = 1;
                } else if (diffs[i] < -gamma) {
                    O[i] = -1;
                } else {
                    O[i] = 0;
                }
            }

            // 统计震荡模式
            int osc_count = 0;
            for (size_t i = 1; i < O.size(); ++i) {
                // 模式1：连续符号变化
                if (O[i - 1] + O[i] == 0 && O[i - 1] != O[i]) {
                    osc_count++;
                }
                // 模式2：间隔符号变化
                if (i + 1 < O.size()) {
                    if (O[i - 1] + O[i + 1] == 0 && O[i - 1] != O[i + 1] && O[i] == 0) {
                        osc_count++;
                    }
                }
            }

            oscillation_freq = (double)osc_count / (double)(N - 2);
        }

        feat.mean_heading = mean_heading;
        feat.std_heading = std_heading;
        feat.oscillation_freq = oscillation_freq;

        result.push_back(feat);
    }

    return result;
}

// ============================================================================
// merge_features: 合并三类特征
// ============================================================================
std::vector<MergedFeature> FeatureExtractor::merge_features(
    const std::vector<SpeedFeature>& speed_features,
    const std::vector<RadialFeature>& radial_features,
    const std::vector<HeadingFeature>& heading_features) {
    std::vector<MergedFeature> result;

    const double NaN = std::numeric_limits<double>::quiet_NaN();

    // 构建查找表
    std::map<int, const SpeedFeature*> speed_map;
    std::map<int, const RadialFeature*> radial_map;
    std::map<int, const HeadingFeature*> heading_map;

    for (size_t i = 0; i < speed_features.size(); ++i) {
        speed_map[speed_features[i].track_id] = &speed_features[i];
    }
    for (size_t i = 0; i < radial_features.size(); ++i) {
        radial_map[radial_features[i].track_id] = &radial_features[i];
    }
    for (size_t i = 0; i < heading_features.size(); ++i) {
        heading_map[heading_features[i].track_id] = &heading_features[i];
    }

    // 收集所有唯一的 track_id
    std::set<int> all_ids;
    for (size_t i = 0; i < speed_features.size(); ++i) {
        all_ids.insert(speed_features[i].track_id);
    }
    for (size_t i = 0; i < radial_features.size(); ++i) {
        all_ids.insert(radial_features[i].track_id);
    }
    for (size_t i = 0; i < heading_features.size(); ++i) {
        all_ids.insert(heading_features[i].track_id);
    }

    // 对每个 track_id 构建 MergedFeature
    for (std::set<int>::iterator it = all_ids.begin(); it != all_ids.end(); ++it) {
        int tid = *it;
        MergedFeature mf;
        mf.track_id = tid;

        // 从 HeadingFeature 获取
        if (heading_map.find(tid) != heading_map.end()) {
            const HeadingFeature* hf = heading_map[tid];
            mf.mean_heading = hf->mean_heading;
            mf.std_heading = hf->std_heading;
            mf.heading_osc_freq = hf->oscillation_freq;
        } else {
            mf.mean_heading = NaN;
            mf.std_heading = NaN;
            mf.heading_osc_freq = NaN;
        }

        // 从 SpeedFeature 获取
        if (speed_map.find(tid) != speed_map.end()) {
            const SpeedFeature* sf = speed_map[tid];
            mf.mean_speed = sf->mean_speed;
            mf.std_speed = sf->std_speed;
            mf.speed_osc_freq = sf->oscillation_freq;
            mf.max_speed = sf->max_speed;
            mf.min_speed = sf->min_speed;
            mf.label = sf->label;
        } else {
            mf.mean_speed = NaN;
            mf.std_speed = NaN;
            mf.speed_osc_freq = NaN;
            mf.max_speed = NaN;
            mf.min_speed = NaN;
            mf.label = 0;
        }

        // 从 RadialFeature 获取
        if (radial_map.find(tid) != radial_map.end()) {
            const RadialFeature* rf = radial_map[tid];
            mf.max_accel = rf->max_accel;
            mf.max_rate = rf->max_rate;
            mf.min_rate = rf->min_rate;
            mf.abs_max_accel = rf->abs_max_accel;
            mf.abs_max_rate = rf->abs_max_rate;
            mf.abs_min_rate = rf->abs_min_rate;
            mf.mean_rcs = rf->mean_rcs;
            mf.mean_height = rf->mean_height;
            mf.std_height = rf->std_height;
        } else {
            mf.max_accel = NaN;
            mf.max_rate = NaN;
            mf.min_rate = NaN;
            mf.abs_max_accel = NaN;
            mf.abs_max_rate = NaN;
            mf.abs_min_rate = NaN;
            mf.mean_rcs = NaN;
            mf.mean_height = NaN;
            mf.std_height = NaN;
        }

        result.push_back(mf);
    }

    return result;
}

// ============================================================================
// drop_na: 去除包含 NaN 的特征记录
// ============================================================================
std::vector<MergedFeature> FeatureExtractor::drop_na(
    const std::vector<MergedFeature>& input) {
    std::vector<MergedFeature> result;

    for (size_t i = 0; i < input.size(); ++i) {
        const MergedFeature& mf = input[i];

        // 检查所有 double 字段是否包含 NaN
        if (is_nan(mf.mean_heading) || is_nan(mf.std_heading) ||
            is_nan(mf.heading_osc_freq) || is_nan(mf.mean_speed) ||
            is_nan(mf.std_speed) || is_nan(mf.speed_osc_freq) ||
            is_nan(mf.max_speed) || is_nan(mf.min_speed) ||
            is_nan(mf.max_accel) || is_nan(mf.max_rate) ||
            is_nan(mf.min_rate) || is_nan(mf.abs_max_accel) ||
            is_nan(mf.abs_max_rate) || is_nan(mf.abs_min_rate) ||
            is_nan(mf.mean_rcs) || is_nan(mf.mean_height) ||
            is_nan(mf.std_height)) {
            continue;  // 跳过包含 NaN 的记录
        }

        result.push_back(mf);
    }

    return result;
}
