PID=$(pgrep -f demo_HNSW_gist1M_search)
# PID=$(pgrep -f demo_IVFFlat_gist1M_search)

 

# 检查 PID 是否为空，防止后续命令报错
if [ -n "$PID" ]; then
    python3 metric.py -d 300 --pid "$PID" -p -s -m --save-local
else
    echo "No process found for demo_sift1M_search"
fi