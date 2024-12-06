      
# echo 'export LARK_APP_ID="value"' >> ~/.bashrc
# echo 'export LARK_APP_SECRET="value"' >> ~/.bashrc
# source ~/.bashrc
import subprocess
import csv
import time
from tqdm import tqdm
import socket
import requests
import psutil
import json
import os
import argparse
from threading import Thread
import re
from datetime import datetime
import getpass

def get_access_token():
    url = 'https://open.feishu.cn/open-apis/auth/v3/tenant_access_token/internal'
    headers = {
        'Content-Type': 'application/json',
    }
    app_id = os.getenv('LARK_APP_ID')
    app_secret = os.getenv('LARK_APP_SECRET')
    if not app_id or not app_secret:
        raise EnvironmentError("Please set the LARK_APP_ID and LARK_APP_SECRET environment variables.")
    response = requests.post(url, headers=headers, json={'app_id': app_id, 'app_secret': app_secret})
    response_json = response.json()
    if response.status_code != 200 or ('code' in response_json and response_json['code'] != 0):
        raise Exception(f"Error getting access token: {response_json}")
    return response.json().get('tenant_access_token')

def batch_save_lark(data, table_id):
    url = f'https://open.feishu.cn/open-apis/bitable/v1/apps/Wv1ubtNTRaOIwCsFtkPcjhcgnHb/tables/{table_id}/records/batch_create'
    headers = {
        'Content-Type': 'application/json',
        'Authorization': f'Bearer {get_access_token()}'
    }
    for i in range(0, len(data), 400):
        batch_data = {"records": []}
        for data_item in data[i:i + 400]:
            batch_data["records"].append({
                "fields": data_item
            })
        # print(batch_data)
        response = requests.post(url, headers=headers, json=batch_data)
        # print(response.status_code)
        # print(response.json())
        response_json = response.json()
        if response.status_code != 200 or ('code' in response_json and response_json['code'] != 0):
            print("Error:", response_json)

def save_data_to_csv(data, filename):
    if not data:
        print("No data to save.")
        return
    keys = set()
    for item in data:
        keys.update(item.keys())
    with open(filename, 'w', newline='') as output_file:
        dict_writer = csv.DictWriter(output_file, fieldnames=keys)
        dict_writer.writeheader()
        dict_writer.writerows(data)

def split_events(events, batch_size):
    return [events[i:i + batch_size] for i in range(0, len(events), batch_size)]

def get_architecture():
    arch = subprocess.check_output(['uname', '-m']).strip().decode()
    if 'x86' in arch:
        return 'x86'
    elif 'arm' in arch or 'aarch64' in arch:
        return 'arm'
    else:
        raise ValueError("Unsupported architecture")
    
def get_perf_event_table():
    architecture = get_architecture()
    # 定义需要监控的事件
    if architecture == 'x86':
        all_events = [
            'frontend_retired.l1i_miss',
            'instructions',
            'mem_inst_retired.all_loads',
            'mem_load_retired.fb_hit',
            'mem_load_retired.l1_hit',
            'mem_load_retired.l1_miss',
            'l2_rqsts.code_rd_hit',
            'l2_rqsts.code_rd_miss',
            'mem_load_retired.l2_hit',
            'mem_load_retired.l2_miss',
            'mem_load_retired.l3_hit',
            'mem_load_retired.l3_miss',
            'branch-misses',
            'branch-instructions'
        ]
        table_name = 'tblFptRCkgwc8IoV'
    elif architecture == 'arm':
        all_events = [
            'l1i_cache',
            'l1i_cache_refill',
            'l1d_cache',
            'l1d_cache_refill',
            'l2i_cache',
            'l2i_cache_refill',
            'l2d_cache',
            'l2d_cache_refill',
            'branch-misses'
        ]
        table_name = 'tblUJJDqxHS4TqHR'
    return all_events, table_name

def monitor_perf(pid=0, duration=10, case_name = '', save_local = False, sudo = True):
    batch_size = 4
    all_events, table_name = get_perf_event_table()
    sub_events_list = split_events(all_events, batch_size)
    data = []
    start_ts = int(time.time())
    for i in tqdm(range(duration//3), desc="Monitoring perf progress"):
        # 每开始一轮周期，就重新记录数据
        if i%(len(sub_events_list)) == 0:
            ts = time.time()
            data_item = {'case_name': case_name, 'ts': int(ts), 'relative_ts': int(ts-start_ts)}
        sub_events = sub_events_list[i%(len(sub_events_list))]
        if sudo:
            perf_command = ['sudo']
        else:
            perf_command = []
        
        if pid != 0:
            perf_command += ['perf', 'stat']+[item for event in sub_events for item in ["-e", event]]+['-p', str(pid)]+['sleep', '1']
        else:
            perf_command += ['perf', 'stat']+[item for event in sub_events for item in ["-e", event]]+['sleep', '1']
        
        try:
            # print(' '.join(perf_command))
            # 使用 subprocess 调用 perf 并捕获输出
            result = subprocess.run(perf_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True)
            # 输出通常在 stderr 中（perf 的统计信息在 stderr 中显示）
            perf_output = result.stderr
            for line in perf_output.splitlines():
                for e in sub_events:
                    if len(line.split()) == 2 and e == line.split()[1]:
                        if 'not supported' in line:
                            data_item[e] = -1
                        else:
                            data_item[e] = int(line.split()[0].replace(',', ''))
                    elif len(line.split()) >2 and e == line.split()[1]:
                        if 'not supported' in line:
                            data_item[e] = -1
                        else:
                            data_item[e] = int(line.split()[0].replace(',', ''))
        except Exception as e:
            print(f"Error running perf: {e}")
        # 周期结束，保存数据
        if (i+1)%(len(sub_events_list)) == 0:
            data.append(data_item)
        elapsed_time = (start_ts + 3*(i + 1) - time.time())
        # print(elapsed_time)
        if elapsed_time > 0:
            time.sleep(elapsed_time)
    if not save_local:
        batch_save_lark(data, table_name)
    else:
        save_data_to_csv(data, f"result/{case_name}_perf_data.csv")
    return data

def monitor_system_usage(duration=10, case_name = '', save_local = False):
    data = []
    start_ts = time.time()
    for i in tqdm(range(duration), desc="Monitoring system usage progress"):
        # 获取当前的 CPU 使用核心数和已使用的内存量
        cpu_percent = psutil.cpu_percent(interval=0.95)
        cpu_usage = psutil.cpu_count() * (cpu_percent / 100)
        mem = psutil.virtual_memory()
        memory_usage = mem.used / (1024 * 1024)  # 转换为 MB
        memory_percent = mem.percent
        # 获取当前时间戳
        ts = time.time()
        # 输出 CPU 和内存使用情况
        data.append({
            'case_name': case_name,
            'ts': int(ts),
            'relative_ts': int(ts-start_ts),
            'cpu_usage': cpu_usage,
            'cpu_percent': cpu_percent,
            'mem_usage': memory_usage,
            'mem_percent': memory_percent
        })
        # 保证时钟对齐
        elapsed_time = (start_ts + i + 1 - time.time())
        if elapsed_time > 0:
            time.sleep(elapsed_time)
    if not save_local:
        batch_save_lark(data, 'tblNpZMhfepLIIqa')
    else:
        save_data_to_csv(data, f"result/{case_name}_system_usage_data.csv")
    return data

def get_case_name():
    save_ts = time.strftime("%Y%m%d%H%M%S")
    hostname = socket.gethostname()
    case_name = f"{hostname}_{save_ts}"
    return case_name

def run_sudo_command():
    # 提示用户输入 sudo 密码
    sudo_password = getpass.getpass("Enter your sudo password: ")

    # 使用密码执行一个简单的 sudo 命令来缓存认证信息
    try:
        # 这里使用 'echo' 命令来验证并缓存 sudo 权限
        command = ['sudo', '-S', 'echo', 'Sudo session initialized']
        result = subprocess.run(command, input=sudo_password + '\n', stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        if result.returncode == 0:
            print(result.stdout)
        else:
            print("Sudo authentication failed")
            return False
    except Exception as e:
        print(f"An error occurred: {e}")
        return False

    return True

def monitor_memory_bandwidth(duration=10, case_name = '', save_local = False, sudo = True):
    # `pqos` 命令，监控所有核心的 IPC, LLC misses, MBL, MBR 等
    if sudo:
        command = ['sudo']
    else:
        command = []
    command += ['pqos', '-m', f'all:0-{psutil.cpu_count()-1}', '-t', str(duration)]  # all:* 表示监控所有 CPU 核心
    
    try:
        # print(' '.join(command))
        # 运行 pqos 命令并捕获输出
        print('Memory BandWith Monitoring progress...')
        result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        
        # 检查命令是否执行成功
        if result.returncode == 0:
            print("Memory Bandwidth Monitoring Results Captured")
            
            # 解析结果并提取需要的信息
            # print(result.stdout)
            data = parse_pqos_output(result.stdout, case_name)
            # print(data)
            if not save_local:
                batch_save_lark(data, 'tbl5IetG3NXe6GFT')
            else:
                save_data_to_csv(data, f"result/{case_name}_mem_bandwidth_data.csv")
        else:
            print("Error in pqos command:")
            print(result.stderr)

    except Exception as e:
        print(f"An error occurred: {e}")

def convert_to_number(value):
    """
    将 '39k', '104M' 等带有 k、M 后缀的字符串转换为对应的数字
    """
    if value.endswith('k'):
        return int(float(value[:-1]) * 1_000)
    elif value.endswith('M'):
        return int(float(value[:-1]) * 1_000_000)
    else:
        return int(value)

def parse_pqos_output(output, case_name):
    """
    解析 pqos 输出，提取 IPC, LLC misses, MBL, MBR 等信息
    """
    data = []
    lines = output.splitlines()
    # 正则表达式匹配时间
    time_pattern = re.compile(r"TIME (\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2})")
    # 正则表达式匹配每个核心的监控数据
    data_pattern = re.compile(r"\s*(\d+)\s+([\d\.]+)\s+([\d\.]+[kM]?)\s+([\d\.]+)\s+([\d\.]+)\s+([\d\.]+)")
    start_ts = 0

    for line in lines:
        # 匹配时间
        time_match = time_pattern.match(line)
        if time_match:
            current_time_str = time_match.group(1)
            # 将时间字符串转换为 datetime 对象
            current_time = datetime.strptime(current_time_str, "%Y-%m-%d %H:%M:%S")
            # 转换为 UNIX 时间戳（秒）
            ts = int(time.mktime(current_time.timetuple()))
            if start_ts == 0:
                start_ts = ts
            continue

        # 匹配每个核心的监控数据
        data_match = data_pattern.match(line)
        if data_match and current_time:
            core_data = {
                'case_name': case_name,
                'ts': ts,
                'relative_ts': ts-start_ts,
                'Core': int(data_match.group(1)),
                'IPC': float(data_match.group(2)),
                'MISSES': convert_to_number(data_match.group(3)),
                'LLC(KB)': float(data_match.group(4)),
                'MBL(MB/s)': float(data_match.group(5)),
                'MBR(MB/s)': float(data_match.group(6))
            }
            data.append(core_data)
    return data

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Run performance monitoring.')
    parser.add_argument('-d', '--duration', type=int, default=10, help='Duration for performance monitoring in seconds')
    parser.add_argument('-n', '--name', type=str, default="default", help='User defined case name')
    parser.add_argument('--pid', type=int, required=False, default=0, help='PID of the process to monitor')
    parser.add_argument('--save-local', action='store_true', default=False, help='Save data locally instead of remotely')
    parser.add_argument('-p', '--perf', action='store_true', default=False, help='Run perf monitoring')
    parser.add_argument('-s', '--system', action='store_true', default=False, help='Run system usage monitoring')
    parser.add_argument('-m', '--memory', action='store_true', default=False, help='Run memory bandwidth monitoring')
    parser.add_argument('--no-sudo', action='store_true', default=False, help='Run without sudo')
    args = parser.parse_args()
    if args.name == "default":
        case_name = get_case_name()
    else:
        case_name = get_case_name()+args.name
    print(f'case_name: {case_name}')
    thread_list = []
    if not args.save_local:
        get_access_token()
    if not args.no_sudo and (args.memory or args.perf):
        if not run_sudo_command():
            raise Exception("Sudo authentication failed.")
    if args.memory:
        thread_list.append(Thread(target=monitor_memory_bandwidth, args=(args.duration, case_name, args.save_local, not args.no_sudo)))
    if args.perf:
        thread_list.append(Thread(target=monitor_perf, args=(args.pid, args.duration, case_name, args.save_local, not args.no_sudo)))
    if args.system:
        thread_list.append(Thread(target=monitor_system_usage, args=(args.duration, case_name, args.save_local)))
    for thread in thread_list:
        thread.start()
    for thread in thread_list:
        thread.join()

    