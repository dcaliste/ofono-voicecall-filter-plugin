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

#include <glib.h>
#include <gio/gio.h>
#include <stdlib.h>

#include "blocker.h"

static void _test_empty(void)
{
    VcfBlocker *blocker = vcf_blocker_new();
    struct ofono_phone_number number;

    g_assert_true(VCF_IS_BLOCKER(blocker));
    
    g_assert_null(vcf_blocker_get_ignored_numbers(blocker));
    g_assert_null(vcf_blocker_get_blocked_numbers(blocker));

    number.type = 0;
    strncpy(number.number, "0123456789", OFONO_MAX_PHONE_NUMBER_LENGTH);
    g_assert_cmpint(vcf_blocker_evaluate(blocker, &number), ==, OFONO_VOICECALL_FILTER_INCOMING_CONTINUE);

    number.type = 0;
    strncpy(number.number, "+33123456789", OFONO_MAX_PHONE_NUMBER_LENGTH);
    g_assert_cmpint(vcf_blocker_evaluate(blocker, &number), ==, OFONO_VOICECALL_FILTER_INCOMING_CONTINUE);

    number.type = 145;
    strncpy(number.number, "33123456789", OFONO_MAX_PHONE_NUMBER_LENGTH);
    g_assert_cmpint(vcf_blocker_evaluate(blocker, &number), ==, OFONO_VOICECALL_FILTER_INCOMING_CONTINUE);

    g_object_unref(blocker);
}

struct SignalSpy
{
    gpointer obj;
    guint id;
    guint triggered;

    GMainLoop *loop;
};

static void _triggered(struct SignalSpy *spy)
{
    spy->triggered += 1;
    if (spy->loop) {
        g_main_loop_quit(spy->loop);
    }
}

static void signal_spy_new(struct SignalSpy *spy, gpointer obj, const gchar *signal)
{
    spy->obj = obj;
    spy->id = g_signal_connect_swapped(obj, signal,
                                       G_CALLBACK(_triggered), spy);
    spy->triggered = 0;
    spy->loop = NULL;
}

static void signal_spy_free(struct SignalSpy *spy)
{
    g_signal_handler_disconnect(spy->obj, spy->id);
}

static gboolean _timeout_triggered(gpointer data)
{
    gboolean *result = (gboolean*)data;
    *result = FALSE;

    return G_SOURCE_REMOVE;
}

static gboolean _wait(struct SignalSpy *spy, guint timeout)
{
    GMainLoop *loop = g_main_loop_new(NULL, TRUE);
    gboolean result = TRUE;

    spy->loop = g_main_loop_ref(loop);
    g_timeout_add(timeout, _timeout_triggered, &result);
    g_timeout_add(timeout, G_SOURCE_FUNC(g_main_loop_quit), loop);
    g_main_loop_run(loop);
    g_main_loop_unref(loop);
    g_main_loop_unref(spy->loop);
    spy->loop = NULL;

    return result;
}

static void _test_ignored(void)
{
    VcfBlocker *blocker = vcf_blocker_new();
    GSettings *settings = g_settings_new("sailfish.voicecall.filter");
    struct ofono_phone_number number;
    const gchar *ignored[] = {"0123456789", "+33123456789", NULL};
    struct SignalSpy notify;

    g_assert_true(VCF_IS_BLOCKER(blocker));
    signal_spy_new(&notify, blocker, "notify::ignored-numbers");
    g_assert_true(_wait(&notify, 3000));
    g_assert_cmpint(notify.triggered, ==, 1);
    
    g_assert_null(vcf_blocker_get_ignored_numbers(blocker));
    g_assert_null(vcf_blocker_get_blocked_numbers(blocker));

    g_settings_set_strv(settings, "ignored-numbers", ignored);
    g_assert_true(_wait(&notify, 3000));
    g_assert_cmpint(notify.triggered, ==, 2);
    signal_spy_free(&notify);
    g_assert_nonnull(vcf_blocker_get_ignored_numbers(blocker));
    g_assert_null(vcf_blocker_get_blocked_numbers(blocker));

    number.type = 0;
    strncpy(number.number, "0123456789", OFONO_MAX_PHONE_NUMBER_LENGTH);
    g_assert_cmpint(vcf_blocker_evaluate(blocker, &number), ==, OFONO_VOICECALL_FILTER_INCOMING_IGNORE);

    number.type = 0;
    strncpy(number.number, "+33123456789", OFONO_MAX_PHONE_NUMBER_LENGTH);
    g_assert_cmpint(vcf_blocker_evaluate(blocker, &number), ==, OFONO_VOICECALL_FILTER_INCOMING_IGNORE);

    number.type = 145;
    strncpy(number.number, "33123456789", OFONO_MAX_PHONE_NUMBER_LENGTH);
    g_assert_cmpint(vcf_blocker_evaluate(blocker, &number), ==, OFONO_VOICECALL_FILTER_INCOMING_IGNORE);

    number.type = 0;
    strncpy(number.number, "0321654987", OFONO_MAX_PHONE_NUMBER_LENGTH);
    g_assert_cmpint(vcf_blocker_evaluate(blocker, &number), ==, OFONO_VOICECALL_FILTER_INCOMING_CONTINUE);

    g_settings_set_strv(settings, "ignored-numbers", NULL);

    g_object_unref(settings);
    g_object_unref(blocker);
}

static void _test_blocked(void)
{
    VcfBlocker *blocker = vcf_blocker_new();
    GSettings *settings = g_settings_new("sailfish.voicecall.filter");
    struct ofono_phone_number number;
    const gchar *blocked[] = {"0123456789", "+33123456789", NULL};
    struct SignalSpy notify;

    g_assert_true(VCF_IS_BLOCKER(blocker));
    signal_spy_new(&notify, blocker, "notify::blocked-numbers");
    g_assert_true(_wait(&notify, 3000));
    g_assert_cmpint(notify.triggered, ==, 1);
    
    g_assert_null(vcf_blocker_get_ignored_numbers(blocker));
    g_assert_null(vcf_blocker_get_blocked_numbers(blocker));

    g_settings_set_strv(settings, "blocked-numbers", blocked);
    g_assert_true(_wait(&notify, 3000));
    g_assert_cmpint(notify.triggered, ==, 2);
    signal_spy_free(&notify);
    g_assert_null(vcf_blocker_get_ignored_numbers(blocker));
    g_assert_nonnull(vcf_blocker_get_blocked_numbers(blocker));

    number.type = 0;
    strncpy(number.number, "0123456789", OFONO_MAX_PHONE_NUMBER_LENGTH);
    g_assert_cmpint(vcf_blocker_evaluate(blocker, &number), ==, OFONO_VOICECALL_FILTER_INCOMING_HANGUP);

    number.type = 0;
    strncpy(number.number, "+33123456789", OFONO_MAX_PHONE_NUMBER_LENGTH);
    g_assert_cmpint(vcf_blocker_evaluate(blocker, &number), ==, OFONO_VOICECALL_FILTER_INCOMING_HANGUP);

    number.type = 145;
    strncpy(number.number, "33123456789", OFONO_MAX_PHONE_NUMBER_LENGTH);
    g_assert_cmpint(vcf_blocker_evaluate(blocker, &number), ==, OFONO_VOICECALL_FILTER_INCOMING_HANGUP);

    number.type = 0;
    strncpy(number.number, "0321654987", OFONO_MAX_PHONE_NUMBER_LENGTH);
    g_assert_cmpint(vcf_blocker_evaluate(blocker, &number), ==, OFONO_VOICECALL_FILTER_INCOMING_CONTINUE);

    g_settings_set_strv(settings, "blocked-numbers", NULL);

    g_object_unref(settings);
    g_object_unref(blocker);
}

/**
   Run test, after compiling the schema:
   glib-compile-schemas src/
   LD_LIBRARY_PATH=src/ GSETTINGS_SCHEMA_DIR=src/ tests/tst_blocker
 */
int main(int argc, char *argv[])
{
  g_test_init(&argc, &argv, NULL);

  g_assert_null(putenv("GSETTINGS_BACKEND=memory"));

  g_test_add_func("/blocker/empty", _test_empty);
  g_test_add_func("/blocker/ignored", _test_ignored);
  g_test_add_func("/blocker/blocked", _test_blocked);

  return g_test_run ();
}
