#pragma once

#include <Arduino.h>

static const int BTN_BACK = 1;
static const int BTN_NOTE_M = 2;
static const int BTN_NOTE_P = 3;
static const int BTN_INST = 4;
static const int BTN_RELEASE = 5;

static const int COLOR_BOX_X = 240;
static const int COLOR_BOX_Y = 60;
static const int COLOR_BOX_W = 50;
static const int COLOR_BOX_H = 24;

void calcLayout();
void drawMain();
void drawCfg(uint8_t keyIndex);
void drawInstPicker(uint8_t keyIndex);
void drawColorPicker(uint8_t keyIndex);
void drawTile(uint8_t keyIndex);
int hitKey(int16_t tx, int16_t ty);
int hitInst(int16_t tx, int16_t ty);
int hitColor(int16_t tx, int16_t ty);
void showColorMsg(const char* msg);
