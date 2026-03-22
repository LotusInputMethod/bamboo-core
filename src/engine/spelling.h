#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace bamboo::engine {

template <std::size_t Capacity>
struct IndexSet final {
    std::array<std::uint8_t, Capacity> values{};
    std::uint8_t size{0};

    [[nodiscard]] bool empty() const noexcept { return size == 0; }

    void push(std::uint8_t value) noexcept {
        if (size < Capacity) {
            values[size++] = value;
        }
    }
};

class Spelling final {
public:
    [[nodiscard]] bool isValidCvc(std::u32string_view firstConsonant,
                                  std::u32string_view vowel,
                                  std::u32string_view lastConsonant,
                                  bool inputIsFullComplete) const noexcept;
    [[nodiscard]] static char32_t normalizeToneless(char32_t codePoint) noexcept;

private:
    using RowMatchSet = IndexSet<8>;

    [[nodiscard]] static bool isValidCv(const RowMatchSet& firstConsonantIndexes,
                                        const RowMatchSet& vowelIndexes) noexcept;
    [[nodiscard]] static bool isValidVc(const RowMatchSet& vowelIndexes,
                                        const RowMatchSet& lastConsonantIndexes) noexcept;
};

}  // namespace bamboo::engine
