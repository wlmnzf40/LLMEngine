#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "llama.h"

class AnalyzerLLM {
 public:
  explicit AnalyzerLLM(const std::string& model_path) {
    llama_backend_init();

    llama_model_params model_params = llama_model_default_params();
    model_ = llama_model_load_from_file(model_path.c_str(), model_params);
    if (!model_) {
      throw std::runtime_error("Failed to load model: " + model_path);
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
    // 1) tokenize
    std::vector<llama_token> prompt_tokens(prompt.size() + 16);
    int n_prompt = llama_tokenize(
        model_, prompt.c_str(), static_cast<int32_t>(prompt.size()), prompt_tokens.data(),
        static_cast<int32_t>(prompt_tokens.size()), true, false);
    if (n_prompt < 0) {
      throw std::runtime_error("Token buffer too small, increase prompt_tokens size");
    }
    prompt_tokens.resize(n_prompt);

    // 2) prefill
    llama_batch batch = llama_batch_get_one(prompt_tokens.data(), static_cast<int32_t>(prompt_tokens.size()));
    if (llama_decode(ctx_, batch) != 0) {
      throw std::runtime_error("llama_decode failed on prompt prefill");
    }

    // 3) autoregressive decode
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

      // detokenize piece
      char piece[16] = {0};
      int n_piece = llama_token_to_piece(model_, next, piece, sizeof(piece), 0, true);
      if (n_piece > 0) {
        output.append(piece, n_piece);
      }

      // feed next token
      batch = llama_batch_get_one(&next, 1);
      if (llama_decode(ctx_, batch) != 0) {
        throw std::runtime_error("llama_decode failed on generation step");
      }
    }

    return output;
  }

 private:
  llama_model* model_ = nullptr;
  llama_context* ctx_ = nullptr;
};

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " /path/to/model.gguf\n";
    return 1;
  }

  try {
    const std::string model_path = argv[1];
    AnalyzerLLM analyzer(model_path);

    const std::string prompt =
        "You are a strict static code analyzer. Analyze the code and return issues with severity.\\n"
        "Code:\\n"
        "int div(int a,int b){return a/b;}";

    std::string result = analyzer.AnalyzeCodeWithLLM(prompt, 200);
    std::cout << "=== Analyzer Result ===\n" << result << "\n";
  } catch (const std::exception& ex) {
    std::cerr << "Error: " << ex.what() << "\n";
    return 2;
  }

  return 0;
}
