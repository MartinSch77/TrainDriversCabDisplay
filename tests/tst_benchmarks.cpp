// Deterministic performance benchmark for the simulation core (the tick is
// called from UI timers at 30 Hz — it must stay far below one frame budget).
#include "rd_test.h"
#include "traincore/train_simulation.h"

#include <chrono>
#include <cstdlib>

using namespace traincore;

//! @tstid TS-PERF-001 @design DES-CORE-SIM
// @relation(REQ-N-006, scope=function)
static void TS_PERF_001_tickRate()
{
    TrainSimulation sim;
    sim.setLever(40);
    for (int i = 0; i < 1000; ++i) // warm-up
        sim.tick(0.033);

    constexpr int kTicks = 100000;
    const auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < kTicks; ++i)
        sim.tick(0.033);
    const auto t1 = std::chrono::steady_clock::now();

    const double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    const double ticksPerSecond = kTicks / (ms / 1000.0);
    const double usPerTick = ms * 1000.0 / kTicks;
    std::printf("RESULT tick_rate=%.0f ticks/s (%.2f us/tick, %d ticks in %.1f ms)\n",
                ticksPerSecond, usPerTick, kTicks, ms);

    // Generous floor: one tick must stay well under 50 us even in Debug
    // (a 30 Hz UI tick budget is ~33 ms). Dynamic instrumentation (valgrind
    // is ~25x) overrides the floor via RD_BENCH_MIN_TPS; the RESULT line
    // above stays for trend comparison either way.
    double minTps = 20000.0;
    if (const char *env = std::getenv("RD_BENCH_MIN_TPS"))
        minTps = std::atof(env);
    RD_CHECK(ticksPerSecond > minTps, "simulation tick rate above the floor");
}

int main(int argc, char **argv)
{
    RD_RUN(TS_PERF_001_tickRate);
    return rdtest::finish("tst_benchmarks", argc, argv);
}
