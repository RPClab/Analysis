#pragma once

#include "fmt/color.h"

void Clear();
void get_terminal_size(int&, int&);
void BoxedText(const fmt::text_style&, const std::string&);
void CenterXText(const fmt::text_style&, const std::string&);
