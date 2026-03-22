#pragma once

#include "rule.h"

#include <string_view>
#include <vector>

namespace bamboo::engine {

[[nodiscard]] InputMethod parseInputMethod(std::string_view inputMethodName);
[[nodiscard]] std::vector<Rule> parseRules(char32_t key, std::string_view line);
[[nodiscard]] std::vector<Rule> parseTonelessRules(char32_t key, std::string_view line);
[[nodiscard]] std::vector<Rule> parseToneLessRule(char32_t key, char32_t effectiveOn, char32_t result, Mark effect);

}  // namespace bamboo::engine
