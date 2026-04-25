# llama.cpp 内嵌 Demo（GGUF 打包进二进制，无 HTTP Server）

这个示例展示了：

- `llama.cpp` 作为库集成进你的程序
- `GGUF` 在**构建期通过 `.incbin` 打包进最终二进制**
- 运行时自动把内嵌模型落到临时文件，再用 `llama_model_load_from_file()` 加载

> 说明：llama.cpp 当前主流加载 API 以文件路径为入口，因此这里采用“内嵌字节 -> 临时文件”的落地方式。

## 目录

- `CMakeLists.txt`：引入 `llama.cpp`，并配置内嵌模型汇编文件生成。
- `src/embedded_model.S.in`：通过 `.incbin` 把 GGUF 原样塞进可执行文件。
- `src/main.cpp`：`AnalyzerLLM` 封装与 `AnalyzeCodeWithLLM(prompt)`。

## 1) 编译 llama.cpp（一次性）

```bash
git clone https://github.com/ggml-org/llama.cpp
cd llama.cpp
cmake -B build -DLLAMA_BUILD_SERVER=OFF -DLLAMA_BUILD_EXAMPLES=OFF
cmake --build build -j
```

## 2) 编译本 demo（把 GGUF 打包到二进制）

假设：

- demo 在 `/workspace/LLMEngine`
- llama.cpp 在 `/workspace/llama.cpp`
- 模型在 `/workspace/Qwen3.5-0.8B-Q4_K_M.gguf`

```bash
cd /workspace/LLMEngine
cmake -S . -B build \
  -DLLAMA_CPP_DIR=/workspace/llama.cpp \
  -DEMBED_GGUF_FILE=/workspace/Qwen3.5-0.8B-Q4_K_M.gguf
cmake --build build -j
```

## 3) 运行

```bash
./build/analyzer_demo
```

## 特性

- 不开放端口，不依赖 HTTP。
- 模型随可执行文件分发（单文件部署）。
- 可直接在业务内暴露 `AnalyzeCodeWithLLM()` 作为内部 API。
