#include "encoder.h"

#include "charset_definition.h"

namespace bamboo::engine {

std::string Encoder::encode(std::string_view charsetName, std::u32string_view input) {
    return CharsetDefinition::encode(charsetName, input);
}

const std::array<std::string_view, 17>& Encoder::charsetNames() noexcept {
    return CharsetDefinition::names();
}

}  // namespace bamboo::engine
