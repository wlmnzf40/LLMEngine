#include <fcntl.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "llama.h"

extern "C" {
extern const unsigned char analyzer_demo_embedded_model_start[];
extern const unsigned char analyzer_demo_embedded_model_end[];
}

class EmbeddedModelFile {
 public:
  EmbeddedModelFile() {
    const std::string pattern = "/tmp/analyzer_embedded_model_XXXXXX.gguf";
    std::vector<char> tmp(pattern.begin(), pattern.end());
    tmp.push_back('\0');

    int fd = mkstemps(tmp.data(), 5);  // suffix ".gguf"
    if (fd == -1) {
      throw std::runtime_error(std::string("mkstemps failed: ") + std::strerror(errno));
    }

    path_ = tmp.data();

    const auto* begin = analyzer_demo_embedded_model_start;
    const auto* end = analyzer_demo_embedded_model_end;
    const size_t size = static_cast<size_t>(end - begin);

    size_t written = 0;
    while (written < size) {
      ssize_t n = write(fd, begin + written, size - written);
      if (n <= 0) {
        close(fd);
        unlink(path_.c_str());
        throw std::runtime_error(std::string("write embedded model failed: ") + std::strerror(errno));
      }
      written += static_cast<size_t>(n);
    }

    if (close(fd) != 0) {
      unlink(path_.c_str());
      throw std::runtime_error(std::string("close temp model file failed: ") + std::strerror(errno));
    }
  }

  ~EmbeddedModelFile() {
    if (!path_.empty()) {
      unlink(path_.c_str());
    }
  }

  const std::string& path() const { return path_; }

 private:
  std::string path_;
};

class AnalyzerLLM {
 public:
  AnalyzerLLM() : embedded_model_() {
    llama_backend_init();

    llama_model_params model_params = llama_model_default_params();
    model_ = llama_model_load_from_file(embedded_model_.path().c_str(), model_params);
    if (!model_) {
      throw std::runtime_error("Failed to load embedded model from temp file");
    }

    llama_context_params ctx_params = llama_context_default_params();
    ctx_params.n_ctx = 4096;
    ctx_ = llama_init_from_model(model_, ctx_params);
    if (!ctx_) {
      llama_model_free(model_);
      throw std::runtime_error("Failed to create llama context");
    }
  }

  ~AnalyzerLLM() {
    if (ctx_) {
      llama_free(ctx_);
    }
    if (model_) {
      llama_model_free(model_);
    }
    llama_backend_free();
  }

  std::string AnalyzeCodeWithLLM(const std::string& prompt, int max_new_tokens = 256) {
    std::vector<llama_token> prompt_tokens(prompt.size() + 16);
    int n_prompt = llama_tokenize(
        model_, prompt.c_str(), static_cast<int32_t>(prompt.size()), prompt_tokens.data(),
        static_cast<int32_t>(prompt_tokens.size()), true, false);
    if (n_prompt < 0) {
      throw std::runtime_error("Token buffer too small, increase prompt_tokens size");
    }
    prompt_tokens.resize(n_prompt);

    llama_batch batch = llama_batch_get_one(prompt_tokens.data(), static_cast<int32_t>(prompt_tokens.size()));
    if (llama_decode(ctx_, batch) != 0) {
      throw std::runtime_error("llama_decode failed on prompt prefill");
    }

    std::string output;
    for (int i = 0; i < max_new_tokens; ++i) {
      const float* logits = llama_get_logits_ith(ctx_, batch.n_tokens - 1);
      if (!logits) {
        throw std::runtime_error("Failed to get logits");
      }

      std::vector<llama_token_data> candidates;
      candidates.reserve(llama_n_vocab(model_));
      for (llama_token tok = 0; tok < llama_n_vocab(model_); ++tok) {
        candidates.push_back({tok, logits[tok], 0.0f});
      }

      llama_token_data_array candidates_p = {candidates.data(), candidates.size(), false};
      llama_sample_top_k(ctx_, &candidates_p, 40, 1);
      llama_sample_top_p(ctx_, &candidates_p, 0.9f, 1);
      llama_sample_temp(ctx_, &candidates_p, 0.7f);
      llama_token next = llama_sample_token(ctx_, &candidates_p);

      if (next == llama_token_eos(model_)) {
        break;
      }

      char piece[16] = {0};
      int n_piece = llama_token_to_piece(model_, next, piece, sizeof(piece), 0, true);
      if (n_piece > 0) {
        output.append(piece, n_piece);
      }

      batch = llama_batch_get_one(&next, 1);
      if (llama_decode(ctx_, batch) != 0) {
        throw std::runtime_error("llama_decode failed on generation step");
      }
    }

    return output;
  }

 private:
  EmbeddedModelFile embedded_model_;
  llama_model* model_ = nullptr;
  llama_context* ctx_ = nullptr;
};

int main() {
  try {
    AnalyzerLLM analyzer;

    const std::string prompt =
        "You are a strict static code analyzer. Analyze the code and return issues with severity.\n"
        "Code:\n"
        "int div(int a,int b){return a/b;}";

    std::string result = analyzer.AnalyzeCodeWithLLM(prompt, 200);
    std::cout << "=== Analyzer Result ===\n" << result << "\n";
  } catch (const std::exception& ex) {
    std::cerr << "Error: " << ex.what() << "\n";
    return 2;
  }

  return 0;
}
