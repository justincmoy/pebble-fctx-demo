#include "settings.h"

void init_settings() {
    settings.FaceColor = GColorWhite;
    settings.MinuteHandColor = GColorBlack;
    settings.MinuteTextColor = GColorWhite;

    persist_read_data(PERSIST_KEY_SETTINGS, &settings, sizeof(settings));
}
