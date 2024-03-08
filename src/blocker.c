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
    gboolean dispose_has_run;

    GThread *worker;
    GMainLoop *loop;
    GSource *quit;

    GMutex mutex;
    gchar **ignored_numbers;
    gchar **blocked_numbers;
    guint ignored_id, blocked_id;
};

static gpointer _start_worker(gpointer data);
static void _get_property(GObject* obj, guint property_id,
                          GValue *value, GParamSpec *pspec);
static void _dispose(GObject* obj);
static void _finalize(GObject* obj);

G_DEFINE_TYPE_WITH_CODE(VcfBlocker, vcf_blocker, G_TYPE_OBJECT,
                        G_ADD_PRIVATE(VcfBlocker))

static void vcf_blocker_class_init(VcfBlockerClass *klass)
{
    G_OBJECT_CLASS(klass)->dispose  = _dispose;
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

static void vcf_blocker_init(VcfBlocker *obj)
{
    GError *error = NULL;
    VcfBlockerPrivate *priv = vcf_blocker_get_instance_private(obj);

    priv->dispose_has_run = FALSE;

    g_mutex_init(&priv->mutex);
    priv->ignored_numbers = NULL;
    priv->blocked_numbers = NULL;
    priv->ignored_id = 0;
    priv->blocked_id = 0;

    priv->loop = NULL;
    priv->quit = NULL;
    priv->worker = g_thread_try_new("filter-database-worker", _start_worker,
                                    obj, &error);
    if (error) {
        ofono_warn("cannot create database worker: %s", error->message);
        g_error_free(error);
    }
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

static void _dispose(GObject* obj)
{
    VcfBlockerPrivate *priv = vcf_blocker_get_instance_private(VCF_BLOCKER(obj));

    g_return_if_fail(priv);
    if (priv->dispose_has_run)
        return;

    g_mutex_lock(&priv->mutex);
    priv->dispose_has_run = TRUE;

    if (priv->loop && priv->quit) {
        g_source_attach(priv->quit, g_main_loop_get_context(priv->loop));
    }
    g_mutex_unlock(&priv->mutex);

    if (priv->worker) {
        g_thread_join(priv->worker);
    }

    if (priv->ignored_id > 0) {
        g_source_remove(priv->ignored_id);
    }
    if (priv->blocked_id > 0) {
        g_source_remove(priv->blocked_id);
    }

    G_OBJECT_CLASS(vcf_blocker_parent_class)->dispose(obj);
}

static void _finalize(GObject* obj)
{
    VcfBlockerPrivate *priv = vcf_blocker_get_instance_private(VCF_BLOCKER(obj));

    g_strfreev(priv->ignored_numbers);
    g_strfreev(priv->blocked_numbers);
    g_mutex_clear(&priv->mutex);

    G_OBJECT_CLASS(vcf_blocker_parent_class)->finalize(obj);
}

static gboolean _notify_ignored_numbers(gpointer data)
{
    VcfBlockerPrivate *priv = vcf_blocker_get_instance_private(VCF_BLOCKER(data));

    g_object_notify_by_pspec(G_OBJECT(data), _properties[IGNORED_PROP]);
    g_mutex_lock(&priv->mutex);
    priv->ignored_id = 0;
    g_mutex_unlock(&priv->mutex);
    return G_SOURCE_REMOVE;
}

static void _set_ignored_numbers(GSettings *obj, gchar *key, gpointer data)
{
    VcfBlockerPrivate *priv = vcf_blocker_get_instance_private(VCF_BLOCKER(data));
    
    g_mutex_lock(&priv->mutex);
    g_strfreev(priv->ignored_numbers);
    priv->ignored_numbers = g_settings_get_strv(obj, key);
    if (!priv->ignored_id) {
        priv->ignored_id = g_idle_add(_notify_ignored_numbers, data);
    }
    g_mutex_unlock(&priv->mutex);
}

static gboolean _notify_blocked_numbers(gpointer data)
{
    VcfBlockerPrivate *priv = vcf_blocker_get_instance_private(VCF_BLOCKER(data));

    g_object_notify_by_pspec(G_OBJECT(data), _properties[BLOCKED_PROP]);
    g_mutex_lock(&priv->mutex);
    priv->blocked_id = 0;
    g_mutex_unlock(&priv->mutex);
    return G_SOURCE_REMOVE;
}

static void _set_blocked_numbers(GSettings *obj, gchar *key, gpointer data)
{
    VcfBlockerPrivate *priv = vcf_blocker_get_instance_private(VCF_BLOCKER(data));
    
    g_mutex_lock(&priv->mutex);
    g_strfreev(priv->blocked_numbers);
    priv->blocked_numbers = g_settings_get_strv(obj, key);
    if (!priv->blocked_id) {
        priv->blocked_id = g_idle_add(_notify_blocked_numbers, data);
    }
    g_mutex_unlock(&priv->mutex);
}

static gpointer _start_worker(gpointer data)
{
    VcfBlockerPrivate *priv = vcf_blocker_get_instance_private(VCF_BLOCKER(data));
    GSettings *settings = g_settings_new("sailfish.voicecall.filter");

    g_signal_connect(settings, "changed::ignored-numbers",
                     G_CALLBACK(_set_ignored_numbers), data);
    _set_ignored_numbers(settings, "ignored-numbers", data);
    g_signal_connect(settings, "changed::blocked-numbers",
                     G_CALLBACK(_set_blocked_numbers), data);    
    _set_blocked_numbers(settings, "blocked-numbers", data);

    g_mutex_lock(&priv->mutex);
    priv->quit = g_idle_source_new();
    priv->loop = g_main_loop_new(g_main_context_get_thread_default(), TRUE);
    g_source_set_callback(priv->quit, G_SOURCE_FUNC(g_main_loop_quit),
                          priv->loop, NULL);
    g_mutex_unlock(&priv->mutex);

    if (!priv->dispose_has_run) {
        g_main_loop_run(priv->loop);
    }

    g_source_destroy(priv->quit);
    priv->quit = NULL;
    g_main_loop_unref(priv->loop);
    priv->loop = NULL;

    g_object_unref(settings);

    return NULL;
}

VcfBlocker* vcf_blocker_new(void)
{
    return g_object_new(VCF_TYPE_BLOCKER, NULL);
}

static gboolean _matchNumber(gchar *filters[], const gchar *number)
{
    for (guint i = 0; filters && filters[i]; i++) {
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
    g_mutex_lock(&priv->mutex);
    if (_matchNumber(priv->ignored_numbers, buffer)) {
        result = OFONO_VOICECALL_FILTER_INCOMING_IGNORE;
    } else if (_matchNumber(priv->blocked_numbers, buffer)) {
        result = OFONO_VOICECALL_FILTER_INCOMING_HANGUP;
    }
    g_mutex_unlock(&priv->mutex);
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
