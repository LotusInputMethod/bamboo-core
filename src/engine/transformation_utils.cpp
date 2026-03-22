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
    case U'a':
    case U'â':
    case U'ă':
        return mark == 1 ? U'â' : (mark == 2 ? U'ă' : U'a');
    case U'e':
    case U'ê':
        return mark == 1 ? U'ê' : U'e';
    case U'o':
    case U'ô':
    case U'ơ':
        return mark == 1 ? U'ô' : (mark == 3 ? U'ơ' : U'o');
    case U'u':
    case U'ư':
        return mark == 3 ? U'ư' : U'u';
    case U'd':
    case U'đ':
        return mark == 4 ? U'đ' : U'd';
    default:
        return chr;
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


[[nodiscard]] std::u32string flattenVietnameseView(const CompositionView& composition,
                                                   bool lowerCase,
                                                   bool toneLess,
                                                   bool markLess,
                                                   const Transformation* extra = nullptr) {
    std::u32string canvas;
    canvas.reserve(composition.size() + (extra != nullptr ? 1U : 0U));
    for (std::size_t index = 0; index < composition.size(); ++index) {
        const Transformation* appending = composition[index];
        if (appending->rule.effectType != EffectType::Appending || appending->rule.key == 0) {
            continue;
        }
        char32_t chr = appending->rule.effectOn;
        for (std::size_t transIndex = 0; transIndex < composition.size(); ++transIndex) {
            const Transformation* trans = composition[transIndex];
            if (trans->target != appending) {
                continue;
            }
            if (trans->rule.effectType == EffectType::MarkTransformation) {
                chr = trans->rule.mark() == Mark::Raw ? appending->rule.key : addMarkToChar(chr, trans->rule.effect);
            } else if (trans->rule.effectType == EffectType::ToneTransformation) {
                chr = addToneToChar(chr, trans->rule.effect);
            }
        }
        if (extra != nullptr && extra->target == appending) {
            if (extra->rule.effectType == EffectType::MarkTransformation) {
                chr = extra->rule.mark() == Mark::Raw ? appending->rule.key : addMarkToChar(chr, extra->rule.effect);
            } else if (extra->rule.effectType == EffectType::ToneTransformation) {
                chr = addToneToChar(chr, extra->rule.effect);
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

[[nodiscard]] bool hasValidTone(const CompositionView& composition, Tone tone);

[[nodiscard]] bool hasValidTone(const CompositionView& composition, Tone tone) {
    if (tone == Tone::None || tone == Tone::Acute || tone == Tone::Dot) {
        return true;
    }
    const std::u32string word = flattenVietnameseView(composition, true, false, false);
    const Segments segments = splitWord(word);
    const std::u32string_view lastConsonants = segments.lastConsonant;
    if (lastConsonants.empty()) {
        return true;
    }
    return !(lastConsonants == U"c" || lastConsonants == U"k" || lastConsonants == U"p" ||
             lastConsonants == U"t" || lastConsonants == U"ch");
}

[[nodiscard]] char32_t renderTargetChar(const CompositionView& composition,
                                        const Transformation* target,
                                        const Transformation* extra = nullptr) {
    char32_t chr = target->rule.effectOn;
    for (std::size_t index = 0; index < composition.size(); ++index) {
        const Transformation* trans = composition[index];
        if (trans->target != target) {
            continue;
        }
        if (trans->rule.effectType == EffectType::MarkTransformation) {
            chr = trans->rule.mark() == Mark::Raw ? target->rule.key : addMarkToChar(chr, trans->rule.effect);
        } else if (trans->rule.effectType == EffectType::ToneTransformation) {
            chr = addToneToChar(chr, trans->rule.effect);
        }
    }
    if (extra != nullptr && extra->target == target) {
        if (extra->rule.effectType == EffectType::MarkTransformation) {
            chr = extra->rule.mark() == Mark::Raw ? target->rule.key : addMarkToChar(chr, extra->rule.effect);
        } else if (extra->rule.effectType == EffectType::ToneTransformation) {
            chr = addToneToChar(chr, extra->rule.effect);
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
    SmallList<Transformation*, 4> vowels;
    for (std::size_t index = 0; index < composition.size(); ++index) {
        Transformation* trans = composition[index];
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
    for (std::size_t index = composition.size(); index > 0; --index) {
        Transformation* trans = composition[index - 1];
        if (trans->rule.effectType != EffectType::Appending || trans->rule.key == 0) {
            continue;
        }
        hasTrailingConsonant = !isVowel(trans->rule.effectOn);
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
    return CompositionView{&composition, 0, composition.size()};
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
    return CompositionView{composition.composition, composition.begin + start, composition.end};
}

PendingTransformation findMarkTarget(const CompositionView& composition,
                                     RuleSpan applicableRules) noexcept {
    const std::u32string current = flattenVietnameseView(composition, true, false, false);
    Transformation* lastAppending = nullptr;
    for (std::size_t index = composition.size(); index > 0; --index) {
        Transformation* trans = composition[index - 1];
        if (trans->rule.effectType == EffectType::Appending && trans->rule.key != 0) {
            lastAppending = trans;
            break;
        }
    }
    for (std::size_t index = composition.size(); index > 0; --index) {
        Transformation* trans = composition[index - 1];
        for (const Rule& rule : applicableRules) {
            if (rule.effectType != EffectType::MarkTransformation || rule.effect == 0 || trans->rule.result != rule.effectOn) {
                continue;
            }
            Transformation probe;
            probe.rule = rule;
            probe.target = findRootTarget(trans);
            const std::u32string mutated = flattenVietnameseView(composition, true, false, false, &probe);
            if (mutated == current) {
                continue;
            }
            if (validateWord(mutated, false) || probe.target == lastAppending) {
                return PendingTransformation{rule, probe.target, false};
            }
        }
    }
    return {};
}

PendingTransformation findTarget(const CompositionView& composition,
                                 RuleSpan applicableRules) noexcept {
    const std::u32string current = flattenVietnameseView(composition, true, false, false);
    for (const Rule& rule : applicableRules) {
        if (rule.effectType == EffectType::ToneTransformation) {
            if (!hasValidTone(composition, rule.tone())) {
                continue;
            }
            if (Transformation* target = findToneTarget(composition); target != nullptr) {
                Transformation probe;
                probe.rule = rule;
                probe.target = target;
                if (flattenVietnameseView(composition, true, false, false, &probe) != current) {
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
        for (std::size_t index = composition.size(); index > 0; --index) {
            Transformation* trans = composition[index - 1];
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

PendingTransformationList generateUndoTransformations(const CompositionView& composition,
                                                      RuleSpan applicableRules) {
    PendingTransformationList result;
    const std::u32string current = flattenVietnameseView(composition, true, true, false);
    for (const Rule& rule : applicableRules) {
        if (rule.effectType == EffectType::ToneTransformation) {
            if (!hasValidTone(composition, rule.tone())) {
                continue;
            }
            Transformation* target = findToneTarget(composition);
            if (target == nullptr) {
                continue;
            }
            Rule undoRule{};
            undoRule.effectType = EffectType::ToneTransformation;
            undoRule.effect = static_cast<std::uint8_t>(Tone::None);
            Transformation probe;
            probe.rule = undoRule;
            probe.target = target;
            if (flattenVietnameseView(composition, true, true, false, &probe) != current) {
                result.push_back(PendingTransformation{undoRule, target, false});
                return result;
            }
        } else if (rule.effectType == EffectType::MarkTransformation) {
            for (std::size_t index = composition.size(); index > 0; --index) {
                Transformation* trans = composition[index - 1];
                if (trans->rule.result == rule.effectOn) {
                    Transformation* target = findRootTarget(trans);
                    Rule undoRule{};
                    undoRule.effectType = EffectType::MarkTransformation;
                    undoRule.effect = static_cast<std::uint8_t>(Mark::None);
                    Transformation probe;
                    probe.rule = undoRule;
                    probe.target = target;
                    if (flattenVietnameseView(composition, true, true, false, &probe) == current) {
                        continue;
                    }
                    result.push_back(PendingTransformation{undoRule, target, false});
                    return result;
                }
            }
        }
    }
    return result;
}

PendingTransformationList generateFallbackTransformations(RuleSpan applicableRules,
                                                          char32_t lowerKey,
                                                          bool isUpperCase) {
    PendingTransformationList result;
    for (const Rule& rule : applicableRules) {
        if (rule.effectType != EffectType::Appending) {
            continue;
        }
        Rule baseRule = rule;
        baseRule.effectOn = toLowerCodePoint(baseRule.effectOn);
        baseRule.result = baseRule.effectOn;
        result.push_back(PendingTransformation{baseRule, nullptr, isUpperCase || rule.effectOn != baseRule.effectOn});
        for (std::size_t index = 0; index < rule.appendedCount; ++index) {
            Rule virtualRule{};
            virtualRule.key = 0;
            virtualRule.effectType = EffectType::Appending;
            virtualRule.effectOn = toLowerCodePoint(rule.appendedChars[index].effectOn);
            virtualRule.result = virtualRule.effectOn;
            result.push_back(PendingTransformation{
                virtualRule,
                nullptr,
                isUpperCase || rule.appendedChars[index].effectOn != virtualRule.effectOn,
            });
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

PendingTransformationList refreshLastToneTarget(const CompositionView& composition) {
    Transformation* latestTone = nullptr;
    for (std::size_t index = composition.size(); index > 0; --index) {
        Transformation* trans = composition[index - 1];
        if (trans->rule.effectType == EffectType::ToneTransformation && trans->target != nullptr) {
            latestTone = trans;
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
    PendingTransformationList result;
    result.push_back(PendingTransformation{undoRule, latestTone->target, false});
    result.push_back(PendingTransformation{overrideRule, newTarget, false});
    return result;
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
    if (word.empty()) {
        return {};
    }

    std::u32string_view head = word;
    std::u32string_view lastConsonant;
    if (!isVowel(word.back())) {
        std::size_t begin = word.size() - 1;
        while (begin > 0 && !isVowel(word[begin - 1])) {
            --begin;
        }
        head = word.substr(0, begin);
        lastConsonant = word.substr(begin);
    }

    std::u32string_view firstConsonant = head;
    std::u32string_view vowel;
    if (!head.empty() && isVowel(head.back())) {
        std::size_t begin = head.size() - 1;
        while (begin > 0 && isVowel(head[begin - 1])) {
            --begin;
        }
        firstConsonant = head.substr(0, begin);
        vowel = head.substr(begin);
    }

    if (head.empty() && !lastConsonant.empty()) {
        return {lastConsonant, {}, {}};
    }

    if (firstConsonant.size() == 1 && !vowel.empty()) {
        const bool isGi =
            firstConsonant[0] == U'g' && vowel[0] == U'i' && vowel.size() > 1 &&
            !(vowel.size() > 1 && vowel[1] == U'e' && !lastConsonant.empty());
        const bool isQu = firstConsonant[0] == U'q' && vowel[0] == U'u';
        if (isGi || isQu) {
            firstConsonant = head.substr(0, firstConsonant.size() + 1);
            vowel = head.substr(firstConsonant.size());
        }
    }

    return {firstConsonant, vowel, lastConsonant};
}

std::deque<Transformation> breakComposition(const CompositionView& composition) {
    std::deque<Transformation> result;
    for (std::size_t index = 0; index < composition.size(); ++index) {
        const Transformation* trans = composition[index];
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
