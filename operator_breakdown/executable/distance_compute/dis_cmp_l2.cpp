#undef _OPENMP  // 禁用 OpenMP
#include <faiss/utils/distances.h>
#include <faiss/utils/simdlib_emulated.h>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <iostream>

#include <sys/time.h>

#include "faiss/utils/FvecL2sqrLogger.h"

// Usage: ./dis_cmp_l2 <path_to_trace_file>

double elapsed() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return tv.tv_sec + tv.tv_usec * 1e-6;
}

namespace dbbench {

/*********************************************************
 * Autovectorized implementations
 */
FAISS_PRAGMA_IMPRECISE_FUNCTION_BEGIN
float fvec_L2sqr(const float* x, const float* y, size_t d) {
    size_t i;
    float res = 0;
    FAISS_PRAGMA_IMPRECISE_LOOP
    for (i = 0; i < d; i++) {
        const float tmp = x[i] - y[i];
        res += tmp * tmp;
    }
    return res;
}
FAISS_PRAGMA_IMPRECISE_FUNCTION_END

FAISS_PRAGMA_IMPRECISE_FUNCTION_BEGIN
void fvec_L2sqr_batch_4(
        const float* x,
        const float* y0,
        const float* y1,
        const float* y2,
        const float* y3,
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
        const float q0 = x[i] - y0[i];
        const float q1 = x[i] - y1[i];
        const float q2 = x[i] - y2[i];
        const float q3 = x[i] - y3[i];
        d0 += q0 * q0;
        d1 += q1 * q1;
        d2 += q2 * q2;
        d3 += q3 * q3;
    }

    dis0 = d0;
    dis1 = d1;
    dis2 = d2;
    dis3 = d3;
}
FAISS_PRAGMA_IMPRECISE_FUNCTION_END

} // namespace dbbench

int main(int argc, const char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <log_file>" << std::endl;
        return 1;
    }

    std::string log_file_name = argv[1];

    FvecL2sqrLogger::instance().switch_on();
    auto a = FvecL2sqrLogger::instance().load_from_file(log_file_name);
    std::cout << "load size: " << a.size() << std::endl;

    int cnt = 0;
    double loop_begin_time = elapsed();
    for (auto entry : a) {
        float dis0, dis1, dis2, dis3;
        dbbench::fvec_L2sqr_batch_4(
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

    std::cout << "cnt size: " << cnt << std::endl;
    std::cout << "Time : " << elapsed() - loop_begin_time << std::endl;

    return 0;
}