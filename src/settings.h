#pragma once
#include <pebble.h>

#define PERSIST_KEY_SETTINGS 5

typedef struct __attribute__((packed)) ClaySettings {
    GColor FaceColor;
    GColor MinuteHandColor;
    GColor MinuteTextColor;
} ClaySettings;
ClaySettings settings;

enum SettingName {
    FACE_COLOR,
    MINUTE_HAND_COLOR,
    MINUTE_TEXT_COLOR,
    PALETTE_SIZE
};

void init_settings();
