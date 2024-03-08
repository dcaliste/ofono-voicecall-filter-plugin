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

#include <ofono/log.h>
#include <ofono/misc.h>

#include <gio/gio.h>

#include "blocker.h"

enum
  {
    PROP_0,
    IGNORED_PROP,
    BLOCKED_PROP,
    N_PROP
  };
static GParamSpec *_properties[N_PROP];

typedef struct _VcfBlockerPrivate VcfBlockerPrivate;
struct _VcfBlockerPrivate
{
    GSettings *settings;

    gchar **ignored_numbers;
    gchar **blocked_numbers;
};

static void _get_property(GObject* obj, guint property_id,
                          GValue *value, GParamSpec *pspec);
static void _finalize(GObject* obj);

G_DEFINE_TYPE_WITH_CODE(VcfBlocker, vcf_blocker, G_TYPE_OBJECT,
                        G_ADD_PRIVATE(VcfBlocker))

static void vcf_blocker_class_init(VcfBlockerClass *klass)
{
    G_OBJECT_CLASS(klass)->finalize  = _finalize;
    G_OBJECT_CLASS(klass)->get_property = _get_property;

    _properties[IGNORED_PROP] = g_param_spec_boxed("ignored-numbers",
                                                   "Ignored numbers",
                                                   "give no answer to these numbers",
                                                   G_TYPE_STRV, G_PARAM_READABLE);
    _properties[BLOCKED_PROP] = g_param_spec_boxed("blocked-numbers",
                                                   "Blocked numbers",
                                                   "hangup to these numbers",
                                                   G_TYPE_STRV, G_PARAM_READABLE);
    
    g_object_class_install_properties(G_OBJECT_CLASS(klass), N_PROP, _properties);
}

static void _set_ignored_numbers(GSettings *obj, gchar *key, gpointer data)
{
    VcfBlockerPrivate *priv = vcf_blocker_get_instance_private(VCF_BLOCKER(data));

    g_strfreev(priv->ignored_numbers);
    priv->ignored_numbers = g_settings_get_strv(obj, key);
    g_object_notify_by_pspec(G_OBJECT(data), _properties[IGNORED_PROP]);
}

static void _set_blocked_numbers(GSettings *obj, gchar *key, gpointer data)
{
    VcfBlockerPrivate *priv = vcf_blocker_get_instance_private(VCF_BLOCKER(data));
    
    g_strfreev(priv->blocked_numbers);
    priv->blocked_numbers = g_settings_get_strv(obj, key);
    g_object_notify_by_pspec(G_OBJECT(data), _properties[BLOCKED_PROP]);
}

static void vcf_blocker_init(VcfBlocker *obj)
{
    VcfBlockerPrivate *priv = vcf_blocker_get_instance_private(obj);

    priv->settings = g_settings_new("sailfish.voicecall.filter");

    g_signal_connect(priv->settings, "changed::ignored-numbers",
                     G_CALLBACK(_set_ignored_numbers), obj);
    g_signal_connect(priv->settings, "changed::blocked-numbers",
                     G_CALLBACK(_set_blocked_numbers), obj);    

    priv->ignored_numbers = g_settings_get_strv(priv->settings, "ignored-numbers");
    priv->blocked_numbers = g_settings_get_strv(priv->settings, "blocked-numbers");;
}

static void _get_property(GObject* obj, guint property_id,
                          GValue *value, GParamSpec *pspec)
{
    VcfBlockerPrivate *priv = vcf_blocker_get_instance_private(VCF_BLOCKER(obj));

    switch (property_id) {
    case IGNORED_PROP:
        g_value_set_boxed(value, priv->ignored_numbers);
        break;
    case BLOCKED_PROP:
        g_value_set_boxed(value, priv->blocked_numbers);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, property_id, pspec);
        break;
    }
}

static void _finalize(GObject* obj)
{
    VcfBlockerPrivate *priv = vcf_blocker_get_instance_private(VCF_BLOCKER(obj));

    g_strfreev(priv->ignored_numbers);
    g_strfreev(priv->blocked_numbers);

    g_object_unref(priv->settings);

    G_OBJECT_CLASS(vcf_blocker_parent_class)->finalize(obj);
}

VcfBlocker* vcf_blocker_new(void)
{
    return g_object_new(VCF_TYPE_BLOCKER, NULL);
}

static gboolean _matchNumber(gchar *filters[], const gchar *number)
{
    for (guint i = 0; filters && filters[i]; i++) {
        DBG("testing '%s' against '%s'.", number, filters[i]);
        if (!g_strcmp0(filters[i], number)) {
            return TRUE;
        }
    }
    return FALSE;
}

enum ofono_voicecall_filter_incoming_result vcf_blocker_evaluate(VcfBlocker *blocker,
                                                                 const struct ofono_phone_number *number)
{
    static char buffer[OFONO_PHONE_NUMBER_BUFFER_SIZE];
    VcfBlockerPrivate *priv = vcf_blocker_get_instance_private(blocker);
    enum ofono_voicecall_filter_incoming_result result
        = OFONO_VOICECALL_FILTER_INCOMING_CONTINUE;

    g_return_val_if_fail(VCF_IS_BLOCKER(blocker), result);

    ofono_phone_number_to_string(number, buffer);
    DBG("'%s' is calling, should I block ?", buffer);
    if (_matchNumber(priv->ignored_numbers, buffer)) {
        result = OFONO_VOICECALL_FILTER_INCOMING_IGNORE;
    } else if (_matchNumber(priv->blocked_numbers, buffer)) {
        result = OFONO_VOICECALL_FILTER_INCOMING_HANGUP;
    }
    return result;
}

gchar* const* vcf_blocker_get_ignored_numbers(VcfBlocker *blocker)
{
    VcfBlockerPrivate *priv = vcf_blocker_get_instance_private(blocker);
    g_return_val_if_fail(VCF_IS_BLOCKER(blocker), NULL);

    return priv->ignored_numbers && !priv->ignored_numbers[0] ? NULL : priv->ignored_numbers;
}

gchar* const* vcf_blocker_get_blocked_numbers(VcfBlocker *blocker)
{
    VcfBlockerPrivate *priv = vcf_blocker_get_instance_private(blocker);
    g_return_val_if_fail(VCF_IS_BLOCKER(blocker), NULL);

    return priv->blocked_numbers && !priv->blocked_numbers[0] ? NULL : priv->blocked_numbers;
}
