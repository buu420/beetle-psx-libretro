#include "accessibility_game.h"

#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "accessibility_dw2_menu.h"
#include "accessibility_dw2_domain_menu.h"
#include "accessibility_dw2_menu_profile.h"
#include "accessibility_dw2_navigation.h"
#include "accessibility_dw2_story.h"
#include "accessibility_dw2_profile.h"
#include "accessibility_dw2_text.h"
#include "accessibility_speech.h"
#include "accessibility_trace.h"

/* Native Digimon World 2 (USA) title-screen cue observed from SLUS_011.93.
 * 0x062A48 is an emulated PS1 RAM address, not a host pointer or screen
 * overlay. Later tracing proved it is display/rendering state after Start,
 * so it is only used for the stable pre-Start "Press Start" cue. */
#define DIGIMON_WORLD_2_TITLE_MENU_OFFSET 0x062A48
#define DIGIMON_WORLD_2_TITLE_MENU_SIG_NEW_GAME 0x0578
/* STAG1000 owns title controller 800635a4; STAG0000 is the intro. */
#define DIGIMON_WORLD_2_TITLE_OVERLAY_TAG 0x00000191u
#define DIGIMON_WORLD_2_TITLE_SELECTION_OFFSET 0x15858C
#define DIGIMON_WORLD_2_TITLE_SELECTION_STABLE_FRAMES 3
#define DIGIMON_WORLD_2_TITLE_SELECTION_INVALID_REARM_FRAMES 3
/* Battle command speech is owned entirely by the native command controller
 * (0x80064b30) in accessibility_dw2_menu.c. The obsolete global-based fallback
 * that used to live here is gone: it required the tamer owner (0x073CC8 == 6)
 * before emitting Battle/Guard, which belong to a Digimon owner, so it could
 * announce a command that was not on screen and could never announce the real
 * one. Only the overlay tag is still needed, for the navigation gate. */
#define DIGIMON_WORLD_2_BATTLE_OVERLAY_TAG 0x00000193u
#define DIGIMON_WORLD_2_DIALOG_INITIAL_STATE_ADDRESS 0x80140E00
#define DIGIMON_WORLD_2_DIALOG_TEXT_POINTER_OFFSET 0x08
#define DIGIMON_WORLD_2_DIALOG_SPEAKER_MAX_CHARS 96
#define DIGIMON_WORLD_2_DIALOG_CHOICE_OFFSET 0x29
#define DIGIMON_WORLD_2_DIALOG_ACTIVE_VALUE 1
#define DIGIMON_WORLD_2_DIALOG_STABLE_FRAMES 3
#define DIGIMON_WORLD_2_DIALOG_MAX_CHARS 192
#define DIGIMON_WORLD_2_TASK_COUNT_OFFSET 0x00050798u
#define DIGIMON_WORLD_2_TASK_LIST_OFFSET 0x0005079cu
#define DIGIMON_WORLD_2_TASK_LIMIT 128u
#define DIGIMON_WORLD_2_RENDER_TASK_TYPE 9u
#define DIGIMON_WORLD_2_RENDER_SLOT_SIZE 0x34u
#define DIGIMON_WORLD_2_RENDER_SLOT_COUNT 50u
#define DIGIMON_WORLD_2_PLAYER_NAME_OFFSET 0x05E634
#define DIGIMON_WORLD_2_DIGI_BETTLE_NAME_OFFSET 0x05E6F1
#define DIGIMON_WORLD_2_SOURCE_NAME_OFFSET 0x05E750
#define DIGIMON_WORLD_2_SOURCE_NAME_STRIDE 0x5C
#define DIGIMON_WORLD_2_NAME_ENTRY_STATE_MIN_BYTES 0x30
#define DIGIMON_WORLD_2_NAME_ENTRY_MAX_NAME_BYTES 16
#define DIGIMON_WORLD_2_NAME_ENTRY_MAX_SOURCE_INDEX 31
#define DIGIMON_WORLD_2_NAME_ENTRY_MAX_COLUMN 10
#define DIGIMON_WORLD_2_NAME_ENTRY_MAX_ROW 7
#define DIGIMON_WORLD_2_NAME_ENTRY_OK_COLUMN 10
#define DIGIMON_WORLD_2_NAME_ENTRY_OK_ROW 7
#define DIGIMON_WORLD_2_NAME_ENTRY_GRID_RESOURCE_OFFSET 6
#define DIGIMON_WORLD_2_NAME_ENTRY_STABLE_FRAMES 3
#define DIGIMON_WORLD_2_NAME_ENTRY_INACTIVE_GRACE_FRAMES 12
#define DIGIMON_WORLD_2_PSX_RAM_MASK 0x1fffff
#define DIGIMON_WORLD_2_MENU_PROBE_CAPACITY 64
#define DIGIMON_WORLD_2_NAVIGATION_KEY_COUNT 6

static bool is_digimon_world_2;
static bool dw2_frame_enabled = true;
static bool dw2_start_pressed;
static bool dw2_start_was_down;
static bool dw2_press_start_ready;
static bool dw2_press_start_spoken;
static bool dw2_title_selection_state_verified;
static bool dw2_title_seen;
static uint32_t dw2_last_movie_cue_sector;
static uint16_t pending_signature;
static unsigned pending_frames;
static uint8_t dw2_pending_title_selection;
static unsigned dw2_pending_title_selection_frames;
static unsigned dw2_invalid_title_selection_frames;
static uint32_t dw2_pending_dialog_text_ptr;
static uint32_t dw2_rendered_dialog_text_ptr;
static unsigned dw2_pending_dialog_frames;
static uint32_t dw2_spoken_dialog_text_ptr;
static bool dw2_pending_dialog_choice_valid;
static uint8_t dw2_pending_dialog_choice;
static unsigned dw2_pending_dialog_choice_frames;
static bool dw2_spoken_dialog_choice_valid;
static uint8_t dw2_spoken_dialog_choice;
static bool dw2_dialog_active;
static uint32_t dw2_dialog_state_address =
   DIGIMON_WORLD_2_DIALOG_INITIAL_STATE_ADDRESS;
static char spoken_label[64];
static char dw2_pending_dialog_text[DIGIMON_WORLD_2_DIALOG_MAX_CHARS];
static char spoken_dialog_text[DIGIMON_WORLD_2_DIALOG_MAX_CHARS];
static bool dw2_name_entry_probe_seen;
static uint32_t dw2_name_entry_probe_state;
static bool dw2_name_entry_selection_seen;
static uint32_t dw2_name_entry_selection_state;
static uint8_t dw2_name_entry_selected;
static bool dw2_name_entry_active;
static unsigned dw2_name_entry_inactive_frames;
static uint32_t dw2_pending_name_entry_state;
static unsigned dw2_pending_name_entry_frames;
static char dw2_pending_name[DIGIMON_WORLD_2_NAME_ENTRY_MAX_NAME_BYTES + 1];
static char dw2_pending_name_focus[32];
static uint16_t dw2_pending_name_column;
static uint16_t dw2_pending_name_row;
static char dw2_spoken_name[DIGIMON_WORLD_2_NAME_ENTRY_MAX_NAME_BYTES + 1];
static char dw2_spoken_name_focus[32];
static uint16_t dw2_spoken_name_column;
static uint16_t dw2_spoken_name_row;
static bool dw2_navigation_keys_down[DIGIMON_WORLD_2_NAVIGATION_KEY_COUNT];
static bool dw2_controller_navigation_enabled = true;
static struct
{
   uint16_t previous;
   uint16_t latched;
   uint16_t blocked;
   uint8_t latched_axes;
   uint8_t blocked_axes;
   unsigned direction_frames[4];
} dw2_controller;
static const uint8_t *dw2_main_ram;
static size_t dw2_main_ram_size;
static uint32_t dw2_active_overlay_tag;
static beetle_dw2_menu_probe_t
   dw2_captured_menu_probes[DIGIMON_WORLD_2_MENU_PROBE_CAPACITY];
static size_t dw2_captured_menu_probe_count;

static uint16_t read_u16le(const uint8_t *ram, size_t offset)
{
   return (uint16_t)(ram[offset] | ((uint16_t)ram[offset + 1] << 8));
}

static uint32_t read_u32le(const uint8_t *ram, size_t offset)
{
   return (uint32_t)ram[offset]
      | ((uint32_t)ram[offset + 1] << 8)
      | ((uint32_t)ram[offset + 2] << 16)
      | ((uint32_t)ram[offset + 3] << 24);
}

static void speak_dw2_title_label(const char *label)
{
   if (!label || !*label)
      return;

   if (spoken_label[0] && !strcmp(spoken_label, label))
      return;

   beetle_accessibility_speak(label, 10, "menu");
   strncpy(spoken_label, label, sizeof(spoken_label) - 1);
   spoken_label[sizeof(spoken_label) - 1] = '\0';
}

static void reset_dw2_dialog_tracking(void)
{
   dw2_dialog_active = false;
   dw2_rendered_dialog_text_ptr = 0;
   dw2_pending_dialog_text_ptr = 0;
   dw2_pending_dialog_frames = 0;
   dw2_spoken_dialog_text_ptr = 0;
   dw2_pending_dialog_choice_valid = false;
   dw2_pending_dialog_choice = 0;
   dw2_pending_dialog_choice_frames = 0;
   dw2_spoken_dialog_choice_valid = false;
   dw2_spoken_dialog_choice = 0;
   dw2_pending_dialog_text[0] = '\0';
   spoken_dialog_text[0] = '\0';
}

static void reset_dw2_name_entry_tracking(void)
{
   dw2_name_entry_probe_seen = false;
   dw2_name_entry_probe_state = 0;
   dw2_name_entry_selection_seen = false;
   dw2_name_entry_selection_state = 0;
   dw2_name_entry_selected = 0;
   dw2_name_entry_active = false;
   dw2_name_entry_inactive_frames = 0;
   dw2_pending_name_entry_state = 0;
   dw2_pending_name_entry_frames = 0;
   dw2_pending_name[0] = '\0';
   dw2_pending_name_focus[0] = '\0';
   dw2_pending_name_column = UINT16_MAX;
   dw2_pending_name_row = UINT16_MAX;
   dw2_spoken_name[0] = '\0';
   dw2_spoken_name_focus[0] = '\0';
   dw2_spoken_name_column = UINT16_MAX;
   dw2_spoken_name_row = UINT16_MAX;
}

static void note_dw2_name_entry_inactive_frame(void)
{
   if (!dw2_name_entry_active)
      return;

   if (dw2_name_entry_inactive_frames
         < DIGIMON_WORLD_2_NAME_ENTRY_INACTIVE_GRACE_FRAMES)
      dw2_name_entry_inactive_frames++;

   if (dw2_name_entry_inactive_frames
         >= DIGIMON_WORLD_2_NAME_ENTRY_INACTIVE_GRACE_FRAMES)
      reset_dw2_name_entry_tracking();
}

static bool append_dw2_text(char *out, size_t out_size, size_t *out_len,
      const char *text)
{
   if (!out || !out_size || !out_len || !text)
      return false;

   while (*text)
   {
      if (*out_len + 1 >= out_size)
         return false;

      out[*out_len] = *text;
      (*out_len)++;
      text++;
   }

   out[*out_len] = '\0';
   return true;
}

static bool append_dw2_char(char *out, size_t out_size, size_t *out_len,
      char value)
{
   char text[2];

   text[0] = value;
   text[1] = '\0';
   return append_dw2_text(out, out_size, out_len, text);
}

static bool append_dw2_encoded_char(char *out, size_t out_size,
      size_t *out_len, uint8_t value)
{
   if (value <= 9)
      return append_dw2_char(out, out_size, out_len, (char)('0' + value));

   if (value >= 0x0A && value <= 0x23)
      return append_dw2_char(out, out_size, out_len,
            (char)('A' + value - 0x0A));

   if (value >= 0x24 && value <= 0x3D)
      return append_dw2_char(out, out_size, out_len,
            (char)('a' + value - 0x24));

   switch (value)
   {
      case 0x42: return append_dw2_char(out, out_size, out_len, '&');
      case 0x44: return append_dw2_char(out, out_size, out_len, '?');
      case 0x45: return append_dw2_char(out, out_size, out_len, '!');
      case 0x46: return append_dw2_char(out, out_size, out_len, '/');
      case 0x49: return append_dw2_char(out, out_size, out_len, '-');
      case 0x54: return append_dw2_char(out, out_size, out_len, ',');
      case 0x55: return append_dw2_char(out, out_size, out_len, '.');
      case 0x56: return append_dw2_char(out, out_size, out_len, '\'');
      case 0x57: return append_dw2_char(out, out_size, out_len, '"');
      case 0x58: return append_dw2_char(out, out_size, out_len, ';');
      case 0x59: return append_dw2_char(out, out_size, out_len, ':');
      case 0x5A: return append_dw2_char(out, out_size, out_len, '%');
      case 0x5B: return append_dw2_char(out, out_size, out_len, '+');
      case 0x5D: return append_dw2_char(out, out_size, out_len, '#');
      case 0xFD: return append_dw2_char(out, out_size, out_len, ' ');
      default: return true;
   }
}

static bool dw2_address_to_main_ram_offset(uint32_t address,
      size_t ram_size, size_t *offset)
{
   uint32_t region = address & 0xffe00000u;

   if (region != 0x80000000u && region != 0xa0000000u)
      return false;

   *offset = address & DIGIMON_WORLD_2_PSX_RAM_MASK;
   return *offset < ram_size;
}

static bool dw2_main_ram_range_valid(uint32_t address, size_t length)
{
   size_t offset;

   return length > 0
      && dw2_main_ram
      && dw2_address_to_main_ram_offset(address, dw2_main_ram_size, &offset)
      && length <= dw2_main_ram_size - offset;
}

static void dw2_update_active_overlay(const uint8_t *main_ram,
      size_t ram_size)
{
   const beetle_dw2_overlay_signature_t *signatures;
   size_t signature_count = 0;
   size_t signature_index;

   dw2_active_overlay_tag = 0;
   signatures = beetle_accessibility_dw2_overlay_signatures(&signature_count);
   for (signature_index = 0; signature_index < signature_count;
         signature_index++)
   {
      const beetle_dw2_overlay_signature_t *signature =
         &signatures[signature_index];
      size_t offset;
      size_t word_index;
      bool matches = true;

      if (!dw2_address_to_main_ram_offset(signature->signature_address,
               ram_size, &offset)
            || BEETLE_DW2_OVERLAY_SIGNATURE_WORDS * 4u > ram_size - offset)
         continue;
      for (word_index = 0;
            word_index < BEETLE_DW2_OVERLAY_SIGNATURE_WORDS; word_index++)
      {
         if (read_u32le(main_ram, offset + word_index * 4u)
               != signature->signature_words[word_index])
         {
            matches = false;
            break;
         }
      }
      if (matches)
      {
         dw2_active_overlay_tag = signature->tag;
         return;
      }
   }
}

static uint32_t dw2_probe_register(const beetle_dw2_menu_probe_rule_t *rule,
      const uint32_t *gpr, uint8_t register_index)
{
   (void)rule;
   return register_index == BEETLE_DW2_MENU_GPR_NONE
      ? 0 : gpr[register_index];
}

static void dw2_reject_menu_probe(uint32_t pc, uint32_t overlay_tag,
      const char *reason)
{
   beetle_accessibility_trace_cpu_menu_reject(pc, overlay_tag, reason);
}

static void trim_dw2_text(char *text)
{
   char *read = text;
   char *write = text;
   char *last_non_space = text;
   bool previous_space = true;

   while (*read)
   {
      bool is_space = *read == ' ';

      if (!is_space || !previous_space)
      {
         *write++ = is_space ? ' ' : *read;
         if (!is_space)
            last_non_space = write;
      }

      previous_space = is_space;
      read++;
   }

   *last_non_space = '\0';
}

static bool decode_dw2_name_buffer(const uint8_t *main_ram, size_t ram_size,
      size_t name_offset, size_t max_name_length, char *out, size_t out_size)
{
   size_t i;
   size_t out_len = 0;
   bool terminated = false;

   if (!main_ram || !out || !out_size
         || name_offset >= ram_size || max_name_length == 0
         || max_name_length > DIGIMON_WORLD_2_NAME_ENTRY_MAX_NAME_BYTES)
      return false;

   out[0] = '\0';

   for (i = 0; i <= max_name_length && name_offset + i < ram_size; i++)
   {
      uint8_t value = main_ram[name_offset + i];

      if (value == 0xff)
      {
         terminated = true;
         break;
      }

      if (!append_dw2_encoded_char(out, out_size, &out_len, value))
         return false;
   }

   if (!terminated)
      return false;

   trim_dw2_text(out);
   return true;
}

static bool dw2_name_entry_buffer_offset(uint32_t type, uint32_t index,
      size_t ram_size, size_t *name_offset)
{
   size_t offset;

   if (!name_offset)
      return false;

   switch (type)
   {
      case 0:
         if (index > DIGIMON_WORLD_2_NAME_ENTRY_MAX_SOURCE_INDEX)
            return false;
         offset = DIGIMON_WORLD_2_SOURCE_NAME_OFFSET
            + (size_t)index * DIGIMON_WORLD_2_SOURCE_NAME_STRIDE;
         break;
      case 1:
         offset = DIGIMON_WORLD_2_PLAYER_NAME_OFFSET;
         break;
      case 2:
         offset = DIGIMON_WORLD_2_DIGI_BETTLE_NAME_OFFSET;
         break;
      default:
         return false;
   }

   if (offset >= ram_size)
      return false;

   *name_offset = offset;
   return true;
}

static bool dw2_name_entry_focus_label(uint16_t column, uint16_t row,
      bool selected_valid, uint8_t selected, char *out, size_t out_size)
{
   /* The game's selector at 0x8001287C splits columns 0-4, 5-9,
    * and 10 across on-disc resources 0x01FD00DA, 0x01FD00DB, and
    * 0x01FD00DC. These are the decoded visible cells from those resources. */
   static const uint8_t grid[8][10] = {
      { 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13 },
      { 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d },
      { 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0xfd, 0xfd, 0xfd, 0xfd },
      { 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d },
      { 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37 },
      { 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0xfd, 0xfd, 0xfd, 0xfd },
      { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09 },
      { 0x44, 0x45, 0x54, 0x55, 0x58, 0x59, 0xfd, 0xfd, 0xfd, 0xfd }
   };
   size_t out_len = 0;
   char decoded[2] = { '\0', '\0' };

   if (!out || !out_size)
      return false;

   out[0] = '\0';

   if (column == DIGIMON_WORLD_2_NAME_ENTRY_OK_COLUMN
         && row == DIGIMON_WORLD_2_NAME_ENTRY_OK_ROW)
      return append_dw2_text(out, out_size, &out_len, "OK");

   if (row < 8 && column < 10)
   {
      selected = grid[row][column];
      selected_valid = true;
   }
   else if (column == 10 && row < 4)
   {
      /* Resource 0x01FD00DC supplies an insertable space in these cells. */
      selected = 0xfd;
      selected_valid = true;
   }

   if (!selected_valid)
      return false;

   if (selected <= 9)
   {
      decoded[0] = (char)('0' + selected);
      return append_dw2_text(out, out_size, &out_len, decoded);
   }
   else if (selected >= 0x0a && selected <= 0x23)
   {
      decoded[0] = (char)('A' + selected - 0x0a);
      return append_dw2_text(out, out_size, &out_len, "Capital ")
         && append_dw2_text(out, out_size, &out_len, decoded);
   }
   else if (selected >= 0x24 && selected <= 0x3d)
   {
      decoded[0] = (char)('a' + selected - 0x24);
      return append_dw2_text(out, out_size, &out_len, "Lowercase ")
         && append_dw2_text(out, out_size, &out_len, decoded);
   }

   switch (selected)
   {
      case 0x3e: return append_dw2_text(out, out_size, &out_len, "Circle");
      case 0x3f: return append_dw2_text(out, out_size, &out_len, "Cross");
      case 0x40: return append_dw2_text(out, out_size, &out_len, "Triangle");
      case 0x41: return append_dw2_text(out, out_size, &out_len, "Square");
      case 0x42: return append_dw2_text(out, out_size, &out_len, "Ampersand");
      case 0x44: return append_dw2_text(out, out_size, &out_len, "Question mark");
      case 0x45: return append_dw2_text(out, out_size, &out_len, "Exclamation mark");
      case 0x46: return append_dw2_text(out, out_size, &out_len, "Slash");
      case 0x47: return append_dw2_text(out, out_size, &out_len, "Music note");
      case 0x49: return append_dw2_text(out, out_size, &out_len, "Hyphen");
      case 0x54: return append_dw2_text(out, out_size, &out_len, "Comma");
      case 0x55: return append_dw2_text(out, out_size, &out_len, "Period");
      case 0x56: return append_dw2_text(out, out_size, &out_len, "Apostrophe");
      case 0x57: return append_dw2_text(out, out_size, &out_len, "Quotation mark");
      case 0x58: return append_dw2_text(out, out_size, &out_len, "Semicolon");
      case 0x59: return append_dw2_text(out, out_size, &out_len, "Colon");
      case 0x5a: return append_dw2_text(out, out_size, &out_len, "Percent");
      case 0x5b: return append_dw2_text(out, out_size, &out_len, "Plus");
      case 0x5d: return append_dw2_text(out, out_size, &out_len, "Number sign");
      case 0xfd:
         return append_dw2_text(out, out_size, &out_len, "Space");
      default: return false;
   }
}

static void speak_dw2_name_entry_state(const char *name, const char *focus,
      bool activation, bool name_changed, bool focus_changed,
      bool name_full_at_ok)
{
   char message[224];
   size_t message_len = 0;

   message[0] = '\0';

   if (activation)
   {
      if (!append_dw2_text(message, sizeof(message), &message_len,
               "Name entry. Current name ")
            || !append_dw2_text(message, sizeof(message), &message_len,
               name[0] ? name : "blank"))
         return;

      if (focus[0]
            && (!append_dw2_text(message, sizeof(message), &message_len, ". ")
               || !append_dw2_text(message, sizeof(message), &message_len,
                  focus)))
         return;
   }
   else if (name_changed)
   {
      if (!append_dw2_text(message, sizeof(message), &message_len,
               "Current name ")
            || !append_dw2_text(message, sizeof(message), &message_len,
               name[0] ? name : "blank"))
         return;

      if (name_full_at_ok
            && !append_dw2_text(message, sizeof(message), &message_len,
               ". Name full. OK"))
         return;
      else if (focus_changed && focus[0]
            && (!append_dw2_text(message, sizeof(message), &message_len, ". ")
               || !append_dw2_text(message, sizeof(message), &message_len,
                  focus)))
         return;
   }
   else if (name_full_at_ok)
   {
      if (!append_dw2_text(message, sizeof(message), &message_len,
               "Name full. OK"))
         return;
   }
   else if (focus_changed && focus[0])
   {
      if (!append_dw2_text(message, sizeof(message), &message_len, focus))
         return;
   }

   if (message[0])
      beetle_accessibility_speak(message, 10, "name_entry");
}

static void poll_dw2_name_entry(const uint8_t *main_ram, size_t ram_size)
{
   uint32_t state = dw2_name_entry_probe_state;
   bool probe_seen = dw2_name_entry_probe_seen;
   bool selected_valid = dw2_name_entry_selection_seen
      && dw2_name_entry_selection_state == state;
   uint8_t selected = dw2_name_entry_selected;
   size_t state_offset;
   size_t name_offset;
   uint32_t type;
   uint32_t index;
   uint32_t max_name_length;
   uint32_t grid_resource_offset;
   uint32_t cursor;
   uint16_t column;
   uint16_t row;
   char name[DIGIMON_WORLD_2_NAME_ENTRY_MAX_NAME_BYTES + 1];
   char focus[32];
   bool activation;
   bool name_changed;
   bool focus_changed;
   bool name_full_at_ok;

   dw2_name_entry_probe_seen = false;
   dw2_name_entry_selection_seen = false;

   if (!probe_seen
         || !dw2_address_to_main_ram_offset(state, ram_size, &state_offset)
         || state_offset + DIGIMON_WORLD_2_NAME_ENTRY_STATE_MIN_BYTES
            > ram_size)
   {
      note_dw2_name_entry_inactive_frame();
      return;
   }

   type = read_u32le(main_ram, state_offset);
   index = read_u32le(main_ram, state_offset + 4);
   max_name_length = read_u32le(main_ram, state_offset + 8);
   grid_resource_offset = read_u32le(main_ram, state_offset + 0x0c);
   cursor = read_u32le(main_ram, state_offset + 0x24);
   column = read_u16le(main_ram, state_offset + 0x2c);
   row = read_u16le(main_ram, state_offset + 0x2e);

   if (max_name_length == 0
         || max_name_length > DIGIMON_WORLD_2_NAME_ENTRY_MAX_NAME_BYTES
         || grid_resource_offset
            != DIGIMON_WORLD_2_NAME_ENTRY_GRID_RESOURCE_OFFSET
         || cursor > max_name_length
         || column > DIGIMON_WORLD_2_NAME_ENTRY_MAX_COLUMN
         || row > DIGIMON_WORLD_2_NAME_ENTRY_MAX_ROW
         || !dw2_name_entry_buffer_offset(type, index, ram_size, &name_offset)
         || !decode_dw2_name_buffer(main_ram, ram_size, name_offset,
            max_name_length, name, sizeof(name)))
   {
      note_dw2_name_entry_inactive_frame();
      return;
   }

   dw2_name_entry_inactive_frames = 0;

   if (!dw2_name_entry_focus_label(column, row, selected_valid, selected,
            focus, sizeof(focus)))
      focus[0] = '\0';

   if (dw2_pending_name_entry_state != state
         || strcmp(dw2_pending_name, name)
         || strcmp(dw2_pending_name_focus, focus)
         || dw2_pending_name_column != column
         || dw2_pending_name_row != row)
   {
      dw2_pending_name_entry_state = state;
      dw2_pending_name_entry_frames = 1;
      strncpy(dw2_pending_name, name, sizeof(dw2_pending_name) - 1);
      dw2_pending_name[sizeof(dw2_pending_name) - 1] = '\0';
      strncpy(dw2_pending_name_focus, focus,
            sizeof(dw2_pending_name_focus) - 1);
      dw2_pending_name_focus[sizeof(dw2_pending_name_focus) - 1] = '\0';
      dw2_pending_name_column = column;
      dw2_pending_name_row = row;
      return;
   }

   if (dw2_pending_name_entry_frames
         < DIGIMON_WORLD_2_NAME_ENTRY_STABLE_FRAMES)
   {
      dw2_pending_name_entry_frames++;
      return;
   }

   activation = !dw2_name_entry_active;
   name_changed = strcmp(dw2_spoken_name, name) != 0;
   focus_changed = strcmp(dw2_spoken_name_focus, focus) != 0
      || dw2_spoken_name_column != column
      || dw2_spoken_name_row != row;
   name_full_at_ok = cursor >= max_name_length
      && column == DIGIMON_WORLD_2_NAME_ENTRY_OK_COLUMN
      && row == DIGIMON_WORLD_2_NAME_ENTRY_OK_ROW
      && focus_changed;

   speak_dw2_name_entry_state(name, focus, activation, name_changed,
         focus_changed, name_full_at_ok);

   dw2_name_entry_active = true;
   strncpy(dw2_spoken_name, name, sizeof(dw2_spoken_name) - 1);
   dw2_spoken_name[sizeof(dw2_spoken_name) - 1] = '\0';
   strncpy(dw2_spoken_name_focus, focus,
         sizeof(dw2_spoken_name_focus) - 1);
   dw2_spoken_name_focus[sizeof(dw2_spoken_name_focus) - 1] = '\0';
   dw2_spoken_name_column = column;
   dw2_spoken_name_row = row;
}

static bool decode_dw2_dialog_text(const uint8_t *main_ram, size_t ram_size,
      uint32_t text_ptr, size_t state_offset, char *out, size_t out_size,
      char *speaker, size_t speaker_size)
{
   uint32_t arguments[4];
   unsigned index;
   for (index = 0; index < 4; index++)
      arguments[index] = read_u32le(main_ram, state_offset + 0x0cu + index * 4u);
   return beetle_accessibility_dw2_decode_formatted_dialog_with_speaker(main_ram,
         ram_size, text_ptr, arguments, 4, out, out_size, speaker, speaker_size);
}

static bool append_dw2_dialog_choice(char *text, size_t text_size,
      const char *choice)
{
   size_t length;
   size_t out_len;

   if (!text || !choice || !*choice)
      return false;
   length = strlen(text);
   out_len = length;
   if (length && text[length - 1] != '.' && text[length - 1] != '!'
         && text[length - 1] != '?')
   {
      if (!append_dw2_text(text, text_size, &out_len, "."))
         return false;
   }
   if (!append_dw2_text(text, text_size, &out_len, " Selected ")
         || !append_dw2_text(text, text_size, &out_len, choice)
         || !append_dw2_text(text, text_size, &out_len, "."))
      return false;
   return true;
}

static bool dw2_dialog_has_live_renderer(const uint8_t *ram, size_t ram_size,
      size_t state_offset)
{
   uint32_t count;
   uint32_t index;

   /* Main renderer 8001a9c8 walks 50 slots at task +2c, stride 34. A slot
    * address captured in a previous scene can survive in recycled memory;
    * its allocation byte and decodable text alone do not establish visibility. */
   if (ram_size < DIGIMON_WORLD_2_TASK_COUNT_OFFSET + 4u)
      return false;
   count = read_u32le(ram, DIGIMON_WORLD_2_TASK_COUNT_OFFSET);
   if (!count || count > DIGIMON_WORLD_2_TASK_LIMIT
         || ram_size < DIGIMON_WORLD_2_TASK_LIST_OFFSET + count * 4u)
      return false;
   for (index = 0; index < count; index++)
   {
      size_t task_offset;
      size_t data_offset;
      size_t relative;
      uint32_t task_address = read_u32le(ram,
            DIGIMON_WORLD_2_TASK_LIST_OFFSET + index * 4u);
      if (!dw2_address_to_main_ram_offset(task_address, ram_size, &task_offset)
            || ram_size - task_offset < 0x30u
            || read_u32le(ram, task_offset) != DIGIMON_WORLD_2_RENDER_TASK_TYPE
            || read_u32le(ram, task_offset + 0x10u) != 1u
            || !dw2_address_to_main_ram_offset(
               read_u32le(ram, task_offset + 0x2cu), ram_size, &data_offset)
            || state_offset < data_offset)
         continue;
      relative = state_offset - data_offset;
      if (relative < DIGIMON_WORLD_2_RENDER_SLOT_COUNT
                  * DIGIMON_WORLD_2_RENDER_SLOT_SIZE
            && relative % DIGIMON_WORLD_2_RENDER_SLOT_SIZE == 0u
            && ram_size - state_offset >= DIGIMON_WORLD_2_RENDER_SLOT_SIZE)
         return true;
   }
   return false;
}

static void poll_dw2_dialog_text(const uint8_t *main_ram, size_t ram_size)
{
   size_t state_offset;
   uint32_t text_ptr;
   uint8_t choice = 0;
   bool has_choice = false;
   bool dialog_changed;
   char decoded_dialog_text[DIGIMON_WORLD_2_DIALOG_MAX_CHARS];
   char decoded_speaker[DIGIMON_WORLD_2_DIALOG_SPEAKER_MAX_CHARS];
   char choice_label[96];

   /* Specialized native menu readers already own their foreground prompts,
    * including F8 choices. Do not speak the shared renderer a second time. */
   if (beetle_accessibility_dw2_menu_blocks_dialog()
         || !dw2_address_to_main_ram_offset(dw2_dialog_state_address, ram_size,
            &state_offset)
         || state_offset + DIGIMON_WORLD_2_DIALOG_TEXT_POINTER_OFFSET + 4
            > ram_size)
   {
      reset_dw2_dialog_tracking();
      return;
   }

   if (main_ram[state_offset] != DIGIMON_WORLD_2_DIALOG_ACTIVE_VALUE
         || !dw2_dialog_has_live_renderer(main_ram, ram_size, state_offset))
   {
      reset_dw2_dialog_tracking();
      return;
   }

   /* A renderer slot can remain allocated after its visible text has closed.
    * Only successfully decoded live text suppresses navigation. */
   dw2_dialog_active = false;

   text_ptr = read_u32le(main_ram, state_offset
         + DIGIMON_WORLD_2_DIALOG_TEXT_POINTER_OFFSET);

   /* A slot is loaded before actor movement, window opening and typewriter
    * rendering. Only the native FB/F8 input-wait boundary establishes that
    * this particular page has actually been displayed. FC may have advanced
    * the slot since the CPU observation, so compare the captured page too. */
   if (!dw2_rendered_dialog_text_ptr
         || text_ptr != dw2_rendered_dialog_text_ptr)
   {
      reset_dw2_dialog_tracking();
      return;
   }

   if (!decode_dw2_dialog_text(main_ram, ram_size, text_ptr, state_offset,
            decoded_dialog_text, sizeof(decoded_dialog_text),
            decoded_speaker, sizeof(decoded_speaker)))
   {
      dw2_pending_dialog_text_ptr = 0;
      dw2_pending_dialog_frames = 0;
      dw2_pending_dialog_choice_valid = false;
      dw2_pending_dialog_choice_frames = 0;
      dw2_pending_dialog_text[0] = '\0';
      return;
   }

   dw2_dialog_active = true;

   if (beetle_accessibility_dw2_text_is_yes_no_prompt(main_ram, ram_size,
            text_ptr))
   {
      if (state_offset + DIGIMON_WORLD_2_DIALOG_CHOICE_OFFSET >= ram_size
            || main_ram[state_offset + DIGIMON_WORLD_2_DIALOG_CHOICE_OFFSET]
               > 1u)
      {
         dw2_pending_dialog_text_ptr = 0;
         dw2_pending_dialog_frames = 0;
         dw2_pending_dialog_choice_valid = false;
         dw2_pending_dialog_choice_frames = 0;
         dw2_pending_dialog_text[0] = '\0';
         return;
      }
      choice = main_ram[state_offset + DIGIMON_WORLD_2_DIALOG_CHOICE_OFFSET];
      has_choice = beetle_accessibility_dw2_decode_choice(main_ram,
            ram_size, text_ptr, choice, choice_label, sizeof(choice_label));
   }

   if (dw2_pending_dialog_text_ptr != text_ptr
         || strcmp(dw2_pending_dialog_text, decoded_dialog_text))
   {
      dw2_pending_dialog_text_ptr = text_ptr;
      dw2_pending_dialog_frames = 1;
      dw2_pending_dialog_choice_valid = false;
      dw2_pending_dialog_choice_frames = 0;
      strncpy(dw2_pending_dialog_text, decoded_dialog_text,
            sizeof(dw2_pending_dialog_text) - 1);
      dw2_pending_dialog_text[sizeof(dw2_pending_dialog_text) - 1] = '\0';
      return;
   }

   if (dw2_pending_dialog_frames < DIGIMON_WORLD_2_DIALOG_STABLE_FRAMES)
   {
      dw2_pending_dialog_frames++;
      return;
   }

   dialog_changed = dw2_spoken_dialog_text_ptr != text_ptr
      || strcmp(spoken_dialog_text, decoded_dialog_text);
   if (dialog_changed)
   {
      char initial_speech[DIGIMON_WORLD_2_DIALOG_MAX_CHARS];

      strncpy(initial_speech, decoded_dialog_text, sizeof(initial_speech) - 1);
      initial_speech[sizeof(initial_speech) - 1] = '\0';
      if (has_choice && !append_dw2_dialog_choice(initial_speech,
               sizeof(initial_speech), choice_label))
         return;
      if (decoded_speaker[0])
         beetle_accessibility_dw2_navigation_observe_speaker(
               decoded_speaker);
      beetle_accessibility_dw2_story_observe_dialogue(main_ram,ram_size,
            decoded_dialog_text);
      beetle_accessibility_speak(initial_speech, 10, "dialog");
      dw2_spoken_dialog_text_ptr = text_ptr;
      strncpy(spoken_dialog_text, decoded_dialog_text,
            sizeof(spoken_dialog_text) - 1);
      spoken_dialog_text[sizeof(spoken_dialog_text) - 1] = '\0';
      dw2_spoken_dialog_choice_valid = has_choice;
      dw2_spoken_dialog_choice = choice;
      dw2_pending_dialog_choice_valid = false;
      dw2_pending_dialog_choice_frames = 0;
      return;
   }

   if (!has_choice)
   {
      dw2_spoken_dialog_choice_valid = false;
      dw2_pending_dialog_choice_valid = false;
      dw2_pending_dialog_choice_frames = 0;
      return;
   }
   if (dw2_spoken_dialog_choice_valid
         && dw2_spoken_dialog_choice == choice)
   {
      dw2_pending_dialog_choice_valid = false;
      dw2_pending_dialog_choice_frames = 0;
      return;
   }
   if (!dw2_pending_dialog_choice_valid
         || dw2_pending_dialog_choice != choice)
   {
      dw2_pending_dialog_choice_valid = true;
      dw2_pending_dialog_choice = choice;
      dw2_pending_dialog_choice_frames = 1;
      return;
   }
   if (dw2_pending_dialog_choice_frames
         < DIGIMON_WORLD_2_DIALOG_STABLE_FRAMES)
   {
      dw2_pending_dialog_choice_frames++;
      return;
   }

   {
      char choice_speech[128];
      snprintf(choice_speech, sizeof(choice_speech), "Selected %s.", choice_label);
      beetle_accessibility_speak(choice_speech, 10, "dialog");
   }
   dw2_spoken_dialog_choice_valid = true;
   dw2_spoken_dialog_choice = choice;
   dw2_pending_dialog_choice_valid = false;
   dw2_pending_dialog_choice_frames = 0;
}

static void reset_dw2_title_choice_session(void)
{
   dw2_start_pressed = false;
   dw2_press_start_ready = true;
   pending_signature = 0;
   pending_frames = 0;
   dw2_pending_title_selection = 0xff;
   dw2_pending_title_selection_frames = 0;
   dw2_invalid_title_selection_frames = 0;
   spoken_label[0] = '\0';
   dw2_dialog_state_address = DIGIMON_WORLD_2_DIALOG_INITIAL_STATE_ADDRESS;
   dw2_main_ram = NULL;
   dw2_main_ram_size = 0;
   dw2_active_overlay_tag = 0;
   dw2_captured_menu_probe_count = 0;
   beetle_accessibility_dw2_menu_reset();
   beetle_accessibility_dw2_navigation_reset();
   reset_dw2_dialog_tracking();
   reset_dw2_name_entry_tracking();
}

void beetle_accessibility_game_set_serial(const char *serial)
{
   is_digimon_world_2 = serial && !strncmp(serial, "SLUS_011.93", 11);
   beetle_accessibility_game_reset();
}

void beetle_accessibility_game_reset(void)
{
   dw2_frame_enabled      = true;
   dw2_start_pressed      = false;
   dw2_start_was_down     = false;
   dw2_press_start_ready  = false;
   dw2_press_start_spoken = false;
   dw2_title_selection_state_verified = false;
   dw2_title_seen         = false;
   dw2_last_movie_cue_sector = 0;
   pending_signature      = 0;
   pending_frames         = 0;
   dw2_pending_title_selection = 0xff;
   dw2_pending_title_selection_frames = 0;
   dw2_invalid_title_selection_frames = 0;
   spoken_label[0]        = '\0';
   dw2_dialog_state_address = DIGIMON_WORLD_2_DIALOG_INITIAL_STATE_ADDRESS;
   dw2_main_ram = NULL;
   dw2_main_ram_size = 0;
   dw2_active_overlay_tag = 0;
   dw2_captured_menu_probe_count = 0;
   memset(dw2_navigation_keys_down, 0,
         sizeof(dw2_navigation_keys_down));
   memset(&dw2_controller, 0, sizeof(dw2_controller));
   beetle_accessibility_dw2_menu_reset();
   beetle_accessibility_dw2_navigation_reset();
   reset_dw2_dialog_tracking();
   reset_dw2_name_entry_tracking();
}

void beetle_accessibility_game_state_discontinuity(void)
{
   beetle_accessibility_game_reset();
}

void beetle_accessibility_game_set_frame_enabled(bool enabled)
{
   dw2_frame_enabled = enabled;
   if (!enabled)
      dw2_captured_menu_probe_count = 0;
}

void beetle_accessibility_game_set_controller_navigation(bool enabled)
{
   if (dw2_controller_navigation_enabled != enabled)
      memset(&dw2_controller, 0, sizeof(dw2_controller));
   dw2_controller_navigation_enabled = enabled;
}

static void dw2_controller_input(retro_input_state_t input_state_cb)
{
   /* Physical positions match the DS/VBA-M layer: Y is the LEFT face
    * button (Square / Xbox X), X is the TOP face button (Triangle / Xbox Y). */
   static const struct
   {
      unsigned button;
      beetle_dw2_navigation_command_t command;
   } bindings[] = {
      { RETRO_DEVICE_ID_JOYPAD_LEFT,  BEETLE_DW2_NAV_COMMAND_PREVIOUS_CATEGORY },
      { RETRO_DEVICE_ID_JOYPAD_RIGHT, BEETLE_DW2_NAV_COMMAND_NEXT_CATEGORY },
      { RETRO_DEVICE_ID_JOYPAD_UP,    BEETLE_DW2_NAV_COMMAND_PREVIOUS_TARGET },
      { RETRO_DEVICE_ID_JOYPAD_DOWN,  BEETLE_DW2_NAV_COMMAND_NEXT_TARGET },
      { RETRO_DEVICE_ID_JOYPAD_Y,     BEETLE_DW2_NAV_COMMAND_REPEAT },
      { RETRO_DEVICE_ID_JOYPAD_L3,    BEETLE_DW2_NAV_COMMAND_START_ROUTE },
      { RETRO_DEVICE_ID_JOYPAD_X,     BEETLE_DW2_NAV_COMMAND_LOCATION }
   };
   uint16_t buttons;
   uint16_t held;
   uint16_t pressed;
   uint8_t held_axes = 0;
   unsigned i;
   bool active;

   if (!input_state_cb || !is_digimon_world_2
         || !dw2_controller_navigation_enabled)
   {
      memset(&dw2_controller, 0, sizeof(dw2_controller));
      return;
   }

   /* Individual IDs work without negotiating GET_INPUT_BITMASKS. The game's
    * input_update still uses its negotiated mask/per-button path below. */
   buttons = 0;
   for (i = 0; i < 16; i++)
      if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i))
         buttons |= (uint16_t)(1u << i);
   active = (buttons & (1u << RETRO_DEVICE_ID_JOYPAD_L2)) != 0;

   /* Pressure-sensitive buttons must be released too, even below the
    * frontend's digital threshold. No pressure value is used as a command. */
   held = buttons;
   for (i = 0; i < 16; i++)
      if (input_state_cb(0, RETRO_DEVICE_ANALOG,
               RETRO_DEVICE_INDEX_ANALOG_BUTTON, i) > 0)
         held |= (uint16_t)(1u << i);
   for (i = 0; i < 4; i++)
   {
      int value = input_state_cb(0, RETRO_DEVICE_ANALOG, i / 2, i % 2);
      /* A small neutral zone lets a real stick release despite center drift.
       * This is only a release threshold, not a new gameplay dead zone. */
      if (value < -4096 || value > 4096)
         held_axes |= (uint8_t)(1u << i);
   }

   dw2_controller.blocked = active ? 0xffffu : dw2_controller.latched & held;
   dw2_controller.blocked_axes = active ? 0x0fu
      : dw2_controller.latched_axes & held_axes;

   /* Input ownership follows physical samples even in a secondary runahead
    * core, which may never run an audio-enabled frame. Otherwise its video
    * would receive a still-held direction as soon as L2 was released. */
   dw2_controller.latched = active ? held : dw2_controller.blocked;
   dw2_controller.latched_axes = active ? held_axes : dw2_controller.blocked_axes;
   /* Speculation must not consume command edges or age repeat timers. */
   if (!dw2_frame_enabled)
      return;
   pressed = buttons & ~dw2_controller.previous;
   dw2_controller.previous = active ? buttons : 0;

   for (i = 0; i < sizeof(bindings) / sizeof(bindings[0]); i++)
   {
      uint16_t bit = (uint16_t)(1u << bindings[i].button);
      bool fire = active && (pressed & bit);

      if (i < 4)
      {
         if (!active || !(buttons & bit))
            dw2_controller.direction_frames[i] = 0;
         else if (++dw2_controller.direction_frames[i] == 32)
         {
            /* Same 26-frame delay / 6-frame repeat cadence as VBA-M. */
            dw2_controller.direction_frames[i] = 26;
            fire = true;
         }
      }
      if (fire)
         beetle_accessibility_dw2_navigation_command(bindings[i].command);
   }
}

int16_t beetle_accessibility_game_filter_input(retro_input_state_t input_state_cb,
      unsigned port, unsigned device, unsigned index, unsigned id)
{
   if (!input_state_cb)
      return 0;
   if (is_digimon_world_2 && dw2_controller_navigation_enabled && port == 0)
   {
      if (device == RETRO_DEVICE_JOYPAD && index == 0)
      {
         if (id == RETRO_DEVICE_ID_JOYPAD_MASK)
            return (int16_t)((uint16_t)input_state_cb(port, device, index, id)
                  & ~dw2_controller.blocked);
         if (id < 16 && (dw2_controller.blocked & (1u << id)))
            return 0;
      }
      else if (device == RETRO_DEVICE_ANALOG)
      {
         if (index == RETRO_DEVICE_INDEX_ANALOG_BUTTON && id < 16
               && (dw2_controller.blocked & (1u << id)))
            return 0;
         if (index < 2 && id < 2
               && (dw2_controller.blocked_axes & (1u << (index * 2 + id))))
            return 0;
      }
   }
   return input_state_cb(port, device, index, id);
}

void beetle_accessibility_game_input(retro_input_state_t input_state_cb)
{
   static const unsigned navigation_keys[] = {
      RETROK_HOME,
      RETROK_END,
      RETROK_PAGEUP,
      RETROK_PAGEDOWN,
      RETROK_DELETE,
      RETROK_KP_ENTER
   };
   bool start_down;
   size_t navigation_key_index;

   dw2_controller_input(input_state_cb);
   if (!is_digimon_world_2 || !dw2_frame_enabled || !input_state_cb)
      return;

   start_down = beetle_accessibility_game_filter_input(input_state_cb,
         0, RETRO_DEVICE_JOYPAD, 0,
         RETRO_DEVICE_ID_JOYPAD_START) != 0;

   /* Poll the frontend's raw keyboard state on the same frame boundary as
    * RetroPad input.  Some Windows input drivers do not deliver the optional
    * keyboard event callback consistently, while RETRO_DEVICE_KEYBOARD is the
    * libretro input-state contract available to every core frame. */
   for (navigation_key_index = 0;
         navigation_key_index < sizeof(navigation_keys)
            / sizeof(navigation_keys[0]); navigation_key_index++)
   {
      unsigned keycode = navigation_keys[navigation_key_index];

      beetle_accessibility_game_keyboard_event(
            input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, keycode) != 0,
            keycode);
   }

   if (start_down && !dw2_start_was_down && dw2_press_start_ready
         && !dw2_start_pressed)
   {
      dw2_start_pressed = true;
      dw2_press_start_ready = false;
      pending_signature = 0;
      pending_frames    = 0;
      dw2_pending_title_selection = 0xff;
      dw2_pending_title_selection_frames = 0;
      dw2_invalid_title_selection_frames = 0;
      spoken_label[0]   = '\0';
      dw2_dialog_state_address =
         DIGIMON_WORLD_2_DIALOG_INITIAL_STATE_ADDRESS;
      reset_dw2_dialog_tracking();
      reset_dw2_name_entry_tracking();
   }

   dw2_start_was_down = start_down;
}

void beetle_accessibility_game_keyboard_event(bool down, unsigned keycode)
{
   beetle_dw2_navigation_command_t command;
   unsigned key_index;

   if (!is_digimon_world_2)
      return;

   switch (keycode)
   {
      case RETROK_HOME:
         key_index = 0;
         command = BEETLE_DW2_NAV_COMMAND_PREVIOUS_CATEGORY;
         break;
      case RETROK_END:
         key_index = 1;
         command = BEETLE_DW2_NAV_COMMAND_NEXT_CATEGORY;
         break;
      case RETROK_PAGEUP:
         key_index = 2;
         command = BEETLE_DW2_NAV_COMMAND_PREVIOUS_TARGET;
         break;
      case RETROK_PAGEDOWN:
         key_index = 3;
         command = BEETLE_DW2_NAV_COMMAND_NEXT_TARGET;
         break;
      case RETROK_DELETE:
         key_index = 4;
         command = BEETLE_DW2_NAV_COMMAND_REPEAT;
         break;
      case RETROK_KP_ENTER:
         key_index = 5;
         command = BEETLE_DW2_NAV_COMMAND_START_ROUTE;
         break;
      default:
         return;
   }

   if (!down)
   {
      if (dw2_navigation_keys_down[key_index])
         beetle_accessibility_trace_navigation_key(keycode, false,
               dw2_active_overlay_tag);
      dw2_navigation_keys_down[key_index] = false;
      return;
   }
   if (!dw2_frame_enabled)
      return;
   if (dw2_navigation_keys_down[key_index])
      return;
   dw2_navigation_keys_down[key_index] = true;
   beetle_accessibility_trace_navigation_key(keycode, true,
         dw2_active_overlay_tag);
   beetle_accessibility_dw2_navigation_command(command);
}

void beetle_accessibility_game_disc_sector(uint32_t disc_sector)
{
   const char *cue;

   if (!is_digimon_world_2 || !dw2_frame_enabled || dw2_title_seen)
      return;

   if (disc_sector == dw2_last_movie_cue_sector)
      return;

   cue = beetle_accessibility_dw2_movie_cue_lookup(disc_sector);
   if (!cue)
      return;

   dw2_last_movie_cue_sector = disc_sector;
   beetle_accessibility_speak(cue, 5, "movie");
}

void beetle_accessibility_game_cpu_dialog_state(uint32_t state)
{
   size_t offset;

   if (!is_digimon_world_2 || !dw2_frame_enabled || !dw2_main_ram
         || !dw2_address_to_main_ram_offset(state, dw2_main_ram_size, &offset)
         || dw2_main_ram_size - offset
            < DIGIMON_WORLD_2_DIALOG_TEXT_POINTER_OFFSET + 4u)
      return;

   if (dw2_dialog_state_address != state)
      reset_dw2_dialog_tracking();
   dw2_dialog_state_address = state;
   dw2_rendered_dialog_text_ptr = read_u32le(dw2_main_ram,
         offset + DIGIMON_WORLD_2_DIALOG_TEXT_POINTER_OFFSET);
}

void beetle_accessibility_game_cpu_name_entry_active(uint32_t state)
{
   if (!is_digimon_world_2 || !dw2_frame_enabled)
      return;

   dw2_name_entry_probe_seen = true;
   dw2_name_entry_probe_state = state;
}

void beetle_accessibility_game_cpu_name_entry_selection(uint32_t state,
      uint8_t selected)
{
   if (!is_digimon_world_2 || !dw2_frame_enabled)
      return;

   dw2_name_entry_selection_seen = true;
   dw2_name_entry_selection_state = state;
   dw2_name_entry_selected = selected;
}

uint32_t beetle_accessibility_game_dw2_overlay_tag_for_pc(uint32_t pc)
{
   const beetle_dw2_overlay_signature_t *signatures;
   size_t signature_count = 0;
   size_t signature_index;

   if (!is_digimon_world_2 || !dw2_active_overlay_tag)
      return 0;
   signatures = beetle_accessibility_dw2_overlay_signatures(&signature_count);
   for (signature_index = 0; signature_index < signature_count;
         signature_index++)
   {
      const beetle_dw2_overlay_signature_t *signature =
         &signatures[signature_index];
      if (signature->tag == dw2_active_overlay_tag
            && pc >= signature->load_base
            && pc - signature->load_base < signature->size)
         return signature->tag;
   }
   return 0;
}

void beetle_accessibility_game_cpu_menu_probe(uint32_t pc,
      const uint32_t *gpr, size_t gpr_count)
{
   const beetle_dw2_menu_probe_rule_t *rule;
   beetle_dw2_menu_probe_t event;
   uint32_t overlay_tag;

   if (!is_digimon_world_2 || !dw2_frame_enabled || !gpr)
      return;
   overlay_tag = beetle_accessibility_game_dw2_overlay_tag_for_pc(pc);
   rule = beetle_accessibility_dw2_menu_rule_lookup(pc, overlay_tag);
   if (!rule)
   {
      dw2_reject_menu_probe(pc, overlay_tag, "overlay_mismatch");
      return;
   }
   if (gpr_count < 32)
   {
      dw2_reject_menu_probe(pc, overlay_tag, "invalid_task");
      return;
   }

   memset(&event, 0, sizeof(event));
   event.pc = pc;
   event.overlay_tag = overlay_tag;
   event.task = dw2_probe_register(rule, gpr, rule->task_reg);
   event.focus = dw2_probe_register(rule, gpr, rule->focus_reg);
   event.label_ref = dw2_probe_register(rule, gpr, rule->label_reg);
   event.detail_ref = dw2_probe_register(rule, gpr, rule->detail_reg);
   event.flags = rule->flags;
   event.context = rule->context;
   event.adapter = rule->adapter;

   if ((rule->flags & BEETLE_DW2_MENU_RULE_TASK_POINTER)
         && !dw2_main_ram_range_valid(event.task, 1))
   {
      dw2_reject_menu_probe(pc, overlay_tag, "invalid_task");
      return;
   }
   if (rule->focus_reg != BEETLE_DW2_MENU_GPR_NONE
         && (event.focus < rule->focus_min || event.focus > rule->focus_max))
   {
      dw2_reject_menu_probe(pc, overlay_tag, "invalid_focus");
      return;
   }
   if ((rule->flags & BEETLE_DW2_MENU_RULE_LABEL_TEXT_POINTER)
         && !dw2_main_ram_range_valid(event.label_ref, 1))
   {
      dw2_reject_menu_probe(pc, overlay_tag, "invalid_label_ref");
      return;
   }
   if ((rule->flags & BEETLE_DW2_MENU_RULE_DETAIL_TEXT_POINTER)
         && !dw2_main_ram_range_valid(event.detail_ref, 1))
   {
      dw2_reject_menu_probe(pc, overlay_tag, "invalid_label_ref");
      return;
   }
   if (dw2_captured_menu_probe_count >= DIGIMON_WORLD_2_MENU_PROBE_CAPACITY)
   {
      dw2_reject_menu_probe(pc, overlay_tag, "stale_probe");
      return;
   }

   dw2_captured_menu_probes[dw2_captured_menu_probe_count++] = event;
   beetle_accessibility_trace_cpu_menu_probe(event.pc, event.overlay_tag,
         event.task, event.focus, event.label_ref, event.detail_ref,
         event.flags);
}

void beetle_accessibility_game_frame(const uint8_t *main_ram, size_t ram_size)
{
   uint32_t previous_overlay_tag;
   size_t menu_probe_index;
   uint16_t signature;
   uint8_t selection;
   const char *label;
   const char *selection_label;

   if (!is_digimon_world_2 || !dw2_frame_enabled || !main_ram
         || ram_size <= DIGIMON_WORLD_2_TITLE_MENU_OFFSET + 1)
      return;

   dw2_main_ram = main_ram;
   dw2_main_ram_size = ram_size;
   previous_overlay_tag = dw2_active_overlay_tag;
   dw2_update_active_overlay(main_ram, ram_size);
   if (previous_overlay_tag != dw2_active_overlay_tag)
      beetle_accessibility_dw2_menu_reset();
   beetle_accessibility_dw2_menu_begin_frame();
   if (previous_overlay_tag == dw2_active_overlay_tag)
   {
      for (menu_probe_index = 0;
            menu_probe_index < dw2_captured_menu_probe_count;
            menu_probe_index++)
         beetle_accessibility_dw2_menu_observe_probe(main_ram, ram_size,
               &dw2_captured_menu_probes[menu_probe_index]);
   }
   beetle_accessibility_dw2_menu_poll_native(main_ram, ram_size,
         dw2_active_overlay_tag);
   beetle_accessibility_dw2_domain_menu_poll(main_ram, ram_size,
         dw2_active_overlay_tag);
   beetle_accessibility_dw2_menu_end_frame();
   dw2_captured_menu_probe_count = 0;

   /* The native command controller is the only source of battle-menu state.
    * Both gates below read its result directly; there is no global-derived
    * fallback that can report a menu the game is not showing. */
   beetle_accessibility_dw2_battle_frame(main_ram, ram_size,
         dw2_active_overlay_tag,
         beetle_accessibility_dw2_menu_battle_active());
   poll_dw2_dialog_text(main_ram, ram_size);
   poll_dw2_name_entry(main_ram, ram_size);
   beetle_accessibility_dw2_navigation_frame(main_ram, ram_size,
         dw2_active_overlay_tag,
         beetle_accessibility_dw2_menu_active()
            || beetle_accessibility_dw2_menu_battle_active()
            || dw2_active_overlay_tag == DIGIMON_WORLD_2_BATTLE_OVERLAY_TAG
            || dw2_dialog_active
            || dw2_name_entry_active);

   /* These title-menu bytes are reused by later scenes. Only STAG1000
    * owns them; neither a stable value nor an old Start press proves that
    * the title is still showing. Clear only title state on departure so
    * gameplay menus, dialogue and navigation continue normally. */
   if (dw2_active_overlay_tag != DIGIMON_WORLD_2_TITLE_OVERLAY_TAG)
   {
      dw2_start_pressed = false;
      dw2_press_start_ready = false;
      dw2_press_start_spoken = false;
      dw2_title_selection_state_verified = false;
      pending_signature = 0;
      pending_frames = 0;
      dw2_pending_title_selection = 0xff;
      dw2_pending_title_selection_frames = 0;
      dw2_invalid_title_selection_frames = 0;
      spoken_label[0] = '\0';
      return;
   }

   signature = read_u16le(main_ram, DIGIMON_WORLD_2_TITLE_MENU_OFFSET);
   label     = beetle_accessibility_dw2_title_lookup(signature,
         dw2_start_pressed);

   if (dw2_start_pressed)
   {
      if (ram_size <= DIGIMON_WORLD_2_TITLE_SELECTION_OFFSET)
         return;

      selection = main_ram[DIGIMON_WORLD_2_TITLE_SELECTION_OFFSET];
      selection_label = beetle_accessibility_dw2_title_selection_lookup(selection);
      if (!selection_label)
      {
         dw2_pending_title_selection = 0xff;
         dw2_pending_title_selection_frames = 0;
         if (dw2_title_selection_state_verified
               && signature == DIGIMON_WORLD_2_TITLE_MENU_SIG_NEW_GAME)
         {
            if (dw2_invalid_title_selection_frames
                  < DIGIMON_WORLD_2_TITLE_SELECTION_INVALID_REARM_FRAMES)
            {
               dw2_invalid_title_selection_frames++;
               return;
            }

            reset_dw2_title_choice_session();
         }
         return;
      }

      dw2_invalid_title_selection_frames = 0;

      if (dw2_pending_title_selection != selection)
      {
         dw2_pending_title_selection = selection;
         dw2_pending_title_selection_frames = 1;
         return;
      }

      if (dw2_pending_title_selection_frames
            < DIGIMON_WORLD_2_TITLE_SELECTION_STABLE_FRAMES)
      {
         dw2_pending_title_selection_frames++;
         return;
      }

      dw2_title_selection_state_verified = true;
      speak_dw2_title_label(selection_label);
      return;
   }

   if (!label)
      return;

   dw2_title_seen = true;

   if (!dw2_start_pressed
         && signature == DIGIMON_WORLD_2_TITLE_MENU_SIG_NEW_GAME)
      dw2_press_start_ready = true;

   if (pending_signature != signature)
   {
      pending_signature = signature;
      pending_frames    = 1;
      return;
   }

   if (pending_frames < 3)
   {
      pending_frames++;
      return;
   }

   if (!dw2_start_pressed
         && signature == DIGIMON_WORLD_2_TITLE_MENU_SIG_NEW_GAME)
   {
      if (dw2_press_start_spoken)
         return;

      dw2_press_start_spoken = true;
      speak_dw2_title_label(label);
      return;
   }

   speak_dw2_title_label(label);
}
