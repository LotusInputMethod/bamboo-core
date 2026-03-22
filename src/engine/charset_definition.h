#pragma once

#include <array>
#include <cstddef>
#include <string>
#include <string_view>

namespace bamboo::engine {

class CharsetDefinition final {
public:
    static constexpr std::string_view kUnicode = "Unicode";

    struct MappingView final {
        char32_t source;
        std::string_view encoded;
    };

    struct DefinitionView final {
        std::string_view name;
        const MappingView* mappings;
        std::size_t size;
    };

    [[nodiscard]] static const DefinitionView* find(std::string_view name) noexcept;
    [[nodiscard]] static std::string_view lookupEncoded(std::string_view name, char32_t codePoint) noexcept;
    [[nodiscard]] static std::string encode(std::string_view name, std::u32string_view input);
    [[nodiscard]] static const std::array<std::string_view, 17>& names() noexcept;
};

}  // namespace bamboo::engine
