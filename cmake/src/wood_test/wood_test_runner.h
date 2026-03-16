#pragma once
#include <map>
#include <vector>
#include <string>
#include <chrono>
#include <fstream>
#include <iostream>
#include <cstdio>
#include <stdexcept>

namespace wood_test {

struct Check {
    int line;
    std::string code_line;
    bool passed;
};

struct Result {
    std::string test_name;
    bool passed;
    double time_ms;
    std::string code;
    std::vector<Check> checks;
};

// Global registry: suite_name → vector of results
// Key format: "shapes_cpp", "folded_plates_cpp", etc.
inline std::map<std::string, std::vector<Result>>& registry() {
    static std::map<std::string, std::vector<Result>> r;
    return r;
}

inline std::string escape_json(const std::string& s) {
    std::string r;
    r.reserve(s.size());
    for (unsigned char c : s) {
        switch (c) {
        case '"':  r += "\\\""; break;
        case '\\': r += "\\\\"; break;
        case '\n': r += "\\n";  break;
        case '\r': r += "\\r";  break;
        case '\t': r += "\\t";  break;
        default:
            if (c < 0x20) {
                char buf[8];
                std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                r += buf;
            } else {
                r += static_cast<char>(c);
            }
        }
    }
    return r;
}

// Write window.TEST_DATA = { "shapes_cpp": [...], ... };
// Compatible with the Vue wood-test viewer.
inline void dump_testdata_js(const std::string& path) {
    std::ofstream f(path);
    if (!f.is_open()) {
        std::cerr << "[wood_test] ERROR: cannot open " << path << "\n";
        return;
    }

    f << "window.TEST_DATA = {\n";
    auto& reg = registry();
    bool first_suite = true;
    for (auto& [suite, results] : reg) {
        if (!first_suite) f << ",\n";
        first_suite = false;
        f << "  \"" << escape_json(suite) << "\": [\n";
        bool first_result = true;
        for (auto& r : results) {
            if (!first_result) f << ",\n";
            first_result = false;
            f << "    {\n";
            f << "      \"test_name\": \"" << escape_json(r.test_name) << "\",\n";
            f << "      \"passed\": " << (r.passed ? "true" : "false") << ",\n";
            f << "      \"time_ms\": " << r.time_ms << ",\n";
            f << "      \"language\": \"cpp\",\n";
            f << "      \"code\": \"" << escape_json(r.code) << "\",\n";
            f << "      \"checks\": [";
            bool first_check = true;
            for (auto& c : r.checks) {
                if (!first_check) f << ", ";
                first_check = false;
                f << "{\"line\": " << c.line
                  << ", \"code_line\": \"" << escape_json(c.code_line) << "\""
                  << ", \"passed\": " << (c.passed ? "true" : "false") << "}";
            }
            f << "]\n";
            f << "    }";
        }
        if (!results.empty()) f << "\n";
        f << "  ]";
    }
    f << "\n};\n";

    int total = 0, passed_count = 0;
    for (auto& [suite, results] : reg) {
        for (auto& r : results) {
            ++total;
            if (r.passed) ++passed_count;
        }
    }
    std::cerr << "[wood_test] Wrote " << path << "  ("
              << passed_count << "/" << total << " passed)\n";
}

} // namespace wood_test

// ─── Macros ──────────────────────────────────────────────────────────────────

// CHECK(expr) — assert inside a TEST_CPP body.
// Records the expression text, line number, and result.
#define CHECK(expr)                                                         \
    do {                                                                    \
        bool _check_r = static_cast<bool>(expr);                           \
        _wt_checks.push_back({__LINE__, #expr, _check_r});                 \
        if (!_check_r) _wt_passed = false;                                 \
    } while (0)

// TEST_CPP(suite_key, test_name, code_display_str, body...)
//   suite_key  — registry key, e.g. "shapes_cpp"
//   test_name  — display name shown in the viewer
//   code_str   — string literal shown in the code block
//   body       — C++ statements to execute (may use CHECK)
#define TEST_CPP(suite_key, test_name_str, code_str, ...)                  \
    do {                                                                    \
        std::vector<wood_test::Check> _wt_checks;                          \
        bool _wt_passed = true;                                            \
        double _wt_ms = 0.0;                                               \
        auto _wt_body = [&]() {                                            \
            __VA_ARGS__                                                     \
        };                                                                  \
        try {                                                               \
            auto _t0 = std::chrono::high_resolution_clock::now();          \
            _wt_body();                                                     \
            auto _t1 = std::chrono::high_resolution_clock::now();          \
            _wt_ms = std::chrono::duration<double, std::milli>(            \
                         _t1 - _t0).count();                               \
        } catch (const std::exception& _ex) {                              \
            _wt_passed = false;                                            \
            _wt_checks.push_back(                                          \
                {0, std::string("exception: ") + _ex.what(), false});      \
        } catch (...) {                                                     \
            _wt_passed = false;                                            \
            _wt_checks.push_back({0, "unknown exception", false});         \
        }                                                                   \
        wood_test::registry()[suite_key].push_back(                        \
            {test_name_str, _wt_passed, _wt_ms, code_str,                 \
             std::move(_wt_checks)});                                       \
    } while (0)
