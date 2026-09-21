#include "accessibility_trace.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <direct.h>
#define BEETLE_MKDIR(path) _mkdir(path)
#define BEETLE_PATH_SEP '\\'
#else
#include <sys/stat.h>
#define BEETLE_MKDIR(path) mkdir(path, 0755)
#define BEETLE_PATH_SEP '/'
#endif

#define TRACE_DIR_ENV "BEETLE_PSX_ACCESSIBILITY_TRACE_DIR"
#define TRACE_DEEP_ENV "BEETLE_PSX_ACCESSIBILITY_DEEP_TRACE"
#define TRACE_SUBDIR "accessibility-traces"
#define DW2_TRACE_SERIAL "SLUS_011.93"
#define DW2_TITLE_MENU_OFFSET 0x062A48
#define DW2_TITLE_CURSOR_OFFSET 0x062575
#define DW2_TITLE_WINDOW_OFFSET 0x062540
#define DW2_TITLE_WINDOW_LENGTH 0x60
#define DW2_TITLE_DISPLAY_LIST_OFFSET 0x062A28
#define DW2_TITLE_DISPLAY_LIST_LENGTH 0x98
#define DW2_BATTLE_MENU_CURSOR_OFFSET 0x0737E0
#define DW2_BATTLE_MENU_CURSOR_LENGTH 0x04
#define DW2_BATTLE_STATE_BLOCK_OFFSET 0x073CC0
#define DW2_BATTLE_STATE_BLOCK_LENGTH 0x20
#define DW2_MAIN_DIALOG_X_ADVANCE_PC 0x8001ACD4
#define DW2_DIALOG_PROBE_INTERVAL_FRAMES 6
#define DW2_DIALOG_PROBE_BYTES 0x40
#define DW2_NAME_ENTRY_SELECTION_RETURN_PC 0x80012914
#define DW2_NAME_ENTRY_PROBE_INTERVAL_FRAMES 6
#define DW2_NAME_ENTRY_STATE_BYTES 0x40
#define DW2_PLAYER_NAME_OFFSET 0x05E634
#define DW2_DIGI_BETTLE_NAME_OFFSET 0x05E6F1
#define DW2_NAME_ENTRY_SOURCE_OFFSET 0x05E750
#define DW2_NAME_BUFFER_TRACE_BYTES 0x10
#define DW2_MENU_PROBE_INTERVAL_FRAMES 6
#define DW2_RAM_DELTA_FRAMES 12
#define DW2_RAM_DELTA_MAX_PER_FRAME 160
#define DW2_RAM_SMALL_DELTA_MAX_PER_FRAME 4096
#define PSX_MAIN_RAM_MASK 0x1fffff
#define PSX_BOOT_SERIAL_LENGTH 11

struct trace_button
{
   unsigned id;
   const char *name;
   uint16_t bit;
};

static const struct trace_button trace_buttons[] = {
   { RETRO_DEVICE_ID_JOYPAD_UP,     "up",     1u << 0  },
   { RETRO_DEVICE_ID_JOYPAD_DOWN,   "down",   1u << 1  },
   { RETRO_DEVICE_ID_JOYPAD_LEFT,   "left",   1u << 2  },
   { RETRO_DEVICE_ID_JOYPAD_RIGHT,  "right",  1u << 3  },
   { RETRO_DEVICE_ID_JOYPAD_START,  "start",  1u << 4  },
   { RETRO_DEVICE_ID_JOYPAD_SELECT, "select", 1u << 5  },
   { RETRO_DEVICE_ID_JOYPAD_A,      "a",      1u << 6  },
   { RETRO_DEVICE_ID_JOYPAD_B,      "b",      1u << 7  },
   { RETRO_DEVICE_ID_JOYPAD_X,      "x",      1u << 8  },
   { RETRO_DEVICE_ID_JOYPAD_Y,      "y",      1u << 9  },
   { RETRO_DEVICE_ID_JOYPAD_L,      "l1",     1u << 10 },
   { RETRO_DEVICE_ID_JOYPAD_R,      "r1",     1u << 11 },
   { RETRO_DEVICE_ID_JOYPAD_L2,     "l2",     1u << 12 },
   { RETRO_DEVICE_ID_JOYPAD_R2,     "r2",     1u << 13 },
};

static FILE *trace_file;
static char trace_dir[4096];
static char trace_path[4096];
static char trace_serial[32] = "unknown";
static uint64_t trace_frame_index;
static bool trace_deep_enabled;
static uint16_t last_input_mask;
static uint16_t last_dw2_title_menu = 0xffff;
static uint8_t last_dw2_title_cursor = 0xff;
static bool log_dw2_title_window_on_next_frame;
static uint8_t *trace_ram_snapshot;
static size_t trace_ram_snapshot_size;
static unsigned trace_ram_delta_frames_remaining;
static uint32_t last_ram_hash;
static uint32_t last_video_hash;
static uint32_t last_audio_hash;
static bool last_dw2_dialog_probe_valid;
static uint64_t last_dw2_dialog_probe_frame;
static uint32_t last_dw2_dialog_probe_s1;
static uint32_t last_dw2_dialog_probe_s2;
static uint32_t last_dw2_dialog_probe_s6;
static uint32_t last_dw2_dialog_probe_sp;
static uint32_t last_dw2_dialog_probe_fp;
static bool last_dw2_name_entry_probe_valid;
static uint64_t last_dw2_name_entry_probe_frame;
static uint32_t last_dw2_name_entry_probe_state;
static uint32_t last_dw2_name_entry_probe_selected;
static uint32_t last_dw2_name_entry_probe_hash;
static bool last_dw2_menu_probe_valid;
static uint64_t last_dw2_menu_probe_frame;
static uint32_t last_dw2_menu_probe_pc;
static uint32_t last_dw2_menu_probe_overlay;
static uint32_t last_dw2_menu_probe_task;
static uint32_t last_dw2_menu_probe_focus;
static uint32_t last_dw2_menu_probe_label_ref;
static uint32_t last_dw2_menu_probe_detail_ref;
static uint32_t last_dw2_menu_probe_flags;
static bool last_dw2_menu_reject_valid;
static uint64_t last_dw2_menu_reject_frame;
static uint32_t last_dw2_menu_reject_pc;
static uint32_t last_dw2_menu_reject_overlay;
static char last_dw2_menu_reject_reason[32];

static const char *const dw2_menu_reject_reasons[] = {
   "overlay_mismatch",
   "invalid_task",
   "invalid_focus",
   "invalid_label_ref",
   "stale_probe",
};

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

static uint32_t fnv1a_step(uint32_t hash, uint8_t value)
{
   hash ^= value;
   hash *= 16777619u;
   return hash;
}

static bool trace_main_ram_address_valid(uint32_t address)
{
   uint32_t region = address & 0xffe00000u;

   return region == 0x80000000u || region == 0xa0000000u;
}

static uint32_t trace_main_ram_hash(const uint8_t *main_ram, size_t ram_size,
      uint32_t address, size_t length)
{
   uint32_t hash = 2166136261u;
   uint32_t offset;
   size_t i;

   if (!main_ram || !trace_main_ram_address_valid(address))
      return 0;

   offset = address & PSX_MAIN_RAM_MASK;
   if (offset >= ram_size)
      return 0;

   if (ram_size - offset < length)
      length = ram_size - offset;

   for (i = 0; i < length; i++)
      hash = fnv1a_step(hash, main_ram[offset + i]);

   return hash;
}

static bool trace_env_enabled(const char *value)
{
   if (!value || !*value)
      return false;

   return value[0] != '0' && value[0] != 'n' && value[0] != 'N'
      && value[0] != 'f' && value[0] != 'F';
}

static void trace_write_quoted(const char *text)
{
   const unsigned char *p = (const unsigned char*)text;

   fputc('"', trace_file);
   if (p)
   {
      while (*p)
      {
         if (*p == '"' || *p == '\\')
            fputc('\\', trace_file);

         if (*p >= 0x20 && *p < 0x7f)
            fputc(*p, trace_file);
         else
            fputc('?', trace_file);

         p++;
      }
   }
   fputc('"', trace_file);
}

static void trace_write_hex_bytes(const uint8_t *data, size_t length)
{
   size_t i;

   for (i = 0; i < length; i++)
      fprintf(trace_file, "%02x", data[i]);
}

static void trace_write_main_ram_bytes(const char *name,
      const uint8_t *main_ram, size_t ram_size, uint32_t address)
{
   uint32_t offset = address & PSX_MAIN_RAM_MASK;
   size_t length = DW2_DIALOG_PROBE_BYTES;

   fprintf(trace_file, " %s", name);

   if (!main_ram || !trace_main_ram_address_valid(address)
         || offset >= ram_size)
   {
      fputs("unavailable", trace_file);
      return;
   }

   if (ram_size - offset < length)
      length = ram_size - offset;

   trace_write_hex_bytes(main_ram + offset, length);
}

static bool trace_range_overlaps(uint32_t offset, unsigned width,
      uint32_t range_offset, uint32_t range_length)
{
   uint32_t end;
   uint32_t range_end;

   if (!width)
      width = 1;

   end = offset + width;
   range_end = range_offset + range_length;

   return offset < range_end && end > range_offset;
}

static bool trace_should_skip_delta_offset(size_t offset)
{
   if (trace_range_overlaps((uint32_t)offset, 1, DW2_TITLE_WINDOW_OFFSET,
            DW2_TITLE_WINDOW_LENGTH))
      return true;

   if (trace_range_overlaps((uint32_t)offset, 1, DW2_TITLE_DISPLAY_LIST_OFFSET,
            DW2_TITLE_DISPLAY_LIST_LENGTH))
      return true;

   return false;
}

static bool trace_is_small_selection_delta(uint8_t old_value,
      uint8_t new_value)
{
   return old_value <= 3 && new_value <= 3;
}

static bool trace_ensure_ram_snapshot(size_t ram_size)
{
   uint8_t *snapshot;

   if (trace_ram_snapshot_size == ram_size && trace_ram_snapshot)
      return true;

   snapshot = (uint8_t*)realloc(trace_ram_snapshot, ram_size);
   if (!snapshot)
   {
      free(trace_ram_snapshot);
      trace_ram_snapshot = NULL;
      trace_ram_snapshot_size = 0;
      trace_ram_delta_frames_remaining = 0;
      return false;
   }

   trace_ram_snapshot = snapshot;
   trace_ram_snapshot_size = ram_size;
   memset(trace_ram_snapshot, 0, trace_ram_snapshot_size);
   return true;
}

static void trace_write_input_ram_delta(const uint8_t *main_ram,
      size_t ram_size)
{
   size_t i;
   unsigned changed = 0;
   unsigned logged = 0;
   unsigned skipped = 0;
   unsigned small_changed = 0;
   unsigned small_logged = 0;

   if (!trace_file || !main_ram || !ram_size || !trace_ram_snapshot)
      return;

   if (!trace_ram_delta_frames_remaining)
      return;

   for (i = 0; i < ram_size; i++)
   {
      uint8_t old_value;
      uint8_t new_value;

      if (trace_ram_snapshot[i] == main_ram[i])
         continue;

      old_value = trace_ram_snapshot[i];
      new_value = main_ram[i];
      changed++;

      if (trace_should_skip_delta_offset(i))
      {
         skipped++;
         continue;
      }

      if (logged < DW2_RAM_DELTA_MAX_PER_FRAME)
      {
         fprintf(trace_file,
               "frame=%llu event=ram_delta reason=input_edge offset=0x%06x old=0x%02x new=0x%02x\n",
               (unsigned long long)trace_frame_index,
               (unsigned)i,
               old_value,
               new_value);
         logged++;
      }

      if (trace_is_small_selection_delta(old_value, new_value))
      {
         small_changed++;
         if (small_logged < DW2_RAM_SMALL_DELTA_MAX_PER_FRAME)
         {
            fprintf(trace_file,
                  "frame=%llu event=ram_small_delta reason=input_edge offset=0x%06x old=0x%02x new=0x%02x\n",
                  (unsigned long long)trace_frame_index,
                  (unsigned)i,
                  old_value,
                  new_value);
            small_logged++;
         }
      }
   }

   fprintf(trace_file,
         "frame=%llu event=ram_delta_summary reason=input_edge changed=%u logged=%u skipped=%u remaining=%u\n",
         (unsigned long long)trace_frame_index,
         changed, logged, skipped, trace_ram_delta_frames_remaining);
   fprintf(trace_file,
         "frame=%llu event=ram_small_delta_summary reason=input_edge small_changed=%u small_logged=%u remaining=%u\n",
         (unsigned long long)trace_frame_index,
         small_changed, small_logged, trace_ram_delta_frames_remaining);

   trace_ram_delta_frames_remaining--;
}

static void trace_write_dw2_title_window(const uint8_t *main_ram,
      size_t ram_size)
{
   size_t length = DW2_TITLE_WINDOW_LENGTH;

   if (!trace_file || !main_ram || ram_size <= DW2_TITLE_WINDOW_OFFSET)
      return;

   if (ram_size - DW2_TITLE_WINDOW_OFFSET < length)
      length = ram_size - DW2_TITLE_WINDOW_OFFSET;

   fprintf(trace_file,
         "frame=%llu event=ram_window name=dw2_title_menu offset=0x%06x length=0x%02x bytes=",
         (unsigned long long)trace_frame_index,
         DW2_TITLE_WINDOW_OFFSET,
         (unsigned)length);
   trace_write_hex_bytes(main_ram + DW2_TITLE_WINDOW_OFFSET, length);
   fputc('\n', trace_file);
}

static void sanitize_serial(const char *serial, char *out, size_t out_size)
{
   size_t i;

   if (!out_size)
      return;

   if (!serial || !*serial)
      serial = "unknown";

   for (i = 0; i + 1 < out_size && i < PSX_BOOT_SERIAL_LENGTH && serial[i]; i++)
   {
      char c = serial[i];
      out[i] = ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.')
         ? c : '_';
   }

   out[i] = '\0';
}

static void ensure_trace_dir(void)
{
   if (trace_dir[0])
      BEETLE_MKDIR(trace_dir);
}

static void trace_write_header(void)
{
   if (!trace_file)
      return;

   fprintf(trace_file,
         "event=session_start serial=%s frame=%llu format=beetle_psx_accessibility_trace_v1 deep_trace=%u\n",
         trace_serial, (unsigned long long)trace_frame_index,
         trace_deep_enabled ? 1 : 0);
   fprintf(trace_file,
         "event=note text=\"native state trace: no OCR, no AI service, no overlay\"\n");
   fflush(trace_file);
}

static void ensure_trace_file(void)
{
   time_t now;
   struct tm tm_now;
   char timestamp[32];
   int r;

   if (trace_file || !trace_dir[0])
      return;

   ensure_trace_dir();

   now = time(NULL);
#ifdef _WIN32
   localtime_s(&tm_now, &now);
#else
   localtime_r(&now, &tm_now);
#endif
   strftime(timestamp, sizeof(timestamp), "%Y%m%d-%H%M%S", &tm_now);

   r = snprintf(trace_path, sizeof(trace_path), "%s%cpsx-%s-%s.log",
         trace_dir, BEETLE_PATH_SEP, timestamp, trace_serial);
   if (r < 0 || r >= (int)sizeof(trace_path))
      return;

   trace_file = fopen(trace_path, "w");
   if (trace_file)
      setvbuf(trace_file, NULL, _IONBF, 0);
   trace_write_header();
}

void beetle_accessibility_trace_configure(const char *base_dir)
{
   const char *override_dir = getenv(TRACE_DIR_ENV);
   const char *deep_trace = getenv(TRACE_DEEP_ENV);
   int r;

   trace_deep_enabled = trace_env_enabled(deep_trace);

   if (override_dir && *override_dir)
      r = snprintf(trace_dir, sizeof(trace_dir), "%s", override_dir);
   else if (base_dir && *base_dir)
      r = snprintf(trace_dir, sizeof(trace_dir), "%s%c%s",
            base_dir, BEETLE_PATH_SEP, TRACE_SUBDIR);
   else
      r = snprintf(trace_dir, sizeof(trace_dir), ".%c%s",
            BEETLE_PATH_SEP, TRACE_SUBDIR);

   if (r < 0 || r >= (int)sizeof(trace_dir))
      trace_dir[0] = '\0';
}

void beetle_accessibility_trace_set_serial(const char *serial)
{
   sanitize_serial(serial, trace_serial, sizeof(trace_serial));
   ensure_trace_file();

   if (trace_file)
   {
      fprintf(trace_file, "frame=%llu event=serial serial=%s\n",
            (unsigned long long)trace_frame_index, trace_serial);
      fflush(trace_file);
   }
}

void beetle_accessibility_trace_reset(const char *reason)
{
   trace_frame_index = 0;
   last_input_mask = 0;
   last_dw2_title_menu = 0xffff;
   last_dw2_title_cursor = 0xff;
   log_dw2_title_window_on_next_frame = false;
   trace_ram_delta_frames_remaining = 0;
   last_ram_hash = 0;
   last_video_hash = 0;
   last_audio_hash = 0;
   last_dw2_dialog_probe_valid = false;
   last_dw2_dialog_probe_frame = 0;
   last_dw2_dialog_probe_s1 = 0;
   last_dw2_dialog_probe_s2 = 0;
   last_dw2_dialog_probe_s6 = 0;
   last_dw2_dialog_probe_sp = 0;
   last_dw2_dialog_probe_fp = 0;
   last_dw2_name_entry_probe_valid = false;
   last_dw2_name_entry_probe_frame = 0;
   last_dw2_name_entry_probe_state = 0;
   last_dw2_name_entry_probe_selected = 0;
   last_dw2_name_entry_probe_hash = 0;
   last_dw2_menu_probe_valid = false;
   last_dw2_menu_probe_frame = 0;
   last_dw2_menu_probe_pc = 0;
   last_dw2_menu_probe_overlay = 0;
   last_dw2_menu_probe_task = 0;
   last_dw2_menu_probe_focus = 0;
   last_dw2_menu_probe_label_ref = 0;
   last_dw2_menu_probe_detail_ref = 0;
   last_dw2_menu_probe_flags = 0;
   last_dw2_menu_reject_valid = false;
   last_dw2_menu_reject_frame = 0;
   last_dw2_menu_reject_pc = 0;
   last_dw2_menu_reject_overlay = 0;
   last_dw2_menu_reject_reason[0] = '\0';

   ensure_trace_file();

   if (trace_file)
   {
      fprintf(trace_file, "frame=0 event=reset reason=%s serial=%s\n",
            reason ? reason : "unknown", trace_serial);
      fflush(trace_file);
   }
}

void beetle_accessibility_trace_input(retro_input_state_t input_state_cb)
{
   uint16_t mask = 0;
   uint16_t changed;
   unsigned i;

   if (!input_state_cb)
      return;

   ensure_trace_file();

   for (i = 0; i < sizeof(trace_buttons) / sizeof(trace_buttons[0]); i++)
   {
      if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, trace_buttons[i].id))
         mask |= trace_buttons[i].bit;
   }

   changed = (uint16_t)(mask ^ last_input_mask);

   if (trace_file && changed)
   {
      for (i = 0; i < sizeof(trace_buttons) / sizeof(trace_buttons[0]); i++)
      {
         if (changed & trace_buttons[i].bit)
            fprintf(trace_file,
                  "frame=%llu event=input_edge button=%s state=%s mask=0x%04x\n",
                  (unsigned long long)trace_frame_index,
                  trace_buttons[i].name,
                  (mask & trace_buttons[i].bit) ? "down" : "up",
                  mask);
      }

      fflush(trace_file);
   }

   if (changed)
   {
      if (trace_deep_enabled)
      {
         log_dw2_title_window_on_next_frame = true;
         trace_ram_delta_frames_remaining = DW2_RAM_DELTA_FRAMES;
      }
   }

   last_input_mask = mask;
}

void beetle_accessibility_trace_frame(const uint8_t *main_ram, size_t ram_size)
{
   uint32_t ram_hash = 2166136261u;
   size_t i;
   uint16_t dw2_title_menu = 0xffff;
   uint8_t dw2_title_cursor = 0xff;
   bool title_menu_changed;
   bool title_cursor_changed;

   ensure_trace_file();

   if (!trace_deep_enabled)
   {
      trace_frame_index++;
      return;
   }

   if (main_ram && ram_size)
   {
      for (i = 0; i < ram_size; i += 256)
         ram_hash = fnv1a_step(ram_hash, main_ram[i]);

      if (ram_size > DW2_TITLE_MENU_OFFSET + 1)
         dw2_title_menu = read_u16le(main_ram, DW2_TITLE_MENU_OFFSET);
      if (ram_size > DW2_TITLE_CURSOR_OFFSET)
         dw2_title_cursor = main_ram[DW2_TITLE_CURSOR_OFFSET];
   }

   if (main_ram && ram_size && trace_ensure_ram_snapshot(ram_size))
      trace_write_input_ram_delta(main_ram, ram_size);

   title_menu_changed = dw2_title_menu != last_dw2_title_menu;
   title_cursor_changed = dw2_title_cursor != last_dw2_title_cursor;

   if (trace_file && title_menu_changed)
   {
      fprintf(trace_file,
            "frame=%llu event=ram_watch watch=dw2_title_menu offset=0x%06x value=0x%04x previous=0x%04x\n",
            (unsigned long long)trace_frame_index,
            DW2_TITLE_MENU_OFFSET, dw2_title_menu, last_dw2_title_menu);
   }

   if (trace_file && title_cursor_changed)
   {
      fprintf(trace_file,
            "frame=%llu event=ram_watch watch=dw2_title_cursor offset=0x%06x value=0x%02x previous=0x%02x\n",
            (unsigned long long)trace_frame_index,
            DW2_TITLE_CURSOR_OFFSET, dw2_title_cursor,
            last_dw2_title_cursor);
   }

   if (trace_file && (title_menu_changed || title_cursor_changed ||
            log_dw2_title_window_on_next_frame))
      trace_write_dw2_title_window(main_ram, ram_size);

   if (trace_file && ((trace_frame_index % 30) == 0 || ram_hash != last_ram_hash))
   {
      fprintf(trace_file,
            "frame=%llu event=frame serial=%s ram_sample_hash=0x%08x dw2_title_menu=0x%04x dw2_title_cursor=0x%02x\n",
            (unsigned long long)trace_frame_index, trace_serial,
            ram_hash, dw2_title_menu, dw2_title_cursor);
   }

   if (trace_file)
      fflush(trace_file);

   last_dw2_title_menu = dw2_title_menu;
   last_dw2_title_cursor = dw2_title_cursor;
   log_dw2_title_window_on_next_frame = false;
   last_ram_hash = ram_hash;
   if (main_ram && ram_size && trace_ram_snapshot
         && trace_ram_snapshot_size == ram_size)
      memcpy(trace_ram_snapshot, main_ram, ram_size);
   trace_frame_index++;
}

void beetle_accessibility_trace_video(const void *pixels, unsigned width,
      unsigned height, unsigned pitch, bool changed)
{
   uint32_t hash = 2166136261u;
   const uint8_t *base = (const uint8_t*)pixels;
   unsigned y_step;
   unsigned x_step;
   unsigned y;

   ensure_trace_file();

   if (!trace_deep_enabled)
      return;

   if (base && width && height && pitch)
   {
      y_step = height > 16 ? height / 16 : 1;
      x_step = width > 16 ? width / 16 : 1;

      for (y = 0; y < height; y += y_step)
      {
         const uint32_t *row = (const uint32_t*)(base + ((size_t)y * pitch));
         unsigned x;

         for (x = 0; x < width; x += x_step)
         {
            uint32_t pixel = row[x];
            hash = fnv1a_step(hash, (uint8_t)(pixel & 0xff));
            hash = fnv1a_step(hash, (uint8_t)((pixel >> 8) & 0xff));
            hash = fnv1a_step(hash, (uint8_t)((pixel >> 16) & 0xff));
            hash = fnv1a_step(hash, (uint8_t)((pixel >> 24) & 0xff));
         }
      }
   }

   if (trace_file && (changed || hash != last_video_hash ||
            (trace_frame_index % 30) == 0))
   {
      fprintf(trace_file,
            "frame=%llu event=video width=%u height=%u changed=%u hash=0x%08x\n",
            (unsigned long long)trace_frame_index, width, height,
            changed ? 1 : 0, hash);
      fflush(trace_file);
   }

   last_video_hash = hash;
}

void beetle_accessibility_trace_audio(const int16_t *samples, size_t frames)
{
   uint32_t hash = 2166136261u;
   uint16_t peak = 0;
   size_t i;

   ensure_trace_file();

   if (!trace_deep_enabled)
      return;

   if (samples && frames)
   {
      size_t sample_count = frames * 2;
      size_t step = sample_count > 512 ? sample_count / 512 : 1;

      for (i = 0; i < sample_count; i += step)
      {
         int sample = samples[i];
         unsigned amp = (unsigned)(sample < 0 ? -sample : sample);
         if (amp > peak)
            peak = (uint16_t)amp;

         hash = fnv1a_step(hash, (uint8_t)(sample & 0xff));
         hash = fnv1a_step(hash, (uint8_t)((sample >> 8) & 0xff));
      }
   }

   if (trace_file && (hash != last_audio_hash || (trace_frame_index % 30) == 0))
   {
      fprintf(trace_file,
            "frame=%llu event=audio frames=%u peak=%u hash=0x%08x\n",
            (unsigned long long)trace_frame_index, (unsigned)frames,
            peak, hash);
      fflush(trace_file);
   }

   last_audio_hash = hash;
}

void beetle_accessibility_trace_speech(const char *text, int priority,
      const char *channel, const char *result)
{
   ensure_trace_file();

   if (!trace_file)
      return;

   fprintf(trace_file,
         "frame=%llu event=speech channel=%s priority=%d result=%s text=",
         (unsigned long long)trace_frame_index,
         channel && *channel ? channel : "default",
         priority,
         result && *result ? result : "unknown");
   trace_write_quoted(text);
   fputc('\n', trace_file);
   fflush(trace_file);
}

void beetle_accessibility_trace_navigation_key(unsigned keycode, bool down,
      uint32_t overlay_tag)
{
   const char *key = "unknown";

   switch (keycode)
   {
      case RETROK_HOME:
         key = "home";
         break;
      case RETROK_END:
         key = "end";
         break;
      case RETROK_PAGEUP:
         key = "page_up";
         break;
      case RETROK_PAGEDOWN:
         key = "page_down";
         break;
      case RETROK_DELETE:
         key = "delete";
         break;
      case RETROK_KP_ENTER:
         key = "keypad_enter";
         break;
      default:
         break;
   }

   ensure_trace_file();
   if (!trace_file)
      return;
   fprintf(trace_file,
         "frame=%llu event=dw2_navigation_key key=%s keycode=%u down=%u overlay=0x%08x\n",
         (unsigned long long)trace_frame_index, key, keycode,
         down ? 1u : 0u, overlay_tag);
   fflush(trace_file);
}

void beetle_accessibility_trace_ram_write(uint32_t pc, uint32_t address,
      uint32_t value, unsigned width, const char *source)
{
   uint32_t offset = address & PSX_MAIN_RAM_MASK;
   const char *watch = NULL;

   if (!trace_deep_enabled)
      return;

   if (strncmp(trace_serial, DW2_TRACE_SERIAL, sizeof(DW2_TRACE_SERIAL) - 1) != 0)
      return;

   if (trace_range_overlaps(offset, width, DW2_TITLE_WINDOW_OFFSET,
            DW2_TITLE_WINDOW_LENGTH))
      watch = "dw2_title_window";
   else if (trace_range_overlaps(offset, width, DW2_TITLE_DISPLAY_LIST_OFFSET,
            DW2_TITLE_DISPLAY_LIST_LENGTH))
      watch = "dw2_title_display_list";
   else if (trace_range_overlaps(offset, width, DW2_BATTLE_MENU_CURSOR_OFFSET,
            DW2_BATTLE_MENU_CURSOR_LENGTH))
      watch = "dw2_battle_menu_cursor";
   else if (trace_range_overlaps(offset, width, DW2_BATTLE_STATE_BLOCK_OFFSET,
            DW2_BATTLE_STATE_BLOCK_LENGTH))
      watch = "dw2_battle_state_block";
   else
      return;

   ensure_trace_file();

   if (!trace_file)
      return;

   fprintf(trace_file,
         "frame=%llu event=ram_write_watch watch=%s pc=0x%08x address=0x%08x offset=0x%06x width=%u value=0x%08x source=%s\n",
         (unsigned long long)trace_frame_index,
         watch, pc, address, offset, width, value,
         source && *source ? source : "unknown");
   fflush(trace_file);
}

void beetle_accessibility_trace_cpu_dialog_probe(uint32_t pc, uint32_t s1,
      uint32_t s2, uint32_t s6, uint32_t sp, uint32_t fp,
      const uint8_t *main_ram, size_t ram_size)
{
   bool same_probe;
   bool recent_probe;
   uint32_t s1_offset = s1 & PSX_MAIN_RAM_MASK;
   uint32_t s2_offset = s2 & PSX_MAIN_RAM_MASK;
   uint32_t s6_offset = s6 & PSX_MAIN_RAM_MASK;
   uint32_t fp_offset = fp & PSX_MAIN_RAM_MASK;
   uint32_t text_ptr = 0;
   uint32_t text_ptr_offset = 0;

   if (strncmp(trace_serial, DW2_TRACE_SERIAL, sizeof(DW2_TRACE_SERIAL) - 1) != 0)
      return;

   if (pc != DW2_MAIN_DIALOG_X_ADVANCE_PC)
      return;

   same_probe = last_dw2_dialog_probe_valid
      && s1 == last_dw2_dialog_probe_s1
      && s2 == last_dw2_dialog_probe_s2
      && s6 == last_dw2_dialog_probe_s6
      && sp == last_dw2_dialog_probe_sp
      && fp == last_dw2_dialog_probe_fp;
   recent_probe = last_dw2_dialog_probe_valid
      && trace_frame_index >= last_dw2_dialog_probe_frame
      && trace_frame_index - last_dw2_dialog_probe_frame <
         DW2_DIALOG_PROBE_INTERVAL_FRAMES;

   if (same_probe && recent_probe)
      return;

   ensure_trace_file();

   if (!trace_file)
      return;

   if (main_ram && s2_offset + 11 < ram_size)
      text_ptr = read_u32le(main_ram, s2_offset + 8);
   text_ptr_offset = text_ptr & PSX_MAIN_RAM_MASK;

   fprintf(trace_file,
         "frame=%llu event=dw2_dialog_probe pc=0x%08x s1=0x%08x s2=0x%08x s6=0x%08x sp=0x%08x fp=0x%08x s1_offset=0x%06x s2_offset=0x%06x s6_offset=0x%06x fp_offset=0x%06x text_ptr=0x%08x text_ptr_offset=0x%06x",
         (unsigned long long)trace_frame_index,
         pc, s1, s2, s6, sp, fp,
         s1_offset,
         s2_offset,
         s6_offset,
         fp_offset,
         text_ptr,
         text_ptr_offset);
   trace_write_main_ram_bytes("s1_bytes=", main_ram, ram_size, s1);
   trace_write_main_ram_bytes("text_ptr_bytes=", main_ram, ram_size, text_ptr);
   trace_write_main_ram_bytes("s2_bytes=", main_ram, ram_size, s2);
   trace_write_main_ram_bytes("s6_bytes=", main_ram, ram_size, s6);
   trace_write_main_ram_bytes("fp_bytes=", main_ram, ram_size, fp);
   fputc('\n', trace_file);
   fflush(trace_file);

   last_dw2_dialog_probe_valid = true;
   last_dw2_dialog_probe_frame = trace_frame_index;
   last_dw2_dialog_probe_s1 = s1;
   last_dw2_dialog_probe_s2 = s2;
   last_dw2_dialog_probe_s6 = s6;
   last_dw2_dialog_probe_sp = sp;
   last_dw2_dialog_probe_fp = fp;
}

void beetle_accessibility_trace_cpu_name_entry_probe(uint32_t pc,
      uint32_t state, uint32_t selected,
      const uint8_t *main_ram, size_t ram_size)
{
   uint32_t state_offset = state & PSX_MAIN_RAM_MASK;
   uint32_t state_hash;
   uint32_t player_name_hash;
   uint32_t digi_bettle_name_hash;
   uint32_t source_name_hash;
   uint32_t combined_hash;
   bool same_probe;
   bool recent_probe;

   if (strncmp(trace_serial, DW2_TRACE_SERIAL,
            sizeof(DW2_TRACE_SERIAL) - 1) != 0)
      return;

   if (pc != DW2_NAME_ENTRY_SELECTION_RETURN_PC)
      return;

   state_hash = trace_main_ram_hash(main_ram, ram_size, state,
         DW2_NAME_ENTRY_STATE_BYTES);
   player_name_hash = trace_main_ram_hash(main_ram, ram_size,
         0x80000000u | DW2_PLAYER_NAME_OFFSET,
         DW2_NAME_BUFFER_TRACE_BYTES);
   digi_bettle_name_hash = trace_main_ram_hash(main_ram, ram_size,
         0x80000000u | DW2_DIGI_BETTLE_NAME_OFFSET,
         DW2_NAME_BUFFER_TRACE_BYTES);
   source_name_hash = trace_main_ram_hash(main_ram, ram_size,
         0x80000000u | DW2_NAME_ENTRY_SOURCE_OFFSET,
         DW2_NAME_BUFFER_TRACE_BYTES);
   combined_hash = state_hash ^ player_name_hash ^ digi_bettle_name_hash
      ^ source_name_hash;

   same_probe = last_dw2_name_entry_probe_valid
      && state == last_dw2_name_entry_probe_state
      && selected == last_dw2_name_entry_probe_selected
      && combined_hash == last_dw2_name_entry_probe_hash;
   recent_probe = last_dw2_name_entry_probe_valid
      && trace_frame_index >= last_dw2_name_entry_probe_frame
      && trace_frame_index - last_dw2_name_entry_probe_frame
         < DW2_NAME_ENTRY_PROBE_INTERVAL_FRAMES;

   if (same_probe && recent_probe)
      return;

   ensure_trace_file();

   if (!trace_file)
      return;

   fprintf(trace_file,
         "frame=%llu event=dw2_name_entry_probe pc=0x%08x state=0x%08x selected=0x%08x state_offset=0x%06x state_hash=0x%08x",
         (unsigned long long)trace_frame_index,
         pc, state, selected, state_offset, state_hash);
   trace_write_main_ram_bytes("state_bytes=", main_ram, ram_size, state);
   trace_write_main_ram_bytes("player_name_bytes=", main_ram, ram_size,
         0x80000000u | DW2_PLAYER_NAME_OFFSET);
   trace_write_main_ram_bytes("digi_bettle_name_bytes=", main_ram, ram_size,
         0x80000000u | DW2_DIGI_BETTLE_NAME_OFFSET);
   trace_write_main_ram_bytes("source_name_bytes=", main_ram, ram_size,
         0x80000000u | DW2_NAME_ENTRY_SOURCE_OFFSET);
   fputc('\n', trace_file);
   fflush(trace_file);

   last_dw2_name_entry_probe_valid = true;
   last_dw2_name_entry_probe_frame = trace_frame_index;
   last_dw2_name_entry_probe_state = state;
   last_dw2_name_entry_probe_selected = selected;
   last_dw2_name_entry_probe_hash = combined_hash;
}

void beetle_accessibility_trace_cpu_menu_probe(uint32_t pc,
      uint32_t overlay_tag, uint32_t task, uint32_t focus,
      uint32_t label_ref, uint32_t detail_ref, uint32_t flags)
{
   bool same_probe;
   bool recent_probe;

   if (strncmp(trace_serial, DW2_TRACE_SERIAL,
            sizeof(DW2_TRACE_SERIAL) - 1) != 0)
      return;
   same_probe = last_dw2_menu_probe_valid
      && pc == last_dw2_menu_probe_pc
      && overlay_tag == last_dw2_menu_probe_overlay
      && task == last_dw2_menu_probe_task
      && focus == last_dw2_menu_probe_focus
      && label_ref == last_dw2_menu_probe_label_ref
      && detail_ref == last_dw2_menu_probe_detail_ref
      && flags == last_dw2_menu_probe_flags;
   recent_probe = last_dw2_menu_probe_valid
      && trace_frame_index >= last_dw2_menu_probe_frame
      && trace_frame_index - last_dw2_menu_probe_frame
         < DW2_MENU_PROBE_INTERVAL_FRAMES;
   if (same_probe && recent_probe)
      return;

   ensure_trace_file();
   if (!trace_file)
      return;
   fprintf(trace_file,
         "frame=%llu event=dw2_menu_probe pc=0x%08x overlay=0x%08x task=0x%08x focus=0x%08x label_ref=0x%08x detail_ref=0x%08x flags=0x%08x\n",
         (unsigned long long)trace_frame_index,
         pc, overlay_tag, task, focus, label_ref, detail_ref, flags);
   fflush(trace_file);

   last_dw2_menu_probe_valid = true;
   last_dw2_menu_probe_frame = trace_frame_index;
   last_dw2_menu_probe_pc = pc;
   last_dw2_menu_probe_overlay = overlay_tag;
   last_dw2_menu_probe_task = task;
   last_dw2_menu_probe_focus = focus;
   last_dw2_menu_probe_label_ref = label_ref;
   last_dw2_menu_probe_detail_ref = detail_ref;
   last_dw2_menu_probe_flags = flags;
}

void beetle_accessibility_trace_cpu_menu_reject(uint32_t pc,
      uint32_t overlay_tag, const char *reason)
{
   bool valid_reason = false;
   bool same_rejection;
   bool recent_rejection;
   size_t reason_index;

   if (strncmp(trace_serial, DW2_TRACE_SERIAL,
            sizeof(DW2_TRACE_SERIAL) - 1) != 0 || !reason)
      return;
   for (reason_index = 0;
         reason_index < sizeof(dw2_menu_reject_reasons)
            / sizeof(dw2_menu_reject_reasons[0]); reason_index++)
   {
      if (!strcmp(reason, dw2_menu_reject_reasons[reason_index]))
      {
         valid_reason = true;
         break;
      }
   }
   if (!valid_reason)
      return;

   same_rejection = last_dw2_menu_reject_valid
      && pc == last_dw2_menu_reject_pc
      && overlay_tag == last_dw2_menu_reject_overlay
      && !strcmp(reason, last_dw2_menu_reject_reason);
   recent_rejection = last_dw2_menu_reject_valid
      && trace_frame_index >= last_dw2_menu_reject_frame
      && trace_frame_index - last_dw2_menu_reject_frame
         < DW2_MENU_PROBE_INTERVAL_FRAMES;
   if (same_rejection && recent_rejection)
      return;

   ensure_trace_file();
   if (!trace_file)
      return;
   fprintf(trace_file,
         "frame=%llu event=dw2_menu_probe_reject pc=0x%08x overlay=0x%08x reason=%s\n",
         (unsigned long long)trace_frame_index, pc, overlay_tag, reason);
   fflush(trace_file);

   last_dw2_menu_reject_valid = true;
   last_dw2_menu_reject_frame = trace_frame_index;
   last_dw2_menu_reject_pc = pc;
   last_dw2_menu_reject_overlay = overlay_tag;
   strncpy(last_dw2_menu_reject_reason, reason,
         sizeof(last_dw2_menu_reject_reason) - 1);
   last_dw2_menu_reject_reason[
      sizeof(last_dw2_menu_reject_reason) - 1] = '\0';
}

void beetle_accessibility_trace_close(void)
{
   if (!trace_file)
      return;

   fprintf(trace_file, "frame=%llu event=session_end serial=%s\n",
         (unsigned long long)trace_frame_index, trace_serial);
   fclose(trace_file);
   trace_file = NULL;
}
