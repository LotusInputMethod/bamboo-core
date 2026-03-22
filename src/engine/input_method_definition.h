#pragma once

#include <array>
#include <cstddef>
#include <string_view>

namespace bamboo::engine {

class InputMethodDefinition final {
public:
    struct MappingView final {
        char32_t key;
        std::string_view action;
    };

    struct DefinitionView final {
        std::string_view name;
        const MappingView* mappings;
        std::size_t size;
    };

    [[nodiscard]] static const DefinitionView* find(std::string_view name) noexcept;
    [[nodiscard]] static std::string_view lookupAction(std::string_view name, char32_t key) noexcept;
    [[nodiscard]] static const std::array<std::string_view, 9>& names() noexcept;
};

}  // namespace bamboo::engine
