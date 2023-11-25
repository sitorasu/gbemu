# 本当にこのバージョンが必要なのかは知らない
cmake_minimum_required(VERSION 3.26)

project(gbemu)

file(GLOB SRCS "src/*.cc")
add_executable(gbemu ${SRCS})
target_compile_features(gbemu PRIVATE cxx_std_17)
target_compile_options(gbemu PRIVATE -Wall -Wextra)
