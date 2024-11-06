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

void build_HNSW_index(){
    double t0 = elapsed();

    // this is typically the fastest one.
    const char* index_key = "HNSW,Flat";
    std::string index_file_name = "";

    faiss::index_factory_verbose = 1;

    faiss::Index* index;

    size_t d;

    {
        printf("[%.3f s] Loading train set\n", elapsed() - t0);

        size_t nt;
        float* xt = fvecs_read("sift1M/sift_learn.fvecs", &d, &nt);

        printf("[%.3f s] Preparing index \"%s\" d=%ld\n",
               elapsed() - t0,
               index_key,
               d);
        index = faiss::index_factory(d, index_key);

        printf("[%.3f s] Training on %ld vectors\n", elapsed() - t0, nt);

        index->train(nt, xt);
        delete[] xt;
    }

    {
        printf("[%.3f s] Loading database\n", elapsed() - t0);

        size_t nb, d2;
        float* xb = fvecs_read("sift1M/sift_base.fvecs", &d2, &nb);
        assert(d == d2 || !"dataset does not have same dimension as train set");

        printf("[%.3f s] Indexing database, size %ld*%ld\n",
               elapsed() - t0,
               nb,
               d);

        index->add(nb, xb);

        delete[] xb;
    }

    // Save the built index to disk
    faiss::write_index(index, "sift1M/sift1M_HNSW_index.faissindex");
    printf("[%.3f s] Saved index to file.\n",
               elapsed() - t0);

    delete index;


}


void build_IVFFlat_index(){
    double t0 = elapsed();

    // this is typically the fastest one.
    const char* index_key = "IVF4096,Flat";
    std::string index_file_name = "";

    faiss::index_factory_verbose = 1;

    faiss::Index* index;

    size_t d;

    {
        printf("[%.3f s] Loading train set\n", elapsed() - t0);

        size_t nt;
        float* xt = fvecs_read("sift1M/sift_learn.fvecs", &d, &nt);

        printf("[%.3f s] Preparing index \"%s\" d=%ld\n",
               elapsed() - t0,
               index_key,
               d);
        index = faiss::index_factory(d, index_key);

        printf("[%.3f s] Training on %ld vectors\n", elapsed() - t0, nt);

        index->train(nt, xt);
        delete[] xt;
    }

    {
        printf("[%.3f s] Loading database\n", elapsed() - t0);

        size_t nb, d2;
        float* xb = fvecs_read("sift1M/sift_base.fvecs", &d2, &nb);
        assert(d == d2 || !"dataset does not have same dimension as train set");

        printf("[%.3f s] Indexing database, size %ld*%ld\n",
               elapsed() - t0,
               nb,
               d);

        index->add(nb, xb);

        delete[] xb;
    }

    // Save the built index to disk
    faiss::write_index(index, "sift1M/sift1M_IVFFlat_index.faissindex");
    printf("[%.3f s] Saved index to file.\n",
               elapsed() - t0);

    delete index;


}


void build_NSG_index(){
    double t0 = elapsed();

    // this is typically the fastest one.
    // const char* index_key = "IVF4096,Flat";
    const char* index_key = "NSG,Flat";
    std::string index_file_name = "";

    faiss::index_factory_verbose = 1;

    faiss::Index* index;

    size_t d;

    {
        printf("[%.3f s] Loading train set\n", elapsed() - t0);

        size_t nt;
        float* xt = fvecs_read("sift1M/sift_learn.fvecs", &d, &nt);

        printf("[%.3f s] Preparing index \"%s\" d=%ld\n",
               elapsed() - t0,
               index_key,
               d);
        index = faiss::index_factory(d, index_key,faiss::METRIC_L2);
        // index = faiss::index_factory

        printf("[%.3f s] Training on %ld vectors\n", elapsed() - t0, nt);

        index->train(nt, xt);
        delete[] xt;
    }

    {
        printf("[%.3f s] Loading database\n", elapsed() - t0);

        size_t nb, d2;
        float* xb = fvecs_read("sift1M/sift_base.fvecs", &d2, &nb);
        assert(d == d2 || !"dataset does not have same dimension as train set");

        printf("[%.3f s] Indexing database, size %ld*%ld\n",
               elapsed() - t0,
               nb,
               d);

        index->add(nb, xb);

        delete[] xb;
    }

    // Save the built index to disk
    faiss::write_index(index, "sift1M/sift1M_NSG_index.faissindex");
    printf("[%.3f s] Saved index to file.\n",
               elapsed() - t0);

    delete index;


}



void build_index(const char* description, faiss::MetricType metric, const char* outPutFileName){
    double t0 = elapsed();

    // this is typically the fastest one.
    // const char* index_key = "IVF4096,Flat";
    // const char* index_key = "NSG,Flat";
    std::string index_file_name = "";

    faiss::index_factory_verbose = 1;

    faiss::Index* index;

    size_t d;

    {
        printf("[%.3f s] Loading train set\n", elapsed() - t0);

        size_t nt;
        float* xt = fvecs_read("sift1M/sift_learn.fvecs", &d, &nt);

        printf("[%.3f s] Preparing index \"%s\" d=%ld\n",
               elapsed() - t0,
               description,
               d);
        index = faiss::index_factory(d, description,metric);
        // index = faiss::index_factory

        printf("[%.3f s] Training on %ld vectors\n", elapsed() - t0, nt);

        index->train(nt, xt);
        delete[] xt;
    }

    {
        printf("[%.3f s] Loading database\n", elapsed() - t0);

        size_t nb, d2;
        float* xb = fvecs_read("sift1M/sift_base.fvecs", &d2, &nb);
        assert(d == d2 || !"dataset does not have same dimension as train set");

        printf("[%.3f s] Indexing database, size %ld*%ld\n",
               elapsed() - t0,
               nb,
               d);

        index->add(nb, xb);

        delete[] xb;
    }

    // Save the built index to disk
    faiss::write_index(index, outPutFileName);
    printf("[%.3f s] Saved index to file.\n",
               elapsed() - t0);

    delete index;


}


int main() {
    

    // build_HNSW_index();

    // build_IVFFlat_index();

    // build_NSG_index();

    // build_index("IVF4096,Flat",faiss::METRIC_L1,"sift1M/sift1M_IVFFlat_index_L1.faissindex");

    build_index("IVF4096,Flat",faiss::METRIC_INNER_PRODUCT,"sift1M/sift1M_IVFFlat_index_InnerProduct.faissindex");

    
    return 0;
}
