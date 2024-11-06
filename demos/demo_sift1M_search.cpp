/**
 * This source code is responsible for building the index.
 */

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <sys/time.h>

#include <faiss/AutoTune.h>
#include <faiss/index_factory.h>
#include <faiss/index_io.h>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <faiss/index_io.h>
#include <faiss/IndexFlat.h>
#include <faiss/IndexNNDescent.h>

#include <string>

/*****************************************************
 * I/O functions for fvecs and ivecs
 *****************************************************/

float* fvecs_read(const char* fname, size_t* d_out, size_t* n_out) {
    FILE* f = fopen(fname, "r");
    if (!f) {
        fprintf(stderr, "could not open %s\n", fname);
        perror("");
        abort();
    }
    int d;
    fread(&d, 1, sizeof(int), f);
    assert((d > 0 && d < 1000000) || !"unreasonable dimension");
    fseek(f, 0, SEEK_SET);
    struct stat st;
    fstat(fileno(f), &st);
    size_t sz = st.st_size;
    assert(sz % ((d + 1) * 4) == 0 || !"weird file size");
    size_t n = sz / ((d + 1) * 4);

    *d_out = d;
    *n_out = n;
    float* x = new float[n * (d + 1)];
    size_t nr = fread(x, sizeof(float), n * (d + 1), f);
    assert(nr == n * (d + 1) || !"could not read whole file");

    // shift array to remove row headers
    for (size_t i = 0; i < n; i++)
        memmove(x + i * d, x + 1 + i * (d + 1), d * sizeof(*x));

    fclose(f);
    return x;
}

// not very clean, but works as long as sizeof(int) == sizeof(float)
int* ivecs_read(const char* fname, size_t* d_out, size_t* n_out) {
    return (int*)fvecs_read(fname, d_out, n_out);
}

double elapsed() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return tv.tv_sec + tv.tv_usec * 1e-6;
}

void search_HNSW_index(){
    double t0 = elapsed();

    size_t d, nq;
    float* xq;

    // Load the pre-built index from disk
    faiss::Index * index = faiss::read_index("sift1M/sift1M_HNSW_index.faissindex");

    {
        printf("[%.3f s] Loading queries\n", elapsed() - t0);

        size_t d2;
        xq = fvecs_read("sift1M/sift_query.fvecs", &d2, &nq);
        printf("%ld\n%ld\n",d,d2);
        //assert(d == d2 || !"query does not have same dimension as train set");
    }

    size_t k;         // nb of results per query in the GT
    faiss::idx_t* gt; // nq * k matrix of ground-truth nearest-neighbors

    {
        printf("[%.3f s] Loading ground truth for %ld queries\n",
               elapsed() - t0,
               nq);

        // load ground-truth and convert int to long
        size_t nq2;
        int* gt_int = ivecs_read("sift1M/sift_groundtruth.ivecs", &k, &nq2);
        assert(nq2 == nq || !"incorrect nb of ground truth entries");

        gt = new faiss::idx_t[k * nq];
        for (int i = 0; i < k * nq; i++) {
            gt[i] = gt_int[i];
        }
        delete[] gt_int;
    }

    // { // Perform a search using the loaded index

    //     printf("[%.3f s] Perform a search on %ld queries\n",
    //            elapsed() - t0,
    //            nq);

        // output buffers
    //     faiss::idx_t* I = new faiss::idx_t[nq * k];
    //     float* D = new float[nq * k];

    //     index->search(nq, xq, k, D, I);

    //     printf("[%.3f s] Compute recalls\n", elapsed() - t0);

    //     // evaluate result by hand.
    //     int n_1 = 0, n_10 = 0, n_100 = 0;
    //     for (int i = 0; i < nq; i++) {
    //         int gt_nn = gt[i * k];
    //         for (int j = 0; j < k; j++) {
    //             if (I[i * k + j] == gt_nn) {
    //                 if (j < 1)
    //                     n_1++;
    //                 if (j < 10)
    //                     n_10++;
    //                 if (j < 100)
    //                     n_100++;
    //             }
    //         }
    //     }
    //     printf("R@1 = %.4f\n", n_1 / float(nq));
    //     printf("R@10 = %.4f\n", n_10 / float(nq));
    //     printf("R@100 = %.4f\n", n_100 / float(nq));

    //     delete[] I;
    //     delete[] D;
    // }

    { // Perform a 100-loop times seach to using perf
        int loop_cnt = 0;
        printf("[%.3f s] Perform 100-times search on %ld queries\n",
               elapsed() - t0,
               nq);

        // output buffers
        faiss::idx_t* I = new faiss::idx_t[nq * k];
        float* D = new float[nq * k];

        for(;loop_cnt<100;loop_cnt++){
            index->search(nq, xq, k, D, I);
            if(loop_cnt % 10 == 0){
                printf("[%.3f s] Complete %dth search on %ld queries\n",    
                    elapsed() - t0,
                    loop_cnt,
                    nq);
            }
        
        }
        printf("[%.3f s] Completed all 100-times search on %ld queries\n",
                elapsed() - t0,
                nq);

        delete[] I;
        delete[] D;
    }

    delete[] xq;
    delete[] gt;
    delete index;


}

void search_index_from_file(std::string filename){
    double t0 = elapsed();

    size_t d, nq;
    float* xq;

    // Load the pre-built index from disk
    faiss::Index * index = faiss::read_index(filename.c_str());

    {
        printf("[%.3f s] Loading queries\n", elapsed() - t0);

        size_t d2;
        xq = fvecs_read("sift1M/sift_query.fvecs", &d2, &nq);
        printf("%ld\n%ld\n",d,d2);
        //assert(d == d2 || !"query does not have same dimension as train set");
    }

    size_t k;         // nb of results per query in the GT
    faiss::idx_t* gt; // nq * k matrix of ground-truth nearest-neighbors

    {
        printf("[%.3f s] Loading ground truth for %ld queries\n",
               elapsed() - t0,
               nq);

        // load ground-truth and convert int to long
        size_t nq2;
        int* gt_int = ivecs_read("sift1M/sift_groundtruth.ivecs", &k, &nq2);
        assert(nq2 == nq || !"incorrect nb of ground truth entries");

        gt = new faiss::idx_t[k * nq];
        for (int i = 0; i < k * nq; i++) {
            gt[i] = gt_int[i];
        }
        delete[] gt_int;
    }

    { // Perform a 100-loop times seach to using perf
        int loop_cnt = 0;
        printf("[%.3f s] Perform 100-times search on %ld queries\n",
               elapsed() - t0,
               nq);

        // output buffers
        faiss::idx_t* I = new faiss::idx_t[nq * k];
        float* D = new float[nq * k];

        for(;loop_cnt<100;loop_cnt++){
            index->search(nq, xq, k, D, I);
            if(loop_cnt % 10 == 0){
                printf("[%.3f s] Complete %dth search on %ld queries\n",    
                    elapsed() - t0,
                    loop_cnt,
                    nq);
            }
        
        }
        printf("[%.3f s] Completed all 100-times search on %ld queries\n",
                elapsed() - t0,
                nq);

        delete[] I;
        delete[] D;
    }

    delete[] xq;
    delete[] gt;
    delete index;
}


int main() {
    

    // search_index_from_file("sift1M/sift1M_IVFFlat_index.faissindex");
    

    search_index_from_file("sift1M/sift1M_NSG_index.faissindex");

    // search_index_from_file("sift1M/sift1M_IVFFlat_index_InnerProduct.faissindex");

    // search_index_from_file("sift1M/sift1M_HNSW_index.faissindex");

    
    return 0;
}

// ./build/demos/demo_sift1M_search


// pgrep -f demo_sift1M_search

// python3 metric.py -d 30 --pid 31857 -p -s -m --save-local
