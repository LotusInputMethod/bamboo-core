#include "rules_parser.h"

#include "input_method_definition.h"
#include "spelling.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <optional>
#include <string>

namespace bamboo::engine {
namespace {

constexpr std::array<std::pair<std::string_view, Tone>, 6> kTones{{
    {"XoaDauThanh", Tone::None},
    {"DauSac", Tone::Acute},
    {"DauHuyen", Tone::Grave},
    {"DauNga", Tone::Tilde},
    {"DauNang", Tone::Dot},
    {"DauHoi", Tone::Hook},
}};

constexpr std::u32string_view kVowels = U"aàáảãạăằắẳẵặâầấẩẫậeèéẻẽẹêềếểễệiìíỉĩịoòóỏõọôồốổỗộơờớởỡợuùúủũụưừứửữựyỳýỷỹỵ";

[[nodiscard]] std::u32string decodeUtf8(std::string_view input) {
    std::u32string output;
    output.reserve(input.size());
    for (std::size_t index = 0; index < input.size();) {
        const unsigned char byte0 = static_cast<unsigned char>(input[index]);
        if (byte0 < 0x80) {
            output.push_back(static_cast<char32_t>(byte0));
            ++index;
        } else if ((byte0 & 0xE0U) == 0xC0U && index + 1 < input.size()) {
            const unsigned char byte1 = static_cast<unsigned char>(input[index + 1]);
            output.push_back(static_cast<char32_t>(((byte0 & 0x1FU) << 6) | (byte1 & 0x3FU)));
            index += 2;
        } else if ((byte0 & 0xF0U) == 0xE0U && index + 2 < input.size()) {
            const unsigned char byte1 = static_cast<unsigned char>(input[index + 1]);
            const unsigned char byte2 = static_cast<unsigned char>(input[index + 2]);
            output.push_back(static_cast<char32_t>(((byte0 & 0x0FU) << 12) | ((byte1 & 0x3FU) << 6) | (byte2 & 0x3FU)));
            index += 3;
        } else if ((byte0 & 0xF8U) == 0xF0U && index + 3 < input.size()) {
            const unsigned char byte1 = static_cast<unsigned char>(input[index + 1]);
            const unsigned char byte2 = static_cast<unsigned char>(input[index + 2]);
            const unsigned char byte3 = static_cast<unsigned char>(input[index + 3]);
            output.push_back(static_cast<char32_t>(((byte0 & 0x07U) << 18) | ((byte1 & 0x3FU) << 12) |
                                                   ((byte2 & 0x3FU) << 6) | (byte3 & 0x3FU)));
            index += 4;
        } else {
            output.push_back(static_cast<char32_t>(byte0));
            ++index;
        }
    }
    return output;
}

[[nodiscard]] char32_t toLowerCodePoint(char32_t codePoint) noexcept {
    if (codePoint >= U'A' && codePoint <= U'Z') {
        return codePoint + 32;
    }
    switch (codePoint) {
    case U'Đ': return U'đ';
    case U'Â': return U'â';
    case U'Ă': return U'ă';
    case U'Ê': return U'ê';
    case U'Ô': return U'ô';
    case U'Ơ': return U'ơ';
    case U'Ư': return U'ư';
    default: return codePoint;
    }
}

[[nodiscard]] bool isAsciiAlphaString(std::string_view value) noexcept {
    return !value.empty() && std::all_of(value.begin(), value.end(), [](unsigned char c) { return std::isalpha(c) != 0; });
}

[[nodiscard]] int findVowelPosition(char32_t chr) noexcept {
    for (std::size_t index = 0; index < kVowels.size(); ++index) {
        if (kVowels[index] == chr) {
            return static_cast<int>(index);
        }
    }
    return -1;
}

[[nodiscard]] bool isVowel(char32_t chr) noexcept {
    return findVowelPosition(chr) >= 0;
}

[[nodiscard]] char32_t addToneToChar(char32_t chr, std::uint8_t tone) noexcept {
    const int position = findVowelPosition(chr);
    if (position < 0) {
        return chr;
    }
    const int currentTone = position % 6;
    return kVowels[static_cast<std::size_t>(position + static_cast<int>(tone) - currentTone)];
}

[[nodiscard]] std::u32string_view markFamily(char32_t chr) noexcept {
    switch (chr) {
    case U'a':
    case U'â':
    case U'ă':
        return U"aâă";
    case U'e':
    case U'ê':
        return U"eê";
    case U'o':
    case U'ô':
    case U'ơ':
        return U"oôơ";
    case U'u':
    case U'ư':
        return U"uư";
    case U'd':
    case U'đ':
        return U"dđ";
    default:
        return U"";
    }
}

[[nodiscard]] std::optional<Mark> findMarkFromChar(char32_t chr) noexcept {
    switch (chr) {
    case U'a':
    case U'e':
    case U'o':
    case U'u':
    case U'd':
        return Mark::None;
    case U'â':
    case U'ê':
    case U'ô':
        return Mark::Hat;
    case U'ă':
        return Mark::Breve;
    case U'ơ':
    case U'ư':
        return Mark::Horn;
    case U'đ':
        return Mark::Dash;
    default:
        return std::nullopt;
    }
}

[[nodiscard]] std::optional<Rule> getAppendingRule(char32_t key, std::u32string_view value) {
    const std::size_t firstUnderscore = value.find(U'_');
    if (firstUnderscore == std::u32string_view::npos || firstUnderscore > 1) {
        return std::nullopt;
    }
    if (firstUnderscore + 1 >= value.size() || value[firstUnderscore + 1] != U'_') {
        return std::nullopt;
    }

    const std::u32string chars(value.substr(firstUnderscore + 2));
    if (chars.empty()) {
        return std::nullopt;
    }

    Rule rule;
    rule.key = key;
    rule.effectType = EffectType::Appending;
    rule.effectOn = chars.front();
    rule.result = chars.front();
    for (std::size_t index = 1; index < chars.size(); ++index) {
        Rule appended;
        appended.key = key;
        appended.effectType = EffectType::Appending;
        appended.effectOn = chars[index];
        appended.result = chars[index];
        rule.appendedRules.push_back(appended);
    }
    return rule;
}

}  // namespace

InputMethod parseInputMethod(std::string_view inputMethodName) {
    InputMethod result;
    const InputMethodDefinition::DefinitionView* definition = InputMethodDefinition::find(inputMethodName);
    if (definition == nullptr) {
        return result;
    }

    result.name = std::string(definition->name);
    result.rules.reserve(definition->size * 4U);
    result.keys.reserve(definition->size);

    for (std::size_t index = 0; index < definition->size; ++index) {
        const auto& mapping = definition->mappings[index];
        std::vector<Rule> parsedRules = parseRules(mapping.key, mapping.action);
        result.rules.insert(result.rules.end(), parsedRules.begin(), parsedRules.end());
        if (mapping.action.find("uo") != std::string_view::npos || mapping.action.find("UO") != std::string_view::npos) {
            result.superKeys.push_back(mapping.key);
        }
        result.keys.push_back(mapping.key);
    }

    for (const Rule& rule : result.rules) {
        if (rule.effectType == EffectType::Appending) {
            result.appendingKeys.push_back(rule.key);
        }
        if (rule.effectType == EffectType::ToneTransformation) {
            result.toneKeys.push_back(rule.key);
        }
    }

    return result;
}

std::vector<Rule> parseRules(char32_t key, std::string_view line) {
    for (const auto& tone : kTones) {
        if (tone.first == line) {
            Rule rule;
            rule.key = key;
            rule.effectType = EffectType::ToneTransformation;
            rule.setTone(tone.second);
            return {rule};
        }
    }
    return parseTonelessRules(key, line);
}

std::vector<Rule> parseTonelessRules(char32_t key, std::string_view line) {
    std::vector<Rule> rules;

    const std::u32string decodedLine = decodeUtf8(line);
    std::u32string normalizedLine;
    normalizedLine.reserve(decodedLine.size());
    for (const char32_t codePoint : decodedLine) {
        normalizedLine.push_back(toLowerCodePoint(codePoint));
    }

    const std::size_t underscore = normalizedLine.find(U'_');
    if (underscore != std::u32string::npos) {
        std::string effectiveOnsAscii;
        effectiveOnsAscii.reserve(underscore);
        bool asciiOnly = true;
        for (std::size_t index = 0; index < underscore; ++index) {
            const char32_t cp = normalizedLine[index];
            if (cp > 0x7F) { asciiOnly = false; break; }
            effectiveOnsAscii.push_back(static_cast<char>(cp));
        }

        if (asciiOnly && isAsciiAlphaString(effectiveOnsAscii)) {
            const std::u32string effectiveOns(normalizedLine.begin(), normalizedLine.begin() + static_cast<std::ptrdiff_t>(underscore));
            const std::u32string rest(normalizedLine.begin() + static_cast<std::ptrdiff_t>(underscore + 1), normalizedLine.end());
            if (rest.size() >= effectiveOns.size()) {
                const std::u32string results(rest.begin(), rest.begin() + static_cast<std::ptrdiff_t>(effectiveOns.size()));
                for (std::size_t index = 0; index < effectiveOns.size(); ++index) {
                    if (const std::optional<Mark> effect = findMarkFromChar(results[index]); effect.has_value()) {
                        std::vector<Rule> parsed = parseToneLessRule(key, effectiveOns[index], results[index], *effect);
                        rules.insert(rules.end(), parsed.begin(), parsed.end());
                    }
                }

                if (rest.size() > effectiveOns.size()) {
                    if (const std::optional<Rule> appendRule = getAppendingRule(key, std::u32string_view(rest).substr(effectiveOns.size())); appendRule.has_value()) {
                        rules.push_back(*appendRule);
                    }
                }
                return rules;
            }
        }
    }

    if (const std::optional<Rule> appendRule = getAppendingRule(key, normalizedLine); appendRule.has_value()) {
        rules.push_back(*appendRule);
    }
    return rules;
}

std::vector<Rule> parseToneLessRule(char32_t key, char32_t effectiveOn, char32_t result, Mark effect) {
    std::vector<Rule> rules;
    constexpr std::array<Tone, 6> tones{{Tone::None, Tone::Dot, Tone::Acute, Tone::Grave, Tone::Hook, Tone::Tilde}};

    const std::u32string_view family = markFamily(effectiveOn);
    for (const char32_t chr : family) {
        if (chr == result) {
            Rule rule;
            rule.key = key;
            rule.effectType = EffectType::MarkTransformation;
            rule.effectOn = result;
            rule.result = effectiveOn;
            rules.push_back(rule);
        } else if (isVowel(chr)) {
            for (const Tone tone : tones) {
                Rule rule;
                rule.key = key;
                rule.effectType = EffectType::MarkTransformation;
                rule.setMark(effect);
                rule.effectOn = addToneToChar(chr, static_cast<std::uint8_t>(tone));
                rule.result = addToneToChar(result, static_cast<std::uint8_t>(tone));
                rules.push_back(rule);
            }
        } else {
            Rule rule;
            rule.key = key;
            rule.effectType = EffectType::MarkTransformation;
            rule.setMark(effect);
            rule.effectOn = chr;
            rule.result = result;
            rules.push_back(rule);
        }
    }

    return rules;
}

}  // namespace bamboo::engine
