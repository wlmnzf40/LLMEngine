# llama.cpp 内嵌 Demo（无 HTTP Server）

这个示例展示了如何把 `llama.cpp` 当作库集成到你自己的二进制里，并暴露内部 API：

- `AnalyzerLLM` 类负责模型加载、上下文初始化、推理与采样。
- `AnalyzeCodeWithLLM(prompt)` 是你可直接对接业务 analyzer 的函数入口。

## 目录

- `CMakeLists.txt`：通过 `add_subdirectory()` 引入外部 `llama.cpp`。
- `src/main.cpp`：最小可运行 demo。

## 1) 编译 llama.cpp（一次性）

```bash
git clone https://github.com/ggml-org/llama.cpp
cd llama.cpp
cmake -B build -DLLAMA_BUILD_SERVER=OFF -DLLAMA_BUILD_EXAMPLES=OFF
cmake --build build -j
```

## 2) 编译本 demo

假设：

- 你的 demo 在 `/workspace/LLMEngine`
- llama.cpp 在 `/workspace/llama.cpp`

```bash
cd /workspace/LLMEngine
cmake -S . -B build -DLLAMA_CPP_DIR=/workspace/llama.cpp
cmake --build build -j
```

## 3) 运行

```bash
./build/analyzer_demo /path/to/Qwen3.5-0.8B-Q4_K_M.gguf
```

## 说明

- 该 demo **不开放端口**、**不走 HTTP**。
- 你后续可把 prompt 模板、采样参数、KV cache 策略独立封装到自己的 Analyzer 模块。
