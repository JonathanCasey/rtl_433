/** @file
    Digoo PIR / Contact Sensor.

    Copyright (C) 2021 Jonathan Casey

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/
/**
Digoo PIR / Contact Sensor.

Tested with:
- Digoo Hosa PIR
  - Also tested non-branded version
- Digoo Hosa Contact Sensor
  - Events: open
  - Also tested non-branded version
- Digoo Hama Contact Sensor
  - Events: open / close
- Digoo remote key FOB (only tested non-branded version)
  - Events: lock, unlock, home?, sos

Note: simple 24 bit fixed ID protocol (x1527 style) and should be handled by
the flex decoder.
There is a leading sync bit with a wide gap which runs into the preceeding
packet, it's ignored as 25th data bit.

Adapted from Karl Lattimer's work on Kerui.
*/

#include "decoder.h"

static int digoo_callback(r_device *decoder, bitbuffer_t *bitbuffer)
{
    data_t *data;
    uint8_t *b;
    int id;
    int cmd;
    char *cmd_str;

    int r = bitbuffer_find_repeated_row(bitbuffer, 1, 25);
    if (r < 0)
        return DECODE_ABORT_LENGTH;

    if (bitbuffer->bits_per_row[r] != 25)
        return DECODE_ABORT_LENGTH;
    b = bitbuffer->bb[r];

    id = (b[0] << 12) | (b[1] << 4) | (b[2] >> 4);
    cmd = b[2] & 0x0F;
    switch (cmd) {
        case 0xa: cmd_str = "motion"; break;
        case 0x9: cmd_str = "open"; break;
        case 0xc: cmd_str = "open"; break;
        case 0x6: cmd_str = "close"; break;
        case 0xe: cmd_str = "locked"; break;
        case 0xd: cmd_str = "unlocked"; break;
        case 0xb: cmd_str = "home"; break;
        case 0x7: cmd_str = "sos"; break;
        default:  cmd_str = "unknown"; break;
    }

    if (!cmd_str)
        return DECODE_ABORT_EARLY;

    /* clang-format off */
    data = data_make(
            "model",        "",                 DATA_STRING, "Digoo-Security",
            "id",           "ID (20bit)",       DATA_FORMAT, "0x%x", DATA_INT, id,
            "cmd",          "Command (4bit)",   DATA_FORMAT, "0x%x", DATA_INT, cmd,
            "motion",       "",                 DATA_COND, cmd == 0xa, DATA_INT, 1,
            "opened",       "",                 DATA_COND, cmd == 0x9, DATA_INT, 1,
            "opened",       "",                 DATA_COND, cmd == 0xc, DATA_INT, 1,
            "opened",       "",                 DATA_COND, cmd == 0x6, DATA_INT, 0,
            "lock",         "",                 DATA_COND, cmd == 0xe, DATA_INT, 1,
            "unlock",       "",                 DATA_COND, cmd == 0xd, DATA_INT, 1,
            "home",         "",                 DATA_COND, cmd == 0xb, DATA_INT, 1,
            "sos",          "",                 DATA_COND, cmd == 0x7, DATA_INT, 1,
            "state",        "State",            DATA_STRING, cmd_str,
            NULL);
    /* clang-format on */

    decoder_output_data(decoder, data);
    return 1;
}

static char *output_fields[] = {
        "model",
        "id",
        "cmd",
        "motion",
        "opened",
        "lock",
        "unlock",
        "home",
        "sos",
        "state",
        NULL,
};

r_device digoo = {
        .name        = "Digoo PIR / Contact Sensor",
        .modulation  = OOK_PULSE_PWM,
        .short_width = 400,
        .long_width  = 1200,
        .gap_limit   = 1500,
        .reset_limit = 9900,
        .tolerance   = 160,
        .decode_fn   = &digoo_callback,
        .fields      = output_fields,
};
