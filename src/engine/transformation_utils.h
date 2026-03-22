#pragma once

#include "engine.h"

#include <array>

namespace bamboo::engine {

struct CompositionView final {
    std::deque<Transformation>* composition{nullptr};
    std::size_t begin{0};
    std::size_t end{0};

    [[nodiscard]] bool empty() const noexcept { return composition == nullptr || begin >= end; }
    [[nodiscard]] std::size_t size() const noexcept { return empty() ? 0 : end - begin; }
    [[nodiscard]] Transformation* operator[](std::size_t index) const noexcept {
        return &(*composition)[begin + index];
    }
    [[nodiscard]] Transformation* front() const noexcept { return (*this)[0]; }
};

template <typename T, std::size_t Capacity>
struct SmallList final {
    std::array<T, Capacity> items{};
    std::size_t count{0};

    [[nodiscard]] bool empty() const noexcept { return count == 0; }
    [[nodiscard]] std::size_t size() const noexcept { return count; }
    [[nodiscard]] const T* begin() const noexcept { return items.data(); }
    [[nodiscard]] const T* end() const noexcept { return items.data() + count; }
    [[nodiscard]] T* begin() noexcept { return items.data(); }
    [[nodiscard]] T* end() noexcept { return items.data() + count; }
    [[nodiscard]] const T& operator[](std::size_t index) const noexcept { return items[index]; }
    [[nodiscard]] T& operator[](std::size_t index) noexcept { return items[index]; }
    [[nodiscard]] const T& front() const noexcept { return items[0]; }
    [[nodiscard]] T& front() noexcept { return items[0]; }

    void push_back(const T& value) noexcept {
        if (count < Capacity) {
            items[count++] = value;
        }
    }
};

struct PendingTransformation final {
    Rule rule;
    Transformation* target{nullptr};
    bool isUpperCase{false};
};

using PendingTransformationList = SmallList<PendingTransformation, 8>;

struct Segments final {
    std::u32string_view firstConsonant;
    std::u32string_view vowel;
    std::u32string_view lastConsonant;
};

[[nodiscard]] CompositionView makeCompositionView(std::deque<Transformation>& composition);
[[nodiscard]] CompositionView extractLastSyllable(const CompositionView& composition);
[[nodiscard]] PendingTransformation findMarkTarget(const CompositionView& composition,
                                                   RuleSpan applicableRules) noexcept;
[[nodiscard]] PendingTransformation findTarget(const CompositionView& composition,
                                               RuleSpan applicableRules) noexcept;
[[nodiscard]] PendingTransformationList generateUndoTransformations(const CompositionView& composition,
                                                                   RuleSpan applicableRules);
[[nodiscard]] PendingTransformationList generateFallbackTransformations(RuleSpan applicableRules,
                                                                       char32_t lowerKey,
                                                                       bool isUpperCase);
[[nodiscard]] PendingTransformationList refreshLastToneTarget(const CompositionView& composition);
[[nodiscard]] std::u32string flattenVietnamese(const std::deque<Transformation>& composition, bool lowerCase);
[[nodiscard]] std::u32string currentWord(const std::deque<Transformation>& composition);
[[nodiscard]] Segments splitWord(std::u32string_view word) noexcept;
[[nodiscard]] std::deque<Transformation> breakComposition(const CompositionView& composition);
[[nodiscard]] char32_t toLowerCodePoint(char32_t codePoint) noexcept;
[[nodiscard]] bool isVowel(char32_t chr) noexcept;

}  // namespace bamboo::engine
