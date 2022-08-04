#pragma once

#include <string>

namespace maf {

using str = std::string;

// https://stackoverflow.com/questions/3418231/replace-part-of-a-string-with-another-string
void ReplaceAll(str &s, const str &from, const str &to);

} // namespace maf
