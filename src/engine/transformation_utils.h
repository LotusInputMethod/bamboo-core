#pragma once

#include "engine.h"

#include <vector>

namespace bamboo::engine {

using CompositionView = std::vector<Transformation*>;

struct PendingTransformation final {
    Rule rule;
    Transformation* target{nullptr};
    bool isUpperCase{false};
};

struct Segments final {
    std::u32string_view firstConsonant;
    std::u32string_view vowel;
    std::u32string_view lastConsonant;
};

[[nodiscard]] CompositionView makeCompositionView(std::deque<Transformation>& composition);
[[nodiscard]] CompositionView extractLastSyllable(const CompositionView& composition);
[[nodiscard]] PendingTransformation findMarkTarget(const CompositionView& composition,
                                                   const std::vector<Rule>& applicableRules) noexcept;
[[nodiscard]] PendingTransformation findTarget(const CompositionView& composition,
                                               const std::vector<Rule>& applicableRules) noexcept;
[[nodiscard]] std::vector<PendingTransformation> generateUndoTransformations(const CompositionView& composition,
                                                                             const std::vector<Rule>& applicableRules);
[[nodiscard]] std::vector<PendingTransformation> generateFallbackTransformations(const std::vector<Rule>& applicableRules,
                                                                                 char32_t lowerKey,
                                                                                 bool isUpperCase);
[[nodiscard]] std::vector<PendingTransformation> refreshLastToneTarget(const CompositionView& composition);
[[nodiscard]] std::u32string flattenVietnamese(const std::deque<Transformation>& composition, bool lowerCase);
[[nodiscard]] std::u32string currentWord(const std::deque<Transformation>& composition);
[[nodiscard]] Segments splitWord(std::u32string_view word) noexcept;
[[nodiscard]] std::deque<Transformation> breakComposition(const CompositionView& composition);
[[nodiscard]] char32_t toLowerCodePoint(char32_t codePoint) noexcept;
[[nodiscard]] bool isVowel(char32_t chr) noexcept;

}  // namespace bamboo::engine
