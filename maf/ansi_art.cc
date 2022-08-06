#include "maf/ansi_art.hh"

#include <cmath>
#include <ft2build.h>
#include <string>
#include <vector>
#include FT_FREETYPE_H
#include <pthread.h>

#include "maf/ansi.hh"
#include "maf/unicode.hh"
#include "maf/str.hh"

namespace maf {

struct Pixel {
  uint8_t r, g, b, a;
  std::string AnsiColor() const {
    return "2;" + std::to_string(r) + ";" + std::to_string(g) + ";" +
          std::to_string(b);
  }
  std::string AnsiBg() const {
    return "\033[48;" + AnsiColor() + "m";
  }
  std::string AnsiFg() const {
    return "\033[38;" + AnsiColor() + "m";
  }
};

struct vec4 {
  float r, g, b, a;
  vec4() : r(0), g(0), b(0), a(0) {}
  vec4(float r, float g, float b, float a) : r(r), g(g), b(b), a(a) {}
  vec4(const Pixel &p)
      : r(p.r / 255.), g(p.g / 255.), b(p.b / 255.), a(p.a / 255.) {}
  vec4 operator*(float f) const { return vec4(r * f, g * f, b * f, a * f); }
  vec4 &operator+=(const vec4 &other) {
    r += other.r;
    g += other.g;
    b += other.b;
    a += other.a;
    return *this;
  }
  vec4 &operator/=(float f) {
    r /= f;
    g /= f;
    b /= f;
    a /= f;
    return *this;
  }
  vec4 &operator*=(float f) {
    r *= f;
    g *= f;
    b *= f;
    a *= f;
    return *this;
  }
  vec4 operator+(const vec4 &other) const {
    return vec4(r + other.r, g + other.g, b + other.b, a + other.a);
  }
  vec4 operator-(const vec4 &other) const {
    return vec4(r - other.r, g - other.g, b - other.b, a - other.a);
  }
  vec4 operator*(const vec4 &other) const {
    return vec4(r * other.r, g * other.g, b * other.b, a * other.a);
  }
  float sum() const { return r + g + b + a; }
  vec4 abs() const { return vec4(fabs(r), fabs(g), fabs(b), fabs(a)); }
  Pixel pixel() const {
    return Pixel{(uint8_t)(r * 255), (uint8_t)(g * 255), (uint8_t)(b * 255),
                 (uint8_t)(a * 255)};
  }
};

class AnsiArtImpl : public AnsiArt {

  struct Glyph {
    int unicode;
    std::string utf8;
    uint8_t *pixels;
  };

  struct Font {
    int glyph_height;
    int glyph_width;
    float aspect;
    std::vector<Glyph> glyphs;
  };

  Font font;

  std::string LoadTTF(const uint8_t *data, size_t size) override {
    glyphs_utf8 = "";
    FT_Library library;
    FT_Error ft_error = FT_Init_FreeType(&library);
    if (ft_error) {
      return "Couldn't initialize freetype: " + std::to_string(ft_error);
    }
    FT_Face face;
    ft_error =
        FT_New_Memory_Face(library, (FT_Byte *)data, (FT_Long)size, 0, &face);
    if (ft_error) {
      return "Couldn't load font";
    }
    int font_size_pt = 15;
    ft_error = FT_Set_Char_Size(face, /* handle to face object           */
                                0,    /* char_width in 1/64th of points  */
                                font_size_pt *
                                    64, /* char_height in 1/64th of points */
                                72,     /* horizontal device resolution    */
                                72);    /* vertical device resolution      */

    font.glyph_width = face->size->metrics.max_advance / 64;
    font.glyph_height = face->size->metrics.height / 64;
    font.aspect = float(font.glyph_height) / font.glyph_width;
    font.glyphs.clear();

    // The unicode '█' starts at the top of the character cell.
    // We use its `bitmap_top` offset to find the baseline position.
    uint32_t block_index = FT_Get_Char_Index(face, 0x2588);
    FT_Load_Glyph(face, block_index, FT_LOAD_RENDER);
    int baseline = face->glyph->bitmap_top;

    FT_ULong charcode;
    FT_UInt gindex;
    charcode = FT_Get_First_Char(face, &gindex);
    int n_chars = 0;
    while (gindex != 0) {

      if (charcode == 9 || charcode == 13) {
        charcode = FT_Get_Next_Char(face, charcode, &gindex);
        continue;
      }
      char buf[80];
      FT_Get_Glyph_Name(face, gindex, buf, sizeof(buf));
      std::string name = buf;

      std::string utf8 = UnicodeToUTF8(charcode);

      if (name.find(".") == std::string::npos &&
          forbidden_characters.find(utf8) == std::string::npos) {

        ft_error = FT_Load_Glyph(face, gindex, FT_LOAD_RENDER);
        if (ft_error) {
          return "FT_Load_Glyph " + std::to_string(ft_error);
        }
        FT_GlyphSlot glyph = face->glyph;

        if (glyph->advance.x / 64 == font.glyph_width) {
          auto bitmap = glyph->bitmap;
          if (bitmap.pixel_mode != FT_PIXEL_MODE_GRAY) {
            return "FT_Bitmap is not FT_PIXEL_MODE_GRAY";
          }
          font.glyphs.emplace_back();
          auto &new_glyph = font.glyphs.back();
          new_glyph.unicode = charcode;
          new_glyph.utf8 = utf8;
          n_chars += 1;

          int n = font.glyph_width * font.glyph_height;
          new_glyph.pixels = new uint8_t[n];
          for (int i = 0; i < n; ++i)
            new_glyph.pixels[i] = 0;

          for (int y = 0; y < bitmap.rows; ++y) {
            for (int x = 0; x < bitmap.width; ++x) {
              int tile_x = glyph->bitmap_left + x;
              if (tile_x >= font.glyph_width)
                continue;
              if (tile_x < 0)
                continue;
              int tile_y = baseline - glyph->bitmap_top + y;
              if (tile_y >= font.glyph_height)
                continue;
              if (tile_y < 0)
                continue;
              int i = tile_y * font.glyph_width + tile_x;
              new_glyph.pixels[i] = bitmap.buffer[y * bitmap.pitch + x];
            }
          }
          glyphs_utf8 += utf8;
        }
      }
      charcode = FT_Get_Next_Char(face, charcode, &gindex);
    }

    return "";
  };

  struct Image {
    int width, height;
    std::vector<Pixel> pixels;
    vec4 Read(float x, float y) {
      if (y >= height || y < 0 || x >= width || x < 0) {
        return vec4(0, 0, 0, 0);
      }
      int nearest_x = (int)roundf(x);
      int nearest_y = (int)roundf(y);
      int i = nearest_y * width + nearest_x;
      Pixel &p = pixels[i];
      return vec4(p.r / 255.f, p.g / 255.f, p.b / 255.f, p.a / 255.f);
    }
  };

  Image image;

  void LoadImage(int width, int height, const uint8_t *rgba_bytes) override {
    image.width = width;
    image.height = height;
    image.pixels.resize(width * height);
    memcpy(&image.pixels[0], rgba_bytes, 4 * width * height);
  }

  struct Task {
    int char_x;
    int char_y;
  };

  struct TaskResult {
    int char_x;
    int char_y;
    vec4 fg;
    vec4 bg;
    Glyph *glyph;
  };

  pthread_t renderer = 0;
  int worker_count = 8;
  std::vector<pthread_t> workers;
  std::vector<Task> tasks;
  std::vector<TaskResult> task_results;
  float progress = 0;
  pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;

  static void *RenderWorkerThread(void *arg) {
    auto self = (AnsiArtImpl *)arg;
    self->RenderWorker();
    pthread_exit(nullptr);
  }

  void RenderWorker() {
    float height = float(image.height) * width / image.width / font.aspect;
    float img_char_width = float(image.width) / width;
    float img_char_height = float(image.height) / height;
    Pixel *result_rgba = (Pixel *)&result_rgba_bytes[0];

    while (!tasks.empty()) {

      pthread_mutex_lock(&mut);
      auto task = tasks.back();
      tasks.pop_back();
      pthread_mutex_unlock(&mut);
      int char_x = task.char_x;
      int char_y = task.char_y;

      TaskResult result;
      result.char_x = char_x;
      result.char_y = char_y;

      float img_x_begin = char_x * img_char_width;
      float img_x_end = (char_x + 1) * img_char_width;
      float img_y_begin = char_y * img_char_height;
      float img_y_end = (char_y + 1) * img_char_height;

      Glyph *best_glyph = nullptr;
      float best_err = 999999.f;
      vec4 best_fg;
      vec4 best_bg;

      for (auto &glyph : font.glyphs) {
        if (forbidden_characters.find(glyph.utf8) != std::string::npos)
          continue;
        float fg_sum = 0;
        vec4 fg_col = {};
        float bg_sum = 0;
        vec4 bg_col = {};
        for (int font_char_x = 0; font_char_x < font.glyph_width;
             ++font_char_x) {
          for (int font_char_y = 0; font_char_y < font.glyph_height;
               ++font_char_y) {
            float fg =
                glyph.pixels[font_char_x + font_char_y * font.glyph_width] /
                255.f;
            float bg = 1.f - fg;
            fg_sum += fg;
            bg_sum += bg;
            float img_x = img_x_begin + img_char_width * (font_char_x + 0.5f) /
                                            font.glyph_width;
            float img_y = img_y_begin + img_char_height * (font_char_y + 0.5f) /
                                            font.glyph_height;
            vec4 col = image.Read(img_x, img_y);
            fg_col += col * fg;
            bg_col += col * bg;
          }
        }
        if (fg_sum) {
          fg_col /= fg_sum;
        }
        fg_col.a = 1;
        if (bg_sum) {
          bg_col /= bg_sum;
        }
        if (bg_col.a < 0.2) {
          bg_col.a = 0;
        } else {
          bg_col.a = 1;
        }
        bg_col *= bg_col.a; // premultiply

        float error = 0;
        for (int font_char_x = 0; font_char_x < font.glyph_width;
             ++font_char_x) {
          for (int font_char_y = 0; font_char_y < font.glyph_height;
               ++font_char_y) {
            float fg =
                glyph.pixels[font_char_x + font_char_y * font.glyph_width] /
                255.f;
            float bg = 1.f - fg;
            float img_x = img_x_begin + img_char_width * (font_char_x + 0.5f) /
                                            font.glyph_width;
            float img_y = img_y_begin + img_char_height * (font_char_y + 0.5f) /
                                            font.glyph_height;
            vec4 col = image.Read(img_x, img_y);
            col *= col.a; // premultiply
            vec4 x = fg_col * fg + bg_col * bg;
            vec4 d = col - x;
            error += (d * d).sum();
          }
        }
        if (error < best_err) {
          best_err = error;
          best_glyph = &glyph;
          best_fg = fg_col;
          best_bg = bg_col;
        }
      }

      result.fg = best_fg;
      result.bg = best_bg;
      result.glyph = best_glyph;

      pthread_mutex_lock(&mut);
      task_results.push_back(result);
      progress =
          task_results.size() / float(task_results.size() + tasks.size() + 1);
      pthread_mutex_unlock(&mut);

      // Blit the character onto result_rgba_bytes
      for (int font_char_x = 0; font_char_x < font.glyph_width; ++font_char_x) {
        for (int font_char_y = 0; font_char_y < font.glyph_height;
             ++font_char_y) {
          int result_x = char_x * font.glyph_width + font_char_x;
          int result_y = char_y * font.glyph_height + font_char_y;
          Pixel &result_pixel =
              result_rgba[result_x + result_y * result_rgba_width];
          float fg =
              best_glyph->pixels[font_char_x + font_char_y * font.glyph_width] /
              255.f;
          float bg = 1.f - fg;
          result_pixel = (best_fg * fg + best_bg * bg).pixel();
        }
      }
      pthread_testcancel();
    }
  }

  void Render() override {

    result_raw = "";
    result_rgba_bytes.clear();
    result_c = "";
    result_bash = "";

    float fheight = float(image.height) * width / image.width / font.aspect;
    float img_char_width = float(image.width) / width;
    float img_char_height = float(image.height) / fheight;
    int height = (int)ceil(fheight);

    int n_chars = width * height;

    result_rgba_width = width * font.glyph_width;
    result_rgba_height = height * font.glyph_height;
    result_rgba_bytes.resize(result_rgba_width * result_rgba_height * 4);
    Pixel *result_rgba = (Pixel *)&result_rgba_bytes[0];

    task_results.clear();
    tasks.clear();
    for (int char_y = 0; char_y < height; ++char_y) {
      for (int char_x = 0; char_x < width; ++char_x) {
        tasks.push_back({char_x, char_y});
      }
    }

    std::sort(tasks.begin(), tasks.end(), [&](Task &a, Task &b) {
      auto dist = [&](Task &t) {
        float dx = t.char_x - (float)(width) / 2;
        float dy = t.char_y - fheight / 2;
        return dx * dx / font.aspect + dy * dy * font.aspect;
      };
      float da = dist(a);
      float db = dist(b);
      if (da == db) {
        if (a.char_x == b.char_x) {
          return a.char_y < b.char_y;
        }
        return a.char_x < b.char_x;
      }
      return da > db;
    });

    workers.resize(worker_count);
    for (int i = 0; i < worker_count; ++i) {
      pthread_create(&workers[i], nullptr, RenderWorkerThread, this);
    }
    bool cancelled = false;
    for (int i = 0; i < worker_count; ++i) {
      void* ret = nullptr;
      pthread_join(workers[i], &ret);
      if (ret == PTHREAD_CANCELED) {
        cancelled = true;
      }
    }
    workers.clear();

    if (cancelled) {
      tasks.clear();
      task_results.clear();
      bzero(result_rgba_bytes.data(), result_rgba_bytes.size());
      return;
    }

    TaskResult *result_idx[height][width];
    for (auto &result : task_results) {
      result_idx[result.char_y][result.char_x] = &result;
    }

    for (int char_y = 0; char_y < height; ++char_y) {
      std::string last_bg = ansi::kResetBG;
      std::string last_fg = ansi::kResetFG;
      for (int char_x = 0; char_x < width; ++char_x) {
        TaskResult &result = *result_idx[char_y][char_x];
        std::string new_bg = result.bg.a < 0.5
                                 ? ansi::kResetBG
                                 : result.bg.pixel().AnsiBg();
        if (new_bg != last_bg) {
          result_raw += new_bg;
          last_bg = new_bg;
        }
        std::string new_fg = result.glyph->unicode == 32
                                 ? ansi::kResetFG
                                 : result.fg.pixel().AnsiFg();
        if (new_fg != last_fg) {
          result_raw += new_fg;
          last_fg = new_fg;
        }
        result_raw += result.glyph->utf8;
      }
      if (last_bg != ansi::kResetBG) {
        result_raw += ansi::kResetBG;
      }
      if (last_fg != ansi::kResetFG) {
        result_raw += ansi::kResetFG;
      }
      // Remove trailing whitespace
      while (result_raw.ends_with(" ")) {
        result_raw.pop_back();
      }
      result_raw += "\n";
    }
    // Remove empty newlines at the end
    while (result_raw.ends_with("\n\n")) {
      result_raw.pop_back();
    }

    result_c = result_raw;
    ReplaceAll(result_c, "\033", "\\033");
    ReplaceAll(result_c, "\n", "\\n");
    ReplaceAll(result_c, "\"", "\\\"");
    result_c = "char kAnsiArt[] = \"" + result_c + "\"";

    result_bash = result_raw;
    ReplaceAll(result_bash, "\\", "\\\\");
    ReplaceAll(result_bash, "\033", "\\e");
    ReplaceAll(result_bash, "\n", "\\n");
    ReplaceAll(result_bash, "'", "\\x27");
    result_bash = "echo -ne '" + result_bash + "'";
  }

  static void *RenderMasterThread(void *arg) {
    auto self = (AnsiArtImpl *)arg;
    self->Render();
    self->progress = 1;
    self->renderer = 0;
    pthread_exit(nullptr);
  }

  void StartRender(int n_threads) override {
    worker_count = n_threads;
    progress = 0;
    if (renderer) {
      return;
    }
    pthread_create(&renderer, nullptr, RenderMasterThread, this);
  }

  float GetRenderProgress() override { return progress; }

  void CancelRender() override {
    for (auto& worker : workers) {
      pthread_cancel(worker);
    }
  }
};

AnsiArt *AnsiArt::New() { return new AnsiArtImpl(); }

} // namespace maf
