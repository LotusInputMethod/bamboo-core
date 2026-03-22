#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace bamboo::engine {

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

struct Rule final {
    char32_t key{0};
    std::uint8_t effect{0};
    EffectType effectType{EffectType::Appending};
    char32_t effectOn{0};
    char32_t result{0};
    std::vector<Rule> appendedRules;

    void setTone(Tone tone) noexcept { effect = static_cast<std::uint8_t>(tone); }
    void setMark(Mark mark) noexcept { effect = static_cast<std::uint8_t>(mark); }
    [[nodiscard]] Tone tone() const noexcept { return static_cast<Tone>(effect); }
    [[nodiscard]] Mark mark() const noexcept { return static_cast<Mark>(effect); }
};

struct InputMethod final {
    std::string name;
    std::vector<Rule> rules;
    std::vector<char32_t> superKeys;
    std::vector<char32_t> toneKeys;
    std::vector<char32_t> appendingKeys;
    std::vector<char32_t> keys;
};

}  // namespace bamboo::engine
