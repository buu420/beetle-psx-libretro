#ifndef BEETLE_PSX_ACCESSIBILITY_SPEECH_H
#define BEETLE_PSX_ACCESSIBILITY_SPEECH_H

#include <stdbool.h>

bool beetle_accessibility_speak(const char *text, int priority,
      const char *channel);

#endif
