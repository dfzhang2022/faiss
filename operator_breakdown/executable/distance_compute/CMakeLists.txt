cmake_minimum_required(VERSION 3.10)
project(dis_cmp_l2)

set(CMAKE_CXX_STANDARD 11)

# 设置为静态编译（包括libstdc++/libgcc）
set(CMAKE_EXE_LINKER_FLAGS "-static -static-libstdc++ -static-libgcc")

# 添加 include 路径（Faiss 头文件）
include_directories(/home/user/dfzhang/faiss)

# Faiss 静态库路径
set(FAISS_LIB_PATH "/home/user/dfzhang/faiss/build/faiss/libfaiss.a")

add_executable(dis_cmp_l2 dis_cmp_l2.cpp)
# 显式链接静态库文件
target_link_libraries(dis_cmp_l2
    ${FAISS_LIB_PATH}
)

add_executable(dis_cmp_ip EXCLUDE_FROM_ALL dis_cmp_ip.cpp)
target_link_libraries(dis_cmp_ip ${FAISS_LIB_PATH})