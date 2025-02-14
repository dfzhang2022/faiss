#!/bin/bash
# make -C build demo_sift1M_search

# 获取当前时间戳，用于命名输出文件
timestamp=$(date +"%Y%m%d_%H%M%S")

# 定义相关目录和程序路径
perf_data_dir="./perf_data"
demo_program="./build/demos/gist1M_search"
metric_data_dir="./perf_data"
output_perf="${perf_data_dir}/perf_${timestamp}.data"
output_script="${perf_data_dir}/out_${timestamp}.perf"
output_folded="${perf_data_dir}/out_${timestamp}.folded"
output_svg="${perf_data_dir}/flamegraph_${timestamp}.svg"


# 创建相关目录（如果不存在）
mkdir -p "$perf_data_dir"



# EXE_NAME=demo_IVFFlat_gist1M_search
EXE_NAME=demo_gist1M_search
# 获取指定进程的PID
PID=$(pgrep -f ${EXE_NAME})



if [ -z "$PID" ]; then
    echo "$param 程序未启动，请检查！"
    exit 1
fi

NAME=${EXE_NAME}_ivf_1 

# python3 metirc_amd.py -d 1200 --pid $PID  --save-local -p -s


# 检查 PID 是否为空，防止后续命令报错
if [ -n "$PID" ]; then
    python3 amd_metric.py -d 600 --pid "$PID" -p -s  --save-local --name "$NAME" --memory
else
    echo "No process found for ${EXE_NAME}"
fi