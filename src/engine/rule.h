#pragma once

#include <array>
#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

namespace bamboo::engine {

constexpr std::size_t kMaxAppendedChars = 4;
constexpr std::size_t kMaxRuleIndexEntries = 32;

enum class EffectType : std::uint8_t {
    Appending = 1u << 0,
    MarkTransformation = 1u << 1,
    ToneTransformation = 1u << 2,
    Replacing = 1u << 3,
};

enum class Mark : std::uint8_t {
    None = 0,
    Hat = 1,
    Breve = 2,
    Horn = 3,
    Dash = 4,
    Raw = 5,
};

enum class Tone : std::uint8_t {
    None = 0,
    Grave = 1,
    Acute = 2,
    Hook = 3,
    Tilde = 4,
    Dot = 5,
};

struct AppendedChar final {
    char32_t effectOn{0};
    char32_t result{0};
};

struct Rule final {
    char32_t key{0};
    std::uint8_t effect{0};
    EffectType effectType{EffectType::Appending};
    char32_t effectOn{0};
    char32_t result{0};
    std::array<AppendedChar, kMaxAppendedChars> appendedChars{};
    std::uint8_t appendedCount{0};

    void setTone(Tone tone) noexcept { effect = static_cast<std::uint8_t>(tone); }
    void setMark(Mark mark) noexcept { effect = static_cast<std::uint8_t>(mark); }
    [[nodiscard]] Tone tone() const noexcept { return static_cast<Tone>(effect); }
    [[nodiscard]] Mark mark() const noexcept { return static_cast<Mark>(effect); }

    void appendChar(char32_t appendedEffectOn, char32_t appendedResult) noexcept {
        if (appendedCount < appendedChars.size()) {
            appendedChars[appendedCount++] = AppendedChar{appendedEffectOn, appendedResult};
        }
    }
};

struct RuleSpan final {
    const Rule* data{nullptr};
    std::size_t size{0};

    [[nodiscard]] const Rule* begin() const noexcept { return data; }
    [[nodiscard]] const Rule* end() const noexcept { return data + size; }
    [[nodiscard]] bool empty() const noexcept { return size == 0; }
    [[nodiscard]] const Rule& operator[](std::size_t index) const noexcept { return data[index]; }
};

struct KeyRuleIndex final {
    char32_t key{0};
    std::uint16_t begin{0};
    std::uint16_t size{0};
};

struct InputMethod final {
    std::string name;
    std::vector<Rule> rules;
    std::array<KeyRuleIndex, kMaxRuleIndexEntries> ruleIndex{};
    std::uint8_t ruleIndexSize{0};
    std::vector<char32_t> superKeys;
    std::vector<char32_t> toneKeys;
    std::vector<char32_t> appendingKeys;
    std::vector<char32_t> keys;

    [[nodiscard]] RuleSpan rulesFor(char32_t key) const noexcept {
        for (std::size_t index = 0; index < ruleIndexSize; ++index) {
            if (ruleIndex[index].key == key) {
                return RuleSpan{rules.data() + ruleIndex[index].begin, ruleIndex[index].size};
            }
        }
        return {};
    }
};

}  // namespace bamboo::engine
