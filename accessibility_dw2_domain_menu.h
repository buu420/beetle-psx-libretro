#ifndef BEETLE_PSX_ACCESSIBILITY_DW2_DOMAIN_MENU_H
#define BEETLE_PSX_ACCESSIBILITY_DW2_DOMAIN_MENU_H

#include <stddef.h>
#include <stdint.h>

/* Domain (STAG4000) inspection and gift windows.
 *
 * Square runs 800681bc, which collects every wild Digimon entity on the floor
 * into the domain state and enters controller state 0x1a; that state owns the
 * 0x20c info window (800671f0). Cross from there, and the object paths in
 * 800682dc, enter state 0x11, which owns the 0x20b item list (80066e48).
 *
 * The poll reads those two task allocations only. It never writes RAM and
 * never reports anything the native code does not draw. Call it once per
 * frame between beetle_accessibility_dw2_menu_begin_frame() and
 * beetle_accessibility_dw2_menu_end_frame(); arbitration, stability and
 * de-duplication stay in accessibility_dw2_menu.c. */
void beetle_accessibility_dw2_domain_menu_poll(const uint8_t *ram,
      size_t ram_size, uint32_t overlay_tag);

#endif
