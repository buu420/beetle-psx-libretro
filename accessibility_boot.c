#include "accessibility_boot.h"

#include "accessibility_speech.h"

struct beetle_accessibility_boot_cue
{
   unsigned frame;
   const char *text;
   int priority;
};

static const struct beetle_accessibility_boot_cue boot_cues[] = {
   { 1,   "PlayStation BIOS startup.",        10 },
   { 90,  "Sony Computer Entertainment.",      9 },
   { 210, "PlayStation logo.",                 9 },
   { 360, "Starting game from disc.",          9 },
};

static unsigned boot_frame_count;
static unsigned boot_next_cue;
static bool boot_narration_active;

void beetle_accessibility_boot_reset(bool skip_bios)
{
   boot_frame_count      = 0;
   boot_next_cue         = 0;
   boot_narration_active = !skip_bios;
}

void beetle_accessibility_boot_frame(void)
{
   if (!boot_narration_active)
      return;

   boot_frame_count++;

   while (boot_next_cue < (sizeof(boot_cues) / sizeof(boot_cues[0]))
         && boot_frame_count >= boot_cues[boot_next_cue].frame)
   {
      beetle_accessibility_speak(boot_cues[boot_next_cue].text,
            boot_cues[boot_next_cue].priority, "bios");
      boot_next_cue++;
   }

   if (boot_next_cue >= (sizeof(boot_cues) / sizeof(boot_cues[0])))
      boot_narration_active = false;
}
