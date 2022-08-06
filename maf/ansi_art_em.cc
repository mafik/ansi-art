#ifdef EMSCRIPTEN

// Example compilation arguments for em++:
// --preload-file maf/JuliaMono-Regular.ttf@maf/JuliaMono-Regular.ttf
// -lembind
// -sUSE_FREETYPE=1
// -sALLOW_MEMORY_GROWTH
// -sPTHREAD_POOL_SIZE=navigator.hardwareConcurrency
//
// IMPORTANT: Make sure to put some font in the ./maf/ directory and replace
//            all instances of "JuliaMono-Regular.ttf" with its name!
//            Or you can also remove the *DefaultTTF functions altogether.

#include <emscripten/bind.h>
#include <emscripten/val.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "maf/ansi_art.hh"
#include "maf/base64.hh"
#include "maf/fs.hh"

using namespace emscripten;
using namespace maf;

std::string EmLoadTTF(AnsiArt &art, std::string s) {
  return art.LoadTTF((uint8_t *)s.data(), s.size());
}

std::string EmLoadDefaultTTF(AnsiArt &art) {
  std::string err;
  std::string ttf = ReadFile("maf/JuliaMono-Regular.ttf", err);
  if (!err.empty()) {
    return err;
  }
  return EmLoadTTF(art, ttf);
}

std::string GetDefaultTTF() {
  std::string err;
  std::string ttf = ReadFile("maf/JuliaMono-Regular.ttf", err);
  if (!err.empty()) {
    printf("%s\n", err.c_str());
  }
  return "data:font/ttf;base64," +
         Base64Encode((uint8_t *)ttf.data(), ttf.size());
}

void EmLoadImage(AnsiArt &art, int width, int height, std::string rgba_bytes) {
  art.LoadImage(width, height, (uint8_t *)rgba_bytes.data());
}

emscripten::val EmGetRgbaBytes(AnsiArt &art) {
  return emscripten::val(emscripten::typed_memory_view(
      art.result_rgba_bytes.size(), art.result_rgba_bytes.data()));
}

EMSCRIPTEN_BINDINGS(unicode_ansi_art) {
  function("GetDefaultTTF", &GetDefaultTTF);
  class_<AnsiArt>("AnsiArt")
      .constructor(&AnsiArt::New, allow_raw_pointers())
      .function("LoadTTF", &EmLoadTTF)
      .function("LoadDefaultTTF", &EmLoadDefaultTTF)
      .function("LoadImage", &EmLoadImage)
      .function("Render", &AnsiArt::Render)
      .function("StartRender", &AnsiArt::StartRender)
      .function("GetRenderProgress", &AnsiArt::GetRenderProgress)
      .function("CancelRender", &AnsiArt::CancelRender)
      .property("width", &AnsiArt::width)
      .property("forbidden_characters", &AnsiArt::forbidden_characters)
      .property("glyphs_utf8", &AnsiArt::glyphs_utf8)
      .property("result_c", &AnsiArt::result_c)
      .property("result_bash", &AnsiArt::result_bash)
      .property("result_raw", &AnsiArt::result_raw)
      .property("result_rgba_width", &AnsiArt::result_rgba_width)
      .property("result_rgba_height", &AnsiArt::result_rgba_height)
      .function("result_rgba_bytes", &EmGetRgbaBytes);
}

#endif // EMSCRIPTEN
