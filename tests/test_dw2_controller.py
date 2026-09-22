"""Exercise DW2 shortcuts through the real input and navigation readers."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


CORE = Path(os.environ.get("DW2_CORE_SOURCE", Path(__file__).resolve().parents[1]))
MODULES = (
    "accessibility_game", "accessibility_dw2_profile", "accessibility_dw2_text",
    "accessibility_dw2_menu_profile", "accessibility_dw2_menu",
    "accessibility_dw2_domain_menu", "accessibility_dw2_navigation",
    "accessibility_dw2_story", "accessibility_dw2_npc", "accessibility_trace",
)
HARNESS = r'''
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "accessibility_game.h"
#include "accessibility_dw2_navigation.h"
#include "accessibility_dw2_menu_profile.h"

static uint16_t buttons;
static bool masks = true;
static unsigned mask_queries;
static bool keys[512];
static int16_t axes[2][2];
static int16_t pressure[16];
static uint8_t ram[0x200000];
static unsigned count;
static char spoken[256][384];
static beetle_dw2_navigation_snapshot_t snapshot;
#define BIT(id) ((uint16_t)(1u << RETRO_DEVICE_ID_JOYPAD_##id))
#define CHECK(c,n) do { if (!(c)) { fprintf(stderr,"check %d, count %u, last: %s\n",n,count,count ? spoken[count-1] : "none"); return n; } } while (0)

bool beetle_accessibility_speak(const char *text, int priority, const char *channel)
{
   (void)priority; (void)channel;
   if (count < 256) snprintf(spoken[count++], sizeof(spoken[0]), "%s", text);
   return true;
}
static int16_t raw(unsigned port, unsigned device, unsigned index, unsigned id)
{
   (void)port;
   if (device == RETRO_DEVICE_KEYBOARD) return id < 512 ? keys[id] : 0;
   if (device == RETRO_DEVICE_ANALOG) {
      if (index == RETRO_DEVICE_INDEX_ANALOG_BUTTON) return id < 16 ? pressure[id] : 0;
      return index < 2 && id < 2 ? axes[index][id] : 0;
   }
   if (device != RETRO_DEVICE_JOYPAD || index != 0) return 123;
   if (id == RETRO_DEVICE_ID_JOYPAD_MASK) {
      mask_queries++;
      return masks ? (int16_t)buttons : -1;
   }
   return id < 16 && (buttons & (1u << id)) ? 1 : 0;
}
static void poll(uint16_t held)
{
   buttons = held;
   beetle_accessibility_game_input(raw);
}
static int16_t filtered(unsigned device, unsigned index, unsigned id)
{
   return beetle_accessibility_game_filter_input(raw, 0, device, index, id);
}
static void chord(uint16_t held)
{
   poll(BIT(L2)); poll(BIT(L2) | held);
}
static bool last(const char *text)
{
   return count && strstr(spoken[count-1], text) != NULL;
}
static void setup(void)
{
   buttons = 0;
   masks = true;
   mask_queries = 0;
   memset(keys, 0, sizeof(keys));
   memset(axes, 0, sizeof(axes));
   memset(pressure, 0, sizeof(pressure));
   memset(ram, 0, sizeof(ram));
   beetle_accessibility_game_set_controller_navigation(true);
   beetle_accessibility_game_set_serial("SLUS_011.93");
   memset(&snapshot, 0, sizeof(snapshot));
   snapshot.context = BEETLE_DW2_NAV_CONTEXT_DOMAIN;
   snapshot.width = 8; snapshot.height = 8;
   snapshot.player_settled = true;
   strcpy(snapshot.location, "SCSI Domain. Floor 2");
   snapshot.target_count = 3;
   snapshot.targets[0].kind = BEETLE_DW2_NAV_TARGET_STORY_EVENT;
   snapshot.targets[0].id = 10; snapshot.targets[0].x = 4;
   strcpy(snapshot.targets[0].label, "Story destination");
   snapshot.targets[1].kind = BEETLE_DW2_NAV_TARGET_ENEMY_DIGIMON;
   snapshot.targets[1].id = 11; snapshot.targets[1].x = 2;
   strcpy(snapshot.targets[1].label, "Agumon");
   snapshot.targets[2].kind = BEETLE_DW2_NAV_TARGET_ENEMY_DIGIMON;
   snapshot.targets[2].id = 12; snapshot.targets[2].x = 3;
   strcpy(snapshot.targets[2].label, "Betamon");
   beetle_accessibility_dw2_navigation_update_snapshot(&snapshot, false);
   count = 0;
}
static int mapping(void)
{
   unsigned i, before;
   setup();
   poll(BIT(L2) | BIT(RIGHT));
   CHECK(count == 1 && strstr(spoken[0], "Enemy Digimon"), 1);
   CHECK(last("Agumon. 1 of 2."), 2);
   chord(BIT(DOWN)); CHECK(last("Betamon. 2 of 2."), 3);
   chord(BIT(UP)); CHECK(last("Agumon. 1 of 2."), 4);
   chord(BIT(Y)); CHECK(last("Agumon. 1 of 2."), 5);
   chord(BIT(LEFT)); CHECK(last("Story Events"), 6);
   chord(BIT(LEFT)); CHECK(last("Treasure Boxes. Empty."), 7);
   chord(BIT(X)); CHECK(last("SCSI Domain. Floor 2. X 0, Y 0."), 8);
   before = count;
   for (i = 0; i < 100; i++) poll(BIT(L2) | BIT(X));
   CHECK(count == before, 9);
   chord(BIT(B) | BIT(A) | BIT(R3)); CHECK(count == before, 10);
   return 0;
}
static int masking(bool support_masks)
{
   unsigned id;
   setup(); masks = support_masks;
   keys[RETROK_a] = true;
   poll(BIT(L2) | BIT(RIGHT) | BIT(Y) | BIT(START));
   CHECK(mask_queries == 0, 1);
   if (support_masks) CHECK(filtered(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_MASK) == 0, 1);
   for (id = 0; id < 16; id++) CHECK(filtered(RETRO_DEVICE_JOYPAD, 0, id) == 0, 2);
   CHECK(beetle_accessibility_game_filter_input(raw, 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT) == 1, 3);
   CHECK(filtered(RETRO_DEVICE_KEYBOARD, 0, RETROK_a) == 1, 4);
   CHECK(filtered(RETRO_DEVICE_MOUSE, 0, 0) == 123, 5);
   poll(BIT(RIGHT) | BIT(Y) | BIT(START) | BIT(A));
   CHECK(filtered(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT) == 0, 6);
   CHECK(filtered(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y) == 0, 7);
   CHECK(filtered(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START) == 0, 8);
   CHECK(filtered(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A) == 1, 9);
   if (support_masks) CHECK((uint16_t)filtered(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_MASK) == BIT(A), 10);
   poll(0); poll(BIT(RIGHT));
   CHECK(filtered(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT) == 1, 11);
   /* A high signed-mask bit must not hide L2 or extend the mask. */
   poll(BIT(L2) | BIT(R3)); CHECK(filtered(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R3) == 0, 12);
   /* A release seen while still in the layer discharges that button's latch. */
   poll(BIT(L2) | BIT(RIGHT)); poll(BIT(L2)); poll(BIT(RIGHT));
   CHECK(filtered(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT) == 1, 13);
   return 0;
}
static int analog(void)
{
   unsigned i;
   setup();
   for (i = 0; i < 4; i++) axes[i/2][i%2] = i % 2 ? -32768 : 24000;
   pressure[RETRO_DEVICE_ID_JOYPAD_Y] = 8000;
   poll(BIT(L2));
   for (i = 0; i < 4; i++) CHECK(filtered(RETRO_DEVICE_ANALOG, i/2, i%2) == 0, 1);
   CHECK(filtered(RETRO_DEVICE_ANALOG, 2, RETRO_DEVICE_ID_JOYPAD_Y) == 0, 2);
   CHECK(beetle_accessibility_game_filter_input(raw, 1, RETRO_DEVICE_ANALOG, 0, 0) == 24000, 3);
   poll(0);
   for (i = 0; i < 4; i++) CHECK(filtered(RETRO_DEVICE_ANALOG, i/2, i%2) == 0, 4);
   CHECK(filtered(RETRO_DEVICE_ANALOG, 2, RETRO_DEVICE_ID_JOYPAD_Y) == 0, 5);
   axes[0][0] = 500; poll(0); axes[0][0] = 24000; poll(0);
   CHECK(filtered(RETRO_DEVICE_ANALOG, 0, 0) == 24000, 6);
   CHECK(filtered(RETRO_DEVICE_ANALOG, 1, 0) == 0, 7);
   pressure[RETRO_DEVICE_ID_JOYPAD_Y] = 0; poll(0);
   pressure[RETRO_DEVICE_ID_JOYPAD_Y] = 8000; poll(0);
   CHECK(filtered(RETRO_DEVICE_ANALOG, 2, RETRO_DEVICE_ID_JOYPAD_Y) == 8000, 8);
   memset(axes, 0, sizeof(axes)); poll(0);
   axes[1][1] = -20000; poll(0);
   CHECK(filtered(RETRO_DEVICE_ANALOG, 1, 1) == -20000, 9);
   return 0;
}
static int repetition(void)
{
   unsigned i;
   setup();
   for (i = 0; i < 100; i++) poll(BIT(RIGHT));
   CHECK(count == 0, 1);
   poll(BIT(L2) | BIT(RIGHT)); CHECK(count == 1, 2);
   for (i = 0; i < 30; i++) poll(BIT(L2) | BIT(RIGHT));
   CHECK(count == 1, 3);
   poll(BIT(L2) | BIT(RIGHT)); CHECK(count == 2, 4);
   for (i = 0; i < 5; i++) poll(BIT(L2) | BIT(RIGHT));
   CHECK(count == 2, 5);
   poll(BIT(L2) | BIT(RIGHT)); CHECK(count == 3, 6);
   poll(0); poll(BIT(L2) | BIT(RIGHT)); CHECK(count == 4, 7);
   return 0;
}
static int speculative(void)
{
   unsigned i;
   setup();
   beetle_accessibility_game_set_frame_enabled(false);
   for (i = 0; i < 50; i++) poll(BIT(L2) | BIT(RIGHT));
   CHECK(count == 0 && filtered(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT) == 0, 1);
   beetle_accessibility_game_set_frame_enabled(true);
   poll(BIT(L2) | BIT(RIGHT)); CHECK(count == 1, 2);
   beetle_accessibility_game_set_frame_enabled(false);
   /* Releasing only the trigger must still mask the held direction. */
   poll(BIT(RIGHT));
   CHECK(filtered(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT) == 0, 3);
   for (i = 0; i < 50; i++) poll(BIT(L2) | BIT(RIGHT));
   beetle_accessibility_game_set_frame_enabled(true);
   for (i = 0; i < 30; i++) poll(BIT(L2) | BIT(RIGHT));
   CHECK(count == 1, 4);
   poll(BIT(L2) | BIT(RIGHT)); CHECK(count == 2, 5);
   return 0;
}
static int secondary_speculative(void)
{
   setup(); beetle_accessibility_game_set_frame_enabled(false);
   axes[0][0] = 24000;
   poll(BIT(L2) | BIT(RIGHT)); poll(BIT(RIGHT));
   CHECK(count == 0, 1);
   CHECK(filtered(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT) == 0, 2);
   CHECK(filtered(RETRO_DEVICE_ANALOG, 0, 0) == 0, 3);
   axes[0][0] = 0; poll(0);
   axes[0][0] = 24000; poll(BIT(RIGHT));
   CHECK(count == 0, 4);
   CHECK(filtered(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT) == 1, 5);
   CHECK(filtered(RETRO_DEVICE_ANALOG, 0, 0) == 24000, 6);
   return 0;
}
static int lifecycle(void)
{
   unsigned before;
   setup();
   poll(BIT(L2) | BIT(RIGHT));
   beetle_accessibility_game_set_controller_navigation(false);
   before = count; poll(BIT(L2) | BIT(RIGHT));
   CHECK(count == before && filtered(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT) == 1, 1);
   keys[RETROK_END] = true; poll(BIT(L2)); CHECK(last("Floor Portal"), 2);
   keys[RETROK_END] = false; poll(0);
   beetle_accessibility_game_reset(); count = 0;
   poll(BIT(L2) | BIT(RIGHT)); CHECK(count == 0, 3);
   beetle_accessibility_game_set_controller_navigation(true);
   poll(BIT(L2) | BIT(RIGHT)); CHECK(last("Navigation unavailable"), 4);
   poll(BIT(RIGHT)); CHECK(filtered(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT) == 0, 5);
   beetle_accessibility_game_state_discontinuity(); poll(BIT(RIGHT));
   CHECK(filtered(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT) == 1, 6);
   beetle_accessibility_game_set_serial("OTHER_GAME"); count = 0;
   poll(BIT(L2) | BIT(RIGHT)); CHECK(count == 0 && filtered(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT) == 1, 7);
   setup(); poll(BIT(L2) | BIT(RIGHT));
   beetle_accessibility_game_set_serial(NULL); count = 0;
   poll(BIT(RIGHT)); CHECK(count == 0 && filtered(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT) == 1, 8);
   beetle_accessibility_game_input(NULL);
   CHECK(beetle_accessibility_game_filter_input(NULL, 0, RETRO_DEVICE_JOYPAD, 0, 4) == 0, 9);
   return 0;
}
static int guidance(void)
{
   unsigned i, before;
   setup(); chord(BIT(L3)); CHECK(last("Right 4."), 1);
   before = count;
   for (i = 0; i < 100; i++) poll(BIT(L2) | BIT(L3));
   CHECK(count == before, 2);
   chord(BIT(L3)); CHECK(last("Guidance stopped."), 3);
   before = count; snapshot.player_x = 4;
   beetle_accessibility_dw2_navigation_update_snapshot(&snapshot, false);
   CHECK(count == before, 4);
   snapshot.player_x = 1; snapshot.player_settled = false;
   beetle_accessibility_dw2_navigation_update_snapshot(&snapshot, false);
   chord(BIT(L3)); CHECK(count == before, 5);
   chord(BIT(L3)); CHECK(last("Guidance stopped."), 6);
   before = count; snapshot.player_settled = true;
   beetle_accessibility_dw2_navigation_update_snapshot(&snapshot, false);
   CHECK(count == before, 7);
   chord(BIT(L3)); CHECK(last("Right 3."), 8);
   beetle_accessibility_dw2_navigation_update_snapshot(NULL, true);
   chord(BIT(L3)); CHECK(last("Guidance stopped."), 9);
   before = count;
   beetle_accessibility_dw2_navigation_update_snapshot(&snapshot, false);
   CHECK(count == before, 10);
   /* The same toggle belongs to keyboard users, including pending story routes. */
   keys[RETROK_KP_ENTER] = true; poll(0); CHECK(last("Right 3."), 11);
   keys[RETROK_KP_ENTER] = false; poll(0);
   beetle_accessibility_dw2_navigation_update_snapshot(NULL, false);
   keys[RETROK_KP_ENTER] = true; poll(0); CHECK(last("Guidance stopped."), 12);
   keys[RETROK_KP_ENTER] = false; poll(0); before = count;
   beetle_accessibility_dw2_navigation_update_snapshot(&snapshot, false);
   CHECK(count == before, 13);
   chord(BIT(Y)); CHECK(last("Story destination"), 14);
   return 0;
}
static void u32(size_t at, uint32_t value)
{
   unsigned i; for (i = 0; i < 4; i++) ram[at+i] = (uint8_t)(value >> (i*8));
}
static void game_frames(void)
{
   unsigned i; for (i = 0; i < 6; i++) beetle_accessibility_game_frame(ram, sizeof(ram));
}
static int title(void)
{
   size_t n, i, j;
   const beetle_dw2_overlay_signature_t *s = beetle_accessibility_dw2_overlay_signatures(&n);
   setup();
   for (i = 0; i < n; i++) if (s[i].tag == 0x191)
      for (j = 0; j < 4; j++) u32((s[i].signature_address & 0x1fffffu) + j*4, s[i].signature_words[j]);
   ram[0x62a48] = 0x78; ram[0x62a49] = 5; game_frames();
   CHECK(last("Press Start"), 1); count = 0;
   poll(BIT(L2) | BIT(START)); game_frames(); CHECK(count == 0, 2);
   poll(BIT(START)); game_frames(); CHECK(count == 0, 3);
   poll(0); poll(BIT(START)); game_frames(); CHECK(last("New Game"), 4);
   return 0;
}
static int location(void)
{
   setup(); snapshot.player_settled = false;
   beetle_accessibility_dw2_navigation_update_snapshot(&snapshot, false);
   chord(BIT(X)); CHECK(last("Moving. Stop to read coordinates.") && !last("X 0"), 1);
   beetle_accessibility_dw2_navigation_update_snapshot(NULL, true);
   chord(BIT(X)); CHECK(last("Navigation paused"), 2);
   beetle_accessibility_dw2_navigation_update_snapshot(NULL, false);
   chord(BIT(X)); CHECK(last("Navigation unavailable"), 3);
   /* Native city scene and renderer coordinates, without synthetic labels. */
   ram[0x5f788] = 36;
   u32(0x50798, 1); u32(0x5079c, 0x80090000);
   u32(0x90000, 0x302); u32(0x90010, 1);
   u32(0x9002c, 0x80091000); u32(0x90038, 0x80092000);
   u32(0x92030, (uint32_t)((4 - 11) * 0x600));
   u32(0x92038, (uint32_t)((11 - 6) * 0x600));
   beetle_accessibility_dw2_navigation_frame(ram, sizeof(ram), 0x192, false);
   chord(BIT(X)); CHECK(last("Jijimon's House. X 4, Y 6."), 4);
   memset(ram, 0, sizeof(ram)); ram[0x5f788] = 42;
   beetle_accessibility_dw2_navigation_frame(ram, sizeof(ram), 0x192, false);
   chord(BIT(X)); CHECK(last("Area Selection. Coordinates unavailable here."), 5);
   /* Never treat a malformed location string as unbounded text. */
   snapshot.player_settled = true;
   memset(snapshot.location, 'a', sizeof(snapshot.location));
   beetle_accessibility_dw2_navigation_update_snapshot(&snapshot, false);
   chord(BIT(X)); CHECK(last("Domain. X 0, Y 0."), 6);
   return 0;
}
int main(int argc, char **argv)
{
   if (argc != 2) return 99;
   if (!strcmp(argv[1], "mapping")) return mapping();
   if (!strcmp(argv[1], "mask")) return masking(true);
   if (!strcmp(argv[1], "buttons")) return masking(false);
   if (!strcmp(argv[1], "analog")) return analog();
   if (!strcmp(argv[1], "repeat")) return repetition();
   if (!strcmp(argv[1], "speculative")) return speculative();
   if (!strcmp(argv[1], "secondary")) return secondary_speculative();
   if (!strcmp(argv[1], "lifecycle")) return lifecycle();
   if (!strcmp(argv[1], "guidance")) return guidance();
   if (!strcmp(argv[1], "title")) return title();
   if (!strcmp(argv[1], "location")) return location();
   return 98;
}
'''


class ControllerTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        compiler = shutil.which(os.environ.get("CC", "gcc"))
        if not compiler and os.name == "nt":
            compiler = r"C:\msys64\mingw64\bin\gcc.exe"
        if not compiler or not Path(compiler).is_file():
            raise unittest.SkipTest("A C compiler is required")
        cls.tmp = tempfile.TemporaryDirectory()
        cls.addClassCleanup(cls.tmp.cleanup)
        source = Path(cls.tmp.name) / "controller.c"
        source.write_text(HARNESS, encoding="utf-8")
        cls.exe = source.with_suffix(".exe" if os.name == "nt" else "")
        cls.env = dict(os.environ)
        cls.env["PATH"] = str(Path(compiler).parent) + os.pathsep + cls.env.get("PATH", "")
        result = subprocess.run(
            [compiler, "-std=c99", "-Wall", "-Wextra", "-Werror",
             "-I", str(CORE), "-I", str(CORE / "libretro-common/include"),
             str(source), *[str(CORE / (name + ".c")) for name in MODULES],
             "-o", str(cls.exe)], capture_output=True, text=True, env=cls.env)
        if result.returncode:
            raise AssertionError(result.stderr)

    def run_scenario(self, name):
        result = subprocess.run([str(self.exe), name], capture_output=True, text=True, env=self.env)
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_common_mapping_and_face_button_positions(self):
        self.run_scenario("mapping")

    def test_bitmask_filter_and_chord_release(self):
        self.run_scenario("mask")

    def test_individual_button_fallback_and_chord_release(self):
        self.run_scenario("buttons")

    def test_sticks_pressure_buttons_and_center_drift(self):
        self.run_scenario("analog")

    def test_direction_repeat_starts_when_layer_is_entered(self):
        self.run_scenario("repeat")

    def test_speculative_frames_mask_without_consuming_commands(self):
        self.run_scenario("speculative")

    def test_secondary_runahead_instance_keeps_release_latches_without_audio(self):
        self.run_scenario("secondary")

    def test_option_reset_unload_other_games_and_keyboard(self):
        self.run_scenario("lifecycle")

    def test_guidance_toggle_cancels_pending_resume_and_story_tracking(self):
        self.run_scenario("guidance")

    def test_masked_start_cannot_advance_title_reader(self):
        self.run_scenario("title")

    def test_location_from_native_city_scene_and_validated_coordinates(self):
        self.run_scenario("location")


if __name__ == "__main__":
    unittest.main()
