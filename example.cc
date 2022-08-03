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
