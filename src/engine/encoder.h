#pragma once

#include <array>
#include <string>
#include <string_view>

namespace bamboo::engine {

class Encoder final {
public:
    [[nodiscard]] static std::string encode(std::string_view charsetName, std::u32string_view input);
    [[nodiscard]] static const std::array<std::string_view, 17>& charsetNames() noexcept;
};

}  // namespace bamboo::engine
