#!/bin/bash
# make -C build demo_sift1M_search
# 检查是否设置了环境变量 FlameGraphPath
if [ -z "$FlameGraphPath" ]; then
  echo "Error: FlameGraphPath environment variable is not set."
  echo "You should execute command like 'export FlameGraphPath=/path/to/FlameGraph'"
  exit 1
fi

# 获取当前时间戳，用于命名输出文件
timestamp=$(date +"%Y%m%d_%H%M%S")

# 定义相关目录和程序路径
demo_program="./build/demos/demo_sift1M_search"
perf_data_dir="./perf_data"
output_perf="${perf_data_dir}/perf_${timestamp}.data"
output_script="${perf_data_dir}/out_${timestamp}.perf"
output_folded="${perf_data_dir}/out_${timestamp}.folded"
output_svg="${perf_data_dir}/flamegraph_${timestamp}.svg"

# 检查FlameGraph相关的脚本是否存在
if [ ! -f "$FlameGraphPath/stackcollapse-perf.pl" ] || [ ! -f "$FlameGraphPath/flamegraph.pl" ]; then
  echo "Error: stackcollapse-perf.pl or flamegraph.pl not found in $FlameGraphPath."
  exit 1
fi

# sudo taskset -c 0 ./build/demos/demo_IVFFlat_sift1M_search
# sudo taskset -c 0 ./build/demos/demo_HNSW_sift1M_search 

# 创建相关目录（如果不存在）
mkdir -p "$perf_data_dir"

# pgrep -f demo_sift1M_search
# taskset -c 0 ./build/demos/demo_sift1M_search &
# echo "Already running"
# sleep 2

# 获取第一个传入的参数 即要进行perf跟踪的
# param=$1

# 获取指定进程的PID
# PID=$(pgrep -f demo_HNSW_sift1M_search )
PID=$(pgrep -f demo_IVFFlat_sift1M_search )
# PID=$(pgrep -f "$param")
# echo "$PID"
# exit 1

if [ -z "$PID" ]; then
    echo "$param 程序未启动，请检查！"
    exit 1
fi

# 步骤 1: 使用 perf 记录程序运行的性能数据
echo "Recording perf data..."
sudo perf record -F 99 -g -o "$output_perf" -p $PID

# sudo perf record -p $(pgrep -f demo_sift1M_search) -o perf.data

# 步骤 2: 将 perf 数据转换为文本格式
echo "Generating perf script output..."
sudo perf script -i "$output_perf" > "$output_script"

# 步骤 3: 折叠 perf 数据为火焰图输入格式
echo "Collapsing stacks for flamegraph..."
"$FlameGraphPath/stackcollapse-perf.pl" "$output_script" > "$output_folded"

# 步骤 4: 生成火焰图 SVG 文件
echo "Generating flamegraph SVG..."
"$FlameGraphPath/flamegraph.pl" "$output_folded" > "$output_svg"

# 完成提示
echo "Flamegraph generated: $output_svg"
