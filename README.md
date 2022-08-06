# ANSI🔥Art

C++ ANSI🔥Art rendering library. Check out https://mrogalski.eu/ansi-art for an interactive demo.

## Background

Original [ANSI Art](https://en.wikipedia.org/wiki/ANSI_art) is typically limited to
    <a href="https://en.wikipedia.org/wiki/ANSI_escape_code#3-bit_and_4-bit">16 colors</a> and the
    character set used by the original <a href="https://www.youtube.com/watch?v=_mZBa3sqTrI&t=1061s">IBM PC</a>.</p>
  <p>Times have changed. Modern terminal emulators support 24-bit RGB colors & can display arbitrary
    Unicode characters. This opens entirely new possibilities for the art that can be displayed in the terminal.</p>
  <p>Let's call this 24-bit, Unicode-capable version of ANSI Art, an <strong>ANSI🔥Art</strong> (pronounced
    just like a regular ANSI Art, although with a sound of blazing fire in the background).</p>
  <p>ANSI🔥Art can be used in the output of interactive <a href="https://www.youtube.com/watch?v=_oHByo8tiEY">CLI</a>
    commands, <a href="http://mewbies.com/how_to_customize_your_console_login_message_tutorial.htm">SSH MOTD</a>, an
    element of a <a href="https://www.youtube.com/watch?v=4G_cthFZeJ8">ncurses</a> interface or even for
    <a href="https://www.youtube.com/watch?v=MJZvWgcxV0M">animation</a>.</p>
  <p>Here are some examples of what can be achieved:</p>
  <img src="sample1.webp"><img src="sample2.webp"><img src="sample3.webp"><img src="sample4.webp">

## Usage

This C++ library allows you to render any bitmap as an ANSI🔥Art.

The only dependency is FreeType. On Debian-based distributions it can be installed with `sudo apt install libfreetype-dev`.

In order to try out the library, run `./run.sh` and observe the results in your terminal:

<img src="example-result.webp">

Example usage can be found in `example.cc`:

```c++
#include "maf/ansi_art.hh"

#include "example-image.h"
#include "example-font.h"

int main() {
  printf("Rendering... (this may take a few seconds)\n");
  auto art = maf::AnsiArt::New();
  art->LoadImage(example_image.width, example_image.height, example_image.pixel_data);
  art->LoadTTF(UbuntuMono_R_ttf, UbuntuMono_R_ttf_len);
  art->width = 80;
  art->Render();
  printf("%s\n", art->result_raw.c_str());
  delete art;
  return 0;
}
```

## API

Full API can be found in `maf/ansi_art.hh`:

```c++
#pragma once

#include <string>

namespace maf {

class AnsiArt {
public:
  enum ColorPalette {
    COLOR_24_BIT = 0,
    COLOR_8_BIT = 1,
    COLOR_WHITE_BIT = 2,
    COLOR_BLACK_BIT = 3,
  };

  static AnsiArt *New();
  
  virtual ~AnsiArt(){};
  virtual std::string LoadTTF(const uint8_t *data, size_t size) = 0;
  virtual void LoadImage(int width, int height, const uint8_t *rgba_bytes) = 0;
  virtual void Render() = 0;

  virtual void StartRender(int n_threads) = 0;
  virtual float GetRenderProgress() = 0;
  virtual void CancelRender() = 0;

  int width = 80;
  ColorPalette color_palette = COLOR_24_BIT;
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

```
