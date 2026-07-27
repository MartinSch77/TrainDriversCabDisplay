#ifndef RD_TEST_H
#define RD_TEST_H

// Minimal dependency-free test harness with JUnit XML output. It keeps the
// core tests framework-free while giving tools/run_tests.sh per-function
// results — the result leg of the traceability chain that
// tools/trace_report.py joins with the requirement/design/test-spec legs.
//
// Usage in a test binary:
//   static void TS_XXX_001_case() { RD_CHECK(cond, "what"); }
//   int main(int argc, char **argv) {
//       RD_RUN(TS_XXX_001_case);
//       return rdtest::finish("tst_name", argc, argv);
//   }
// Run with `--junit out.xml` to record the per-function results.

#include <chrono>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

namespace rdtest {

struct CaseResult {
    std::string name;
    std::string failure; // empty = pass
    double ms = 0.0;
};

inline std::vector<CaseResult> &results()
{
    static std::vector<CaseResult> r;
    return r;
}

inline std::string &currentFailure()
{
    static std::string f;
    return f;
}

inline void check(bool ok, const std::string &what, const char *file, int line)
{
    std::printf("%s  %s\n", ok ? "[PASS]" : "[FAIL]", what.c_str());
    if (!ok && currentFailure().empty())
        currentFailure() = what + " (" + file + ":" + std::to_string(line) + ")";
}

template <typename Fn>
void runCase(const char *name, Fn fn)
{
    std::printf("=== %s ===\n", name);
    currentFailure().clear();
    const auto t0 = std::chrono::steady_clock::now();
    fn();
    const auto t1 = std::chrono::steady_clock::now();
    results().push_back({name, currentFailure(),
                         std::chrono::duration<double, std::milli>(t1 - t0).count()});
}

inline std::string xmlEscape(const std::string &s)
{
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
        case '&': out += "&amp;"; break;
        case '<': out += "&lt;"; break;
        case '>': out += "&gt;"; break;
        case '"': out += "&quot;"; break;
        default: out += c;
        }
    }
    return out;
}

inline int finish(const char *suite, int argc, char **argv)
{
    int failed = 0;
    double totalMs = 0.0;
    for (const auto &r : results()) {
        failed += r.failure.empty() ? 0 : 1;
        totalMs += r.ms;
    }

    const char *junit = nullptr;
    for (int i = 1; i + 1 < argc; ++i)
        if (std::string(argv[i]) == "--junit")
            junit = argv[i + 1];
    if (junit) {
        std::ofstream xml(junit);
        xml << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            << "<testsuite name=\"" << xmlEscape(suite) << "\" tests=\""
            << results().size() << "\" failures=\"" << failed << "\" time=\""
            << totalMs / 1000.0 << "\">\n";
        for (const auto &r : results()) {
            xml << "  <testcase classname=\"" << xmlEscape(suite) << "\" name=\""
                << xmlEscape(r.name) << "\" time=\"" << r.ms / 1000.0 << "\"";
            if (r.failure.empty())
                xml << "/>\n";
            else
                xml << ">\n    <failure message=\"" << xmlEscape(r.failure)
                    << "\"/>\n  </testcase>\n";
        }
        xml << "</testsuite>\n";
    }

    std::printf("\n%s: %zu tests, %d failed%s%s\n", suite, results().size(), failed,
                junit ? ", JUnit: " : "", junit ? junit : "");
    return failed ? 1 : 0;
}

} // namespace rdtest

#define RD_RUN(fn) rdtest::runCase(#fn, fn)
#define RD_CHECK(cond, what) rdtest::check((cond), (what), __FILE__, __LINE__)

#endif // RD_TEST_H
