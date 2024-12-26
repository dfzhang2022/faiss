/**
 * This source code is responsible for searching the index.
 */

#include <cassert>
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

#include <iostream>

#include <boost/program_options.hpp>
namespace po = boost::program_options;

const std::string make_program_description(const char *executable_name, const char *description)
{
    return std::string("\n")
        .append(description)
        .append("\n\n")
        .append("Usage: ")
        .append(executable_name)
        .append(" [OPTIONS]");
}



double elapsed() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return tv.tv_sec + tv.tv_usec * 1e-6;
}
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
int main(int argc, char **argv) {
    double t0 = elapsed();

    std::string data_type, dist_fn, index_file_path;

    uint32_t execute_duration;
    po::options_description desc{
        make_program_description("demo_gist1m_search", "Searches on faiss indexes on gist1M datasets.")};

    try
    {
        desc.add_options()("help,h", "Print information on arguments");

        // Required parameters
        po::options_description required_configs("Required");
        required_configs.add_options()("index_file_path", po::value<std::string>(&index_file_path)->required(),"path of index file");

        // Optional parameters
        po::options_description optional_configs("Optional");
        optional_configs.add_options()("execute_duration",
                                       po::value<uint32_t>(&execute_duration)->default_value(600),
                                       "Duration that you want search been performing(s).");
        // Merge required and optional parameters
        desc.add(required_configs).add(optional_configs);

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        if (vm.count("help"))
        {
            std::cout << desc;
            return 0;
        }
        po::notify(vm);

    }
    catch (const std::exception &ex)
    {
        std::cerr << ex.what() << '\n';
        return -1;
    }

    size_t d, nq;
    float* xq;

    // Load the pre-built index from disk
    std::cout<<"Loading index from disk... path: "<<index_file_path<<std::endl;
    faiss::Index * index = faiss::read_index(index_file_path.c_str());

    {
        printf("[%.3f s] Loading queries\n", elapsed() - t0);

        size_t d2;
        xq = fvecs_read("gist1M/gist_query.fvecs", &d2, &nq);
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
        int* gt_int = ivecs_read("gist1M/gist_groundtruth.ivecs", &k, &nq2);
        assert(nq2 == nq || !"incorrect nb of ground truth entries");

        gt = new faiss::idx_t[k * nq];
        for (int i = 0; i < k * nq; i++) {
            gt[i] = gt_int[i];
        }
        delete[] gt_int;
    }



    { // Perform a {execute_duration} seach to using perf

        
        double loop_duration = execute_duration;
        size_t execute_cnt = 0;

        printf("[%.3f s] Perform %.1f(s)-duration search on %ld queries\n",
               elapsed() - t0,
                loop_duration,
                nq);

        // output buffers
        faiss::idx_t* I = new faiss::idx_t[nq * k];
        float* D = new float[nq * k];

        double loop_begin_time = elapsed();
        double tmp_time = loop_begin_time;
        for(;elapsed() - loop_begin_time<loop_duration;){
            index->search(nq, xq, k, D, I);
            execute_cnt++;
            if(elapsed()-tmp_time > 30){
                printf("[%.3f s] Complete %.3fs search on %ld queries\n",    
                    elapsed() - t0,
                    elapsed() - loop_begin_time,
                    nq);
                tmp_time = elapsed();
            }
        
        }

        printf("[%.3f s] Completed all %.3f-duration search on %ld queries for %ld times.\n",
               elapsed() - t0,
                loop_duration,
                nq,
                execute_cnt);

        delete[] I;
        delete[] D;
    }

    delete[] xq;
    delete[] gt;
    delete index;
    return 0;
}
