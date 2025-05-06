find_package(Boost COMPONENTS program_options)

add_executable(dis_cmp_ip EXCLUDE_FROM_ALL dis_cmp.cpp)
target_link_libraries(dis_cmp_ip PRIVATE faiss)