#include "engine.h"

#include "charset_definition.h"
#include "encoder.h"
#include "rules_parser.h"
#include "spelling.h"
#include "transformation_utils.h"

#include <algorithm>

namespace bamboo::engine {
namespace {

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
        } else {
            output.push_back(static_cast<char32_t>(byte0));
            ++index;
        }
    }
    return output;
}

}  // namespace

Engine::Engine(std::string_view dataDirPath, std::string_view inputMethod)
    : dataDirPath_(dataDirPath),
      inputMethod_(parseInputMethod(inputMethod.empty() ? std::string_view{"Telex"} : inputMethod)) {
    if (inputMethod_.name.empty()) {
        inputMethod_ = parseInputMethod("Telex");
    }
    encodedCache_.reserve(256);
}

void Engine::setMode(api::Mode mode) { mode_ = mode; }
api::Mode Engine::getMode() const { return mode_; }

void Engine::reset() {
    composition_.clear();
    encodedCache_.clear();
    encodedCacheDirty_ = true;
}

std::vector<Rule> Engine::applicableRules(char32_t key) const {
    std::vector<Rule> rules;
    for (const Rule& rule : inputMethod_.rules) {
        if (rule.key == key) {
            rules.push_back(rule);
        }
    }
    return rules;
}

bool Engine::canProcessKey(char32_t key) const noexcept {
    if ((key >= U'a' && key <= U'z') || (key >= U'A' && key <= U'Z')) {
        return true;
    }
    return std::find(inputMethod_.keys.begin(), inputMethod_.keys.end(), key) != inputMethod_.keys.end();
}

void Engine::appendRawKey(char32_t key, bool isUpperCase) {
    Transformation trans;
    trans.isUpperCase = isUpperCase;
    trans.rule.key = key;
    trans.rule.effectOn = key;
    trans.rule.result = key;
    trans.rule.effectType = EffectType::Appending;
    composition_.push_back(trans);
    encodedCacheDirty_ = true;
}

void Engine::processKey(char32_t key) {
    const char32_t lowerKey = toLowerCodePoint(key);
    const bool isUpperCase = key != lowerKey;

    if (mode_ == api::Mode::English || !canProcessKey(lowerKey)) {
        appendRawKey(lowerKey, isUpperCase);
        return;
    }

    const std::vector<Rule> rules = applicableRules(lowerKey);
    if (std::find(inputMethod_.superKeys.begin(), inputMethod_.superKeys.end(), lowerKey) != inputMethod_.superKeys.end()) {
        const std::u32string word = currentWord(composition_);
        if (word.size() >= 2) {
            const std::u32string tail = word.substr(word.size() - 2);
            const bool isUoShortcut = tail == U"uo" || tail == U"ưo";
            const bool isUongShortcut = word.size() >= 5 && word.substr(word.size() - 5) == U"uong";
            if (isUoShortcut || isUongShortcut) {
                Transformation* uTarget = nullptr;
                Transformation* oTarget = nullptr;
                for (auto it = composition_.rbegin(); it != composition_.rend(); ++it) {
                    if (it->rule.effectType != EffectType::Appending) {
                        continue;
                    }
                    if (oTarget == nullptr && it->rule.effectOn == U'o') {
                        oTarget = &*it;
                    } else if (uTarget == nullptr && it->rule.effectOn == U'u') {
                        uTarget = &*it;
                    }
                }
                for (const Rule& rule : rules) {
                    if (isUongShortcut && uTarget != nullptr && oTarget != nullptr &&
                        rule.effectType == EffectType::MarkTransformation) {
                        if (rule.effectOn == U'u') {
                            uTarget->rule.effectOn = U'ư';
                            uTarget->rule.result = U'ư';
                        } else if (rule.effectOn == U'o') {
                            oTarget->rule.effectOn = U'ơ';
                            oTarget->rule.result = U'ơ';
                        }
                        continue;
                    }
                    if (rule.effectType == EffectType::MarkTransformation && rule.effectOn == U'o' && oTarget != nullptr) {
                        Transformation trans;
                        trans.rule = rule;
                        trans.target = oTarget;
                        trans.isUpperCase = isUpperCase;
                        composition_.push_back(trans);
                    }
                    if (((word.size() == 2 && tail == U"uo") || isUongShortcut) &&
                        rule.effectType == EffectType::MarkTransformation && rule.effectOn == U'u' && uTarget != nullptr) {
                        Transformation trans;
                        trans.rule = rule;
                        trans.target = uTarget;
                        trans.isUpperCase = isUpperCase;
                        composition_.push_back(trans);
                    }
                }
                if (oTarget != nullptr) {
                    encodedCacheDirty_ = true;
                    return;
                }
            }
        }
    }

    CompositionView syllable = extractLastSyllable(makeCompositionView(composition_));
    std::vector<PendingTransformation> pending;

    if (const PendingTransformation direct = findTarget(syllable, rules); direct.target != nullptr) {
        pending.push_back(direct);
    } else {
        pending = generateUndoTransformations(syllable, rules);
        if (!pending.empty()) {
            Rule rawRule{};
            rawRule.key = lowerKey;
            rawRule.effectOn = lowerKey;
            rawRule.result = lowerKey;
            rawRule.effectType = EffectType::Appending;
            pending.push_back(PendingTransformation{rawRule, nullptr, isUpperCase});
        } else {
            pending = generateFallbackTransformations(rules, lowerKey, isUpperCase);
        }
    }

    for (PendingTransformation& item : pending) {
        Transformation trans;
        trans.rule = item.rule;
        trans.target = item.target;
        trans.isUpperCase = item.isUpperCase;
        composition_.push_back(trans);
    }

    CompositionView updated = extractLastSyllable(makeCompositionView(composition_));
    for (PendingTransformation& item : refreshLastToneTarget(updated)) {
        Transformation trans;
        trans.rule = item.rule;
        trans.target = item.target;
        trans.isUpperCase = item.isUpperCase;
        composition_.push_back(trans);
    }

    encodedCacheDirty_ = true;
}

void Engine::processString(std::string_view str) {
    for (char32_t cp : decodeUtf8(str)) {
        processKey(cp);
    }
}

std::string Engine::getProcessedString() const {
    if (!encodedCacheDirty_) {
        return encodedCache_;
    }
    encodedCache_ = Encoder::encode(CharsetDefinition::kUnicode, flattenVietnamese(composition_, false));
    encodedCacheDirty_ = false;
    return encodedCache_;
}

bool Engine::isValid(bool inputIsFullComplete) const {
    if (mode_ == api::Mode::English) {
        return true;
    }
    const std::u32string word = currentWord(composition_);
    if (word.empty()) {
        return true;
    }
    const Segments segments = splitWord(word);
    Spelling spelling;
    return spelling.isValidCvc(segments.firstConsonant, segments.vowel, segments.lastConsonant, inputIsFullComplete);
}

void Engine::removeLastChar(bool /*refreshLastToneTarget*/) {
    if (!composition_.empty()) {
        composition_.pop_back();
        while (!composition_.empty() && composition_.back().rule.key == 0) {
            composition_.pop_back();
        }
        encodedCacheDirty_ = true;
    }
}

void Engine::restoreLastWord(bool toVietnamese) {
    if (toVietnamese) {
        mode_ = api::Mode::Vietnamese;
        return;
    }

    CompositionView syllable = extractLastSyllable(makeCompositionView(composition_));
    std::deque<Transformation> broken = breakComposition(syllable);
    while (!composition_.empty()) {
        const Transformation& back = composition_.back();
        if (back.rule.effectType == EffectType::Appending &&
            (back.rule.key == U' ' || back.rule.key == U'\n' || back.rule.key == U'\t')) {
            break;
        }
        composition_.pop_back();
    }
    for (Transformation& trans : broken) {
        composition_.push_back(trans);
    }
    mode_ = api::Mode::English;
    encodedCacheDirty_ = true;
}

}  // namespace bamboo::engine

namespace bamboo::api {

std::unique_ptr<IEngine> createEngine(std::string_view dataDirPath, std::string_view inputMethod) {
    return std::make_unique<engine::Engine>(dataDirPath, inputMethod);
}

}  // namespace bamboo::api
