// RailDeck Pro - Slint frontend entry point. Owns the traincore simulation,
// pumps one TrainState snapshot per tick into the Backend global of the
// generated Slint UI and forwards operator input back to the core - nothing
// but tick()/setLever()/command() and the published state (REQ-N-001).
#include "Main.h" // generated from ui/Main.slint

#include "traincore/train_simulation.h"

#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

using namespace traincore;

namespace {

// printf-style formatting into a Slint string (fixed decimals etc. are not
// expressible in the .slint language).
template <typename... Args>
slint::SharedString fmt(const char *format, Args... args)
{
    char buf[64];
    (void)std::snprintf(buf, sizeof buf, format, args...);
    return slint::SharedString{buf};
}

Command toCommand(CabCommand cmd)
{
    switch (cmd) {
    case CabCommand::SifaAcknowledge: return Command::SifaAcknowledge;
    case CabCommand::PaToggle: return Command::PaToggle;
    case CabCommand::RadioToggle: return Command::RadioToggle;
    case CabCommand::DoorsLeftRelease: return Command::DoorsLeftRelease;
    case CabCommand::DoorsLeftClose: return Command::DoorsLeftClose;
    case CabCommand::DoorsRightRelease: return Command::DoorsRightRelease;
    case CabCommand::DoorsRightClose: return Command::DoorsRightClose;
    case CabCommand::PantographToggle: return Command::PantographToggle;
    case CabCommand::AfbToggle: return Command::AfbToggle;
    case CabCommand::AfbIncrease: return Command::AfbIncrease;
    case CabCommand::AfbDecrease: return Command::AfbDecrease;
    case CabCommand::EmergencyBrakeToggle: return Command::EmergencyBrakeToggle;
    }
    return Command::SifaAcknowledge;
}

// Refresh every Backend property from the current simulation snapshot.
void refresh(const Backend &b, const TrainSimulation &sim)
{
    const TrainState &s = sim.state();

    // Motion & supervision
    b.set_speedKmh((float)s.speedKmh);
    b.set_accelText(fmt("%+.2f", s.accelMs2));
    b.set_accelNegative(s.accelMs2 < 0);
    b.set_permittedSpeedKmh((float)s.permittedSpeedKmh);
    b.set_targetSpeedKmh((float)s.targetSpeedKmh);
    b.set_distanceToTargetM((float)s.distanceToTargetM);
    b.set_distanceToTargetText(
        s.distanceToTargetM >= 1000
            ? fmt("%.1f km", s.distanceToTargetM / 1000.0)
            : fmt("%d m", ((int)s.distanceToTargetM / 10) * 10));
    b.set_overspeedWarning(s.overspeedWarning);
    b.set_brakeIntervention(s.brakeIntervention);

    // Driver controls
    b.set_leverPercent((float)s.leverPercent);
    b.set_emergencyBrake(s.emergencyBrake);
    b.set_afbEnabled(s.afbEnabled);
    b.set_afbSetKmh((float)s.afbSetKmh);
    b.set_advisor((int)s.advisor);

    // Traction & electric
    b.set_tractionEffortKn((float)s.tractionEffortKn);
    b.set_effortText(fmt("%+d", (int)std::lround(s.tractionEffortKn)));
    b.set_powerText(s.powerMw >= 0 ? fmt("%.2f", s.powerMw)
                                   : fmt("−%.2f", -s.powerMw));
    b.set_powerPositive(s.powerMw >= 0);
    b.set_energyText(fmt("%.0f", s.energyKwh));
    b.set_lineVoltageKv((float)s.lineVoltageKv);
    b.set_lineVoltageText(fmt("%.1f", s.lineVoltageKv));
    b.set_motorCurrentA((float)s.motorCurrentA);
    b.set_motorCurrentText(fmt("%.0f", s.motorCurrentA));
    b.set_pantographUp(s.pantographUp);

    // Pneumatics
    b.set_brakePipeBar((float)s.brakePipeBar);
    b.set_brakePipeText(fmt("%.1f", s.brakePipeBar));
    b.set_brakeCylinderBar((float)s.brakeCylinderBar);
    b.set_brakeCylinderText(fmt("%.1f", s.brakeCylinderBar));
    b.set_mainReservoirBar((float)s.mainReservoirBar);
    b.set_mainReservoirText(fmt("%.1f", s.mainReservoirBar));

    // Vigilance / doors / comms
    b.set_sifaStage((int)s.sifa);
    b.set_sifaCountdownS((float)s.sifaCountdownS);
    b.set_doorLeft((int)s.doorLeft);
    b.set_doorRight((int)s.doorRight);
    b.set_paActive(s.paActive);
    b.set_radioState((int)s.radio);

    // Journey
    b.set_clockText(fmt("%02d:%02d:%02d", s.clockHour, s.clockMinute, s.clockSecond));
    b.set_nextStationText(fmt("→ %s · %.1f km", s.nextStationName.c_str(),
                              s.nextStationDistanceM / 1000.0));
    b.set_gradientPermille((float)s.gradientPermille);
    b.set_gradientText(s.gradientPermille > 0   ? fmt("▲ +%.0f", s.gradientPermille)
                       : s.gradientPermille < 0 ? fmt("▼ %.0f", s.gradientPermille)
                                                : fmt("%.0f", s.gradientPermille));
    b.set_routePositionM((float)s.routePositionM);

    // Alert ticker: cycle through concurrent alerts every 3 s (sim time, so
    // the frontend stays free of wall-clock state - same as the LVGL one).
    if (s.alerts.empty()) {
        b.set_hasAlert(false);
        b.set_alertText({});
        b.set_alertSeverity(0);
    } else {
        const auto idx = (std::size_t)(s.simTimeS / 3.0) % s.alerts.size();
        const Alert &a = s.alerts[idx];
        b.set_hasAlert(true);
        b.set_alertText(slint::SharedString(a.text));
        b.set_alertSeverity((int)a.severity);
    }
}

// Minimal 24-bit BMP writer for --screenshot verification (same container
// as the LVGL frontend's writer, fed from Slint's RGBA snapshot).
bool save_screenshot(const slint::Window &window, const char *path)
{
    const auto snapshot = window.take_snapshot();
    if (!snapshot.has_value())
        return false;

    const auto &buf = snapshot.value();
    const auto w = (int32_t)buf.width();
    const auto h = (int32_t)buf.height();
    const slint::Rgba8Pixel *pixels = buf.begin();
    const uint32_t rowSize = (3 * (uint32_t)w + 3) & ~3U;
    const uint32_t dataSize = rowSize * (uint32_t)h;
    const uint32_t fileSize = 54 + dataSize;

    std::vector<uint8_t> row(rowSize, 0);

    // RAII: any exit (including exceptions) closes the file. The deleter is
    // the best-effort backstop; the primary close below checks the return.
    struct FileCloser {
        void operator()(FILE *fp) const { (void)std::fclose(fp); } // NOLINT(cert-err33-c)
    };
    std::unique_ptr<FILE, FileCloser> file(std::fopen(path, "wb"));
    if (!file)
        return false;

    uint8_t hdr[54] = {'B', 'M'};
    auto put32 = [&hdr](int off, uint32_t v) {
        hdr[off] = v & 0xFF; hdr[off + 1] = (v >> 8) & 0xFF;
        hdr[off + 2] = (v >> 16) & 0xFF; hdr[off + 3] = (v >> 24) & 0xFF;
    };
    put32(2, fileSize);
    put32(10, 54);
    put32(14, 40);
    put32(18, (uint32_t)w);
    put32(22, (uint32_t)h);
    hdr[26] = 1;            // planes
    hdr[28] = 24;           // bpp
    put32(34, dataSize);

    bool ok = std::fwrite(hdr, 1, 54, file.get()) == 54;
    for (int32_t y = h - 1; ok && y >= 0; --y) {
        const slint::Rgba8Pixel *src = pixels + (std::size_t)y * (std::size_t)w;
        for (int32_t x = 0; x < w; ++x) {
            row[3 * (std::size_t)x + 0] = src[x].b;
            row[3 * (std::size_t)x + 1] = src[x].g;
            row[3 * (std::size_t)x + 2] = src[x].r;
        }
        ok = std::fwrite(row.data(), 1, rowSize, file.get()) == rowSize;
    }
    ok = (std::fclose(file.release()) == 0) && ok;
    return ok;
}

} // namespace

int main(int argc, char **argv)
{
    const char *screenshotPath = nullptr;
    uint64_t screenshotDelayMs = 1500;
    for (int i = 1; i + 1 < argc; ++i) {
        if (std::strcmp(argv[i], "--screenshot") == 0)
            screenshotPath = argv[i + 1];
        else if (std::strcmp(argv[i], "--screenshot-delay") == 0)
            screenshotDelayMs = std::strtoul(argv[i + 1], nullptr, 10);
    }

    TrainSimulation sim;
    auto ui = MainWindow::create();
    const auto &backend = ui->global<Backend>();

    // Static route data for the planning strip.
    std::vector<PlanEvent> events;
    for (const RouteSegment &seg : sim.route())
        events.push_back({(float)seg.startM, false, fmt("%d", (int)seg.limitKmh)});
    for (const RouteStation &st : sim.stations())
        events.push_back({(float)st.positionM, true, slint::SharedString(st.name)});
    backend.set_planEvents(std::make_shared<slint::VectorModel<PlanEvent>>(events));
    backend.set_routeLengthM((float)sim.routeLengthM());
    backend.set_serviceId(slint::SharedString(sim.state().serviceId));

    backend.on_command([&](CabCommand cmd) {
        sim.command(toCommand(cmd));
        refresh(backend, sim);
    });
    backend.on_setLever([&](float percent) {
        sim.setLever(percent);
        refresh(backend, sim);
    });

    // ~30 Hz simulation/UI refresh, dt from a monotonic clock (frontend only;
    // the core itself stays deterministic).
    auto last = std::chrono::steady_clock::now();
    slint::Timer tickTimer;
    tickTimer.start(slint::TimerMode::Repeated, std::chrono::milliseconds(33),
                    [&] {
                        const auto now = std::chrono::steady_clock::now();
                        const double dt =
                            std::chrono::duration<double>(now - last).count();
                        last = now;
                        sim.tick(dt);
                        refresh(backend, sim);
                    });
    refresh(backend, sim);

    // Headless verification: raildeck-slint --screenshot out.bmp [--screenshot-delay ms]
    slint::Timer screenshotTimer;
    if (screenshotPath != nullptr) {
        screenshotTimer.start(
            slint::TimerMode::SingleShot,
            std::chrono::milliseconds(screenshotDelayMs), [&] {
                const bool ok = save_screenshot(ui->window(), screenshotPath);
                std::printf("screenshot %s: %s\n", screenshotPath, ok ? "ok" : "FAILED");
                slint::quit_event_loop();
            });
    }

    ui->run();
    return 0;
}
