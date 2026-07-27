// RailDeck Pro - LVGL frontend. Mirrors the Qt Quick layout 1:1, styled
// exclusively from the generated shared/theme_tokens.h design tokens and
// driven by the same traincore simulation API.
#include "ui.h"

#include "lvgl.h"
#include "theme_tokens.h"

#include <cmath>
#include <cstdio>

using namespace traincore;

namespace {

lv_color_t C(uint32_t hex) { return lv_color_hex(hex); }

// ---------------------------------------------------------------- helpers --

lv_obj_t *make_panel(lv_obj_t *parent, int x, int y, int w, int h,
                     int radius = RD_METRIC_RADIUS, uint32_t bg = RD_COLOR_PANEL)
{
    lv_obj_t *p = lv_obj_create(parent);
    lv_obj_remove_style_all(p);
    lv_obj_set_pos(p, x, y);
    lv_obj_set_size(p, w, h);
    lv_obj_set_style_radius(p, radius, 0);
    lv_obj_set_style_bg_color(p, C(bg), 0);
    lv_obj_set_style_bg_opa(p, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(p, C(RD_COLOR_LINE), 0);
    lv_obj_set_style_border_width(p, RD_METRIC_BORDER_WIDTH, 0);
    lv_obj_remove_flag(p, LV_OBJ_FLAG_SCROLLABLE);
    return p;
}

lv_obj_t *make_label(lv_obj_t *parent, const char *text, const lv_font_t *font,
                     uint32_t color)
{
    lv_obj_t *l = lv_label_create(parent);
    lv_label_set_text(l, text);
    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_set_style_text_color(l, C(color), 0);
    return l;
}

lv_obj_t *make_title(lv_obj_t *panel, const char *text)
{
    lv_obj_t *l = make_label(panel, text, &lv_font_montserrat_12, RD_COLOR_TEXT_SECONDARY);
    lv_obj_align(l, LV_ALIGN_TOP_MID, 0, 7);
    return l;
}

// An arc used purely as a colored ring segment (no knob, not clickable).
lv_obj_t *make_ring_arc(lv_obj_t *parent, int size, int rotation, int spanDeg,
                        uint32_t color, int width)
{
    lv_obj_t *a = lv_arc_create(parent);
    lv_obj_set_size(a, size, size);
    lv_obj_center(a);
    lv_arc_set_rotation(a, rotation);
    lv_arc_set_bg_angles(a, 0, spanDeg);
    lv_arc_set_angles(a, 0, 0);
    lv_obj_remove_style(a, nullptr, LV_PART_KNOB);
    lv_obj_remove_flag(a, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_opa(a, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_arc_color(a, C(color), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(a, width, LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(a, false, LV_PART_INDICATOR);
    return a;
}

struct Zone { double from, to; uint32_t color; };

struct Gauge {
    lv_obj_t *panel = nullptr;
    lv_obj_t *scale = nullptr;
    lv_obj_t *needle = nullptr;
    lv_obj_t *value = nullptr;
    double min = 0, max = 10;
    int decimals = 1;
    int needleLen = 0;
};

// Small round instrument: 240 deg, colored zones, needle, digital value.
Gauge make_gauge(lv_obj_t *parent, int x, int y, int w, int h,
                 const char *title, const char *unit,
                 double min, double max, int decimals, int majorTicks,
                 const Zone *zones, int zoneCount)
{
    Gauge g;
    g.min = min; g.max = max; g.decimals = decimals;
    g.panel = make_panel(parent, x, y, w, h);
    make_title(g.panel, title);

    const int dial = std::min(w - 26, h - 40);
    g.needleLen = dial / 2 - 12;

    lv_obj_t *track = make_ring_arc(g.panel, dial, 150, 240, RD_COLOR_GAUGE_TRACK, 7);
    lv_obj_align(track, LV_ALIGN_TOP_MID, 0, 24);
    lv_arc_set_angles(track, 0, 240);

    auto zoneAngle = [&](double v) {
        return (int)std::lround((v - min) / (max - min) * 240.0);
    };
    for (int i = 0; i < zoneCount; ++i) {
        lv_obj_t *z = make_ring_arc(g.panel, dial, 150, 240, zones[i].color, 7);
        lv_obj_align(z, LV_ALIGN_TOP_MID, 0, 24);
        lv_arc_set_angles(z, zoneAngle(zones[i].from), zoneAngle(zones[i].to));
    }

    g.scale = lv_scale_create(g.panel);
    lv_obj_set_size(g.scale, dial, dial);
    lv_obj_align(g.scale, LV_ALIGN_TOP_MID, 0, 24);
    lv_scale_set_mode(g.scale, LV_SCALE_MODE_ROUND_INNER);
    lv_scale_set_range(g.scale, (int)min, (int)max);
    lv_scale_set_angle_range(g.scale, 240);
    lv_scale_set_rotation(g.scale, 150);
    lv_scale_set_total_tick_count(g.scale, majorTicks + 1);
    lv_scale_set_major_tick_every(g.scale, 1);
    lv_scale_set_label_show(g.scale, false);
    lv_obj_set_style_arc_width(g.scale, 0, LV_PART_MAIN);
    lv_obj_set_style_line_color(g.scale, C(RD_COLOR_GAUGE_TICK), LV_PART_INDICATOR);
    lv_obj_set_style_line_width(g.scale, 2, LV_PART_INDICATOR);
    lv_obj_set_style_length(g.scale, 7, LV_PART_INDICATOR);

    g.needle = lv_line_create(g.scale);
    lv_obj_set_style_line_color(g.needle, C(RD_COLOR_NEEDLE), 0);
    lv_obj_set_style_line_width(g.needle, 3, 0);
    lv_obj_set_style_line_rounded(g.needle, true, 0);
    lv_scale_set_line_needle_value(g.scale, g.needle, g.needleLen, (int)min);

    lv_obj_t *hub = lv_obj_create(g.scale);
    lv_obj_remove_style_all(hub);
    lv_obj_set_size(hub, 8, 8);
    lv_obj_set_style_radius(hub, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(hub, C(RD_COLOR_LINE_BRIGHT), 0);
    lv_obj_set_style_bg_opa(hub, LV_OPA_COVER, 0);
    lv_obj_center(hub);

    g.value = make_label(g.panel, "0", &lv_font_montserrat_20, RD_COLOR_TEXT_PRIMARY);
    lv_obj_align(g.value, LV_ALIGN_BOTTOM_MID, 0, -16);
    lv_obj_t *u = make_label(g.panel, unit, &lv_font_montserrat_12, RD_COLOR_TEXT_DIM);
    lv_obj_align(u, LV_ALIGN_BOTTOM_MID, 0, -3);
    return g;
}

void gauge_update(Gauge &g, double v)
{
    const double clamped = std::fmax(g.min, std::fmin(g.max, v));
    lv_scale_set_line_needle_value(g.scale, g.needle, g.needleLen, (int)std::lround(clamped));
    lv_label_set_text_fmt(g.value, "%.*f", g.decimals, v);
}

struct Tile {
    lv_obj_t *panel = nullptr;
    lv_obj_t *value = nullptr;
};

Tile make_tile(lv_obj_t *parent, int x, int y, int w, int h, const char *title)
{
    Tile t;
    t.panel = make_panel(parent, x, y, w, h, RD_METRIC_RADIUS_SMALL, RD_COLOR_PANEL_INSET);
    lv_obj_t *l = make_label(t.panel, title, &lv_font_montserrat_12, RD_COLOR_TEXT_DIM);
    lv_obj_align(l, LV_ALIGN_TOP_MID, 0, 6);
    t.value = make_label(t.panel, "", &lv_font_montserrat_16, RD_COLOR_TEXT_PRIMARY);
    lv_obj_align(t.value, LV_ALIGN_BOTTOM_MID, 0, -6);
    return t;
}

struct Btn {
    lv_obj_t *btn = nullptr;
    lv_obj_t *title = nullptr;
    lv_obj_t *sub = nullptr;
};

struct Badge {
    lv_obj_t *box = nullptr;
    lv_obj_t *label = nullptr;
};

Badge make_badge(lv_obj_t *parent, const char *text, int w, int h)
{
    Badge b;
    b.box = make_panel(parent, 0, 0, w, h, RD_METRIC_RADIUS_SMALL, RD_COLOR_PANEL);
    b.label = make_label(b.box, text, &lv_font_montserrat_12, RD_COLOR_TEXT_DIM);
    lv_obj_center(b.label);
    return b;
}

void badge_set(Badge &b, bool on, uint32_t onColor)
{
    lv_obj_set_style_border_color(b.box, on ? C(onColor) : C(RD_COLOR_LINE), 0);
    lv_obj_set_style_bg_color(b.box, on ? lv_color_darken(C(onColor), 200)
                                        : C(RD_COLOR_PANEL), 0);
    lv_obj_set_style_text_color(b.label, on ? C(onColor) : C(RD_COLOR_TEXT_DIM), 0);
}

// ------------------------------------------------------------------ state --

struct PlanEvent {
    lv_obj_t *line = nullptr;
    lv_obj_t *chip = nullptr;      // limit chip or station label
    double absM = 0;
    bool isStation = false;
};

struct UiState {
    TrainSimulation *sim = nullptr;

    // header
    Badge mode;
    lv_obj_t *alertBox = nullptr;
    lv_obj_t *alertLabel = nullptr;
    Badge pan, rad, pa;
    lv_obj_t *station = nullptr;
    lv_obj_t *clock = nullptr;

    // gauges
    Gauge pipe, cyl, res, volt, curr;
    Tile accel, power, energy, gradient;

    // speedometer
    lv_obj_t *speedoPanel = nullptr;
    lv_obj_t *arcPerm = nullptr, *arcTarget = nullptr, *arcWarn = nullptr, *arcDanger = nullptr;
    lv_obj_t *speedScale = nullptr, *speedNeedle = nullptr;
    lv_obj_t *speedLabel = nullptr;
    lv_obj_t *targetBox = nullptr, *targetLabel = nullptr, *distLabel = nullptr;
    lv_obj_t *permLabel = nullptr;
    lv_obj_t *afbMarker = nullptr;

    // advisor + lever
    lv_obj_t *advisorGlyph = nullptr, *advisorText = nullptr;
    lv_obj_t *leverTitle = nullptr, *slider = nullptr, *leverValue = nullptr;

    // effort
    lv_obj_t *effortBar = nullptr, *effortValue = nullptr;

    // planning strip
    lv_obj_t *strip = nullptr;
    lv_obj_t *targetDot = nullptr;
    PlanEvent events[24];
    int eventCount = 0;
    int stripH = 0;

    // line-ahead view (perspective projection drawn with primitives)
    lv_obj_t *laGround = nullptr;
    lv_obj_t *laRailL = nullptr, *laRailR = nullptr;
    lv_point_precise_t laRailPtsL[2] = {};
    lv_point_precise_t laRailPtsR[2] = {};
    lv_obj_t *laSleepers[12] = {};
    lv_obj_t *laMastPole[3] = {}, *laMastArm[3] = {};
    lv_obj_t *laBoardPole = nullptr, *laBoardDisc = nullptr;

    // buttons
    Btn sifa, paBtn, radio, dLRel, dLCls, dRRel, dRCls, panto, afb, afbMinus, afbPlus, emerg;
};

// File-local UI context: the established LVGL module pattern (one mutable
// context struct per screen translation unit).
UiState g; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

constexpr double kWindowM = 4000.0;
constexpr int kSpeedoMax = RD_METRIC_SPEEDO_MAX_KMH;

int speedoAngle(double v)
{
    const double t = std::fmax(0.0, std::fmin(1.0, v / kSpeedoMax));
    return (int)std::lround(t * RD_METRIC_SPEEDO_SWEEP_DEG);
}

// ---------------------------------------------------------------- buttons --

void button_cb(lv_event_t *e)
{
    const auto cmd = static_cast<Command>(
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<uintptr_t>(lv_event_get_user_data(e)));
    g.sim->command(cmd);
}

Btn make_button(lv_obj_t *parent, int x, int w, const char *label,
                const char *sub, Command cmd, int h = 72, int y = 12)
{
    Btn b;
    b.btn = lv_button_create(parent);
    lv_obj_remove_style_all(b.btn);
    lv_obj_set_pos(b.btn, x, y);
    lv_obj_set_size(b.btn, w, h);
    lv_obj_set_style_radius(b.btn, RD_METRIC_RADIUS, 0);
    lv_obj_set_style_bg_color(b.btn, C(RD_COLOR_BUTTON_FACE), 0);
    lv_obj_set_style_bg_opa(b.btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(b.btn, C(RD_COLOR_BUTTON_BORDER), 0);
    lv_obj_set_style_border_width(b.btn, RD_METRIC_BORDER_WIDTH, 0);
    lv_obj_set_style_bg_color(b.btn, lv_color_lighten(C(RD_COLOR_BUTTON_FACE), 60),
                              LV_STATE_PRESSED);
    lv_obj_set_style_opa(b.btn, LV_OPA_40, LV_STATE_DISABLED);

    b.title = make_label(b.btn, label, &lv_font_montserrat_14, RD_COLOR_TEXT_PRIMARY);
    const bool hasSub = sub != nullptr && sub[0] != '\0';
    lv_obj_align(b.title, LV_ALIGN_CENTER, 0, hasSub ? -10 : 0);
    if (hasSub) {
        b.sub = make_label(b.btn, sub, &lv_font_montserrat_12, RD_COLOR_TEXT_DIM);
        lv_obj_align(b.sub, LV_ALIGN_CENTER, 0, 11);
    }
    // LVGL's user_data is a void*; smuggling the enum through the pointer
    // value is the supported idiom there (no allocation, no lifetime).
    lv_obj_add_event_cb(b.btn, button_cb, LV_EVENT_CLICKED,
                        // NOLINTNEXTLINE(performance-no-int-to-ptr,cppcoreguidelines-pro-type-reinterpret-cast)
                        reinterpret_cast<void *>(static_cast<uintptr_t>(cmd)));
    return b;
}

void btn_set(Btn &b, bool active, uint32_t activeColor,
             bool alarm = false, bool blinkOn = false)
{
    uint32_t border = active ? activeColor : RD_COLOR_BUTTON_BORDER;
    if (alarm) border = blinkOn ? RD_COLOR_SIFA_PULSE : RD_COLOR_BUTTON_BORDER;
    lv_obj_set_style_bg_color(b.btn, active ? C(RD_COLOR_BUTTON_FACE_ACTIVE)
                                            : C(RD_COLOR_BUTTON_FACE), 0);
    lv_obj_set_style_border_color(b.btn, C(border), 0);
    lv_obj_set_style_border_width(b.btn, (active || alarm) ? 2 : 1, 0);
    lv_obj_set_style_text_color(b.title, active ? C(activeColor)
                                                : C(RD_COLOR_TEXT_PRIMARY), 0);
    if (b.sub != nullptr)
        lv_obj_set_style_text_color(b.sub, alarm && blinkOn ? C(RD_COLOR_DANGER)
                                    : active ? C(activeColor) : C(RD_COLOR_TEXT_DIM), 0);
}

void slider_cb(lv_event_t *e)
{
    auto *slider = static_cast<lv_obj_t *>(lv_event_get_target(e));
    if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
        g.sim->setLever(lv_slider_get_value(slider));
    } else if (lv_event_get_code(e) == LV_EVENT_RELEASED) {
        if (std::abs(lv_slider_get_value(slider)) < 8) {
            lv_slider_set_value(slider, 0, LV_ANIM_OFF);
            g.sim->setLever(0);
        }
    }
}

const char *door_text(DoorState d)
{
    switch (d) {
    case DoorState::Locked: return "LOCKED";
    case DoorState::Released: return "RELEASED";
    case DoorState::Opening: return "OPENING";
    case DoorState::Open: return "OPEN";
    case DoorState::Closing: return "CLOSING";
    }
    return "";
}

// ------------------------------------------------------------------ build --

void build_header(lv_obj_t *scr)
{
    lv_obj_t *bar = make_panel(scr, 0, 0, 1280, RD_METRIC_HEADER_HEIGHT, 0,
                               RD_COLOR_PANEL_INSET);

    Badge service = make_badge(bar, "", 92, 28);
    lv_obj_set_pos(service.box, 12, 9);
    lv_obj_set_style_border_color(service.box, C(RD_COLOR_ACCENT), 0);
    lv_obj_set_style_text_color(service.label, C(RD_COLOR_TEXT_PRIMARY), 0);
    lv_obj_set_style_text_font(service.label, &lv_font_montserrat_14, 0);
    lv_label_set_text(service.label, g.sim->state().serviceId.c_str());

    g.mode = make_badge(bar, "MANUAL", 110, 28);
    lv_obj_set_pos(g.mode.box, 112, 9);
    lv_obj_set_style_text_font(g.mode.label, &lv_font_montserrat_14, 0);

    g.alertBox = make_panel(bar, 350, 8, 420, 30, RD_METRIC_RADIUS_SMALL,
                            RD_COLOR_PANEL_INSET);
    lv_obj_set_style_border_width(g.alertBox, 1, 0);
    g.alertLabel = make_label(g.alertBox, "", &lv_font_montserrat_14,
                              RD_COLOR_TEXT_SECONDARY);
    lv_obj_center(g.alertLabel);

    g.pan = make_badge(bar, "PAN", 42, 24);
    lv_obj_align(g.pan.box, LV_ALIGN_RIGHT_MID, -434, 0);
    g.rad = make_badge(bar, "RAD", 42, 24);
    lv_obj_align(g.rad.box, LV_ALIGN_RIGHT_MID, -386, 0);
    g.pa = make_badge(bar, "PA", 42, 24);
    lv_obj_align(g.pa.box, LV_ALIGN_RIGHT_MID, -338, 0);

    g.station = make_label(bar, "", &lv_font_montserrat_14, RD_COLOR_TEXT_SECONDARY);
    lv_obj_align(g.station, LV_ALIGN_RIGHT_MID, -100, 0);
    g.clock = make_label(bar, "00:00:00", &lv_font_montserrat_16, RD_COLOR_TEXT_PRIMARY);
    lv_obj_align(g.clock, LV_ALIGN_RIGHT_MID, -12, 0);
}

void build_speedometer(lv_obj_t *scr)
{
    const int size = 470;
    g.speedoPanel = make_panel(scr, 264, 58, size, size, LV_RADIUS_CIRCLE);

    const int ring = 448;
    const int rot = RD_METRIC_SPEEDO_START_DEG;
    const int sweep = RD_METRIC_SPEEDO_SWEEP_DEG;
    lv_obj_t *track = make_ring_arc(g.speedoPanel, ring, rot, sweep, RD_COLOR_GAUGE_TRACK, 10);
    lv_arc_set_angles(track, 0, sweep);
    g.arcPerm = make_ring_arc(g.speedoPanel, ring, rot, sweep, RD_COLOR_ARC_PERMITTED, 10);
    g.arcTarget = make_ring_arc(g.speedoPanel, ring, rot, sweep, RD_COLOR_ARC_TARGET, 10);
    g.arcWarn = make_ring_arc(g.speedoPanel, ring, rot, sweep, RD_COLOR_ARC_WARNING, 10);
    g.arcDanger = make_ring_arc(g.speedoPanel, ring, rot, sweep, RD_COLOR_ARC_DANGER, 10);

    g.speedScale = lv_scale_create(g.speedoPanel);
    lv_obj_set_size(g.speedScale, 400, 400);
    lv_obj_center(g.speedScale);
    lv_scale_set_mode(g.speedScale, LV_SCALE_MODE_ROUND_INNER);
    lv_scale_set_range(g.speedScale, 0, kSpeedoMax);
    lv_scale_set_angle_range(g.speedScale, sweep);
    lv_scale_set_rotation(g.speedScale, rot);
    lv_scale_set_total_tick_count(g.speedScale, kSpeedoMax / 10 + 1);
    lv_scale_set_major_tick_every(g.speedScale, 2);
    lv_scale_set_label_show(g.speedScale, true);
    lv_obj_set_style_arc_width(g.speedScale, 0, LV_PART_MAIN);
    lv_obj_set_style_text_color(g.speedScale, C(RD_COLOR_TEXT_SECONDARY), LV_PART_INDICATOR);
    lv_obj_set_style_text_font(g.speedScale, &lv_font_montserrat_14, LV_PART_INDICATOR);
    lv_obj_set_style_line_color(g.speedScale, C(RD_COLOR_GAUGE_TICK), LV_PART_INDICATOR);
    lv_obj_set_style_line_width(g.speedScale, 3, LV_PART_INDICATOR);
    lv_obj_set_style_length(g.speedScale, 14, LV_PART_INDICATOR);
    lv_obj_set_style_line_color(g.speedScale, C(RD_COLOR_GAUGE_TRACK), LV_PART_ITEMS);
    lv_obj_set_style_line_width(g.speedScale, 2, LV_PART_ITEMS);
    lv_obj_set_style_length(g.speedScale, 8, LV_PART_ITEMS);

    g.speedNeedle = lv_line_create(g.speedScale);
    lv_obj_set_style_line_color(g.speedNeedle, C(RD_COLOR_NEEDLE), 0);
    lv_obj_set_style_line_width(g.speedNeedle, 5, 0);
    lv_obj_set_style_line_rounded(g.speedNeedle, true, 0);
    lv_scale_set_line_needle_value(g.speedScale, g.speedNeedle, 150, 0);

    g.afbMarker = lv_obj_create(g.speedoPanel);
    lv_obj_remove_style_all(g.afbMarker);
    lv_obj_set_size(g.afbMarker, 10, 10);
    lv_obj_set_style_radius(g.afbMarker, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(g.afbMarker, C(RD_COLOR_ACCENT), 0);
    lv_obj_set_style_bg_opa(g.afbMarker, LV_OPA_COVER, 0);
    lv_obj_add_flag(g.afbMarker, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *hub = make_panel(g.speedoPanel, 0, 0, 190, 190, LV_RADIUS_CIRCLE,
                               RD_COLOR_PANEL_INSET);
    lv_obj_center(hub);
    g.speedLabel = make_label(hub, "0", &lv_font_montserrat_48, RD_COLOR_TEXT_PRIMARY);
    lv_obj_align(g.speedLabel, LV_ALIGN_CENTER, 0, -10);
    lv_obj_t *kmh = make_label(hub, "km/h", &lv_font_montserrat_14, RD_COLOR_TEXT_DIM);
    lv_obj_align(kmh, LV_ALIGN_CENTER, 0, 28);

    g.targetBox = make_panel(g.speedoPanel, 0, 0, 176, 40, RD_METRIC_RADIUS_SMALL,
                             RD_COLOR_PANEL_INSET);
    lv_obj_align(g.targetBox, LV_ALIGN_BOTTOM_MID, 0, -46);
    lv_obj_set_style_border_color(g.targetBox, C(RD_COLOR_ARC_TARGET), 0);
    g.targetLabel = make_label(g.targetBox, "0", &lv_font_montserrat_28, RD_COLOR_ARC_TARGET);
    lv_obj_align(g.targetLabel, LV_ALIGN_LEFT_MID, 18, 0);
    g.distLabel = make_label(g.targetBox, "", &lv_font_montserrat_16, RD_COLOR_TEXT_SECONDARY);
    lv_obj_align(g.distLabel, LV_ALIGN_RIGHT_MID, -14, 0);

    g.permLabel = make_label(g.speedoPanel, "PERM 0", &lv_font_montserrat_12,
                             RD_COLOR_ARC_PERMITTED);
    lv_obj_align(g.permLabel, LV_ALIGN_BOTTOM_MID, 0, -18);
}

void build_advisor_lever(lv_obj_t *scr)
{
    lv_obj_t *adv = make_panel(scr, 249, 538, 110, 122);
    make_title(adv, "ADVISOR");
    g.advisorGlyph = make_label(adv, "-", &lv_font_montserrat_28, RD_COLOR_TEXT_DIM);
    lv_obj_align(g.advisorGlyph, LV_ALIGN_CENTER, 0, 2);
    g.advisorText = make_label(adv, "IDLE", &lv_font_montserrat_14, RD_COLOR_TEXT_DIM);
    lv_obj_align(g.advisorText, LV_ALIGN_BOTTOM_MID, 0, -10);

    lv_obj_t *lev = make_panel(scr, 369, 538, 380, 122);
    g.leverTitle = make_label(lev, "MASTER CONTROLLER", &lv_font_montserrat_12,
                              RD_COLOR_TEXT_SECONDARY);
    lv_obj_align(g.leverTitle, LV_ALIGN_TOP_MID, 0, 7);

    g.slider = lv_slider_create(lev);
    lv_obj_remove_style_all(g.slider); // drop theme styles, restyle from tokens
    lv_obj_set_size(g.slider, 310, 10);
    lv_obj_align(g.slider, LV_ALIGN_CENTER, 0, 4);
    lv_slider_set_range(g.slider, -100, 100);
    lv_slider_set_value(g.slider, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(g.slider, C(RD_COLOR_GAUGE_TRACK), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(g.slider, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(g.slider, 5, LV_PART_MAIN);
    lv_obj_set_style_bg_color(g.slider, C(RD_COLOR_TEXT_PRIMARY), LV_PART_KNOB);
    lv_obj_set_style_bg_opa(g.slider, LV_OPA_COVER, LV_PART_KNOB);
    lv_obj_set_style_border_color(g.slider, C(RD_COLOR_LINE_BRIGHT), LV_PART_KNOB);
    lv_obj_set_style_border_width(g.slider, 1, LV_PART_KNOB);
    lv_obj_set_style_radius(g.slider, 6, LV_PART_KNOB);
    lv_obj_set_style_pad_ver(g.slider, 11, LV_PART_KNOB);
    lv_obj_set_style_pad_hor(g.slider, 4, LV_PART_KNOB);
    lv_obj_set_style_opa(g.slider, LV_OPA_60, LV_STATE_DISABLED);
    lv_obj_add_flag(g.slider, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(g.slider, slider_cb, LV_EVENT_VALUE_CHANGED, nullptr);
    lv_obj_add_event_cb(g.slider, slider_cb, LV_EVENT_RELEASED, nullptr);

    // centre detent mark
    lv_obj_t *detent = lv_obj_create(lev);
    lv_obj_remove_style_all(detent);
    lv_obj_set_size(detent, 2, 22);
    lv_obj_align(detent, LV_ALIGN_CENTER, 0, 4);
    lv_obj_set_style_bg_color(detent, C(RD_COLOR_TEXT_SECONDARY), 0);
    lv_obj_set_style_bg_opa(detent, LV_OPA_COVER, 0);

    lv_obj_t *brake = make_label(lev, "BRAKE", &lv_font_montserrat_12, RD_COLOR_WARN);
    lv_obj_align(brake, LV_ALIGN_BOTTOM_LEFT, 12, -8);
    lv_obj_t *trac = make_label(lev, "TRACTION", &lv_font_montserrat_12, RD_COLOR_TRACTION);
    lv_obj_align(trac, LV_ALIGN_BOTTOM_RIGHT, -12, -8);
    g.leverValue = make_label(lev, "0 %", &lv_font_montserrat_14, RD_COLOR_TEXT_PRIMARY);
    lv_obj_align(g.leverValue, LV_ALIGN_BOTTOM_MID, 0, -6);
}

void build_effort(lv_obj_t *scr, int x, int y)
{
    lv_obj_t *p = make_panel(scr, x, y, 225, 190);
    make_title(p, "EFFORT");

    g.effortBar = lv_bar_create(p);
    lv_obj_set_size(g.effortBar, 26, 108);
    lv_obj_align(g.effortBar, LV_ALIGN_TOP_MID, 0, 30);
    lv_bar_set_range(g.effortBar, -250, 250);
    lv_bar_set_mode(g.effortBar, LV_BAR_MODE_SYMMETRICAL);
    lv_bar_set_value(g.effortBar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(g.effortBar, C(RD_COLOR_GAUGE_TRACK), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(g.effortBar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(g.effortBar, 5, LV_PART_MAIN);
    lv_obj_set_style_bg_color(g.effortBar, C(RD_COLOR_TRACTION), LV_PART_INDICATOR);
    lv_obj_set_style_radius(g.effortBar, 5, LV_PART_INDICATOR);

    lv_obj_t *kn = make_label(p, "kN", &lv_font_montserrat_12, RD_COLOR_TEXT_DIM);
    lv_obj_align(kn, LV_ALIGN_TOP_MID, -34, 30);

    // zero line over the bar centre
    lv_obj_t *zero = lv_obj_create(p);
    lv_obj_remove_style_all(zero);
    lv_obj_set_size(zero, 26, 2);
    lv_obj_align(zero, LV_ALIGN_TOP_MID, 0, 30 + 54 - 1);
    lv_obj_set_style_bg_color(zero, C(RD_COLOR_TEXT_SECONDARY), 0);
    lv_obj_set_style_bg_opa(zero, LV_OPA_COVER, 0);

    g.effortValue = make_label(p, "+0", &lv_font_montserrat_20, RD_COLOR_TRACTION);
    lv_obj_align(g.effortValue, LV_ALIGN_BOTTOM_MID, 0, -10);
}

// Perspective "LINE AHEAD" view, matching the Qt Quick 3D scene: rails,
// sleepers and catenary masts moving with the train, horizon shifting with
// the gradient, the supervised braking target as a yellow lineside board.
// Scene colors mirror the Qt TrackView3D materials.
namespace la {
constexpr int kX = 998, kY = 58, kW = 270, kH = 240;
constexpr int kCx = kW / 2, kBottom = kH - 8;
constexpr double kZNear = 0.55;
constexpr uint32_t kGround = 0x10141E, kRail = 0x8E9AAF,
                   kSleeper = 0x39415A, kMast = 0x4A5570;
}

lv_obj_t *make_rect(lv_obj_t *parent, uint32_t color)
{
    lv_obj_t *r = lv_obj_create(parent);
    lv_obj_remove_style_all(r);
    lv_obj_set_style_bg_color(r, C(color), 0);
    lv_obj_set_style_bg_opa(r, LV_OPA_COVER, 0);
    return r;
}

void build_lineahead(lv_obj_t *scr)
{
    lv_obj_t *p = make_panel(scr, la::kX, la::kY, la::kW, la::kH);

    g.laGround = make_rect(p, la::kGround);

    for (auto &s : g.laSleepers)
        s = make_rect(p, la::kSleeper);

    g.laRailL = lv_line_create(p);
    g.laRailR = lv_line_create(p);
    for (lv_obj_t *rail : {g.laRailL, g.laRailR}) {
        lv_obj_set_style_line_color(rail, C(la::kRail), 0);
        lv_obj_set_style_line_width(rail, 3, 0);
    }

    for (int m = 0; m < 3; ++m) {
        g.laMastPole[m] = make_rect(p, la::kMast);
        g.laMastArm[m] = make_rect(p, la::kMast);
    }

    g.laBoardPole = make_rect(p, la::kMast);
    g.laBoardDisc = make_rect(p, RD_COLOR_YELLOW);
    lv_obj_set_style_radius(g.laBoardDisc, LV_RADIUS_CIRCLE, 0);

    make_title(p, "LINE AHEAD");
}

void update_lineahead(const TrainState &s)
{
    using namespace la;
    int horizon = 78 - (int)std::lround(s.gradientPermille * 1.5);
    horizon = std::max(56, std::min(100, horizon));
    const double cp = (kBottom - horizon) * kZNear; // y - horizon ~ 1/z
    auto yOf = [&](double z) { return horizon + cp / z; };
    auto hwOf = [&](double z) { return 66.0 * kZNear / z; };

    lv_obj_set_pos(g.laGround, 1, horizon);
    lv_obj_set_size(g.laGround, kW - 2, kBottom + 8 - horizon);

    g.laRailPtsL[0] = {(lv_value_precise_t)(kCx - 66), (lv_value_precise_t)kBottom};
    g.laRailPtsL[1] = {(lv_value_precise_t)(kCx - 1), (lv_value_precise_t)(horizon + 2)};
    g.laRailPtsR[0] = {(lv_value_precise_t)(kCx + 66), (lv_value_precise_t)kBottom};
    g.laRailPtsR[1] = {(lv_value_precise_t)(kCx + 1), (lv_value_precise_t)(horizon + 2)};
    lv_line_set_points(g.laRailL, g.laRailPtsL, 2);
    lv_line_set_points(g.laRailR, g.laRailPtsR, 2);

    // sleepers, 60 cm spacing (same as the Qt scene)
    const double phase = std::fmod(s.routePositionM * 100.0, 60.0) / 60.0;
    for (int i = 0; i < 12; ++i) {
        const double z = kZNear + (i + 1.0 - phase) * 0.42;
        const double hw = hwOf(z) * 1.18;
        const int th = std::max(1, (int)(6 * kZNear / z));
        lv_obj_set_pos(g.laSleepers[i], (int)(kCx - hw), (int)(yOf(z)) - th);
        lv_obj_set_size(g.laSleepers[i], (int)(2 * hw), th);
    }

    // catenary masts, 40 m spacing
    const double mphase = std::fmod(s.routePositionM * 100.0, 4000.0) / 4000.0;
    for (int m = 0; m < 3; ++m) {
        const double z = 0.7 + (m + 1.0 - mphase) * 3.4;
        const int x = (int)(kCx - hwOf(z) * 3.1);
        const int hpx = (int)(150 * kZNear / z);
        const int w = std::max(1, (int)(4 * kZNear / z));
        const int yBase = (int)yOf(z);
        lv_obj_set_pos(g.laMastPole[m], x, yBase - hpx);
        lv_obj_set_size(g.laMastPole[m], w, hpx);
        const int aw = (int)(26 * kZNear / z);
        const int ah = std::max(1, aw / 6);
        lv_obj_set_pos(g.laMastArm[m], x + w, yBase - hpx + hpx / 8);
        lv_obj_set_size(g.laMastArm[m], aw, ah);
    }

    // braking-target board (compressed distance, as in the Qt scene)
    if (s.distanceToTargetM >= 0 && s.distanceToTargetM < 2200) {
        lv_obj_remove_flag(g.laBoardPole, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(g.laBoardDisc, LV_OBJ_FLAG_HIDDEN);
        const double z = kZNear + s.distanceToTargetM / 110.0;
        const int x = (int)(kCx + hwOf(z) * 3.0);
        const int pw = std::max(1, (int)(3 * kZNear / z));
        const int ph = (int)(70 * kZNear / z);
        const int dia = std::max(3, (int)(22 * kZNear / z));
        const int yBase = (int)yOf(z);
        lv_obj_set_pos(g.laBoardPole, x, yBase - ph);
        lv_obj_set_size(g.laBoardPole, pw, ph);
        lv_obj_set_pos(g.laBoardDisc, x - dia / 2 + pw / 2, yBase - ph - dia);
        lv_obj_set_size(g.laBoardDisc, dia, dia);
    } else {
        lv_obj_add_flag(g.laBoardPole, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(g.laBoardDisc, LV_OBJ_FLAG_HIDDEN);
    }
}

void build_planning(lv_obj_t *scr)
{
    lv_obj_t *p = make_panel(scr, 998, 308, 270, 320);
    make_title(p, "ROUTE - 4 KM AHEAD");

    const int stripX = 44;
    const int stripY = 30;
    g.stripH = 320 - stripY - 14;
    g.strip = make_panel(p, stripX, stripY, 270 - stripX - 12, g.stripH,
                         RD_METRIC_RADIUS_SMALL, RD_COLOR_PANEL_INSET);

    // distance ruler
    for (int i = 0; i <= 4; ++i) {
        const int yy = stripY + g.stripH - (int)(i / 4.0 * g.stripH);
        lv_obj_t *line = lv_obj_create(p);
        lv_obj_remove_style_all(line);
        lv_obj_set_pos(line, stripX + 1, std::max(stripY, yy - 1));
        lv_obj_set_size(line, 270 - stripX - 14, 1);
        lv_obj_set_style_bg_color(line, C(RD_COLOR_LINE), 0);
        lv_obj_set_style_bg_opa(line, LV_OPA_COVER, 0);
        lv_obj_t *l = make_label(p, "", &lv_font_montserrat_12, RD_COLOR_TEXT_DIM);
        lv_label_set_text_fmt(l, i == 0 ? "0" : "%dk", i);
        lv_obj_set_pos(l, 14, std::min(stripY + g.stripH - 14, yy - 7));
    }

    // one item per route event, repositioned every tick
    const auto &route = g.sim->route();
    const auto &stations = g.sim->stations();
    int n = 0;
    for (const auto &seg : route) {
        PlanEvent &ev = g.events[n++];
        ev.absM = seg.startM;
        ev.isStation = false;
        ev.line = lv_obj_create(g.strip);
        lv_obj_remove_style_all(ev.line);
        lv_obj_set_size(ev.line, lv_obj_get_style_width(g.strip, 0) - 8, 2);
        lv_obj_set_style_bg_color(ev.line, C(RD_COLOR_YELLOW), 0);
        lv_obj_set_style_bg_opa(ev.line, LV_OPA_80, 0);
        ev.chip = make_panel(g.strip, 0, 0, 40, 20, 10, RD_COLOR_PANEL_INSET);
        lv_obj_set_style_border_color(ev.chip, C(RD_COLOR_YELLOW), 0);
        lv_obj_t *cl = make_label(ev.chip, "", &lv_font_montserrat_12, RD_COLOR_YELLOW);
        lv_label_set_text_fmt(cl, "%d", (int)seg.limitKmh);
        lv_obj_center(cl);
    }
    for (const auto &st : stations) {
        PlanEvent &ev = g.events[n++];
        ev.absM = st.positionM;
        ev.isStation = true;
        ev.line = lv_obj_create(g.strip);
        lv_obj_remove_style_all(ev.line);
        lv_obj_set_size(ev.line, lv_obj_get_style_width(g.strip, 0) - 8, 2);
        lv_obj_set_style_bg_color(ev.line, C(RD_COLOR_ACCENT), 0);
        lv_obj_set_style_bg_opa(ev.line, LV_OPA_COVER, 0);
        ev.chip = make_label(g.strip, "", &lv_font_montserrat_12, RD_COLOR_ACCENT);
        lv_label_set_text_fmt(ev.chip, LV_SYMBOL_STOP " %s", st.name.c_str());
    }
    g.eventCount = n;

    g.targetDot = lv_obj_create(p);
    lv_obj_remove_style_all(g.targetDot);
    lv_obj_set_size(g.targetDot, 8, 8);
    lv_obj_set_style_radius(g.targetDot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(g.targetDot, C(RD_COLOR_ARC_TARGET), 0);
    lv_obj_set_style_bg_opa(g.targetDot, LV_OPA_COVER, 0);

    lv_obj_t *train = make_label(g.strip, LV_SYMBOL_UP, &lv_font_montserrat_14,
                                 RD_COLOR_TEXT_PRIMARY);
    lv_obj_align(train, LV_ALIGN_BOTTOM_MID, 0, -4);

    g.gradient = make_tile(scr, 998, 638, 270, 54, "GRADIENT");
}

void build_buttons(lv_obj_t *scr)
{
    lv_obj_t *bar = make_panel(scr, 0, 704, 1280, 96, 0, RD_COLOR_PANEL_INSET);

    g.sifa = make_button(bar, 12, 140, "SIFA", "STANDBY", Command::SifaAcknowledge);
    g.paBtn = make_button(bar, 160, 96, "PA", "PASSENGERS", Command::PaToggle);
    g.radio = make_button(bar, 264, 96, "RADIO", "CONTROL CTR", Command::RadioToggle);
    g.dLRel = make_button(bar, 368, 104, "L RELEASE", "LOCKED", Command::DoorsLeftRelease);
    g.dLCls = make_button(bar, 480, 104, "L CLOSE", "DOORS LEFT", Command::DoorsLeftClose);
    g.dRRel = make_button(bar, 592, 104, "R RELEASE", "LOCKED", Command::DoorsRightRelease);
    g.dRCls = make_button(bar, 704, 104, "R CLOSE", "DOORS RIGHT", Command::DoorsRightClose);
    g.panto = make_button(bar, 816, 96, "PANTO", "UP", Command::PantographToggle);
    g.afb = make_button(bar, 920, 96, "AFB", "CRUISE", Command::AfbToggle);
    g.afbMinus = make_button(bar, 1024, 52, "-", "", Command::AfbDecrease);
    g.afbPlus = make_button(bar, 1084, 52, "+", "", Command::AfbIncrease);
    g.emerg = make_button(bar, 1148, 120, "EMERG", "BRAKE", Command::EmergencyBrakeToggle);
}

} // namespace

// ------------------------------------------------------------------ public --

void ui_create(TrainSimulation *sim)
{
    g.sim = sim;

    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, C(RD_COLOR_BG), 0);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    build_header(scr);

    // left column: pneumatics
    static const Zone pipeZones[] = {{0.0, 3.2, RD_COLOR_DANGER}, {4.6, 5.2, RD_COLOR_OK}};
    static const Zone cylZones[] = {{3.8, 5.0, RD_COLOR_WARN}};
    static const Zone resZones[] = {{0.0, 6.5, RD_COLOR_DANGER}, {8.0, 10.0, RD_COLOR_OK}};
    g.pipe = make_gauge(scr, 12, 58, 225, 172, "BRAKE PIPE", "bar", 0, 6, 1, 6, pipeZones, 2);
    g.cyl = make_gauge(scr, 12, 240, 225, 172, "BRAKE CYLINDER", "bar", 0, 5, 1, 5, cylZones, 1);
    g.res = make_gauge(scr, 12, 422, 225, 172, "MAIN RESERVOIR", "bar", 0, 11, 1, 11, resZones, 2);
    g.accel = make_tile(scr, 12, 604, 225, 58, "ACCELERATION");

    build_speedometer(scr);
    build_advisor_lever(scr);

    // right column: traction & electric
    static const Zone voltZones[] = {{0.0, 11.0, RD_COLOR_DANGER}, {13.5, 16.5, RD_COLOR_OK}};
    static const Zone currZones[] = {{320, 400, RD_COLOR_WARN}};
    g.volt = make_gauge(scr, 761, 58, 225, 172, "LINE VOLTAGE", "kV", 0, 18, 1, 6, voltZones, 2);
    g.curr = make_gauge(scr, 761, 240, 225, 172, "MOTOR CURRENT", "A", 0, 400, 0, 8, currZones, 1);
    build_effort(scr, 761, 422);
    g.power = make_tile(scr, 761, 622, 107, 58, "POWER");
    g.energy = make_tile(scr, 879, 622, 107, 58, "ENERGY");

    build_lineahead(scr);
    build_planning(scr);
    build_buttons(scr);

    ui_update();
}

void ui_update()
{
    const TrainState &s = g.sim->state();
    const bool blinkOn = ((int)(s.simTimeS * 1000) / 350) % 2 == 0;
    const bool standstill = s.speedKmh < 0.5;

    // header
    lv_label_set_text(g.mode.label, s.emergencyBrake ? "EMERGENCY"
                      : s.afbEnabled ? "AFB" : "MANUAL");
    badge_set(g.mode, s.afbEnabled || s.emergencyBrake,
              s.emergencyBrake ? RD_COLOR_DANGER : RD_COLOR_ACCENT);
    badge_set(g.pan, s.pantographUp, RD_COLOR_OK);
    badge_set(g.rad, s.radio != RadioState::Idle,
              s.radio == RadioState::Calling ? RD_COLOR_YELLOW : RD_COLOR_ACCENT);
    badge_set(g.pa, s.paActive, RD_COLOR_ACCENT);
    lv_label_set_text_fmt(g.station, "%s %s - %.1f km", LV_SYMBOL_RIGHT,
                          s.nextStationName.c_str(), s.nextStationDistanceM / 1000.0);
    lv_obj_align(g.station, LV_ALIGN_RIGHT_MID, -100, 0);
    lv_label_set_text_fmt(g.clock, "%02d:%02d:%02d", s.clockHour, s.clockMinute, s.clockSecond);

    if (s.alerts.empty()) {
        lv_label_set_text(g.alertLabel, "");
        lv_obj_set_style_border_color(g.alertBox, C(RD_COLOR_PANEL_INSET), 0);
        lv_obj_set_style_bg_color(g.alertBox, C(RD_COLOR_PANEL_INSET), 0);
    } else {
        const int idx = ((int)(s.simTimeS / 3.0)) % (int)s.alerts.size();
        const Alert &a = s.alerts[idx];
        const uint32_t col = a.severity == AlertSeverity::Critical ? RD_COLOR_DANGER
                           : a.severity == AlertSeverity::Warning ? RD_COLOR_WARN
                           : RD_COLOR_TEXT_SECONDARY;
        lv_label_set_text(g.alertLabel, a.text.c_str());
        lv_obj_set_style_text_color(g.alertLabel, C(col), 0);
        lv_obj_set_style_text_opa(g.alertLabel,
            (a.severity == AlertSeverity::Critical && !blinkOn) ? LV_OPA_40 : LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(g.alertBox, C(col), 0);
        lv_obj_set_style_bg_color(g.alertBox,
            C(a.severity == AlertSeverity::Critical ? RD_COLOR_DANGER_DIM : RD_COLOR_PANEL), 0);
    }

    // gauges & tiles
    gauge_update(g.pipe, s.brakePipeBar);
    gauge_update(g.cyl, s.brakeCylinderBar);
    gauge_update(g.res, s.mainReservoirBar);
    gauge_update(g.volt, s.lineVoltageKv);
    gauge_update(g.curr, s.motorCurrentA);
    lv_label_set_text_fmt(g.accel.value, "%+.2f m/s2", s.accelMs2);
    lv_label_set_text_fmt(g.power.value, "%.2f MW", s.powerMw);
    lv_obj_set_style_text_color(g.power.value,
        C(s.powerMw >= 0 ? RD_COLOR_TRACTION : RD_COLOR_REGEN), 0);
    lv_label_set_text_fmt(g.energy.value, "%.0f kWh", s.energyKwh);

    // speedometer
    const bool hasTarget = s.distanceToTargetM >= 0
                           && s.targetSpeedKmh < s.permittedSpeedKmh - 0.5;
    lv_arc_set_angles(g.arcPerm, 0, speedoAngle(s.permittedSpeedKmh));
    lv_arc_set_angles(g.arcTarget, speedoAngle(s.targetSpeedKmh),
                      hasTarget ? speedoAngle(s.permittedSpeedKmh) : speedoAngle(s.targetSpeedKmh));
    if (s.brakeIntervention) {
        lv_arc_set_angles(g.arcWarn, 0, 0);
        lv_arc_set_angles(g.arcDanger, speedoAngle(s.permittedSpeedKmh),
                          speedoAngle(std::fmax(s.speedKmh, s.permittedSpeedKmh + 4)));
    } else {
        lv_arc_set_angles(g.arcDanger, 0, 0);
        lv_arc_set_angles(g.arcWarn, speedoAngle(s.permittedSpeedKmh),
                          s.overspeedWarning ? speedoAngle(s.permittedSpeedKmh + 8) : speedoAngle(s.permittedSpeedKmh));
    }
    lv_scale_set_line_needle_value(g.speedScale, g.speedNeedle, 150,
                                   (int)std::lround(s.speedKmh));
    const uint32_t speedCol = s.brakeIntervention ? RD_COLOR_DANGER
                            : s.overspeedWarning ? RD_COLOR_WARN : RD_COLOR_TEXT_PRIMARY;
    lv_obj_set_style_line_color(g.speedNeedle, C(speedCol), 0);
    lv_label_set_text_fmt(g.speedLabel, "%d", (int)std::lround(s.speedKmh));
    lv_obj_set_style_text_color(g.speedLabel, C(speedCol), 0);
    lv_label_set_text_fmt(g.permLabel, "PERM %d", (int)std::lround(s.permittedSpeedKmh));

    if (hasTarget) {
        lv_obj_remove_flag(g.targetBox, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text_fmt(g.targetLabel, "%d", (int)std::lround(s.targetSpeedKmh));
        if (s.distanceToTargetM >= 1000)
            lv_label_set_text_fmt(g.distLabel, "%.1f km", s.distanceToTargetM / 1000.0);
        else
            lv_label_set_text_fmt(g.distLabel, "%d m", ((int)s.distanceToTargetM / 10) * 10);
    } else {
        lv_obj_add_flag(g.targetBox, LV_OBJ_FLAG_HIDDEN);
    }

    if (s.afbEnabled) {
        lv_obj_remove_flag(g.afbMarker, LV_OBJ_FLAG_HIDDEN);
        const double a = (RD_METRIC_SPEEDO_START_DEG
                          + speedoAngle(s.afbSetKmh)) * M_PI / 180.0;
        const double r = 448 / 2.0 - 22;
        lv_obj_set_pos(g.afbMarker,
                       (int)(235 + r * std::cos(a)) - 5,
                       (int)(235 + r * std::sin(a)) - 5);
    } else {
        lv_obj_add_flag(g.afbMarker, LV_OBJ_FLAG_HIDDEN);
    }

    // advisor
    static const char *glyphs[] = {"-", LV_SYMBOL_UP, LV_SYMBOL_MINUS, LV_SYMBOL_DOWN, LV_SYMBOL_STOP};
    static const char *labels[] = {"IDLE", "POWER", "HOLD", "COAST", "BRAKE"};
    static const uint32_t cols[] = {RD_COLOR_TEXT_DIM, RD_COLOR_ACCENT, RD_COLOR_TEXT_SECONDARY,
                                    RD_COLOR_YELLOW, RD_COLOR_WARN};
    const int hint = (int)s.advisor;
    lv_label_set_text(g.advisorGlyph, glyphs[hint]);
    lv_label_set_text(g.advisorText, labels[hint]);
    lv_obj_set_style_text_color(g.advisorGlyph, C(cols[hint]), 0);
    lv_obj_set_style_text_color(g.advisorText, C(cols[hint]), 0);

    // lever
    lv_label_set_text(g.leverTitle, s.afbEnabled ? "MASTER CONTROLLER - AFB"
                                                 : "MASTER CONTROLLER");
    lv_obj_set_style_text_color(g.leverTitle,
        C(s.afbEnabled ? RD_COLOR_ACCENT : RD_COLOR_TEXT_SECONDARY), 0);
    if (s.afbEnabled)
        lv_obj_add_state(g.slider, LV_STATE_DISABLED);
    else
        lv_obj_remove_state(g.slider, LV_STATE_DISABLED);
    if (!lv_obj_has_state(g.slider, LV_STATE_PRESSED))
        lv_slider_set_value(g.slider, (int)std::lround(s.leverPercent), LV_ANIM_OFF);
    lv_label_set_text_fmt(g.leverValue, "%+d %%", (int)std::lround(s.leverPercent));

    // effort
    lv_bar_set_value(g.effortBar, (int)std::lround(s.tractionEffortKn), LV_ANIM_OFF);
    lv_obj_set_style_bg_color(g.effortBar,
        C(s.tractionEffortKn >= 0 ? RD_COLOR_TRACTION : RD_COLOR_REGEN), LV_PART_INDICATOR);
    lv_label_set_text_fmt(g.effortValue, "%+d", (int)std::lround(s.tractionEffortKn));
    lv_obj_set_style_text_color(g.effortValue,
        C(s.tractionEffortKn >= 0 ? RD_COLOR_TRACTION : RD_COLOR_REGEN), 0);

    update_lineahead(s);

    // planning strip
    const double L = g.sim->routeLengthM();
    const int stripW = lv_obj_get_style_width(g.strip, 0);
    for (int i = 0; i < g.eventCount; ++i) {
        PlanEvent &ev = g.events[i];
        const double d = std::fmod(ev.absM - s.routePositionM + L, L);
        if (d <= kWindowM) {
            const int y = g.stripH - (int)(d / kWindowM * g.stripH);
            lv_obj_remove_flag(ev.line, LV_OBJ_FLAG_HIDDEN);
            lv_obj_remove_flag(ev.chip, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_pos(ev.line, 4, y - 1);
            if (ev.isStation)
                lv_obj_set_pos(ev.chip, 6, y - 18);
            else
                lv_obj_set_pos(ev.chip, stripW - 46, y - 22);
        } else {
            lv_obj_add_flag(ev.line, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(ev.chip, LV_OBJ_FLAG_HIDDEN);
        }
    }
    if (s.distanceToTargetM >= 0 && s.distanceToTargetM <= kWindowM) {
        lv_obj_remove_flag(g.targetDot, LV_OBJ_FLAG_HIDDEN);
        const int y = g.stripH - (int)(s.distanceToTargetM / kWindowM * g.stripH);
        lv_obj_set_pos(g.targetDot, 44 - 4, 30 + y - 4);
    } else {
        lv_obj_add_flag(g.targetDot, LV_OBJ_FLAG_HIDDEN);
    }
    lv_label_set_text_fmt(g.gradient.value, "%+.0f mm/m", s.gradientPermille);
    lv_obj_set_style_text_color(g.gradient.value,
        C(s.gradientPermille > 0 ? RD_COLOR_GRADIENT_CUT : RD_COLOR_OK), 0);

    // buttons
    const bool sifaAlarm = s.sifa == SifaStage::VisualWarning
                        || s.sifa == SifaStage::AudibleWarning
                        || s.sifa == SifaStage::EmergencyBrake;
    btn_set(g.sifa, sifaAlarm, RD_COLOR_DANGER, sifaAlarm, blinkOn);
    switch (s.sifa) {
    case SifaStage::Inactive: lv_label_set_text(g.sifa.sub, "STANDBY"); break;
    case SifaStage::Armed:
        lv_label_set_text_fmt(g.sifa.sub, "%d s", (int)std::ceil(s.sifaCountdownS)); break;
    case SifaStage::EmergencyBrake: lv_label_set_text(g.sifa.sub, "PENALTY BRAKE"); break;
    default:
        lv_label_set_text_fmt(g.sifa.sub, "ACK NOW - %d s", (int)std::ceil(s.sifaCountdownS));
    }

    btn_set(g.paBtn, s.paActive, RD_COLOR_ACCENT);
    lv_label_set_text(g.paBtn.sub, s.paActive ? "LIVE" : "PASSENGERS");
    btn_set(g.radio, s.radio != RadioState::Idle, RD_COLOR_ACCENT);
    lv_label_set_text(g.radio.sub, s.radio == RadioState::Idle ? "CONTROL CTR"
                      : s.radio == RadioState::Calling ? "CALLING..." : "ONLINE");

    btn_set(g.dLRel, s.doorLeft != DoorState::Locked, RD_COLOR_DOOR_OPEN);
    lv_label_set_text(g.dLRel.sub, door_text(s.doorLeft));
    if (standstill || s.doorLeft != DoorState::Locked)
        lv_obj_remove_state(g.dLRel.btn, LV_STATE_DISABLED);
    else
        lv_obj_add_state(g.dLRel.btn, LV_STATE_DISABLED);
    btn_set(g.dLCls, false, RD_COLOR_DOOR_CLOSED);
    btn_set(g.dRRel, s.doorRight != DoorState::Locked, RD_COLOR_DOOR_OPEN);
    lv_label_set_text(g.dRRel.sub, door_text(s.doorRight));
    if (standstill || s.doorRight != DoorState::Locked)
        lv_obj_remove_state(g.dRRel.btn, LV_STATE_DISABLED);
    else
        lv_obj_add_state(g.dRRel.btn, LV_STATE_DISABLED);
    btn_set(g.dRCls, false, RD_COLOR_DOOR_CLOSED);

    btn_set(g.panto, s.pantographUp, RD_COLOR_OK);
    lv_label_set_text(g.panto.sub, s.pantographUp ? "UP" : "DOWN");
    btn_set(g.afb, s.afbEnabled, RD_COLOR_ACCENT);
    if (s.afbEnabled)
        lv_label_set_text_fmt(g.afb.sub, "%d km/h", (int)s.afbSetKmh);
    else
        lv_label_set_text(g.afb.sub, "CRUISE");
    btn_set(g.afbMinus, false, RD_COLOR_ACCENT);
    btn_set(g.afbPlus, false, RD_COLOR_ACCENT);
    btn_set(g.emerg, s.emergencyBrake, RD_COLOR_DANGER, s.emergencyBrake, blinkOn);
    lv_label_set_text(g.emerg.sub, s.emergencyBrake ? "APPLIED" : "BRAKE");
}
