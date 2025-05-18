#include <vector>
#include <algorithm>

struct MinimaxHeap {
    int n;
    int k;
    int nvalid;

    std::vector<storage_idx_t> ids;
    std::vector<float> dis;
    typedef faiss::CMax<float, storage_idx_t> HC;

    explicit MinimaxHeap(int n) : n(n), k(0), nvalid(0), ids(n), dis(n) {}

    void push(storage_idx_t i, float v);

    float max() const;

    int size() const;

    void clear();

    int pop_min(float* vmin_out = nullptr);

    int count_below(float thresh);
};

int main() {
    MinimaxHeap(10); // create a MinimaxHeap object with size 10
}