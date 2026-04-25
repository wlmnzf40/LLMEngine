cmake_minimum_required(VERSION 3.20)
project(llama_analyzer_demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 指向你本地 llama.cpp 源码目录，例如:
# cmake -S . -B build -DLLAMA_CPP_DIR=/path/to/llama.cpp
set(LLAMA_CPP_DIR "" CACHE PATH "Path to llama.cpp source tree")

if(NOT LLAMA_CPP_DIR)
  message(FATAL_ERROR "Please set -DLLAMA_CPP_DIR=/path/to/llama.cpp")
endif()

add_subdirectory(${LLAMA_CPP_DIR} ${CMAKE_BINARY_DIR}/llama.cpp)

add_executable(analyzer_demo src/main.cpp)
target_link_libraries(analyzer_demo PRIVATE llama ggml)
target_include_directories(analyzer_demo PRIVATE ${LLAMA_CPP_DIR}/include)
