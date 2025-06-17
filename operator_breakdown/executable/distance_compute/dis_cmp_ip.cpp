#include <faiss/utils/distances.h>
#include <faiss/utils/simdlib_emulated.h>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <iostream>

#include <sys/time.h>
#include <sys/stat.h>

#include "faiss/utils/FvecL2sqrLogger.h"

// Usage: ./dis_cmp_ip <path_to_trace_file>


double elapsed() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return tv.tv_sec + tv.tv_usec * 1e-6;
}
bool fileExists(const std::string& path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}
namespace dbbench {

/*********************************************************
 * Autovectorized implementations
 */

FAISS_PRAGMA_IMPRECISE_FUNCTION_BEGIN
float fvec_inner_product(const float* x, const float* y, size_t d) {
    float res = 0.F;
    FAISS_PRAGMA_IMPRECISE_LOOP
    for (size_t i = 0; i != d; ++i) {
        res += x[i] * y[i];
    }
    return res;
}
FAISS_PRAGMA_IMPRECISE_FUNCTION_END


/// Special version of inner product that computes 4 distances
/// between x and yi
FAISS_PRAGMA_IMPRECISE_FUNCTION_BEGIN
void fvec_inner_product_batch_4(
        const float* __restrict x,
        const float* __restrict y0,
        const float* __restrict y1,
        const float* __restrict y2,
        const float* __restrict y3,
        const size_t d,
        float& dis0,
        float& dis1,
        float& dis2,
        float& dis3) {
    float d0 = 0;
    float d1 = 0;
    float d2 = 0;
    float d3 = 0;
    FAISS_PRAGMA_IMPRECISE_LOOP
    for (size_t i = 0; i < d; ++i) {
        d0 += x[i] * y0[i];
        d1 += x[i] * y1[i];
        d2 += x[i] * y2[i];
        d3 += x[i] * y3[i];
    }

    dis0 = d0;
    dis1 = d1;
    dis2 = d2;
    dis3 = d3;
}
FAISS_PRAGMA_IMPRECISE_FUNCTION_END

} // namespace dbbench
float a[2] = {1,2};
float b[2] = {3,4};

int main(int argc, const char** argv) {
    float res = dbbench::fvec_inner_product(a, b, 2);
    printf("%f\n", res);
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <log_file>" << std::endl;
        return 1;
    }

    std::string log_file_name = argv[1];

    FvecL2sqrLogger::instance().switch_on();
    double load_begin_time = elapsed();
    if (!fileExists(log_file_name)) {
        std::cerr << "Error: Log file '" << log_file_name << "' does not exist!" << std::endl;
        return 0;
    }
    std::cout << "Log file exists. Proceeding... File path:"<<log_file_name << std::endl;
    auto a = FvecL2sqrLogger::instance().load_from_file(log_file_name);
    std::cout << "load size: " << a.size() << std::endl;
    std::cout << "Time : " << elapsed() - load_begin_time << std::endl;

    int cnt = 0;
    double loop_begin_time = elapsed();
    for (auto entry : a) {
        float dis0, dis1, dis2, dis3;
        dbbench::fvec_inner_product_batch_4(
                entry.x.data(),
                entry.y0.data(),
                entry.y1.data(),
                entry.y2.data(),
                entry.y3.data(),
                entry.d,
                dis0,
                dis1,
                dis2,
                dis3);
        cnt++;
    }

    std::cout << "computation size: " << cnt << std::endl;
    std::cout << std::fixed;
    std::cout.precision(6); // 可根据需要调整小数位数
    std::cout << "Time : " << static_cast<double>(elapsed() - loop_begin_time) << std::endl;

    return 0;
}