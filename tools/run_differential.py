#!/usr/bin/env python3
"""Run a lightweight Go-vs-C++ differential harness for the migrated engine."""

from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys
import tempfile
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]
CASES_PATH = REPO_ROOT / "tests" / "differential_cases.json"


def cpp_string_literal(value: str) -> str:
    return json.dumps(value)


def emit_cpp_cases(cases: list[dict]) -> str:
    lines: list[str] = []
    for case in cases:
        lines.append("    cases.push_back(TestCase{")
        lines.append(f"        {cpp_string_literal(case['id'])},")
        lines.append(f"        {cpp_string_literal(case['input_method'])},")
        lines.append(f"        parseModeName({cpp_string_literal(case['initial_mode'])}),")
        lines.append("        {")
        for step in case["steps"]:
            lines.append("            Step{")
            lines.append(f"                {cpp_string_literal(step['op'])},")
            lines.append(f"                {cpp_string_literal(step.get('value', ''))},")
            lines.append(f"                {cpp_string_literal(step.get('mode', ''))},")
            lines.append(f"                {'true' if step.get('refresh', False) else 'false'}")
            lines.append("            },")
        lines.append("        }")
        lines.append("    });")
    return "\n".join(lines)


def generate_cpp_runner(cases: list[dict]) -> str:
    emitted_cases = emit_cpp_cases(cases)
    return f"""#include "bamboo/IEngine.h"

#include <cstdint>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using bamboo::api::IEngine;
using bamboo::api::Mode;

struct Step {{
    std::string op;
    std::string value;
    std::string mode;
    bool refresh;
}};

struct TestCase {{
    std::string id;
    std::string input_method;
    Mode initial_mode;
    std::vector<Step> steps;
}};

struct Snapshot {{
    std::string mode;
    std::string text;
    bool valid_partial;
    bool valid_full;
}};

Mode parseModeName(const std::string& mode) {{
    if (mode == "English") {{
        return Mode::English;
    }}
    return Mode::Vietnamese;
}}

std::string modeName(Mode mode) {{
    return mode == Mode::English ? "English" : "Vietnamese";
}}

std::string escapeJson(const std::string& input) {{
    std::ostringstream out;
    for (unsigned char ch : input) {{
        switch (ch) {{
        case '\\\\': out << "\\\\\\\\"; break;
        case '"': out << "\\\\\\""; break;
        case '\\n': out << "\\\\n"; break;
        case '\\r': out << "\\\\r"; break;
        case '\\t': out << "\\\\t"; break;
        default:
            if (ch < 0x20) {{
                out << "\\\\u";
                const char* digits = "0123456789abcdef";
                out << '0' << '0' << digits[(ch >> 4) & 0xF] << digits[ch & 0xF];
            }} else {{
                out << static_cast<char>(ch);
            }}
        }}
    }}
    return out.str();
}}

char32_t decodeSingleCodePoint(const std::string& input) {{
    if (input.empty()) {{
        return 0;
    }}
    const unsigned char byte0 = static_cast<unsigned char>(input[0]);
    if (byte0 < 0x80) {{
        return static_cast<char32_t>(byte0);
    }}
    if ((byte0 & 0xE0U) == 0xC0U && input.size() >= 2) {{
        const unsigned char byte1 = static_cast<unsigned char>(input[1]);
        return static_cast<char32_t>(((byte0 & 0x1FU) << 6) | (byte1 & 0x3FU));
    }}
    if ((byte0 & 0xF0U) == 0xE0U && input.size() >= 3) {{
        const unsigned char byte1 = static_cast<unsigned char>(input[1]);
        const unsigned char byte2 = static_cast<unsigned char>(input[2]);
        return static_cast<char32_t>(((byte0 & 0x0FU) << 12) | ((byte1 & 0x3FU) << 6) | (byte2 & 0x3FU));
    }}
    if ((byte0 & 0xF8U) == 0xF0U && input.size() >= 4) {{
        const unsigned char byte1 = static_cast<unsigned char>(input[1]);
        const unsigned char byte2 = static_cast<unsigned char>(input[2]);
        const unsigned char byte3 = static_cast<unsigned char>(input[3]);
        return static_cast<char32_t>(((byte0 & 0x07U) << 18) | ((byte1 & 0x3FU) << 12) |
                                     ((byte2 & 0x3FU) << 6) | (byte3 & 0x3FU));
    }}
    return static_cast<char32_t>(byte0);
}}

Snapshot snapshot(const IEngine& engine) {{
    return Snapshot{{modeName(engine.getMode()), engine.getProcessedString(), engine.isValid(false), engine.isValid(true)}};
}}

int main() {{
    std::vector<TestCase> cases;
{emitted_cases}

    std::ostringstream out;
    out << "[";
    for (std::size_t case_index = 0; case_index < cases.size(); ++case_index) {{
        const TestCase& test_case = cases[case_index];
        std::unique_ptr<IEngine> engine = bamboo::api::createEngine("", test_case.input_method);
        engine->setMode(test_case.initial_mode);

        out << "{{\\"id\\":\\"" << escapeJson(test_case.id) << "\\",\\"snapshots\\":[";
        for (std::size_t step_index = 0; step_index < test_case.steps.size(); ++step_index) {{
            const Step& step = test_case.steps[step_index];
            if (step.op == "set_mode") {{
                engine->setMode(parseModeName(step.mode));
            }} else if (step.op == "process_string") {{
                engine->processString(step.value);
            }} else if (step.op == "process_key") {{
                engine->processKey(decodeSingleCodePoint(step.value));
            }} else if (step.op == "remove_last_char") {{
                engine->removeLastChar(step.refresh);
            }} else if (step.op == "restore_last_word") {{
                engine->restoreLastWord(step.mode == "Vietnamese");
            }} else if (step.op == "reset") {{
                engine->reset();
            }} else {{
                throw std::runtime_error("unknown op: " + step.op);
            }}

            const Snapshot state = snapshot(*engine);
            if (step_index != 0) {{
                out << ",";
            }}
            out << "{{\\"mode\\":\\"" << state.mode
                << "\\",\\"text\\":\\"" << escapeJson(state.text)
                << "\\",\\"valid_partial\\":" << (state.valid_partial ? "true" : "false")
                << ",\\"valid_full\\":" << (state.valid_full ? "true" : "false") << "}}";
        }}
        out << "]}}";
        if (case_index + 1 != cases.size()) {{
            out << ",";
        }}
    }}
    out << "]";
    std::cout << out.str();
    return 0;
}}
"""


def generate_go_runner(cases: list[dict]) -> str:
    cases_json = json.dumps(cases, ensure_ascii=False)
    return f"""package main

import (
    "encoding/json"
    "fmt"
    bamboo "github.com/LotusInputMethod/bamboo-core"
)

type Step struct {{
    Op      string `json:"op"`
    Value   string `json:"value"`
    Mode    string `json:"mode"`
    Refresh bool   `json:"refresh"`
}}

type TestCase struct {{
    ID          string `json:"id"`
    InputMethod string `json:"input_method"`
    InitialMode string `json:"initial_mode"`
    Steps       []Step `json:"steps"`
}}

type Snapshot struct {{
    Mode         string `json:"mode"`
    Text         string `json:"text"`
    ValidPartial bool   `json:"valid_partial"`
    ValidFull    bool   `json:"valid_full"`
}}

type Result struct {{
    ID        string     `json:"id"`
    Snapshots []Snapshot `json:"snapshots"`
}}

func modeFromName(name string) bamboo.Mode {{
    if name == "English" {{
        return bamboo.EnglishMode
    }}
    return bamboo.VietnameseMode
}}

func modeName(mode bamboo.Mode) string {{
    if mode == bamboo.EnglishMode {{
        return "English"
    }}
    return "Vietnamese"
}}

func textMode(mode bamboo.Mode) bamboo.Mode {{
    if mode == bamboo.EnglishMode {{
        return bamboo.EnglishMode | bamboo.FullText
    }}
    return bamboo.VietnameseMode | bamboo.FullText
}}

func main() {{
    var cases []TestCase
    if err := json.Unmarshal([]byte({json.dumps(cases_json, ensure_ascii=False)}), &cases); err != nil {{
        panic(err)
    }}

    results := make([]Result, 0, len(cases))
    for _, tc := range cases {{
        im := bamboo.ParseInputMethod(bamboo.InputMethodDefinitions, tc.InputMethod)
        eng := bamboo.NewEngine(im, bamboo.EstdFlags)
        currentMode := modeFromName(tc.InitialMode)
        snapshots := make([]Snapshot, 0, len(tc.Steps))

        for _, step := range tc.Steps {{
            switch step.Op {{
            case "set_mode":
                currentMode = modeFromName(step.Mode)
            case "process_string":
                eng.ProcessString(step.Value, currentMode)
            case "process_key":
                runes := []rune(step.Value)
                if len(runes) != 1 {{
                    panic("process_key requires exactly one rune")
                }}
                eng.ProcessKey(runes[0], currentMode)
            case "remove_last_char":
                eng.RemoveLastChar(step.Refresh)
            case "restore_last_word":
                toVietnamese := step.Mode == "Vietnamese"
                eng.RestoreLastWord(toVietnamese)
                currentMode = modeFromName(step.Mode)
            case "reset":
                eng.Reset()
            default:
                panic("unknown op: " + step.Op)
            }}

            snapshots = append(snapshots, Snapshot{{
                Mode:         modeName(currentMode),
                Text:         eng.GetProcessedString(textMode(currentMode)),
                ValidPartial: eng.IsValid(false),
                ValidFull:    eng.IsValid(true),
            }})
        }}

        results = append(results, Result{{ID: tc.ID, Snapshots: snapshots}})
    }}

    data, err := json.Marshal(results)
    if err != nil {{
        panic(err)
    }}
    fmt.Print(string(data))
}}
"""


def run_command(cmd: list[str], cwd: Path) -> subprocess.CompletedProcess[str]:
    return subprocess.run(cmd, cwd=cwd, text=True, capture_output=True, check=False)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--cases", default=str(CASES_PATH))
    args = parser.parse_args()

    with open(args.cases, "r", encoding="utf-8") as fh:
        cases = json.load(fh)

    with tempfile.TemporaryDirectory(prefix="bamboo-diff-") as tmp:
        tmpdir = Path(tmp)
        cpp_src = tmpdir / "runner.cpp"
        cpp_bin = tmpdir / "runner_cpp"
        go_src = tmpdir / "runner.go"

        cpp_src.write_text(generate_cpp_runner(cases), encoding="utf-8")
        go_src.write_text(generate_go_runner(cases), encoding="utf-8")

        cpp_cmd = [
            "g++",
            "-std=c++17",
            "-Iinclude",
            str(cpp_src),
            "src/engine/engine.cpp",
            "src/engine/spelling.cpp",
            "src/engine/encoder.cpp",
            "src/engine/charset_definition.cpp",
            "src/engine/input_method_definition.cpp",
            "src/engine/rules_parser.cpp",
            "src/engine/transformation_utils.cpp",
            "-o",
            str(cpp_bin),
        ]
        cpp_build = run_command(cpp_cmd, REPO_ROOT)
        if cpp_build.returncode != 0:
            sys.stderr.write(cpp_build.stdout)
            sys.stderr.write(cpp_build.stderr)
            return cpp_build.returncode

        go_run = run_command(["go", "run", str(go_src)], REPO_ROOT)
        if go_run.returncode != 0:
            sys.stderr.write(go_run.stdout)
            sys.stderr.write(go_run.stderr)
            return go_run.returncode

        cpp_run = run_command([str(cpp_bin)], REPO_ROOT)
        if cpp_run.returncode != 0:
            sys.stderr.write(cpp_run.stdout)
            sys.stderr.write(cpp_run.stderr)
            return cpp_run.returncode

        go_results = json.loads(go_run.stdout)
        cpp_results = json.loads(cpp_run.stdout)
        if go_results != cpp_results:
            sys.stderr.write("Differential mismatch detected.\\n")
            sys.stderr.write("Go results:\\n")
            sys.stderr.write(json.dumps(go_results, ensure_ascii=False, indent=2) + "\\n")
            sys.stderr.write("C++ results:\\n")
            sys.stderr.write(json.dumps(cpp_results, ensure_ascii=False, indent=2) + "\\n")
            return 1

        print(json.dumps(cpp_results, ensure_ascii=False, indent=2))
        return 0


if __name__ == "__main__":
    raise SystemExit(main())
