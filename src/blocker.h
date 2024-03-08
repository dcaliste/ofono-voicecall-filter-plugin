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

#ifndef BLOCKER_H
#define BLOCKER_H

#include <glib-object.h>
#include <ofono/voicecall-filter.h>

#define VCF_TYPE_BLOCKER vcf_blocker_get_type()
G_DECLARE_DERIVABLE_TYPE(VcfBlocker, vcf_blocker, VCF, BLOCKER, GObject)

struct _VcfBlockerClass {
    GObjectClass parent;
};

VcfBlocker* vcf_blocker_new(void);

enum ofono_voicecall_filter_incoming_result vcf_blocker_evaluate(VcfBlocker *blocker,
                                                                 const struct ofono_phone_number *number);

gchar* const* vcf_blocker_get_ignored_numbers(VcfBlocker *blocker);
gchar* const* vcf_blocker_get_blocked_numbers(VcfBlocker *blocker);

#endif
