#pragma once

#include "ProjectConfig.h"
#include "icon_piano.h"
#include "icon_guitar.h"
#include "icon_drums.h"
#include "icon_trumpet.h"
#include "icon_violin.h"
#include "icon_accordion.h"
#include "icon_oud.h"
#include "icon_fan_fluit.h"

// Icon bitmap data stays in the original icon_*.h files.
static const uint8_t ICON_W = 64;
static const uint8_t ICON_H = 64;

static const uint16_t* const INST_ICONS[INST_COUNT] = {
  icon_piano_bitmap,
  icon_guitar_bitmap,
  icon_drums_bitmap,
  icon_trumpet_bitmap,
  icon_violin_bitmap,
  icon_accordion_bitmap,
  icon_oud_bitmap,
  icon_fan_fluit_bitmap
};
