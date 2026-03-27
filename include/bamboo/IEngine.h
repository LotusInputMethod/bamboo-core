#pragma once

#include <memory>
#include <string>
#include <string_view>

namespace bamboo::api {

enum class Mode { Vietnamese, English };

class IEngine {
public:
    virtual ~IEngine() = default;

    virtual void setMode(Mode mode) = 0;
    [[nodiscard]] virtual Mode getMode() const = 0;
    virtual void reset() = 0;
    virtual void processKey(char32_t key) = 0;
    virtual void processString(std::string_view str) = 0;
    [[nodiscard]] virtual std::string getProcessedString() const = 0;
    [[nodiscard]] virtual bool isValid(bool inputIsFullComplete) const = 0;
    virtual void removeLastChar(bool refreshLastToneTarget) = 0;
    virtual void restoreLastWord(bool toVietnamese) = 0;
};

[[nodiscard]] std::unique_ptr<IEngine> createEngine(std::string_view dataDirPath,
                                                     std::string_view inputMethod);

}  // namespace bamboo::api
