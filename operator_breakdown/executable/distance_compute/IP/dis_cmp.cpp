#include <faiss/utils/distances.h>
#include <faiss/utils/simdlib_emulated.h>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "faiss/utils/FvecL2sqrLogger.h"

// #define FAISS_PRAGMA_IMPRECISE_LOOP
// #define FAISS_PRAGMA_IMPRECISE_FUNCTION_BEGIN \
//     _Pragma("GCC push_options") \
//     _Pragma("GCC optimize
//     (\"unroll-loops,associative-math,no-signed-zeros\")")
// #define FAISS_PRAGMA_IMPRECISE_FUNCTION_END \
//     _Pragma("GCC pop_options")
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

} // namespace dbbench
float a[2] = {1,2};
float b[2] = {3,4};

int main(int argc, const char** argv) {
    float res = dbbench::fvec_inner_product(a, b, 2);
    printf("%f\n", res);
    return 0;
}