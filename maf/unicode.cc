#include "maf/unicode.hh"

namespace maf {

std::string UnicodeToUTF8(unsigned int codepoint) {
  std::string s;
  if (codepoint <= 0x7f) {
    s.push_back(static_cast<char>(codepoint));
  } else if (codepoint <= 0x7ff) {
    s.push_back(static_cast<char>(0xc0 | ((codepoint >> 6) & 0x1f)));
    s.push_back(static_cast<char>(0x80 | (codepoint & 0x3f)));
  } else if (codepoint <= 0xffff) {
    s.push_back(static_cast<char>(0xe0 | ((codepoint >> 12) & 0x0f)));
    s.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3f)));
    s.push_back(static_cast<char>(0x80 | (codepoint & 0x3f)));
  } else {
    s.push_back(static_cast<char>(0xf0 | ((codepoint >> 18) & 0x07)));
    s.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3f)));
    s.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3f)));
    s.push_back(static_cast<char>(0x80 | (codepoint & 0x3f)));
  }
  return s;
}

} // namespace maf