#include "input_method_definition.h"

#include <array>

namespace bamboo::engine {
namespace {

using MappingView = InputMethodDefinition::MappingView;
using DefinitionView = InputMethodDefinition::DefinitionView;

constexpr std::array<MappingView, 11> kTelexMappings{{
    {U'z', "XoaDauThanh"},
    {U's', "DauSac"},
    {U'f', "DauHuyen"},
    {U'r', "DauHoi"},
    {U'x', "DauNga"},
    {U'j', "DauNang"},
    {U'a', "A_Â"},
    {U'e', "E_Ê"},
    {U'o', "O_Ô"},
    {U'w', "UOA_ƯƠĂ"},
    {U'd', "D_Đ"},
}};

constexpr std::array<MappingView, 10> kVniMappings{{
    {U'0', "XoaDauThanh"},
    {U'1', "DauSac"},
    {U'2', "DauHuyen"},
    {U'3', "DauHoi"},
    {U'4', "DauNga"},
    {U'5', "DauNang"},
    {U'6', "AEO_ÂÊÔ"},
    {U'7', "UO_ƯƠ"},
    {U'8', "A_Ă"},
    {U'9', "D_Đ"},
}};

constexpr std::array<MappingView, 11> kViqrMappings{{
    {U'0', "XoaDauThanh"},
    {U'\'', "DauSac"},
    {U'`', "DauHuyen"},
    {U'?', "DauHoi"},
    {U'~', "DauNga"},
    {U'.', "DauNang"},
    {U'^', "AEO_ÂÊÔ"},
    {U'+', "UO_ƯƠ"},
    {U'*', "UO_ƯƠ"},
    {U'(', "A_Ă"},
    {U'd', "D_Đ"},
}};

constexpr std::array<MappingView, 19> kMicrosoftMappings{{
    {U'8', "DauSac"},
    {U'5', "DauHuyen"},
    {U'6', "DauHoi"},
    {U'7', "DauNga"},
    {U'9', "DauNang"},
    {U'1', "__ă"},
    {U'!', "_Ă"},
    {U'2', "__â"},
    {U'@', "_Â"},
    {U'3', "__ê"},
    {U'#', "_Ê"},
    {U'4', "__ô"},
    {U'$', "_Ô"},
    {U'0', "__đ"},
    {U')', "_Đ"},
    {U'[', "__ư"},
    {U'{', "_Ư"},
    {U']', "__ơ"},
    {U'}', "_Ơ"},
}};

constexpr std::array<MappingView, 15> kTelex2Mappings{{
    {U'z', "XoaDauThanh"},
    {U's', "DauSac"},
    {U'f', "DauHuyen"},
    {U'r', "DauHoi"},
    {U'x', "DauNga"},
    {U'j', "DauNang"},
    {U'a', "A_Â"},
    {U'e', "E_Ê"},
    {U'o', "O_Ô"},
    {U'w', "UOA_ƯƠĂ__Ư"},
    {U'd', "D_Đ"},
    {U']', "__ư"},
    {U'[', "__ơ"},
    {U'}', "_Ư"},
    {U'{', "_Ơ"},
}};

constexpr std::array<MappingView, 21> kTelexVniMappings{{
    {U'z', "XoaDauThanh"},
    {U's', "DauSac"},
    {U'f', "DauHuyen"},
    {U'r', "DauHoi"},
    {U'x', "DauNga"},
    {U'j', "DauNang"},
    {U'a', "A_Â"},
    {U'e', "E_Ê"},
    {U'o', "O_Ô"},
    {U'w', "UOA_ƯƠĂ"},
    {U'd', "D_Đ"},
    {U'0', "XoaDauThanh"},
    {U'1', "DauSac"},
    {U'2', "DauHuyen"},
    {U'3', "DauHoi"},
    {U'4', "DauNga"},
    {U'5', "DauNang"},
    {U'6', "AEO_ÂÊÔ"},
    {U'7', "UO_ƯƠ"},
    {U'8', "A_Ă"},
    {U'9', "D_Đ"},
}};

constexpr std::array<MappingView, 31> kTelexVniViqrMappings{{
    {U'z', "XoaDauThanh"},
    {U's', "DauSac"},
    {U'f', "DauHuyen"},
    {U'r', "DauHoi"},
    {U'x', "DauNga"},
    {U'j', "DauNang"},
    {U'a', "A_Â"},
    {U'e', "E_Ê"},
    {U'o', "O_Ô"},
    {U'w', "UOA_ƯƠĂ"},
    {U'd', "D_Đ"},
    {U'0', "XoaDauThanh"},
    {U'1', "DauSac"},
    {U'2', "DauHuyen"},
    {U'3', "DauHoi"},
    {U'4', "DauNga"},
    {U'5', "DauNang"},
    {U'6', "AEO_ÂÊÔ"},
    {U'7', "UO_ƯƠ"},
    {U'8', "A_Ă"},
    {U'9', "D_Đ"},
    {U'\'', "DauSac"},
    {U'`', "DauHuyen"},
    {U'?', "DauHoi"},
    {U'~', "DauNga"},
    {U'.', "DauNang"},
    {U'^', "AEO_ÂÊÔ"},
    {U'+', "UO_ƯƠ"},
    {U'*', "UO_ƯƠ"},
    {U'(', "A_Ă"},
    {U'\\', "D_Đ"},
}};

constexpr std::array<MappingView, 10> kFrenchVniMappings{{
    {U'&', "XoaDauThanh"},
    {U'é', "DauSac"},
    {U'"', "DauHuyen"},
    {U'\'', "DauHoi"},
    {U'(', "DauNga"},
    {U'-', "DauNang"},
    {U'è', "AEO_ÂÊÔ"},
    {U'_', "UO_ƯƠ"},
    {U'ç', "A_Ă"},
    {U'à', "D_Đ"},
}};

constexpr std::array<MappingView, 11> kTelexWMappings{{
    {U'z', "XoaDauThanh"},
    {U's', "DauSac"},
    {U'f', "DauHuyen"},
    {U'r', "DauHoi"},
    {U'x', "DauNga"},
    {U'j', "DauNang"},
    {U'a', "A_Â"},
    {U'e', "E_Ê"},
    {U'o', "O_Ô"},
    {U'w', "UOA_ƯƠĂ__Ư"},
    {U'd', "D_Đ"},
}};

constexpr std::array<DefinitionView, 9> kDefinitions{{
    {"Telex", kTelexMappings.data(), kTelexMappings.size()},
    {"VNI", kVniMappings.data(), kVniMappings.size()},
    {"VIQR", kViqrMappings.data(), kViqrMappings.size()},
    {"Microsoft layout", kMicrosoftMappings.data(), kMicrosoftMappings.size()},
    {"Telex 2", kTelex2Mappings.data(), kTelex2Mappings.size()},
    {"Telex + VNI", kTelexVniMappings.data(), kTelexVniMappings.size()},
    {"Telex + VNI + VIQR", kTelexVniViqrMappings.data(), kTelexVniViqrMappings.size()},
    {"VNI Bàn phím tiếng Pháp", kFrenchVniMappings.data(), kFrenchVniMappings.size()},
    {"Telex W", kTelexWMappings.data(), kTelexWMappings.size()},
}};

constexpr std::array<std::string_view, 9> kNames{{
    "Telex",
    "VNI",
    "VIQR",
    "Microsoft layout",
    "Telex 2",
    "Telex + VNI",
    "Telex + VNI + VIQR",
    "VNI Bàn phím tiếng Pháp",
    "Telex W",
}};

[[nodiscard]] std::string_view lookupInMappings(const DefinitionView& definition, char32_t key) noexcept {
    for (std::size_t index = 0; index < definition.size; ++index) {
        if (definition.mappings[index].key == key) {
            return definition.mappings[index].action;
        }
    }
    return {};
}

}  // namespace

const InputMethodDefinition::DefinitionView* InputMethodDefinition::find(std::string_view name) noexcept {
    for (const auto& definition : kDefinitions) {
        if (definition.name == name) {
            return &definition;
        }
    }
    return nullptr;
}

std::string_view InputMethodDefinition::lookupAction(std::string_view name, char32_t key) noexcept {
    const DefinitionView* definition = find(name);
    if (definition == nullptr) {
        return {};
    }
    return lookupInMappings(*definition, key);
}

const std::array<std::string_view, 9>& InputMethodDefinition::names() noexcept {
    return kNames;
}

}  // namespace bamboo::engine
