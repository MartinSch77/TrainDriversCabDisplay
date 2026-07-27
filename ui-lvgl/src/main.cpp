// RailDeck Pro - LVGL frontend entry point (SDL desktop simulator).
#include "ui.h"

#include "lvgl.h"

#include <SDL2/SDL.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <vector>

namespace {

uint32_t tick_cb() { return SDL_GetTicks(); }

struct SimCtx {
    traincore::TrainSimulation sim;
    uint32_t lastMs = 0;
};

void sim_timer_cb(lv_timer_t *timer)
{
    auto *ctx = static_cast<SimCtx *>(lv_timer_get_user_data(timer));
    const uint32_t now = SDL_GetTicks();
    ctx->sim.tick((now - ctx->lastMs) / 1000.0);
    ctx->lastMs = now;
    ui_update();
}

// Minimal 24-bit BMP writer for --screenshot verification.
bool save_screenshot(const char *path)
{
    lv_draw_buf_t *buf = lv_snapshot_take(lv_screen_active(), LV_COLOR_FORMAT_XRGB8888);
    if (buf == nullptr)
        return false;

    const int32_t w = buf->header.w;
    const int32_t h = buf->header.h;
    const uint32_t stride = buf->header.stride;
    const uint32_t rowSize = (3 * w + 3) & ~3U;
    const uint32_t dataSize = rowSize * h;
    const uint32_t fileSize = 54 + dataSize;

    std::vector<uint8_t> row(rowSize, 0);

    // RAII: any exit (including exceptions) closes the file. The deleter is
    // the best-effort backstop; the primary close below checks the return.
    struct FileCloser {
        void operator()(FILE *fp) const { (void)std::fclose(fp); } // NOLINT(cert-err33-c)
    };
    // GCC's -fanalyzer (experimental in C++) does not model the unique_ptr
    // deleter and reports the fopen result as leaking on every path —
    // audited false positive: ownership is released only into std::fclose.
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wanalyzer-file-leak"
#pragma GCC diagnostic ignored "-Wanalyzer-malloc-leak"
#endif
    std::unique_ptr<FILE, FileCloser> file(std::fopen(path, "wb"));
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif
    if (!file) {
        lv_draw_buf_destroy(buf);
        return false;
    }

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
        const uint8_t *src = buf->data + (size_t)y * stride;
        for (int32_t x = 0; x < w; ++x) {
            row[3 * x + 0] = src[4 * x + 0]; // B
            row[3 * x + 1] = src[4 * x + 1]; // G
            row[3 * x + 2] = src[4 * x + 2]; // R
        }
        ok = std::fwrite(row.data(), 1, rowSize, file.get()) == rowSize;
    }
    ok = (std::fclose(file.release()) == 0) && ok;
    lv_draw_buf_destroy(buf);
    return ok;
}

} // namespace

int main(int argc, char **argv)
{
    const char *screenshotPath = nullptr;
    uint32_t screenshotDelayMs = 1500;
    for (int i = 1; i + 1 < argc; ++i) {
        if (std::strcmp(argv[i], "--screenshot") == 0)
            screenshotPath = argv[i + 1];
        else if (std::strcmp(argv[i], "--screenshot-delay") == 0)
            screenshotDelayMs = (uint32_t)std::strtoul(argv[i + 1], nullptr, 10);
    }

    lv_init();
    lv_tick_set_cb(tick_cb);

    lv_display_t *disp = lv_sdl_window_create(1280, 800);
    lv_sdl_window_set_title(disp, "RailDeck Pro - Cab Display (LVGL)");
    lv_sdl_mouse_create();

    static SimCtx ctx;
    ctx.lastMs = SDL_GetTicks();
    ui_create(&ctx.sim);
    lv_timer_create(sim_timer_cb, 33, &ctx);

    const uint32_t startMs = SDL_GetTicks();
    while (lv_display_get_default() != nullptr) {
        lv_timer_handler();
        if (screenshotPath != nullptr
            && SDL_GetTicks() - startMs > screenshotDelayMs) {
            const bool ok = save_screenshot(screenshotPath);
            std::printf("screenshot %s: %s\n", screenshotPath, ok ? "ok" : "FAILED");
            break;
        }
        SDL_Delay(5);
    }
    return 0;
}
