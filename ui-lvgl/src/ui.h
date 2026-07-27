#ifndef RAILDECK_LVGL_UI_H
#define RAILDECK_LVGL_UI_H

#include "traincore/train_simulation.h"

// Builds the complete cab display on the active LVGL screen.
void ui_create(traincore::TrainSimulation *sim);

// Refresh every widget from the current simulation state (call each tick).
void ui_update();

#endif // RAILDECK_LVGL_UI_H
