#include "transformation_utils.h"
#include "spelling.h"

#include <array>

namespace bamboo::engine {
namespace {

constexpr std::u32string_view kVowels = U"aàáảãạăằắẳẵặâầấẩẫậeèéẻẽẹêềếểễệiìíỉĩịoòóỏõọôồốổỗộơờớởỡợuùúủũụưừứửữựyỳýỷỹỵ";

[[nodiscard]] char32_t toUpperCodePoint(char32_t codePoint) noexcept {
    if (codePoint >= U'a' && codePoint <= U'z') {
        return codePoint - 32;
    }
    switch (codePoint) {
    case U'đ': return U'Đ';
    case U'â': return U'Â';
    case U'ă': return U'Ă';
    case U'ê': return U'Ê';
    case U'ô': return U'Ô';
    case U'ơ': return U'Ơ';
    case U'ư': return U'Ư';
    default: return codePoint;
    }
}

[[nodiscard]] int findVowelPosition(char32_t chr) noexcept {
    const auto pos = kVowels.find(chr);
    return pos == std::u32string_view::npos ? -1 : static_cast<int>(pos);
}

[[nodiscard]] char32_t addToneToChar(char32_t chr, std::uint8_t tone) noexcept {
    const int position = findVowelPosition(chr);
    if (position < 0) {
        return chr;
    }
    const int currentTone = position % 6;
    return kVowels[static_cast<std::size_t>(position + static_cast<int>(tone) - currentTone)];
}

[[nodiscard]] char32_t addMarkToTonelessChar(char32_t chr, std::uint8_t mark) noexcept {
    switch (chr) {
    case U'a': return mark == 1 ? U'â' : (mark == 2 ? U'ă' : U'a');
    case U'â': return mark == 0 ? U'a' : U'â';
    case U'ă': return mark == 0 ? U'a' : U'ă';
    case U'e': return mark == 1 ? U'ê' : U'e';
    case U'ê': return mark == 0 ? U'e' : U'ê';
    case U'o': return mark == 1 ? U'ô' : (mark == 3 ? U'ơ' : U'o');
    case U'ô': return mark == 0 ? U'o' : U'ô';
    case U'ơ': return mark == 0 ? U'o' : U'ơ';
    case U'u': return mark == 3 ? U'ư' : U'u';
    case U'ư': return mark == 0 ? U'u' : U'ư';
    case U'd': return mark == 4 ? U'đ' : U'd';
    case U'đ': return mark == 0 ? U'd' : U'đ';
    default: return chr;
    }
}

[[nodiscard]] char32_t addMarkToChar(char32_t chr, std::uint8_t mark) noexcept {
    const std::uint8_t tone = static_cast<std::uint8_t>(findVowelPosition(chr) >= 0 ? findVowelPosition(chr) % 6 : 0);
    const char32_t toneless = addToneToChar(chr, 0);
    return addToneToChar(addMarkToTonelessChar(toneless, mark), tone);
}

[[nodiscard]] char32_t stripToneAndMark(char32_t chr) noexcept {
    return addMarkToTonelessChar(addToneToChar(chr, 0), 0);
}


[[nodiscard]] std::u32string flattenVietnameseView(const CompositionView& composition, bool lowerCase, bool toneLess, bool markLess) {
    std::u32string canvas;
    canvas.reserve(composition.size());
    for (const Transformation* appending : composition) {
        if (appending->rule.effectType != EffectType::Appending || appending->rule.key == 0) {
            continue;
        }
        char32_t chr = appending->rule.effectOn;
        for (const Transformation* trans : composition) {
            if (trans->target != appending) {
                continue;
            }
            if (trans->rule.effectType == EffectType::MarkTransformation) {
                chr = trans->rule.mark() == Mark::Raw ? appending->rule.key : addMarkToChar(chr, trans->rule.effect);
            } else if (trans->rule.effectType == EffectType::ToneTransformation) {
                chr = addToneToChar(chr, trans->rule.effect);
            }
        }
        if (toneLess) {
            chr = addToneToChar(chr, 0);
        }
        if (markLess) {
            chr = addMarkToTonelessChar(chr, 0);
        }
        if (lowerCase) {
            chr = toLowerCodePoint(chr);
        }
        canvas.push_back(chr);
    }
    return canvas;
}

[[nodiscard]] bool validateWord(std::u32string_view word, bool full) {
    const Segments segments = splitWord(word);
    Spelling spelling;
    return spelling.isValidCvc(segments.firstConsonant, segments.vowel, segments.lastConsonant, full);
}

[[nodiscard]] char32_t renderTargetChar(const CompositionView& composition, const Transformation* target) {
    char32_t chr = target->rule.effectOn;
    for (const Transformation* trans : composition) {
        if (trans->target != target) {
            continue;
        }
        if (trans->rule.effectType == EffectType::MarkTransformation) {
            chr = trans->rule.mark() == Mark::Raw ? target->rule.key : addMarkToChar(chr, trans->rule.effect);
        } else if (trans->rule.effectType == EffectType::ToneTransformation) {
            chr = addToneToChar(chr, trans->rule.effect);
        }
    }
    return chr;
}

[[nodiscard]] Transformation* findRootTarget(Transformation* target) noexcept {
    while (target != nullptr && target->target != nullptr) {
        target = target->target;
    }
    return target;
}

[[nodiscard]] Transformation* findToneTarget(const CompositionView& composition) noexcept {
    std::vector<Transformation*> vowels;
    vowels.reserve(3);
    for (Transformation* trans : composition) {
        if (trans->rule.effectType == EffectType::Appending && isVowel(trans->rule.effectOn)) {
            vowels.push_back(trans);
        }
    }
    if (vowels.empty()) {
        return nullptr;
    }
    if (vowels.size() == 1) {
        return vowels.front();
    }

    std::u32string vowelShape;
    vowelShape.reserve(vowels.size());
    for (Transformation* vowel : vowels) {
        vowelShape.push_back(stripToneAndMark(renderTargetChar(composition, vowel)));
    }
    bool hasTrailingConsonant = false;
    for (auto it = composition.rbegin(); it != composition.rend(); ++it) {
        if ((*it)->rule.effectType != EffectType::Appending || (*it)->rule.key == 0) {
            continue;
        }
        hasTrailingConsonant = !isVowel((*it)->rule.effectOn);
        break;
    }

    if (vowels.size() == 2) {
        if (hasTrailingConsonant) {
            return vowels[1];
        }
        if (vowelShape == U"oa" || vowelShape == U"oe" || vowelShape == U"uy" || vowelShape == U"ue" || vowelShape == U"uo") {
            return vowels[1];
        }
        return vowels[0];
    }

    if (vowelShape == U"uye") {
        return vowels[2];
    }
    return vowels[1];
}

}  // namespace

char32_t toLowerCodePoint(char32_t codePoint) noexcept {
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

bool isVowel(char32_t chr) noexcept {
    return kVowels.find(chr) != std::u32string_view::npos;
}

CompositionView makeCompositionView(std::deque<Transformation>& composition) {
    CompositionView view;
    view.reserve(composition.size());
    for (Transformation& trans : composition) {
        view.push_back(&trans);
    }
    return view;
}

CompositionView extractLastSyllable(const CompositionView& composition) {
    std::size_t start = 0;
    for (std::size_t index = composition.size(); index > 0; --index) {
        const Transformation* trans = composition[index - 1];
        if (trans->rule.effectType == EffectType::Appending &&
            (trans->rule.key == U' ' || trans->rule.key == U'\n' || trans->rule.key == U'\t')) {
            start = index;
            break;
        }
    }
    return CompositionView(composition.begin() + static_cast<std::ptrdiff_t>(start), composition.end());
}

PendingTransformation findMarkTarget(const CompositionView& composition,
                                     const std::vector<Rule>& applicableRules) noexcept {
    const std::u32string current = flattenVietnameseView(composition, true, false, false);
    for (const Rule& rule : applicableRules) {
        if (rule.effectType != EffectType::MarkTransformation) {
            continue;
        }
        for (auto it = composition.rbegin(); it != composition.rend(); ++it) {
            Transformation* trans = *it;
            if (trans->rule.effectType != EffectType::Appending || trans->rule.effectOn != rule.effectOn) {
                continue;
            }
            std::u32string mutated = current;
            const char32_t rendered = renderTargetChar(composition, trans);
            for (std::size_t index = mutated.size(); index > 0; --index) {
                if (mutated[index - 1] == rendered) {
                    mutated[index - 1] = addMarkToChar(mutated[index - 1], rule.effect);
                    break;
                }
            }
            if (mutated != current && validateWord(mutated, false)) {
                return PendingTransformation{rule, findRootTarget(trans), false};
            }
        }
    }
    return {};
}

PendingTransformation findTarget(const CompositionView& composition,
                                 const std::vector<Rule>& applicableRules) noexcept {
    const std::u32string current = flattenVietnameseView(composition, true, false, false);
    for (const Rule& rule : applicableRules) {
        if (rule.effectType == EffectType::ToneTransformation) {
            if (Transformation* target = findToneTarget(composition); target != nullptr) {
                std::u32string mutated = current;
                const char32_t rendered = renderTargetChar(composition, target);
                for (std::size_t index = mutated.size(); index > 0; --index) {
                    if (mutated[index - 1] == rendered) {
                        mutated[index - 1] = addToneToChar(mutated[index - 1], rule.effect);
                        break;
                    }
                }
                if (mutated != current) {
                    return PendingTransformation{rule, target, false};
                }
            }
        }
    }
    PendingTransformation markTarget = findMarkTarget(composition, applicableRules);
    if (markTarget.target != nullptr) {
        return markTarget;
    }

    const std::u32string normalized = flattenVietnameseView(composition, true, true, true);
    if (normalized.find(U"uo") != std::u32string::npos || normalized.find(U"ươ") != std::u32string::npos || normalized.find(U"ưo") != std::u32string::npos) {
        Transformation* vowelTarget = nullptr;
        for (auto it = composition.rbegin(); it != composition.rend(); ++it) {
            Transformation* trans = *it;
            if (trans->rule.effectType == EffectType::Appending && isVowel(trans->rule.effectOn)) {
                vowelTarget = trans;
                break;
            }
        }
        if (vowelTarget != nullptr) {
            for (const Rule& rule : applicableRules) {
                if (rule.effectType == EffectType::MarkTransformation && rule.effectOn == vowelTarget->rule.effectOn) {
                    return PendingTransformation{rule, vowelTarget, false};
                }
            }
        }
    }
    return {};
}

std::vector<PendingTransformation> generateUndoTransformations(const CompositionView& composition,
                                                               const std::vector<Rule>& applicableRules) {
    std::vector<PendingTransformation> result;
    for (const Rule& rule : applicableRules) {
        if (rule.effectType == EffectType::ToneTransformation) {
            for (auto it = composition.rbegin(); it != composition.rend(); ++it) {
                if ((*it)->rule.effectType == EffectType::ToneTransformation && (*it)->target != nullptr) {
                    Rule undoRule{};
                    undoRule.effectType = EffectType::ToneTransformation;
                    undoRule.effect = static_cast<std::uint8_t>(Tone::None);
                    result.push_back(PendingTransformation{undoRule, (*it)->target, false});
                    return result;
                }
            }
        } else if (rule.effectType == EffectType::MarkTransformation) {
            for (auto it = composition.rbegin(); it != composition.rend(); ++it) {
                if ((*it)->rule.effectType == EffectType::MarkTransformation && (*it)->target != nullptr) {
                    Rule undoRule{};
                    undoRule.effectType = EffectType::MarkTransformation;
                    undoRule.effect = static_cast<std::uint8_t>(Mark::None);
                    result.push_back(PendingTransformation{undoRule, (*it)->target, false});
                    return result;
                }
            }
        }
    }
    return result;
}

std::vector<PendingTransformation> generateFallbackTransformations(const std::vector<Rule>& applicableRules,
                                                                   char32_t lowerKey,
                                                                   bool isUpperCase) {
    std::vector<PendingTransformation> result;
    for (const Rule& rule : applicableRules) {
        if (rule.effectType != EffectType::Appending) {
            continue;
        }
        Rule baseRule = rule;
        baseRule.effectOn = toLowerCodePoint(baseRule.effectOn);
        baseRule.result = baseRule.effectOn;
        result.push_back(PendingTransformation{baseRule, nullptr, isUpperCase || rule.effectOn != baseRule.effectOn});
        for (const Rule& appendedRule : rule.appendedRules) {
            Rule virtualRule = appendedRule;
            virtualRule.key = 0;
            virtualRule.effectOn = toLowerCodePoint(virtualRule.effectOn);
            virtualRule.result = virtualRule.effectOn;
            result.push_back(PendingTransformation{virtualRule, nullptr, isUpperCase || appendedRule.effectOn != virtualRule.effectOn});
        }
        return result;
    }

    Rule rawRule{};
    rawRule.key = lowerKey;
    rawRule.effectOn = lowerKey;
    rawRule.result = lowerKey;
    rawRule.effectType = EffectType::Appending;
    result.push_back(PendingTransformation{rawRule, nullptr, isUpperCase});
    return result;
}

std::vector<PendingTransformation> refreshLastToneTarget(const CompositionView& composition) {
    Transformation* latestTone = nullptr;
    for (auto it = composition.rbegin(); it != composition.rend(); ++it) {
        if ((*it)->rule.effectType == EffectType::ToneTransformation && (*it)->target != nullptr) {
            latestTone = *it;
            break;
        }
    }
    if (latestTone == nullptr) {
        return {};
    }
    Transformation* newTarget = findToneTarget(composition);
    if (newTarget == nullptr || newTarget == latestTone->target) {
        return {};
    }
    Rule undoRule{};
    undoRule.effectType = EffectType::ToneTransformation;
    undoRule.effect = static_cast<std::uint8_t>(Tone::None);
    Rule overrideRule = latestTone->rule;
    overrideRule.key = 0;
    return {PendingTransformation{undoRule, latestTone->target, false}, PendingTransformation{overrideRule, newTarget, false}};
}

std::u32string flattenVietnamese(const std::deque<Transformation>& composition, bool lowerCase) {
    std::u32string canvas;
    canvas.reserve(composition.size());
    for (const Transformation& appending : composition) {
        if (appending.rule.effectType != EffectType::Appending || appending.rule.key == 0) {
            continue;
        }

        char32_t chr = appending.rule.effectOn;
        for (const Transformation& trans : composition) {
            if (trans.target != &appending) {
                continue;
            }
            if (trans.rule.effectType == EffectType::MarkTransformation) {
                if (trans.rule.mark() == Mark::Raw) {
                    chr = appending.rule.key;
                } else {
                    chr = addMarkToChar(chr, trans.rule.effect);
                }
            } else if (trans.rule.effectType == EffectType::ToneTransformation) {
                chr = addToneToChar(chr, trans.rule.effect);
            }
        }

        if (lowerCase) {
            chr = toLowerCodePoint(chr);
        } else if (appending.isUpperCase) {
            chr = toUpperCodePoint(chr);
        }
        canvas.push_back(chr);
    }
    return canvas;
}

std::u32string currentWord(const std::deque<Transformation>& composition) {
    const std::u32string text = flattenVietnamese(composition, true);
    const auto pos = text.find_last_of(U" \n\t");
    if (pos == std::u32string::npos) {
        return text;
    }
    return text.substr(pos + 1);
}

Segments splitWord(std::u32string_view word) noexcept {
    std::size_t firstVowelIndex = 0;
    while (firstVowelIndex < word.size() && !isVowel(word[firstVowelIndex])) {
        ++firstVowelIndex;
    }
    std::size_t trailingConsonantIndex = word.size();
    while (trailingConsonantIndex > firstVowelIndex && !isVowel(word[trailingConsonantIndex - 1])) {
        --trailingConsonantIndex;
    }
    return {word.substr(0, firstVowelIndex), word.substr(firstVowelIndex, trailingConsonantIndex - firstVowelIndex), word.substr(trailingConsonantIndex)};
}

std::deque<Transformation> breakComposition(const CompositionView& composition) {
    std::deque<Transformation> result;
    for (const Transformation* trans : composition) {
        if (trans->rule.key == 0) {
            continue;
        }
        Transformation broken;
        broken.isUpperCase = trans->isUpperCase;
        broken.rule.key = trans->rule.key;
        broken.rule.effectOn = trans->rule.key;
        broken.rule.result = trans->rule.key;
        broken.rule.effectType = EffectType::Appending;
        result.push_back(broken);
    }
    return result;
}

}  // namespace bamboo::engine
