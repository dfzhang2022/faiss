# EXE_NAME=demo_IVFFlat_gist1M_search
EXE_NAME=demo_gist1M_search

# PID=$(pgrep -f demo_HNSW_gist1M_search)
# PID=$(pgrep -f demo_IVFFlat_gist1M_search)
PID=$(pgrep -f ${EXE_NAME})

NAME=${EXE_NAME}_hnsw_1 

# 检查 PID 是否为空，防止后续命令报错
if [ -n "$PID" ]; then
    python3 metric.py -d 600 --pid "$PID" -p -s -m --save-local --name "$NAME"
else
    echo "No process found for demo_sift1M_search"
fi