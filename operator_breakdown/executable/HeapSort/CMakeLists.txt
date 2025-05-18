find_package(Boost COMPONENTS program_options)

add_executable(heap_sort EXCLUDE_FROM_ALL heap_sort.cpp)
target_link_libraries(heap_sort PRIVATE faiss)