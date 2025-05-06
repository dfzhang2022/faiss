find_package(Boost COMPONENTS program_options)

add_executable(dis_cmp_l2 EXCLUDE_FROM_ALL dis_cmp.cpp)
target_link_libraries(dis_cmp_l2 PRIVATE faiss)