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

#include <ofono/plugin.h>
#include <ofono/voicecall-filter.h>

#include "blocker.h"

static VcfBlocker* _blocker = NULL;

static void _cancel(unsigned int id)
{
}

static unsigned int _dial(struct ofono_voicecall *vc,
                          const struct ofono_phone_number *number,
                          enum ofono_clir_option clir,
                          ofono_voicecall_filter_dial_cb_t cb,
                          void *data)
{
    // No op
    if (cb) {
        cb(OFONO_VOICECALL_FILTER_DIAL_CONTINUE, data);
    }
    return 0;
}

static unsigned int _incoming(struct ofono_voicecall *vc,
                              const struct ofono_call *call,
                              ofono_voicecall_filter_incoming_cb_t cb,
                              void *data)
{
    DBG("voicecall-filter: getting an incoming call.");
    if (cb) {
        cb(vcf_blocker_evaluate(_blocker, &call->phone_number), data);
    }
    return 0;
}

static struct ofono_voicecall_filter _filter;

static int _init_voicecall_filter(void)
{   
    _filter.name = "number-based-filter";
    _filter.api_version = 0;
    _filter.priority = OFONO_VOICECALL_FILTER_PRIORITY_DEFAULT;
    _filter.filter_cancel = _cancel;
    _filter.filter_dial = _dial;
    _filter.filter_incoming = _incoming;

    _blocker = vcf_blocker_new();

    DBG("Initialising voicecall filter plugin.");

    return ofono_voicecall_filter_register(&_filter);
}

static void _exit_voicecall_filter(void)
{
    DBG("De-initialising voicecall filter plugin.");

    ofono_voicecall_filter_unregister(&_filter);
    g_object_unref(_blocker);
}

OFONO_PLUGIN_DEFINE("voicecall-filter", "block incoming calls based on numbers",
                    VERSION, OFONO_PLUGIN_PRIORITY_DEFAULT,
                    _init_voicecall_filter, _exit_voicecall_filter)
