#include "accessibility_speech.h"

#include "accessibility_trace.h"
#include "libretro.h"
#include "libretro_cbs.h"

bool beetle_accessibility_speak(const char *text, int priority,
      const char *channel)
{
   struct retro_accessibility_speech speech;
   struct retro_message_ext msg_ext;
   struct retro_message msg;
   unsigned osd_priority = priority > 0 ? (unsigned)priority : 0;

   if (!environ_cb || !text || !*text)
   {
      beetle_accessibility_trace_speech(text, priority, channel, "invalid");
      return false;
   }

   speech.text     = text;
   speech.priority = priority;
   speech.channel  = channel;
   speech.flags    = 0;

   if (environ_cb(RETRO_ENVIRONMENT_ACCESSIBILITY_SPEAK, &speech))
   {
      beetle_accessibility_trace_speech(text, priority, channel,
            "native_accessibility");
      return true;
   }

   msg_ext.msg      = text;
   msg_ext.duration = 3000;
   msg_ext.priority = osd_priority;
   msg_ext.level    = RETRO_LOG_INFO;
   msg_ext.target   = RETRO_MESSAGE_TARGET_ALL;
   msg_ext.type     = RETRO_MESSAGE_TYPE_NOTIFICATION;
   msg_ext.progress = -1;

   if (environ_cb(RETRO_ENVIRONMENT_SET_MESSAGE_EXT, &msg_ext))
   {
      beetle_accessibility_trace_speech(text, priority, channel,
            "message_ext_fallback");
      return true;
   }

   msg.msg    = text;
   msg.frames = 180;

   if (environ_cb(RETRO_ENVIRONMENT_SET_MESSAGE, &msg))
   {
      beetle_accessibility_trace_speech(text, priority, channel,
            "message_fallback");
      return true;
   }

   beetle_accessibility_trace_speech(text, priority, channel, "failed");
   return false;
}
