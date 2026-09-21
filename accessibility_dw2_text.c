#include "accessibility_dw2_text.h"

#include <string.h>

#define DW2_TEXT_MAX_INPUT_BYTES 160
#define DW2_TEXT_CHUNK_MAX 192
#define DW2_PLAYER_NAME_OFFSET 0x05e634
#define DW2_PLAYER_NAME_MAX_BYTES 6
#define DW2_DIGI_BETTLE_NAME_OFFSET 0x05e6f1
#define DW2_DIGI_BETTLE_NAME_MAX_BYTES 12
#define DW2_PSX_RAM_MASK 0x1fffff
#define DW2_TEXT_FORMAT_ARGUMENT_LIMIT 4
#define DW2_TEXT_FORMAT_DEPTH_LIMIT 4
#define DW2_TEXT_CHOICE_MODE ((beetle_dw2_text_mode_t)2)

static bool dw2_text_append(char *out, size_t out_size, size_t *out_len,
      const char *text)
{
   if (!out || !out_size || !out_len || !text)
      return false;

   while (*text)
   {
      if (*out_len + 1 >= out_size)
         return false;

      out[(*out_len)++] = *text++;
   }

   out[*out_len] = '\0';
   return true;
}

static bool dw2_text_append_char(char *out, size_t out_size,
      size_t *out_len, char value)
{
   char text[2];

   text[0] = value;
   text[1] = '\0';
   return dw2_text_append(out, out_size, out_len, text);
}

static bool dw2_text_append_encoded_char(char *out, size_t out_size,
      size_t *out_len, uint8_t value)
{
   if (value <= 9)
      return dw2_text_append_char(out, out_size, out_len,
            (char)('0' + value));

   if (value >= 0x0a && value <= 0x23)
      return dw2_text_append_char(out, out_size, out_len,
            (char)('A' + value - 0x0a));

   if (value >= 0x24 && value <= 0x3d)
      return dw2_text_append_char(out, out_size, out_len,
            (char)('a' + value - 0x24));

   switch (value)
   {
      case 0x42: return dw2_text_append_char(out, out_size, out_len, '&');
      case 0x44: return dw2_text_append_char(out, out_size, out_len, '?');
      case 0x45: return dw2_text_append_char(out, out_size, out_len, '!');
      case 0x46: return dw2_text_append_char(out, out_size, out_len, '/');
      case 0x49: return dw2_text_append_char(out, out_size, out_len, '-');
      case 0x54: return dw2_text_append_char(out, out_size, out_len, ',');
      case 0x55: return dw2_text_append_char(out, out_size, out_len, '.');
      case 0x56: return dw2_text_append_char(out, out_size, out_len, '\'');
      case 0x57: return dw2_text_append_char(out, out_size, out_len, '"');
      case 0x58: return dw2_text_append_char(out, out_size, out_len, ';');
      case 0x59: return dw2_text_append_char(out, out_size, out_len, ':');
      case 0x5a: return dw2_text_append_char(out, out_size, out_len, '%');
      case 0x5b: return dw2_text_append_char(out, out_size, out_len, '+');
      case 0x5d: return dw2_text_append_char(out, out_size, out_len, '#');
      default: return true;
   }
}

static bool dw2_text_address_to_offset(uint32_t address, size_t ram_size,
      size_t *offset)
{
   uint32_t region = address & 0xffe00000u;

   if (!offset || (region != 0x80000000u && region != 0xa0000000u))
      return false;

   *offset = address & DW2_PSX_RAM_MASK;
   return *offset < ram_size;
}

static void dw2_text_trim(char *text)
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

static bool dw2_text_is_alpha(char value)
{
   return (value >= 'A' && value <= 'Z')
      || (value >= 'a' && value <= 'z');
}

static bool dw2_text_is_upper(char value)
{
   return value >= 'A' && value <= 'Z';
}

static bool dw2_decoder_has_speech(const char *text)
{
   unsigned alnum_count = 0;
   unsigned alpha_count = 0;

   while (text && *text)
   {
      if (dw2_text_is_alpha(*text) || (*text >= '0' && *text <= '9'))
         alnum_count++;
      if (dw2_text_is_alpha(*text))
         alpha_count++;
      text++;
   }

   return alpha_count > 0 && alnum_count >= 2;
}

static bool dw2_text_chunk_has_speech(const char *text)
{
   if (dw2_decoder_has_speech(text))
      return true;

   return text && text[0] && !text[1]
      && (text[0] == 'I' || text[0] == 'A' || text[0] == 'a');
}

static bool dw2_text_fragment_has_content(const char *text)
{
   while (text && *text)
   {
      if (dw2_text_is_alpha(*text) || (*text >= '0' && *text <= '9'))
         return true;
      text++;
   }

   return false;
}

static bool dw2_text_flush_chunk(char *out, size_t out_size,
      size_t *out_len, char *chunk, size_t *chunk_len,
      bool allow_fragment, char *speaker, size_t speaker_size,
      bool *speaker_candidate)
{
   char *text = chunk;
   bool native_speaker_marker = false;

   if (!chunk_len || !*chunk_len)
      return true;

   chunk[*chunk_len] = '\0';
   dw2_text_trim(chunk);

   /* Message resources prefix speaker-name rows with a native q marker. */
   if (chunk[0] == 'q' && dw2_text_is_upper(chunk[1]))
   {
      text = chunk + 1;
      native_speaker_marker = true;
   }

   dw2_text_trim(text);
   if (speaker && speaker_size && !speaker[0]
         && dw2_text_chunk_has_speech(text)
         && (native_speaker_marker
            || (speaker_candidate && *speaker_candidate)))
   {
      strncpy(speaker, text, speaker_size - 1);
      speaker[speaker_size - 1] = '\0';
   }
   if (speaker_candidate && dw2_text_fragment_has_content(text))
      *speaker_candidate = false;
   if (dw2_text_chunk_has_speech(text)
         || (allow_fragment && dw2_text_fragment_has_content(text)))
   {
      if (*out_len && !dw2_text_append_char(out, out_size, out_len, ' '))
         return false;
      if (!dw2_text_append(out, out_size, out_len, text))
         return false;
   }

   *chunk_len = 0;
   chunk[0] = '\0';
   return true;
}

static bool dw2_text_decode_internal(
      const uint8_t *main_ram, size_t ram_size, uint32_t psx_address,
      const uint32_t *arguments, size_t argument_count,
      beetle_dw2_text_mode_t mode, bool allow_fragments, unsigned depth,
      char *out, size_t out_size, char *speaker, size_t speaker_size);

static const char *dw2_text_token(uint8_t token)
{
   switch (token)
   {
      case 0x06: return "Digimon";
      case 0x07: return "you";
      case 0x08: return "the";
      case 0x09: return "Digi-Beetle";
      case 0x0a: return "Domain";
      case 0x0b: return "Guard";
      case 0x0c: return "Tamer";
      case 0x0d: return "here";
      case 0x0e: return "have";
      case 0x0f: return "Knights";
      case 0x10: return "and";
      case 0x11: return "thing";
      case 0x12: return "Security";
      case 0x13: return "that";
      case 0x14: return "Bertran";
      case 0x15: return "Tournament";
      case 0x16: return "Crimson";
      case 0x17: return "Vendor";
      case 0x18: return "something";
      case 0x19: return "Item";
      case 0x1a: return "Falcon";
      case 0x1b: return "for";
      case 0x1c: return "That's";
      case 0x1d: return "Commander";
      case 0x1e: return "Blood";
      case 0x1f: return "Leader";
      case 0x20: return "Attendant";
      case 0x21: return "Cecilia";
      case 0x22: return "all";
      case 0x23: return "mission";
      case 0x24: return "this";
      case 0x25: return "MasterTyrannomon";
      case 0x26: return "Archive";
      case 0x27: return "Black";
      case 0x28: return "I'll";
      case 0x29: return "are";
      case 0x2a: return "Sword";
      case 0x2b: return "right";
      case 0x2c: return "digivolve";
      case 0x2d: return "enter";
      case 0x2e: return "What";
      case 0x2f: return "will";
      case 0x30: return "come";
      case 0x31: return "You";
      case 0x32: return "Coliseum";
      case 0x33: return "about";
      case 0x34: return "don't";
      case 0x35: return "anything";
      case 0x36: return "Vandar";
      case 0x37: return "Parts";
      case 0x38: return "where";
      case 0x39: return "The";
      case 0x3a: return "know";
      case 0x3b: return "Leomon";
      case 0x3c: return "want";
      case 0x3d: return "Oldman";
      case 0x3e: return "like";
      case 0x3f: return "need";
      case 0x40: return "Chief";
      case 0x41: return "with";
      case 0x42: return "Thank";
      case 0x43: return "strange";
      case 0x44: return "Island";
      case 0x45: return "can";
      case 0x46: return "really";
      case 0x47: return "Blue";
      case 0x48: return "time";
      default: return NULL;
   }
}

static bool dw2_text_decode_name(const uint8_t *main_ram, size_t ram_size,
      size_t offset, size_t max_bytes, char *out, size_t out_size)
{
   size_t i;
   size_t out_len = 0;

   if (!main_ram || !out || !out_size || offset >= ram_size)
      return false;

   out[0] = '\0';
   for (i = 0; i < max_bytes && offset + i < ram_size; i++)
   {
      uint8_t value = main_ram[offset + i];

      if (value == 0xff)
      {
         dw2_text_trim(out);
         return out[0] != '\0';
      }

      /* Names use the same native space glyph as dialogue. Rejecting it
       * discarded every page addressing a player whose name contains FD. */
      if (value == 0xfd)
      {
         if (!dw2_text_append_char(out, out_size, &out_len, ' '))
            return false;
         continue;
      }

      if (value >= 0xef
            || !dw2_text_append_encoded_char(out, out_size, &out_len, value))
         return false;
   }

   return false;
}

static bool dw2_text_append_token(const uint8_t *main_ram, size_t ram_size,
      uint8_t token_id, const uint32_t *arguments, size_t argument_count,
      beetle_dw2_text_mode_t mode, unsigned depth, char *chunk,
      size_t chunk_size, size_t *chunk_len, bool *fragment_appended)
{
   char argument_text[DW2_TEXT_CHUNK_MAX];
   char dynamic_name[DW2_DIGI_BETTLE_NAME_MAX_BYTES + 1];
   const char *token = NULL;

   if (fragment_appended)
      *fragment_appended = false;

   if (token_id >= 1 && token_id <= DW2_TEXT_FORMAT_ARGUMENT_LIMIT)
   {
      size_t argument_index = (size_t)token_id - 1u;

      if (!arguments || argument_index >= argument_count
            || !arguments[argument_index]
            || depth >= DW2_TEXT_FORMAT_DEPTH_LIMIT)
         return true;
      if (!dw2_text_decode_internal(main_ram, ram_size,
               arguments[argument_index], arguments, argument_count, mode,
               true, depth + 1u, argument_text, sizeof(argument_text),
               NULL, 0))
         return true;
      if (!dw2_text_append(chunk, chunk_size, chunk_len, argument_text))
         return false;
      if (fragment_appended)
         *fragment_appended = true;
      return true;
   }

   if (token_id == 0)
   {
      if (!dw2_text_decode_name(main_ram, ram_size, DW2_PLAYER_NAME_OFFSET,
               DW2_PLAYER_NAME_MAX_BYTES, dynamic_name,
               sizeof(dynamic_name)))
         return false;
      token = dynamic_name;
   }
   else if (token_id == 5)
   {
      if (!dw2_text_decode_name(main_ram, ram_size,
               DW2_DIGI_BETTLE_NAME_OFFSET,
               DW2_DIGI_BETTLE_NAME_MAX_BYTES, dynamic_name,
               sizeof(dynamic_name)))
         return false;
      token = dynamic_name;
   }
   else
      token = dw2_text_token(token_id);

   /* Table entries beyond 0x48 require other live native context. Their
    * command is valid, but this decoder cannot resolve them safely. */
   return !token || dw2_text_append(chunk, chunk_size, chunk_len, token);
}

static bool dw2_text_digits_valid(const uint8_t *bytes, size_t count)
{
   size_t i;

   for (i = 0; i < count; i++)
      if (bytes[i] > 9)
         return false;

   return true;
}

static bool dw2_text_decode_internal(
      const uint8_t *main_ram, size_t ram_size, uint32_t psx_address,
      const uint32_t *arguments, size_t argument_count,
      beetle_dw2_text_mode_t mode, bool allow_fragments, unsigned depth,
      char *out, size_t out_size, char *speaker, size_t speaker_size)
{
   size_t offset;
   size_t index = 0;
   size_t out_len = 0;
   char chunk[DW2_TEXT_CHUNK_MAX];
   size_t chunk_len = 0;
   bool chunk_is_fragment = false;
   bool terminated = false;
   bool speaker_candidate = false;

   if (!main_ram || !out || !out_size
         || argument_count > DW2_TEXT_FORMAT_ARGUMENT_LIMIT
         || (argument_count && !arguments)
         || depth > DW2_TEXT_FORMAT_DEPTH_LIMIT
         || (mode != BEETLE_DW2_TEXT_DIALOG
            && mode != BEETLE_DW2_TEXT_MENU
            && mode != DW2_TEXT_CHOICE_MODE)
         || !dw2_text_address_to_offset(psx_address, ram_size, &offset))
      return false;

   out[0] = '\0';
   if (speaker && speaker_size)
      speaker[0] = '\0';
   chunk[0] = '\0';

   while (index < DW2_TEXT_MAX_INPUT_BYTES && offset + index < ram_size)
   {
      uint8_t value = main_ram[offset + index];
      size_t command_size = 0;

      if (value < 0xef)
      {
         if (!dw2_text_append_encoded_char(chunk, sizeof(chunk),
                  &chunk_len, value))
            return false;
         index++;
         continue;
      }

      if (value == 0xff)
      {
         if (!dw2_text_flush_chunk(out, out_size, &out_len, chunk,
                  &chunk_len, allow_fragments || chunk_is_fragment,
                  speaker, speaker_size, &speaker_candidate))
            return false;
         chunk_is_fragment = false;
         terminated = true;
         break;
      }

      if (value == 0xfd || value == 0xfe)
      {
         if (!dw2_text_flush_chunk(out, out_size, &out_len, chunk,
                  &chunk_len, allow_fragments || chunk_is_fragment,
                  speaker, speaker_size, &speaker_candidate))
            return false;
         chunk_is_fragment = false;
         index++;
         continue;
      }

      if (value == 0xfb || value == 0xfc)
      {
         if (!dw2_text_flush_chunk(out, out_size, &out_len, chunk,
                  &chunk_len, allow_fragments || chunk_is_fragment,
                  speaker, speaker_size, &speaker_candidate))
            return false;
         chunk_is_fragment = false;
         index++;
         if (mode == BEETLE_DW2_TEXT_DIALOG && out_len)
         {
            terminated = true;
            break;
         }
         continue;
      }

      /* Dictionary substitutions are inline glyphs, not word boundaries: the native
       * technique resource encodes Fireball as "Fireb" + token 22 ("all").
       * Only an actual separator/control below should flush this chunk. */
      if (value == 0xf0)
      {
         bool fragment = false;
         command_size = 2;
         if (index + command_size > DW2_TEXT_MAX_INPUT_BYTES
               || offset + index + command_size > ram_size)
            return false;
         /* Keep named/numeric renderer arguments as separate spoken fields,
          * such as "Level 12", while joining compressed word fragments. */
         if (main_ram[offset + index + 1] < 6u)
         {
            if (!dw2_text_flush_chunk(out, out_size, &out_len,
                     chunk, &chunk_len, allow_fragments || chunk_is_fragment,
                     speaker, speaker_size, &speaker_candidate))
               return false;
            chunk_is_fragment = false;
         }
         if (!dw2_text_append_token(main_ram, ram_size,
                  main_ram[offset + index + 1], arguments,
                  argument_count, mode, depth, chunk, sizeof(chunk),
                  &chunk_len, &fragment))
            return false;
         chunk_is_fragment = chunk_is_fragment || fragment;
         index += command_size;
         continue;
      }

      if (!dw2_text_flush_chunk(out, out_size, &out_len, chunk, &chunk_len,
               allow_fragments || chunk_is_fragment, speaker, speaker_size,
               &speaker_candidate))
         return false;
      chunk_is_fragment = false;

      if (value == 0xf8 && mode == DW2_TEXT_CHOICE_MODE)
      {
         terminated = true;
         break;
      }

      switch (value)
      {
         case 0xef:
            command_size = 2;
            break;
         case 0xf1:
         case 0xf2:
            command_size = 4;
            break;
         case 0xf3:
            command_size = 2;
            break;
         case 0xf4:
            if (index + 2 > DW2_TEXT_MAX_INPUT_BYTES
                  || offset + index + 2 > ram_size)
               return false;
            command_size = main_ram[offset + index + 1] < 0x30 ? 5 : 2;
            if (command_size == 5 && mode == BEETLE_DW2_TEXT_DIALOG
                  && depth == 0 && !out_len)
               speaker_candidate = true;
            break;
         case 0xf5:
            command_size = 1;
            break;
         case 0xf6:
         case 0xf7:
            command_size = 10;
            break;
         case 0xf8:
            command_size = 2;
            break;
         case 0xf9:
            if (index + 2 > DW2_TEXT_MAX_INPUT_BYTES
                  || offset + index + 2 > ram_size)
               return false;
            command_size = (main_ram[offset + index + 1] & 1) ? 2 : 5;
            break;
         case 0xfa:
            command_size = 2;
            break;
         default:
            return false;
      }

      if (index + command_size > DW2_TEXT_MAX_INPUT_BYTES
            || offset + index + command_size > ram_size)
         return false;

      if ((value == 0xf1 || value == 0xf2)
            && !dw2_text_digits_valid(main_ram + offset + index + 1, 3))
         return false;
      if (value == 0xf4 && command_size == 5
            && !dw2_text_digits_valid(main_ram + offset + index + 2, 3))
         return false;
      if ((value == 0xf6 || value == 0xf7)
            && !dw2_text_digits_valid(main_ram + offset + index + 1, 9))
         return false;
      if (value == 0xf9 && command_size == 5
            && !dw2_text_digits_valid(main_ram + offset + index + 2, 3))
         return false;

      index += command_size;
   }

   if (!terminated)
      return false;

   dw2_text_trim(out);
   return allow_fragments
      ? dw2_text_fragment_has_content(out)
      : dw2_decoder_has_speech(out);
}

bool beetle_accessibility_dw2_decode_text(
      const uint8_t *main_ram, size_t ram_size, uint32_t psx_address,
      beetle_dw2_text_mode_t mode, char *out, size_t out_size)
{
   return dw2_text_decode_internal(main_ram, ram_size, psx_address,
         NULL, 0, mode, false, 0, out, out_size, NULL, 0);
}

bool beetle_accessibility_dw2_decode_dialog_with_speaker(
      const uint8_t *main_ram, size_t ram_size, uint32_t psx_address,
      char *out, size_t out_size, char *speaker, size_t speaker_size)
{
   if (!speaker || !speaker_size)
      return false;
   return dw2_text_decode_internal(main_ram, ram_size, psx_address,
         NULL, 0, BEETLE_DW2_TEXT_DIALOG, false, 0, out, out_size,
         speaker, speaker_size);
}

bool beetle_accessibility_dw2_decode_formatted_dialog_with_speaker(
      const uint8_t *main_ram, size_t ram_size, uint32_t psx_address,
      const uint32_t *arguments, size_t argument_count,
      char *out, size_t out_size, char *speaker, size_t speaker_size)
{
   if (!speaker || !speaker_size)
      return false;
   return dw2_text_decode_internal(main_ram, ram_size, psx_address,
         arguments, argument_count, BEETLE_DW2_TEXT_DIALOG, false, 0,
         out, out_size, speaker, speaker_size);
}

bool beetle_accessibility_dw2_decode_speaker_label(
      const uint8_t *main_ram, size_t ram_size, uint32_t psx_address,
      char *out, size_t out_size)
{
   size_t offset;
   size_t index = 0;
   size_t out_len = 0;
   bool active = false;
   bool pending_space = false;

   if (!main_ram || !out || !out_size
         || !dw2_text_address_to_offset(psx_address, ram_size, &offset))
      return false;
   out[0] = '\0';

   while (index < DW2_TEXT_MAX_INPUT_BYTES && offset + index < ram_size)
   {
      uint8_t value = main_ram[offset + index];
      size_t command_size = 0;

      if (value < 0xef)
      {
         if (!active)
         {
            /* Native message resources prefix the visible speaker row with
             * lowercase q followed immediately by an uppercase glyph. */
            if (value != 0x34u
                  || index + 1u >= DW2_TEXT_MAX_INPUT_BYTES
                  || offset + index + 1u >= ram_size
                  || main_ram[offset + index + 1u] < 0x0au
                  || main_ram[offset + index + 1u] > 0x23u)
               return false;
            active = true;
            index++;
            continue;
         }
         if (pending_space && out_len
               && !dw2_text_append_char(out, out_size, &out_len, ' '))
            return false;
         pending_space = false;
         if (!dw2_text_append_encoded_char(out, out_size, &out_len, value))
            return false;
         index++;
         continue;
      }

      if (value == 0xfd)
      {
         if (active && out_len)
            pending_space = true;
         index++;
         continue;
      }

      if (active && (value == 0xf4 || value == 0xfe || value == 0xff
            || value == 0xfb || value == 0xfc))
      {
         dw2_text_trim(out);
         return dw2_decoder_has_speech(out);
      }
      if (!active && (value == 0xfe || value == 0xfb || value == 0xfc))
      {
         index++;
         continue;
      }
      if (value == 0xff)
         return false;

      if (value == 0xf0)
      {
         bool fragment_appended = false;

         if (index + 2u > DW2_TEXT_MAX_INPUT_BYTES
               || offset + index + 2u > ram_size)
            return false;
         if (active)
         {
            if (pending_space && out_len
                  && !dw2_text_append_char(out, out_size, &out_len, ' '))
               return false;
            pending_space = false;
            if (!dw2_text_append_token(main_ram, ram_size,
                     main_ram[offset + index + 1u], NULL, 0,
                     BEETLE_DW2_TEXT_DIALOG, 0, out, out_size, &out_len,
                     &fragment_appended))
               return false;
         }
         index += 2u;
         continue;
      }

      switch (value)
      {
         case 0xef:
         case 0xf3:
         case 0xf8:
         case 0xfa:
            command_size = 2u;
            break;
         case 0xf1:
         case 0xf2:
            command_size = 4u;
            break;
         case 0xf4:
            if (index + 2u > DW2_TEXT_MAX_INPUT_BYTES
                  || offset + index + 2u > ram_size)
               return false;
            command_size = main_ram[offset + index + 1u] < 0x30u
               ? 5u : 2u;
            /* Native speaker colour command, including compressed names
             * such as F0 25 (MasterTyrannomon). The 34 byte is an operand,
             * not a literal lowercase q before the first name glyph. */
            if (main_ram[offset + index + 1u] == 0x34u)
               active = true;
            break;
         case 0xf5:
            command_size = 1u;
            break;
         case 0xf6:
         case 0xf7:
            command_size = 10u;
            break;
         case 0xf9:
            if (index + 2u > DW2_TEXT_MAX_INPUT_BYTES
                  || offset + index + 2u > ram_size)
               return false;
            command_size = (main_ram[offset + index + 1u] & 1u)
               ? 2u : 5u;
            break;
         default:
            return false;
      }

      if (index + command_size > DW2_TEXT_MAX_INPUT_BYTES
            || offset + index + command_size > ram_size)
         return false;
      if ((value == 0xf1 || value == 0xf2)
            && !dw2_text_digits_valid(main_ram + offset + index + 1u, 3u))
         return false;
      if (value == 0xf4 && command_size == 5u
            && !dw2_text_digits_valid(main_ram + offset + index + 2u, 3u))
         return false;
      if ((value == 0xf6 || value == 0xf7)
            && !dw2_text_digits_valid(main_ram + offset + index + 1u, 9u))
         return false;
      if (value == 0xf9 && command_size == 5u
            && !dw2_text_digits_valid(main_ram + offset + index + 2u, 3u))
         return false;
      index += command_size;
   }

   return false;
}

bool beetle_accessibility_dw2_decode_formatted_text(
      const uint8_t *main_ram, size_t ram_size, uint32_t psx_address,
      const uint32_t *arguments, size_t argument_count,
      beetle_dw2_text_mode_t mode, char *out, size_t out_size)
{
   return dw2_text_decode_internal(main_ram, ram_size, psx_address,
      arguments, argument_count, mode, false, 0, out, out_size, NULL, 0);
}

bool beetle_accessibility_dw2_decode_text_fragment(
      const uint8_t *main_ram, size_t ram_size, uint32_t psx_address,
      beetle_dw2_text_mode_t mode, char *out, size_t out_size)
{
   return dw2_text_decode_internal(main_ram, ram_size, psx_address,
         NULL, 0, mode, true, 0, out, out_size, NULL, 0);
}

static bool dw2_text_binary_choice(
      const uint8_t *main_ram, size_t ram_size, uint32_t psx_address,
      uint8_t choice, uint32_t *choice_address)
{
   size_t offset;
   size_t index = 0;
   bool saw_yes = false;
   bool saw_no = false;
   bool saw_end = false;
   bool saw_body = false;

   if (!main_ram
         || !dw2_text_address_to_offset(psx_address, ram_size, &offset))
      return false;

   while (index < DW2_TEXT_MAX_INPUT_BYTES && offset + index < ram_size)
   {
      uint8_t value = main_ram[offset + index];
      size_t command_size;

      if (value == 0xff)
         return saw_yes && saw_no && saw_end;
      if (value == 0xfb || value == 0xfc)
      {
         /* FUN_8001A9C8 advances its live text pointer at FC, and the dialog
          * decoder treats either marker after visible content as the current
          * segment boundary. Never borrow F8 choices from the next segment. */
         if (saw_body)
            return saw_yes && saw_no && saw_end;
         index++;
         continue;
      }
      if (value < 0xef || value == 0xfd || value == 0xfe)
      {
         if (value < 0xef)
            saw_body = true;
         index++;
         continue;
      }

      switch (value)
      {
         case 0xef:
         case 0xf3:
         case 0xfa:
            command_size = 2;
            break;
         case 0xf0:
            command_size = 2;
            saw_body = true;
            break;
         case 0xf1:
         case 0xf2:
            command_size = 4;
            saw_body = true;
            break;
         case 0xf4:
            if (index + 2 > DW2_TEXT_MAX_INPUT_BYTES
                  || offset + index + 2 > ram_size)
               return false;
            command_size = main_ram[offset + index + 1] < 0x30 ? 5 : 2;
            break;
         case 0xf5:
            command_size = 1;
            break;
         case 0xf6:
         case 0xf7:
            command_size = 10;
            break;
         case 0xf8:
            command_size = 2;
            if (index + command_size > DW2_TEXT_MAX_INPUT_BYTES
                  || offset + index + command_size > ram_size)
               return false;
            value = main_ram[offset + index + 1];
            if (choice_address && value == choice + 1u)
               *choice_address = psx_address + (uint32_t)index + 2u;
            if (value == 1)
               saw_yes = true;
            else if (value == 2)
               saw_no = true;
            else if (value == 0)
               saw_end = true;
            break;
         case 0xf9:
            if (index + 2 > DW2_TEXT_MAX_INPUT_BYTES
                  || offset + index + 2 > ram_size)
               return false;
            command_size = (main_ram[offset + index + 1] & 1) ? 2 : 5;
            break;
         default:
            return false;
      }

      if (index + command_size > DW2_TEXT_MAX_INPUT_BYTES
            || offset + index + command_size > ram_size)
         return false;
      index += command_size;
   }
   return false;
}

bool beetle_accessibility_dw2_text_is_yes_no_prompt(
      const uint8_t *main_ram, size_t ram_size, uint32_t psx_address)
{
   return dw2_text_binary_choice(main_ram, ram_size, psx_address, 0, NULL);
}

bool beetle_accessibility_dw2_decode_choice(
      const uint8_t *main_ram, size_t ram_size, uint32_t psx_address,
      uint8_t choice, char *out, size_t out_size)
{
   uint32_t address = 0;
   if (choice > 1 || !dw2_text_binary_choice(main_ram, ram_size,
            psx_address, choice, &address) || !address
         || !dw2_text_decode_internal(main_ram, ram_size, address, NULL, 0,
            DW2_TEXT_CHOICE_MODE, false, 0, out, out_size, NULL, 0))
      return false;
   if (!strcmp(out, "YES"))
      strcpy(out, "Yes");
   else if (!strcmp(out, "NO"))
      strcpy(out, "No");
   return true;
}
