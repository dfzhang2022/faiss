
# chmod +755 ./auto_make.sh
make -C build demo_IVFFlat_sift1M_search
make -C build demo_IVFFlat_gist1M_search

make -C build demo_IVFFlat_sift1M_build


make -C build demo_HNSW_sift1M_search
make -C build demo_HNSW_gist1M_search


make -C build demo_HNSW_sift1M_build

make -C build demo_sift1M_build

make -C build demo_sift1M_search

make -C build demo_gist1M_build

make -C build demo_gist1M_search