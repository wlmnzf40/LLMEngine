cmake_minimum_required(VERSION 3.20)
project(llama_analyzer_demo LANGUAGES CXX ASM)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 指向你本地 llama.cpp 源码目录，例如:
# cmake -S . -B build -DLLAMA_CPP_DIR=/path/to/llama.cpp
set(LLAMA_CPP_DIR "" CACHE PATH "Path to llama.cpp source tree")

# 指向要打包进二进制的 GGUF 模型文件
# cmake -S . -B build -DEMBED_GGUF_FILE=/path/to/model.gguf
set(EMBED_GGUF_FILE "" CACHE FILEPATH "Path to GGUF model to embed into binary")

if(NOT LLAMA_CPP_DIR)
  message(FATAL_ERROR "Please set -DLLAMA_CPP_DIR=/path/to/llama.cpp")
endif()

if(NOT EMBED_GGUF_FILE)
  message(FATAL_ERROR "Please set -DEMBED_GGUF_FILE=/path/to/model.gguf")
endif()

if(NOT EXISTS ${EMBED_GGUF_FILE})
  message(FATAL_ERROR "EMBED_GGUF_FILE does not exist: ${EMBED_GGUF_FILE}")
endif()

add_subdirectory(${LLAMA_CPP_DIR} ${CMAKE_BINARY_DIR}/llama.cpp)

set(EMBED_GGUF_ABS "${EMBED_GGUF_FILE}")
if(NOT IS_ABSOLUTE "${EMBED_GGUF_ABS}")
  get_filename_component(EMBED_GGUF_ABS "${EMBED_GGUF_FILE}" ABSOLUTE)
endif()

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/src/embedded_model.S.in
  ${CMAKE_CURRENT_BINARY_DIR}/embedded_model.S
  @ONLY
)

add_executable(
  analyzer_demo
  src/main.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/embedded_model.S
)

target_link_libraries(analyzer_demo PRIVATE llama ggml)
target_include_directories(analyzer_demo PRIVATE ${LLAMA_CPP_DIR}/include)
