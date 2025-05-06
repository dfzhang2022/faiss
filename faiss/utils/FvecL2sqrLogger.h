#pragma once
#include <vector>
#include <fstream>
#include <mutex>
#include <string>
#include <iostream>

class FvecL2sqrLogger {
public:
    struct Entry {
        size_t d;
        std::vector<float> x, y0, y1, y2, y3;
    };

    // 获取全局单例
    static FvecL2sqrLogger& instance() {
        static FvecL2sqrLogger inst;
        return inst;
    }
    void switch_on() { logger_switched_on_ = true; }
    void switch_off() { logger_switched_on_ = false; }

    // 记录一次调用输入
    void record(const float* x,
                const float* y0,
                const float* y1,
                const float* y2,
                const float* y3,
                size_t d)
    {
        if(!logger_switched_on_){
            return;
        }
        Entry e;
        e.d = d;
        e.x.assign(x, x + d);
        e.y0.assign(y0, y0 + d);
        e.y1.assign(y1, y1 + d);
        e.y2.assign(y2, y2 + d);
        e.y3.assign(y3, y3 + d);

        // std::cout<<"1 ";
        std::lock_guard<std::mutex> lock(mu_);
        buffer_.emplace_back(std::move(e));
    }

    // 写入到二进制文件
    void dump_to_file(const std::string& filename) const {
        if(!logger_switched_on_){
            std::cout<<"Logger has been switched off"<<std::endl;
            return;
        }
        std::lock_guard<std::mutex> lock(mu_);
        std::ofstream out(filename, std::ios::binary);
        std::cout<<"Entry size: "<<buffer_.size()<<std::endl;
        for (const auto& e : buffer_) {
            out.write(reinterpret_cast<const char*>(&e.d), sizeof(size_t));
            out.write(reinterpret_cast<const char*>(e.x.data()), sizeof(float) * e.d);
            out.write(reinterpret_cast<const char*>(e.y0.data()), sizeof(float) * e.d);
            out.write(reinterpret_cast<const char*>(e.y1.data()), sizeof(float) * e.d);
            out.write(reinterpret_cast<const char*>(e.y2.data()), sizeof(float) * e.d);
            out.write(reinterpret_cast<const char*>(e.y3.data()), sizeof(float) * e.d);
        }
    }
    std::vector<Entry> load_from_file(const std::string& filename) {
        if(!logger_switched_on_){
            std::cout<<"Logger has been switched off"<<std::endl;
            return std::vector<Entry>();
        }
        std::vector<Entry> entries;
        std::ifstream in(filename, std::ios::binary);
        
        if (!in.is_open()) {
            std::cerr << "Open file failed: " << filename << std::endl;
            return entries;
        }
    
        while (true) {
            size_t d;
            in.read(reinterpret_cast<char*>(&d), sizeof(d));
            
            // 检查是否到达文件末尾（正常退出）
            if (in.eof()) {
                break;
            }
            
            // 检查读取d是否失败
            if (!in) {
                std::cerr << "读取d时发生错误" << std::endl;
                break;
            }
    
            Entry entry;
            entry.d = d;
    
            // 辅助lambda函数，读取数组并检查错误
            auto read_array = [&in](std::vector<float>& vec, size_t size, const char* name) {
                vec.resize(size);
                in.read(reinterpret_cast<char*>(vec.data()), sizeof(float) * size);
                if (!in || in.gcount() != sizeof(float) * size) {
                    std::cerr << "读取" << name << "时发生错误或数据不完整" << std::endl;
                    return false;
                }
                return true;
            };
    
            // 依次读取五个数组
            if (!read_array(entry.x, d, "x")) break;
            if (!read_array(entry.y0, d, "y0")) break;
            if (!read_array(entry.y1, d, "y1")) break;
            if (!read_array(entry.y2, d, "y2")) break;
            if (!read_array(entry.y3, d, "y3")) break;
    
            entries.push_back(std::move(entry));
        }
        std::cout<<"Load entries size: "<<entries.size()<<std::endl;
    
        return entries;
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mu_);
        buffer_.clear();
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mu_);
        return buffer_.size();
    }

private:
    FvecL2sqrLogger() = default;
    ~FvecL2sqrLogger() = default;

    FvecL2sqrLogger(const FvecL2sqrLogger&) = delete;
    FvecL2sqrLogger& operator=(const FvecL2sqrLogger&) = delete;

    mutable std::mutex mu_;
    std::vector<Entry> buffer_;
    bool logger_switched_on_ = false;
    
};
