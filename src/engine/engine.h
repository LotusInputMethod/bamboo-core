#pragma once

#include "bamboo/IEngine.h"
#include "rule.h"

#include <deque>
#include <string>
#include <string_view>

namespace bamboo::engine {

struct Transformation final {
    Rule rule;
    Transformation* target{nullptr};
    bool isUpperCase{false};
};

class Engine final : public api::IEngine {
public:
    Engine(std::string_view dataDirPath, std::string_view inputMethod);
    ~Engine() override = default;

    void setMode(api::Mode mode) override;
    [[nodiscard]] api::Mode getMode() const override;
    void reset() override;
    void processKey(char32_t key) override;
    void processString(std::string_view str) override;
    [[nodiscard]] std::string getProcessedString() const override;
    [[nodiscard]] bool isValid(bool inputIsFullComplete) const override;
    void removeLastChar(bool refreshLastToneTarget) override;
    void restoreLastWord(bool toVietnamese) override;

private:
    [[nodiscard]] std::vector<Rule> applicableRules(char32_t key) const;
    [[nodiscard]] bool canProcessKey(char32_t key) const noexcept;
    void appendRawKey(char32_t key, bool isUpperCase);

    api::Mode mode_{api::Mode::Vietnamese};
    std::string dataDirPath_;
    InputMethod inputMethod_;
    std::deque<Transformation> composition_;
    mutable std::string encodedCache_;
    mutable bool encodedCacheDirty_{true};
};

}  // namespace bamboo::engine
