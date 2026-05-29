cmake_minimum_required(VERSION 3.10)
project(RadarDataProcessing CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 公共源文件（基础数据结构和CSV工具）
set(COMMON_SOURCES
    src/radar_types.cpp
)

add_library(radar_common STATIC ${COMMON_SOURCES})
target_include_directories(radar_common PUBLIC src)

# ---- 5 个独立模块可执行文件 ----

# 模块A：数据增强
add_executable(augment
    src/main_augment.cpp
    src/data_augment.cpp)
target_link_libraries(augment radar_common)

# 模块B：统计合并
add_executable(statistics
    src/main_statistics.cpp
    src/statistics_merge.cpp)
target_link_libraries(statistics radar_common)

# 模块C：数据清洗与速度计算
add_executable(preprocess
    src/main_preprocess.cpp
    src/data_preprocessor.cpp)
target_link_libraries(preprocess radar_common)

# 模块D：特征提取
add_executable(extract
    src/main_extract.cpp
    src/feature_extractor.cpp)
target_link_libraries(extract radar_common)

# 模块E：完整流水线
add_executable(pipeline
    src/main_pipeline.cpp
    src/data_augment.cpp
    src/statistics_merge.cpp
    src/data_preprocessor.cpp
    src/feature_extractor.cpp
    src/pipeline_orchestrator.cpp)
target_link_libraries(pipeline radar_common)
