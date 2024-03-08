/*
 *
 *  oFono voice call filter
 *
 *  Copyright (C) 2024  Damien Caliste <dcaliste@free.fr>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <ofono/voicecall-filter.h>
#include <ofono/types.h>

#include <string.h>

int ofono_voicecall_filter_register(const struct ofono_voicecall_filter *f)
{
    return 0;
}

void ofono_voicecall_filter_unregister(const struct ofono_voicecall_filter *f)
{
}

const char *ofono_phone_number_to_string(const struct ofono_phone_number *ph,
			char buffer[/* OFONO_PHONE_NUMBER_BUFFER_SIZE
                                       */])
{
    if (ph->type == 145 && (strlen(ph->number) > 0) &&
        ph->number[0] != '+') {
        buffer[0] = '+';
        strncpy(buffer + 1, ph->number, OFONO_MAX_PHONE_NUMBER_LENGTH);
        buffer[OFONO_MAX_PHONE_NUMBER_LENGTH + 1] = '\0';
    } else {
        strncpy(buffer, ph->number, OFONO_MAX_PHONE_NUMBER_LENGTH + 1);
        buffer[OFONO_MAX_PHONE_NUMBER_LENGTH + 1] = '\0';
    }

    return buffer;
}

void ofono_dbg()
{
}

void ofono_warn()
{
}
