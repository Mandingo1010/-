#include "data_augment.h"
#include <iostream>

// ============================================================================
// 学生任务：实现 DataAugmentor 类的所有成员函数
// ============================================================================
//
// 需要实现的函数：
//   1. DataAugmentor() - 构造函数，初始化成员变量
//   2. ~DataAugmentor() - 析构函数
//   3. set_params() - 设置增强参数
//   4. augment() - 核心增强算法
//   5. get_num_tracks() - 获取原始航迹数
//   6. get_num_windows() - 获取生成的窗口总数
//
// 核心算法提示（augment函数）：
//   步骤1: 将输入数据按 track_id 分组
//         提示: 遍历input，当track_id变化时开始新组
//
//   步骤2: 对每个 (stride, x) 组合：
//         提示: 双重循环遍历 strides_ 和 x_values_
//
//   步骤3: 对每个航迹组：
//         a. 如果组长度 < x * stride，跳过
//         b. 使用滑动窗口（步进3）截取子序列
//         c. 窗口起始位置: start = 0, 3, 6, ...
//         d. 窗口内取点: idx = start + k * stride, k = 0, 1, ..., x-1
//         e. 窗口内第一点的track_id保留，其余设为-1
//
//   步骤4: 将每个有效窗口作为一个vector<RadarPoint>加入结果
//
// ============================================================================

DataAugmentor::DataAugmentor() : num_tracks_(0), num_windows_(0) {
}                                                   //原始轨迹数量,初始窗口数设置为0

DataAugmentor::~DataAugmentor() {
}                                                 

void DataAugmentor::set_params(const std::vector<int>& strides,
                               const std::vector<int>& x_values) {
       strides_ = strides;    //设置步长
    x_values_ = x_values;     //设置每个窗口取多少个点
}

std::vector<std::vector<RadarPoint> > DataAugmentor::augment(            //该函数把长串的雷达数据划分成了多个小窗口，实现数据增强
    const std::vector<RadarPoint>& input) {
    std::vector<std::vector<RadarPoint> > result;
// 每次增强前重置统计
    num_tracks_ = 0;
    num_windows_ = 0;

    // ====================== 步骤1：按 track_id 分组 ======================
    std::vector<std::vector<RadarPoint>> track_groups;
    if (!input.empty()) {
        int current_id = input[0].track_id;
        std::vector<RadarPoint> current_group;

        for (const auto& p : input) {
            if (p.track_id != current_id) {
                // track_id 变化，保存当前组，新建组
                track_groups.push_back(current_group);
                current_group.clear();
                current_id = p.track_id;
            }
            current_group.push_back(p);
        }
        // 把最后一组加进去
        track_groups.push_back(current_group);
    }

    // 记录原始航迹数
    num_tracks_ = track_groups.size();

    // ====================== 步骤2：遍历所有参数组合 ======================
    for (int stride : strides_) {
        for (int x : x_values_) {

            // ==================== 步骤3：对每个航迹组滑动窗口 ====================
            for (const auto& track : track_groups) {
                int track_len = track.size();
                int min_len = x * stride;

                // a. 长度不足则跳过
                if (track_len < min_len) continue;

                // b. 滑动窗口，步进 3
                for (int start = 0; start <= track_len - min_len; start += 3) {
                    std::vector<RadarPoint> window;

                    // d. 按 stride 采样点
                    for (int k = 0; k < x; ++k) {
                        int idx = start + k * stride;
                        RadarPoint point = track[idx];

                        // e. 为每个窗口分配唯一自增 track_id，其余点设为 -1
                        if (k == 0) {
                            static int window_id = 0;
                            window_id++;
                            point.track_id = window_id;
                        } else {
                            point.track_id = -1;
                        }
                        window.push_back(point);
                    }

                    // 加入结果
                    result.push_back(window);
                    num_windows_++;
                }
            }
        }
    }
    return result;
}

int DataAugmentor::get_num_tracks() const {
   return num_tracks_;   //获取原始轨迹数
}

int DataAugmentor::get_num_windows() const {
    return num_windows_;    //获取生成的窗口总数
}