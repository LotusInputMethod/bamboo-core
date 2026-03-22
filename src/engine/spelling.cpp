#include "spelling.h"

#include <array>

namespace bamboo::engine {
namespace {

constexpr std::size_t kMaxTokensPerRow = 31;

struct TokenRow final {
    std::array<std::u32string_view, kMaxTokensPerRow> tokens{};
    std::uint8_t size{0};
};

constexpr TokenRow makeRow(std::initializer_list<std::u32string_view> init) {
    TokenRow row{};
    row.size = static_cast<std::uint8_t>(init.size());
    std::size_t index = 0;
    for (const auto token : init) {
        row.tokens[index++] = token;
    }
    return row;
}

constexpr std::array kFirstConsonantRows{
    makeRow({U"b", U"d", U"đ", U"g", U"gh", U"m", U"n", U"nh", U"p", U"ph", U"r", U"s", U"t", U"tr", U"v", U"z"}),
    makeRow({U"c", U"h", U"k", U"kh", U"kr", U"qu", U"th"}),
    makeRow({U"ch", U"gi", U"l", U"ng", U"ngh", U"x"}),
    makeRow({U"b", U"đ", U"l"}),
    makeRow({U"h"}),
};

constexpr std::array kVowelRows{
    makeRow({U"ê", U"i", U"ua", U"uê", U"uy", U"y"}),
    makeRow({U"a", U"iê", U"oa", U"uyê", U"yê"}),
    makeRow({U"â", U"ă", U"e", U"o", U"oo", U"ô", U"ơ", U"oe", U"u", U"ư", U"uâ", U"uô", U"ươ"}),
    makeRow({U"oă"}),
    makeRow({U"uơ"}),
    makeRow({U"ai", U"ao", U"au", U"âu", U"ay", U"ây", U"eo", U"êu", U"ia", U"iêu", U"iu", U"oai", U"oao", U"oay", U"oeo", U"oi", U"ôi", U"ơi", U"ưa", U"uây", U"ui", U"ưi", U"uôi", U"ươi", U"ươu", U"ưu", U"uya", U"uyu", U"uêu", U"yêu"}),
    makeRow({U"ă", U"u"}),
    makeRow({U"i"}),
};

constexpr std::array kLastConsonantRows{
    makeRow({U"ch", U"nh"}),
    makeRow({U"c", U"ng"}),
    makeRow({U"m", U"n", U"p", U"t"}),
    makeRow({U"k"}),
    makeRow({U"c"}),
};

constexpr std::array<std::array<std::uint8_t, 6>, 5> kCvMatrix{{
    {{0, 1, 2, 5, 0xFF, 0xFF}},
    {{0, 1, 2, 3, 4, 5}},
    {{0, 1, 2, 3, 5, 0xFF}},
    {{6, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}},
    {{7, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}},
}};
constexpr std::array<std::uint8_t, 5> kCvMatrixSizes{{4, 6, 5, 1, 1}};

constexpr std::array<std::array<std::uint8_t, 3>, 8> kVcMatrix{{
    {{0, 2, 0xFF}},
    {{0, 1, 2}},
    {{1, 2, 0xFF}},
    {{1, 2, 0xFF}},
    {{0xFF, 0xFF, 0xFF}},
    {{0xFF, 0xFF, 0xFF}},
    {{3, 0xFF, 0xFF}},
    {{4, 0xFF, 0xFF}},
}};
constexpr std::array<std::uint8_t, 8> kVcMatrixSizes{{2, 3, 2, 2, 0, 0, 1, 1}};

bool matchesRow(const TokenRow& row,
                              std::u32string_view input,
                              bool inputIsFull,
                              bool inputIsComplete,
                              std::uint8_t rowIndex,
                              IndexSet<8>& result) noexcept {
    for (std::uint8_t tokenIndex = 0; tokenIndex < row.size; ++tokenIndex) {
        const std::u32string_view token = row.tokens[tokenIndex];
        if (token.size() < input.size() || (inputIsFull && token.size() > input.size())) {
            continue;
        }

        bool isMatch = true;
        for (std::size_t i = 0; i < input.size(); ++i) {
            if (input[i] == token[i]) {
                continue;
            }
            if (!inputIsComplete && Spelling::normalizeToneless(token[i]) == input[i]) {
                continue;
            }
            isMatch = false;
            break;
        }
        if (isMatch) {
            result.push(rowIndex);
            return true;
        }
    }
    return false;
}

template <std::size_t RowCount>
auto lookupRows(const std::array<TokenRow, RowCount>& rows,
                              std::u32string_view input,
                              bool inputIsFull,
                              bool inputIsComplete) noexcept {
    IndexSet<8> result;
    for (std::uint8_t rowIndex = 0; rowIndex < rows.size(); ++rowIndex) {
        matchesRow(rows[rowIndex], input, inputIsFull, inputIsComplete, rowIndex, result);
    }
    return result;
}

}  // namespace

char32_t Spelling::normalizeToneless(char32_t codePoint) noexcept {
    switch (codePoint) {
    case U'â':
    case U'ă':
        return U'a';
    case U'ê':
        return U'e';
    case U'ô':
    case U'ơ':
        return U'o';
    case U'ư':
        return U'u';
    case U'đ':
        return U'd';
    default:
        return codePoint;
    }
}

bool Spelling::isValidCv(const RowMatchSet& firstConsonantIndexes,
                         const RowMatchSet& vowelIndexes) noexcept {
    for (std::uint8_t i = 0; i < firstConsonantIndexes.size; ++i) {
        const std::uint8_t firstConsonant = firstConsonantIndexes.values[i];
        for (std::uint8_t cvIndex = 0; cvIndex < kCvMatrixSizes[firstConsonant]; ++cvIndex) {
            const std::uint8_t allowedVowel = kCvMatrix[firstConsonant][cvIndex];
            for (std::uint8_t j = 0; j < vowelIndexes.size; ++j) {
                if (allowedVowel == vowelIndexes.values[j]) {
                    return true;
                }
            }
        }
    }
    return false;
}

bool Spelling::isValidVc(const RowMatchSet& vowelIndexes,
                         const RowMatchSet& lastConsonantIndexes) noexcept {
    for (std::uint8_t i = 0; i < vowelIndexes.size; ++i) {
        const std::uint8_t vowel = vowelIndexes.values[i];
        for (std::uint8_t vcIndex = 0; vcIndex < kVcMatrixSizes[vowel]; ++vcIndex) {
            const std::uint8_t allowedLastConsonant = kVcMatrix[vowel][vcIndex];
            for (std::uint8_t j = 0; j < lastConsonantIndexes.size; ++j) {
                if (allowedLastConsonant == lastConsonantIndexes.values[j]) {
                    return true;
                }
            }
        }
    }
    return false;
}

bool Spelling::isValidCvc(std::u32string_view firstConsonant,
                          std::u32string_view vowel,
                          std::u32string_view lastConsonant,
                          bool inputIsFullComplete) const noexcept {
    RowMatchSet firstConsonantIndexes;
    RowMatchSet vowelIndexes;
    RowMatchSet lastConsonantIndexes;

    if (!firstConsonant.empty()) {
        firstConsonantIndexes = lookupRows(kFirstConsonantRows,
                                           firstConsonant,
                                           inputIsFullComplete || !vowel.empty(),
                                           true);
        if (firstConsonantIndexes.empty()) {
            return false;
        }
    }

    if (!vowel.empty()) {
        vowelIndexes = lookupRows(kVowelRows,
                                  vowel,
                                  inputIsFullComplete || !lastConsonant.empty(),
                                  inputIsFullComplete);
        if (vowelIndexes.empty()) {
            return false;
        }
    }

    if (!lastConsonant.empty()) {
        lastConsonantIndexes = lookupRows(kLastConsonantRows,
                                          lastConsonant,
                                          inputIsFullComplete,
                                          true);
        if (lastConsonantIndexes.empty()) {
            return false;
        }
    }

    if (vowelIndexes.empty()) {
        return !firstConsonantIndexes.empty();
    }

    if (!firstConsonantIndexes.empty()) {
        const bool validCv = isValidCv(firstConsonantIndexes, vowelIndexes);
        if (!validCv || lastConsonantIndexes.empty()) {
            return validCv;
        }
    }

    if (!lastConsonantIndexes.empty()) {
        return isValidVc(vowelIndexes, lastConsonantIndexes);
    }

    return true;
}

}  // namespace bamboo::engine
