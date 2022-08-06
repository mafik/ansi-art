#pragma once

#include <string>

namespace maf {

class AnsiArt {
public:
  static AnsiArt *New();
  
  virtual ~AnsiArt(){};
  virtual std::string LoadTTF(const uint8_t *data, size_t size) = 0;
  virtual void LoadImage(int width, int height, const uint8_t *rgba_bytes) = 0;
  virtual void Render() = 0;

  virtual void StartRender(int n_threads) = 0;
  virtual float GetRenderProgress() = 0;
  virtual void CancelRender() = 0;

  int width = 80;
  std::string forbidden_characters = "";

  std::string glyphs_utf8;       // populated by LoadTTF
  std::string result_c;          // populated by Render
  std::string result_bash;       // populated by Render
  std::string result_raw;        // populated by Render
  int result_rgba_width;         // populated by Render
  int result_rgba_height;        // populated by Render
  std::string result_rgba_bytes; // populated by Render
};

} // namespace maf
