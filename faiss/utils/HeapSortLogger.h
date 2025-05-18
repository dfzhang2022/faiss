#pragma once
#include <vector>
#include <fstream>
#include <mutex>
#include <string>
#include <iostream>

class HeapSortLogger
{
public:
    struct Entry {
        float id_or_num;
        int operation_code;
    };
    static HeapSortLogger& instance() {
        static HeapSortLogger inst;
        return inst;
    }
    HeapSortLogger(/* args */);
    ~HeapSortLogger();
    std::vector<Entry> load_from_file(const std::string& filename);
    void dump_to_file(const std::string& filename) const;

    void record(float id_or_num, int operation_code) {
        if(!logger_switched_on_) {
            return;
        }
        Entry e;
        e.id_or_num = id_or_num;
        e.operation_code = operation_code;

        std::lock_guard<std::mutex> lock(mu_);
        buffer_.emplace_back(std::move(e));
    }

private:
    /* data */
    HeapSortLogger() = default;
    ~HeapSortLogger() = default;
    HeapSortLogger(const HeapSortLogger&) = delete;
    HeapSortLogger& operator=(const HeapSortLogger&) = delete;

    mutable std::mutex mu_;
    std::vector<Entry> buffer_;
    bool logger_switched_on_ = false;

};


{
}

HeapSortLogger::~HeapSortLogger()
{
}
