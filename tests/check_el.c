/**
 * Copyright (C) 2005-06 Nokia Corporation.
 * Contact: Naba Kumar <naba.kumar@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

/**
 * @example check_el.c
 * Shows how to initialize the framework.
 */

#include "rtcom-eventlogger/eventlogger.h"

#include <glib.h>
#include <glib/gstdio.h>

#include <check.h>

#include <sqlite3.h>

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "canned-data.h"

#define SERVICE "RTCOM_EL_SERVICE_TEST"
#define EVENT_TYPE "RTCOM_EL_EVENTTYPE_TEST_ET1"
#define FLAGS 0
#define BYTES_SENT 10
#define BYTES_RECEIVED 9
#define REMOTE_EBOOK_UID "ebook-uid-1"
#define LOCAL_UID "ext-salvatore.iovene@nokia.com"
#define LOCAL_NAME "Salvatore Iovene"

/* FIXME: OMG, syntax! */
#define REMOTE_UID "1@foo.org"
#define REMOTE_NAME "1,2"

#define CHANNEL "chavo"
#define FREE_TEXT "Test free_text"

#define HEADER_KEY "Foo"
#define HEADER_VAL "Bar"

#define ATTACH_DESC "Foo attachment."

#define REMOTE_EBOOK_UID_2 "ebook-uid-2"
#define REMOTE_UID_2 "ext-salvatore.iovene-2@nokia.com"
#define REMOTE_NAME_2 "Salvatore Iovene 2"

#define REMOTE_EBOOK_UID_3 "ebook-uid-3"
#define REMOTE_UID_3 "ext-salvatore.iovene-3@nokia.com"
#define REMOTE_NAME_3 "Salvatore Iovene 3"

static RTComEl *el = NULL;

static RTComElEvent *
event_new_lite (void)
{
    RTComElEvent *ev;

    ev = rtcom_el_event_new();
    g_return_val_if_fail (ev != NULL, NULL);

    RTCOM_EL_EVENT_SET_FIELD(ev, service, g_strdup (SERVICE));
    RTCOM_EL_EVENT_SET_FIELD(ev, event_type, g_strdup (EVENT_TYPE));
    RTCOM_EL_EVENT_SET_FIELD(ev, local_uid, g_strdup (LOCAL_UID));
    RTCOM_EL_EVENT_SET_FIELD(ev, start_time, time(NULL));

    return ev;
}

static RTComElEvent *
event_new_full (time_t t)
{
    RTComElEvent *ev;

    ev = rtcom_el_event_new();
    g_return_val_if_fail (ev != NULL, NULL);

    /* Setting everything here for testing purposes, but usually
     * you wouldn't need to. */
    /* FIXME: RTComElEvent structure:
     * 1) OMG, it's full of string IDs that want to be quarks or enums;
     * 2) it's painful to care about string member ownership.
     */
    RTCOM_EL_EVENT_SET_FIELD(ev, service, g_strdup (SERVICE));
    RTCOM_EL_EVENT_SET_FIELD(ev, event_type, g_strdup (EVENT_TYPE));
    RTCOM_EL_EVENT_SET_FIELD(ev, start_time, t);
    RTCOM_EL_EVENT_SET_FIELD(ev, end_time, t);
    RTCOM_EL_EVENT_SET_FIELD(ev, flags, FLAGS);
    RTCOM_EL_EVENT_SET_FIELD(ev, bytes_sent, BYTES_SENT);
    RTCOM_EL_EVENT_SET_FIELD(ev, bytes_received, BYTES_RECEIVED);
    RTCOM_EL_EVENT_SET_FIELD(ev, local_uid, g_strdup (LOCAL_UID));
    RTCOM_EL_EVENT_SET_FIELD(ev, local_name, g_strdup (LOCAL_NAME));
    RTCOM_EL_EVENT_SET_FIELD(ev, remote_uid, g_strdup (REMOTE_UID));
    RTCOM_EL_EVENT_SET_FIELD(ev, remote_name, g_strdup (REMOTE_NAME));
    RTCOM_EL_EVENT_SET_FIELD(ev, channel, g_strdup (CHANNEL));
    RTCOM_EL_EVENT_SET_FIELD(ev, free_text, g_strdup (FREE_TEXT));

    return ev;
}

static void
core_setup (void)
{
#if !GLIB_CHECK_VERSION(2,35,0)
    g_type_init ();
#endif
    const gchar *home;
    gchar *fn;

    home = g_getenv ("RTCOM_EL_HOME");
    if (!home)
        home = g_get_home_dir();

    fn = g_build_filename (home, ".rtcom-eventlogger", "el-v1.db", NULL);
    g_unlink (fn);
    g_free (fn);

    /* Make a new EL instance for the tests (this will repopulate the DB) */
    el = rtcom_el_new ();
    g_assert (el != NULL);

    /* Push in some canned data (the service-specific plugins are able to
     * provide more realistic data) */
    add_canned_events (el);
}

static void
core_teardown (void)
{
    /* Leave the tables in place, so el.db can be inspected */
    g_assert (el != NULL);
    g_object_unref (el);
    el = NULL;
}

static gint
iter_count_results (RTComElIter *it)
{
    gint i = 1;

    if (it == NULL)
    {
        return 0;
    }

    if (!rtcom_el_iter_first (it))
    {
        return 0;
    }

    while (rtcom_el_iter_next (it))
    {
        i += 1;
    }

    return i;
}

START_TEST(test_add_event)
{
    RTComElQuery * query = NULL;
    RTComElEvent * ev = NULL;
    gint event_id = -1;
    gint service_id = -1;
    gint event_type_id = -1;
    RTComElIter * it = NULL;
    GHashTable * values = NULL;
    time_t t = 0;

    t = time (NULL);

    ev = event_new_full (t);
    if(!ev)
    {
        ck_abort_msg("Failed to create event.");
    }

    event_id = rtcom_el_add_event(el, ev, NULL);
    ck_assert_msg (event_id >= 0, "Failed to add event");

    query = rtcom_el_query_new(el);

    /* exercise its GObject properties */
    {
        RTComEl *q_el = NULL;
        gboolean q_is_caching = 0xDEADBEEF;
        gint q_limit = -42;
        gint q_offset = -42;
        gint q_group_by = -42;

        g_object_get (query,
                "el", &q_el,
                "is-caching", &q_is_caching,
                "limit", &q_limit,
                "offset", &q_offset,
                "group-by", &q_group_by,
                NULL);

        ck_assert (q_el == el);
        ck_assert (q_is_caching == FALSE);
        ck_assert (q_limit == -1);
        ck_assert (q_offset == 0);
        ck_assert (q_group_by == RTCOM_EL_QUERY_GROUP_BY_NONE);
    }

    if(!rtcom_el_query_prepare(
                query,
                "id", event_id, RTCOM_EL_OP_EQUAL,
                NULL))
    {
        ck_abort_msg("Failed to prepare the query.");
    }

    it = rtcom_el_get_events(el, query);
    g_object_unref(query);

    ck_assert_msg (it != NULL, "Failed to get iterator");

    ck_assert(rtcom_el_iter_get_values(it, "service-id",
        &service_id, "event-type-id", &event_type_id, NULL));

    ck_assert (rtcom_el_get_service_id(el, SERVICE) == service_id);
    ck_assert (rtcom_el_get_eventtype_id(el, EVENT_TYPE) == event_type_id);

    /* exercise its GObject properties */
    {
        gpointer el_db = NULL;
        RTComEl *it_el = NULL;
        gpointer it_query = NULL;
        gpointer it_db = NULL;
        gpointer it_stmt = NULL;
        gchar *it_sql = NULL;
        GHashTable *it_plugins = NULL;
        gboolean it_atomic = 0xDEADBEEF;

        g_object_get (el,
                "db", &el_db,
                NULL);

        g_object_get (it,
                "el", &it_el,
                "query", &it_query,
                "sqlite3-database", &it_db,
                "sqlite3-statement", &it_stmt,
                "plugins-table", &it_plugins,
                "atomic", &it_atomic,
                NULL);

        ck_assert (it_el == el);
        ck_assert (it_query != NULL);
        ck_assert (it_db != NULL);
        ck_assert (it_db == el_db);
        ck_assert (it_stmt != NULL);
        ck_assert (it_plugins != NULL);
        ck_assert (it_atomic == TRUE || it_atomic == FALSE);

        g_free (it_sql);
    }

    ck_assert_msg (rtcom_el_iter_first(it), "Failed to start iterator");

    values = rtcom_el_iter_get_value_map(
            it,
            "start-time",
            "end-time",
            "flags",
            "bytes-sent",
            "bytes-received",
            "local-uid",
            "local-name",
            "remote-uid",
            "remote-name",
            "channel",
            "free-text",
            NULL);

    if(!values)
    {
        ck_abort_msg("Failed to get values.");
    }

    ck_assert_int_eq(t,
            g_value_get_int64(g_hash_table_lookup(values, "start-time")));
    ck_assert_int_eq(t,
            g_value_get_int64(g_hash_table_lookup(values, "end-time")));

    ck_assert_int_eq(FLAGS,
            g_value_get_int(g_hash_table_lookup(values, "flags")));

    ck_assert_int_eq(BYTES_SENT,
            g_value_get_int(g_hash_table_lookup(values, "bytes-sent")));
    ck_assert_int_eq(BYTES_RECEIVED,
            g_value_get_int(g_hash_table_lookup(values, "bytes-received")));
    ck_assert_str_eq(LOCAL_UID,
            g_value_get_string(g_hash_table_lookup(values, "local-uid")));
    ck_assert_str_eq(LOCAL_NAME,
            g_value_get_string(g_hash_table_lookup(values, "local-name")));
    ck_assert_str_eq(REMOTE_UID,
            g_value_get_string(g_hash_table_lookup(values, "remote-uid")));
    ck_assert_str_eq(REMOTE_NAME,
            g_value_get_string(g_hash_table_lookup(values, "remote-name")));
    ck_assert_str_eq(CHANNEL,
            g_value_get_string(g_hash_table_lookup(values, "channel")));
    ck_assert_str_eq(FREE_TEXT,
            g_value_get_string(g_hash_table_lookup(values, "free-text")));

    ck_assert_msg(!rtcom_el_iter_next(it),
                  "Iterator should only return one row");

    g_hash_table_destroy(values);
    g_object_unref(it);
    rtcom_el_event_free_contents (ev);
    rtcom_el_event_free (ev);
}
END_TEST

START_TEST(test_add_full)
{
    const time_t time = 1000000;
    RTComElEvent *ev;
    RTComElQuery *query;
    RTComElIter *it;
    RTComElAttachIter *att_it;
    RTComElAttachment *att;
    GHashTable *headers;
    GList *attachments = NULL;
    gint fd;
    gint event_id;
    gchar *path1;
    gchar *path2;
    gchar *contents;
    gsize length;
    gchar *basename1;
    gchar *basename2;

    headers = g_hash_table_new_full (g_str_hash, g_str_equal,
            g_free, g_free);
    g_hash_table_insert (headers, g_strdup (HEADER_KEY),
            g_strdup ("add_full"));

    fd = g_file_open_tmp ("attachment1.XXXXXX", &path1, NULL);
    ck_assert (fd >= 0);
    ck_assert (path1 != NULL);
    close (fd);
    ck_assert (g_file_set_contents (path1, "some text\n", -1, NULL));

    fd = g_file_open_tmp ("attachment2.XXXXXX", &path2, NULL);
    ck_assert (fd >= 0);
    ck_assert (path2 != NULL);
    close (fd);
    ck_assert (g_file_set_contents (path2, "other text\n", -1, NULL));

    attachments = g_list_prepend (attachments,
            rtcom_el_attachment_new (path1, "some file"));
    attachments = g_list_prepend (attachments,
            rtcom_el_attachment_new (path2, NULL));

    ev = event_new_full (time);
    ck_assert_msg (ev != NULL, "Failed to create event.");

    event_id = rtcom_el_add_event_full (el, ev,
            headers, attachments, NULL);

    ck_assert_msg (event_id >= 0, "Failed to add event");

    g_unlink (path1);
    g_unlink (path2);
    g_hash_table_destroy (headers);
    headers = NULL;
    g_list_foreach (attachments, (GFunc) rtcom_el_free_attachment, NULL);
    g_list_free (attachments);
    attachments = NULL;

    /* now iterate over the attachments */

    query = rtcom_el_query_new(el);
    ck_assert (rtcom_el_query_prepare (query,
                "id", event_id, RTCOM_EL_OP_EQUAL,
                NULL));

    it = rtcom_el_get_events(el, query);
    g_object_unref(query);

    ck_assert_msg (it != NULL, "Failed to get iterator");
    ck_assert_msg (rtcom_el_iter_first(it), "Failed to start iterator");

    ck_assert (rtcom_el_iter_get_values (it, HEADER_KEY, &contents, NULL));
    ck_assert_str_eq ("add_full", contents);
    g_free (contents);

    att_it = rtcom_el_iter_get_attachments(it);
    ck_assert_msg (att_it != NULL, "Failed to get attachment iterator");

    g_object_unref (it);

    ck_assert_msg (rtcom_el_attach_iter_first(att_it),
                   "Failed to start attachment iterator");

    att = rtcom_el_attach_iter_get (att_it);
    ck_assert_msg (att != NULL, "failed to get attachment data");

    ck_assert_msg (event_id == att->event_id,
                   "attachment event ID doesn't match");
    ck_assert_str_eq (basename1 = g_path_get_basename (path2),
                              basename2 = g_path_get_basename (att->path));
    g_free(basename1);
    g_free(basename2);
    ck_assert (NULL == att->desc);
    ck_assert (g_file_get_contents (att->path, &contents, &length, NULL));
    ck_assert_uint_eq (length, strlen ("other text\n"));
    ck_assert_str_eq (contents, "other text\n");
    g_free (contents);
    rtcom_el_free_attachment (att);

    ck_assert_msg (rtcom_el_attach_iter_next (att_it),
                   "Failed to advance attachment iterator");

    att = rtcom_el_attach_iter_get (att_it);
    ck_assert_msg (att != NULL, "failed to get attachment data");

    ck_assert_msg (event_id == att->event_id,
                   "attachment event ID doesn't match");
    ck_assert_str_eq (basename1 = g_path_get_basename (path1),
                              basename2 = g_path_get_basename (att->path));
    g_free(basename1);
    g_free(basename2);
    ck_assert_str_eq ("some file", att->desc);
    ck_assert (g_file_get_contents (att->path, &contents, &length, NULL));
    ck_assert_uint_eq (length, strlen ("some text\n"));
    ck_assert_str_eq (contents, "some text\n");
    g_free (contents);
    rtcom_el_free_attachment (att);

    ck_assert (!rtcom_el_attach_iter_next (att_it));

    g_object_unref (att_it);

    g_free (path1);
    g_free (path2);

    rtcom_el_event_free_contents (ev);
    rtcom_el_event_free (ev);
}
END_TEST

START_TEST(test_header)
{
    RTComElQuery * query = NULL;
    RTComElEvent * ev = NULL;
    gint event_id = -1;
    gint header_id = -1;
    RTComElIter * it = NULL;
    GHashTable *headers;
    gchar *contents;

    ev = event_new_lite ();
    if(!ev)
    {
        ck_abort_msg("Failed to create event.");
    }

    event_id = rtcom_el_add_event(el, ev, NULL);
    ck_assert_msg (event_id >= 0, "Failed to add event");

    header_id = rtcom_el_add_header(
            el, event_id,
            HEADER_KEY,
            HEADER_VAL,
            NULL);
    ck_assert_msg (header_id >= 0, "Failed to add header");

    query = rtcom_el_query_new(el);
    if(!rtcom_el_query_prepare(
                query,
                "id", event_id, RTCOM_EL_OP_EQUAL,
                NULL))
    {
        ck_abort_msg("Failed to prepare the query.");
    }

    it = rtcom_el_get_events(el, query);
    g_object_unref(query);

    ck_assert_msg (it != NULL, "Failed to get iterator");
    ck_assert_msg (rtcom_el_iter_first(it), "Failed to start iterator");

    ck_assert(rtcom_el_iter_get_values(it, HEADER_KEY, &contents, NULL));
    ck_assert_str_eq(HEADER_VAL, contents);
    g_free (contents);

    ck_assert_msg(!rtcom_el_iter_next(it),
                  "Iterator should only return one row");

    g_object_unref(it);
    rtcom_el_event_free_contents (ev);
    rtcom_el_event_free (ev);

    headers = rtcom_el_fetch_event_headers (el, event_id);

    ck_assert (headers != NULL);
    ck_assert_int_eq (g_hash_table_size (headers), 1);
    ck_assert_str_eq (g_hash_table_lookup (headers, HEADER_KEY), HEADER_VAL);

    g_hash_table_destroy (headers);
}
END_TEST

START_TEST(test_attach)
{
    RTComElQuery * query = NULL;
    RTComElEvent * ev = NULL;
    gint event_id = -1;
    gint attachment_id = -1;
    RTComElIter * it = NULL;
    RTComElAttachIter * att_it = NULL;
    RTComElAttachment *att = NULL;
    gchar *contents;
    gsize length;
    GError *error = NULL;
    gchar *attach_path;
    gint fd;
    gchar *basename1;
    gchar *basename2;

    ev = event_new_lite ();
    ck_assert_msg (ev != NULL, "Failed to create event");

    event_id = rtcom_el_add_event(el, ev, NULL);
    ck_assert_msg (event_id >= 0, "Failed to add event");

    attachment_id = rtcom_el_add_attachment(
            el, event_id,
            "/nonexistent", ATTACH_DESC,
            &error);
    ck_assert_msg (attachment_id == -1, "Should have failed to add nonexistent "
            "attachment");
    ck_assert_uint_eq (error->domain, RTCOM_EL_ERROR);
    ck_assert_int_eq (error->code, RTCOM_EL_INTERNAL_ERROR);
    g_clear_error (&error);

    query = rtcom_el_query_new(el);
    if(!rtcom_el_query_prepare(
                query,
                "id", event_id, RTCOM_EL_OP_EQUAL,
                NULL))
    {
        ck_abort_msg("Failed to prepare the query.");
    }

    it = rtcom_el_get_events(el, query);
    g_object_unref(query);

    att_it = rtcom_el_iter_get_attachments(it);
    ck_assert_msg (att_it == NULL, "Should start with no attachments");

    g_object_unref(it);

    fd = g_file_open_tmp ("attachment.XXXXXX", &attach_path, NULL);
    ck_assert (fd >= 0);
    ck_assert (attach_path != NULL);
    close (fd);
    ck_assert (g_file_set_contents (attach_path, "lalala", 6, NULL));

    attachment_id = rtcom_el_add_attachment(
            el, event_id,
            attach_path, ATTACH_DESC,
            NULL);
    ck_assert_msg (attachment_id >= 0, "Failed to add attachment");

    g_unlink (attach_path);

    query = rtcom_el_query_new(el);
    if(!rtcom_el_query_prepare(
                query,
                "id", event_id, RTCOM_EL_OP_EQUAL,
                NULL))
    {
        ck_abort_msg("Failed to prepare the query.");
    }

    it = rtcom_el_get_events(el, query);
    g_object_unref(query);

    ck_assert_msg (it != NULL, "Failed to get iterator");
    ck_assert_msg (rtcom_el_iter_first(it), "Failed to start iterator");

    att_it = rtcom_el_iter_get_attachments(it);
    ck_assert_msg (att_it != NULL, "Failed to get attachment iterator");

    /* Exercise the attachment iterator's GObject properties in a basic way */
    {
        gpointer el_db;
        gpointer ai_db;
        gpointer ai_stmt;

        g_object_get (el,
                "db", &el_db,
                NULL);

        g_object_get (att_it,
                "sqlite3-database", &ai_db,
                "sqlite3-statement", &ai_stmt,
                NULL);

        ck_assert (ai_db != NULL);
        ck_assert (ai_db == el_db);
        ck_assert (ai_stmt != NULL);
    }

    ck_assert_msg (rtcom_el_attach_iter_first(att_it),
                   "Failed to start attachment iterator");

    att = rtcom_el_attach_iter_get (att_it);
    ck_assert_msg (att != NULL, "failed to get attachment data");

    ck_assert_msg (event_id == att->event_id,
                   "attachment event ID doesn't match");
    ck_assert_str_eq(basename1 = g_path_get_basename(attach_path),
                             basename2 = g_path_get_basename(att->path));
    g_free(basename1);
    g_free(basename2);
    ck_assert_str_eq(ATTACH_DESC, att->desc);
    ck_assert (g_file_get_contents (att->path, &contents, &length, NULL));
    ck_assert_uint_eq (length, 6);
    ck_assert_str_eq (contents, "lalala");
    g_free (contents);

    ck_assert (!rtcom_el_attach_iter_next (att_it));

    g_free (attach_path);
    rtcom_el_free_attachment (att);
    g_object_unref(att_it);
    g_object_unref(it);
    rtcom_el_event_free_contents (ev);
    rtcom_el_event_free (ev);
}
END_TEST

START_TEST(test_read)
{
    RTComElQuery * query = NULL;
    RTComElEvent * ev = NULL;
    gint event_id = -1;
    RTComElIter * it = NULL;
    gint i;
    gboolean is_read;
    gint count;
    gint ids[4] = { 0, 0, 0, 0 }; /* three IDs plus 0-terminator */

    ev = event_new_full (time (NULL));
    if(!ev)
    {
        ck_abort_msg("Failed to create event.");
    }

    event_id = rtcom_el_add_event(el, ev, NULL);
    ck_assert_msg (event_id >= 0, "Failed to add event");

    /* All events are initially unread */

    rtcom_el_set_read_event(el, event_id, TRUE, NULL);

    query = rtcom_el_query_new(el);
    rtcom_el_query_set_limit(query, 5);
    if(!rtcom_el_query_prepare(
                query,
                "is-read", TRUE, RTCOM_EL_OP_EQUAL,
                NULL))
    {
        ck_abort_msg("Failed to prepare the query.");
    }

    it = rtcom_el_get_events(el, query);
    g_object_unref(query);

    ck_assert_msg (it != NULL, "Failed to get iterator");
    ck_assert_msg (rtcom_el_iter_first(it), "Failed to start iterator");

    ck_assert(rtcom_el_iter_get_values(it, "is-read", &is_read, NULL));
    ck_assert_msg (is_read == TRUE, "is-read flag doesn't match");

    /* At this point exactly one event has is-read = TRUE */
    count = iter_count_results (it);
    ck_assert_int_eq (count, 1);

    g_object_unref(it);

    /* Mark the three most recently added events as read (that should be
     * the one we just added, plus two more) */
    query = rtcom_el_query_new (el);
    rtcom_el_query_set_limit (query, 3);
    ck_assert (rtcom_el_query_prepare(query,
                NULL));
    it = rtcom_el_get_events(el, query);
    g_object_unref (query);

    ck_assert (it != NULL);
    ck_assert (rtcom_el_iter_first (it));
    i = 0;

    for (i = 0; i < 3; i++)
    {
        if (i > 0)
        {
            ck_assert (rtcom_el_iter_next (it));
        }

        ck_assert (rtcom_el_iter_get_values (it, "id", ids + i, NULL));
    }

    ck_assert_msg (!rtcom_el_iter_next (it), "Iterator should run out after 3");

    ck_assert_int_eq (ids[0], event_id);

    g_object_unref(it);

    ck_assert_int_eq (rtcom_el_set_read_events (el, ids, TRUE, NULL), 0);

    query = rtcom_el_query_new (el);
    ck_assert (rtcom_el_query_prepare(query,
                "is-read", TRUE, RTCOM_EL_OP_EQUAL,
                NULL));
    it = rtcom_el_get_events(el, query);
    g_object_unref (query);
    count = iter_count_results (it);

    ck_assert_int_eq (count, 3);
    g_object_unref(it);

    ck_assert_int_eq (rtcom_el_set_read_events (el, ids, FALSE, NULL), 0);

    query = rtcom_el_query_new (el);
    ck_assert (rtcom_el_query_prepare(query,
                "is-read", TRUE, RTCOM_EL_OP_EQUAL,
                NULL));
    it = rtcom_el_get_events(el, query);
    g_object_unref (query);
    ck_assert_msg (it == NULL, "all read flags should have been unset");

    rtcom_el_event_free_contents (ev);
    rtcom_el_event_free (ev);
}
END_TEST

START_TEST(test_flags)
{
    RTComElQuery * query = NULL;
    RTComElEvent * ev = NULL;
    gint event_id = -1;
    RTComElIter * it = NULL;
    gint test_flag1 = 0;
    gint flags = 0;

    ev = event_new_full (time (NULL));
    if(!ev)
    {
        ck_abort_msg("Failed to create event.");
    }

    event_id = rtcom_el_add_event(el, ev, NULL);
    ck_assert_msg (event_id >= 0, "Fail to add event");

    rtcom_el_set_event_flag(el, event_id, "RTCOM_EL_FLAG_TEST_FLAG1", NULL);

    query = rtcom_el_query_new(el);
    rtcom_el_query_set_limit(query, 5);
    if(!rtcom_el_query_prepare(
                query,
                "id", event_id, RTCOM_EL_OP_EQUAL,
                NULL))
    {
        ck_abort_msg("Failed to prepare the query.");
    }

    it = rtcom_el_get_events(el, query);
    g_object_unref(query);

    ck_assert_msg (it != NULL, "Failed to get iterator");
    ck_assert_msg (rtcom_el_iter_first(it), "Failed to start iterator");

    ck_assert(rtcom_el_iter_get_values(it, "flags", &flags, NULL));

    test_flag1 = rtcom_el_get_flag_value(el, "RTCOM_EL_FLAG_TEST_FLAG1");

    ck_assert_msg ((flags & test_flag1) != 0, "flags don't match");

    g_object_unref(it);
    rtcom_el_event_free_contents (ev);
    rtcom_el_event_free (ev);
}
END_TEST


START_TEST(test_get)
{
    RTComElQuery * query = NULL;
    RTComElEvent * ev = NULL;
    RTComElEvent *result = NULL;
    gint event_id = -1;
    RTComElIter * it = NULL;

    ev = event_new_full (time (NULL));
    if(!ev)
    {
        ck_abort_msg("Failed to create event.");
    }

    event_id = rtcom_el_add_event(el, ev, NULL);
    ck_assert_msg (event_id >= 0, "Fail to add event");

    query = rtcom_el_query_new(el);
    if(!rtcom_el_query_prepare(
                query,
                "id", event_id, RTCOM_EL_OP_EQUAL,
                NULL))
    {
        ck_abort_msg("Failed to prepare the query.");
    }

    it = rtcom_el_get_events(el, query);
    g_object_unref(query);

    ck_assert_msg (it != NULL, "Failed to get iterator");
    ck_assert_msg (rtcom_el_iter_first(it), "Failed to start iterator");

    result = rtcom_el_event_new ();
    ck_assert_msg (result != NULL, "failed to create result event");

    ck_assert_msg (rtcom_el_iter_get_full (it, result),
                   "Failed to get event from iterator");

    ck_assert_msg (rtcom_el_event_equals (ev, result),
                   "Retrieved event doesn't match created one");

    g_object_unref(it);
    rtcom_el_event_free_contents (result);
    rtcom_el_event_free (result);
    rtcom_el_event_free_contents (ev);
    rtcom_el_event_free (ev);
}
END_TEST

START_TEST(test_unique_remotes)
{
    RTComElEvent * ev = NULL;
    gint event_id = -1;
    GList * remote_ebook_uids = NULL;
    GList * remote_uids = NULL;
    GList * remote_names = NULL;
    GList *iter = NULL;

    ev = event_new_full (time (NULL));
    if(!ev)
    {
        ck_abort_msg("Failed to create event.");
    }

    RTCOM_EL_EVENT_SET_FIELD(ev, remote_ebook_uid, g_strdup (REMOTE_EBOOK_UID));

    event_id = rtcom_el_add_event(el, ev, NULL);
    ck_assert_msg (event_id >= 0, "Fail to add event");

    /* mikhailz: a living API horror? */
    g_free (RTCOM_EL_EVENT_GET_FIELD(ev, remote_ebook_uid));
    g_free (RTCOM_EL_EVENT_GET_FIELD(ev, remote_uid));
    g_free (RTCOM_EL_EVENT_GET_FIELD(ev, remote_name));
    RTCOM_EL_EVENT_SET_FIELD(ev, remote_ebook_uid, g_strdup (REMOTE_EBOOK_UID_2));
    RTCOM_EL_EVENT_SET_FIELD(ev, remote_uid, g_strdup (REMOTE_UID_2));
    RTCOM_EL_EVENT_SET_FIELD(ev, remote_name, g_strdup (REMOTE_NAME_2));

    /* The group_uid field was allocated in the last add_event,
     * because we didn't provide a group_uid of our own.
     */
    g_free(RTCOM_EL_EVENT_GET_FIELD(ev, group_uid));
    RTCOM_EL_EVENT_UNSET_FIELD(ev, group_uid);

    event_id = rtcom_el_add_event(el, ev, NULL);
    ck_assert_msg (event_id >= 0, "Fail to add event");

    g_free (RTCOM_EL_EVENT_GET_FIELD(ev, remote_ebook_uid));
    g_free (RTCOM_EL_EVENT_GET_FIELD(ev, remote_uid));
    g_free (RTCOM_EL_EVENT_GET_FIELD(ev, remote_name));
    RTCOM_EL_EVENT_SET_FIELD(ev, remote_ebook_uid, g_strdup (REMOTE_EBOOK_UID_3));
    RTCOM_EL_EVENT_SET_FIELD(ev, remote_uid, g_strdup (REMOTE_UID_3));
    RTCOM_EL_EVENT_SET_FIELD(ev, remote_name, g_strdup (REMOTE_NAME_3));

    /* The group_uid field was allocated in the last add_event,
     * because we didn't provide a group_uid of our own */
    g_free(RTCOM_EL_EVENT_GET_FIELD(ev, group_uid));
    RTCOM_EL_EVENT_UNSET_FIELD(ev, group_uid);

    event_id = rtcom_el_add_event(el, ev, NULL);
    ck_assert_msg (event_id >= 0, "Fail to add event");

    remote_ebook_uids = rtcom_el_get_unique_remote_ebook_uids(el);
    ck_assert_msg (remote_ebook_uids != NULL,
                   "Fail to get unique remote_ebook_uids");

    ck_assert_msg (g_list_length(remote_ebook_uids) > 1,
             "remote_ebook_uids's length doesn't match");

    for (iter = remote_ebook_uids; iter != NULL; iter = iter->next)
        g_debug("Unique remote_ebook_uid: %s", (const guchar *) iter->data);

    remote_uids =  rtcom_el_get_unique_remote_uids(el);
    ck_assert_msg (remote_uids != NULL, "Fail to get unique remote_uids");

    ck_assert_msg (g_list_length(remote_uids) > 1, "remote_uids's length doesn't match");

    for (iter = remote_uids; iter != NULL; iter = iter->next)
        g_debug("Unique remote_uid: %s", (const guchar *) iter->data);

    remote_names = rtcom_el_get_unique_remote_names(el);
    ck_assert_msg (remote_names != NULL, "Fail to get unique remote_names");

    ck_assert_msg (g_list_length(remote_names) > 1,
             "remote_names's length doesn't match");

    for (iter = remote_names; iter != NULL; iter = iter->next)
        g_debug("Unique remote_name: %s", (const guchar *) iter->data);

    rtcom_el_event_free_contents (ev);
    rtcom_el_event_free (ev);
    g_list_foreach(remote_ebook_uids, (GFunc) g_free, NULL);
    g_list_free(remote_ebook_uids);
    g_list_foreach(remote_uids, (GFunc) g_free, NULL);
    g_list_free(remote_uids);
    g_list_foreach(remote_names, (GFunc) g_free, NULL);
    g_list_free(remote_names);
}
END_TEST

START_TEST(test_get_string)
{
    RTComElQuery * query = NULL;
    RTComElEvent * ev = NULL;
    gint event_id = -1;
    RTComElIter * it = NULL;
    gint header_id;
    gchar *bar;

    ev = event_new_full (time (NULL));
    if(!ev)
    {
        ck_abort_msg("Failed to create event.");
    }

    event_id = rtcom_el_add_event(el, ev, NULL);
    ck_assert_msg (event_id >= 0, "Fail to add event");

    header_id = rtcom_el_add_header(
            el, event_id,
            HEADER_KEY,
            HEADER_VAL,
            NULL);
    ck_assert_msg (header_id >= 0, "Failed to add header");

    query = rtcom_el_query_new(el);
    if(!rtcom_el_query_prepare(
                query,
                "id", event_id, RTCOM_EL_OP_EQUAL,
                NULL))
    {
        ck_abort_msg("Failed to prepare the query.");
    }

    it = rtcom_el_get_events(el, query);
    g_object_unref(query);

    ck_assert_msg(it != NULL, "Failed to get iterator");
    ck_assert_msg(rtcom_el_iter_first(it), "Failed to start iterator");

    bar = GUINT_TO_POINTER (0xDEADBEEF);
    ck_assert_msg(
            !rtcom_el_iter_get_values(it, "there is no such key", &bar, NULL),
            "Shouldn't be able to get a missing value as a string");
    ck_assert_msg(bar == GUINT_TO_POINTER (0xDEADBEEF),
            "bar should be left untouched in this case");

    ck_assert(rtcom_el_iter_get_values(it, HEADER_KEY,  &bar, NULL));
    ck_assert (bar != NULL);
    ck_assert (bar != GUINT_TO_POINTER (0xDEADBEEF));
    ck_assert_str_eq(bar, HEADER_VAL);

    g_free(bar);
    g_object_unref(it);
    rtcom_el_event_free_contents (ev);
    rtcom_el_event_free (ev);
}
END_TEST

START_TEST(test_get_int)
{
    RTComElQuery * query = NULL;
    RTComElEvent * ev = NULL;
    gint event_id = -1;
    RTComElIter * it = NULL;
    gint retrieved;

    ev = event_new_full (time (NULL));
    if(!ev)
    {
        ck_abort_msg("Failed to create event.");
    }

    event_id = rtcom_el_add_event(el, ev, NULL);
    ck_assert_msg (event_id >= 0, "Fail to add event");

    query = rtcom_el_query_new(el);
    if(!rtcom_el_query_prepare(
                query,
                "id", event_id, RTCOM_EL_OP_EQUAL,
                NULL))
    {
        ck_abort_msg("Failed to prepare the query.");
    }

    it = rtcom_el_get_events(el, query);
    g_object_unref(query);

    ck_assert_msg (it != NULL, "Failed to get iterator");
    ck_assert_msg (rtcom_el_iter_first(it), "Failed to start iterator");

    ck_assert_msg(rtcom_el_iter_get_values(it, "bytes-sent", &retrieved, NULL),
                  "Failed to get bytes-sent");
    ck_assert_int_eq(retrieved, BYTES_SENT);

    g_object_unref(it);
    rtcom_el_event_free_contents (ev);
    rtcom_el_event_free (ev);
}
END_TEST

START_TEST(test_get_time)
{
    RTComElQuery * query = NULL;
    RTComElEvent * ev = NULL;
    gint event_id = -1;
    RTComElIter * it = NULL;
    time_t retrieved;
    time_t t = time (NULL);

    ev = event_new_full (t);
    if(!ev)
    {
        ck_abort_msg("Failed to create event.");
    }

    event_id = rtcom_el_add_event(el, ev, NULL);
    ck_assert_msg (event_id >= 0, "Fail to add event");

    query = rtcom_el_query_new(el);
    if(!rtcom_el_query_prepare(
                query,
                "id", event_id, RTCOM_EL_OP_EQUAL,
                NULL))
    {
        ck_abort_msg("Failed to prepare the query.");
    }

    it = rtcom_el_get_events(el, query);
    g_object_unref(query);

    ck_assert_msg (it != NULL, "Failed to get iterator");
    ck_assert_msg (rtcom_el_iter_first(it), "Failed to start iterator");

    ck_assert_msg(rtcom_el_iter_get_values(it, "start-time", &retrieved, NULL),
                  "Failed to get start-time");
    ck_assert_int_eq(retrieved, t);

    g_object_unref(it);
    rtcom_el_event_free_contents (ev);
    rtcom_el_event_free (ev);
}
END_TEST

START_TEST(test_ends_with)
{
    RTComElQuery * query = NULL;
    RTComElIter * it = NULL;
    gchar *contents;

    query = rtcom_el_query_new(el);
    if(!rtcom_el_query_prepare(
                query,
                "remote-name", "ve", RTCOM_EL_OP_STR_ENDS_WITH,
                NULL))
    {
        ck_abort_msg("Failed to prepare the query.");
    }

    it = rtcom_el_get_events(el, query);
    g_object_unref(query);

    ck_assert_msg (it != NULL, "Failed to get iterator");
    ck_assert_msg (rtcom_el_iter_first(it), "Failed to start iterator");

    ck_assert(rtcom_el_iter_get_values(it, "free-text", &contents, NULL));

    /* It is an API guarantee that we're sorting by event ID, which ascends
     * as time goes on, so Eve's recent message comes before Dave's older
     * message */
    ck_assert_str_eq("I am online", contents);
    g_free(contents);

    ck_assert (rtcom_el_iter_next (it));

    ck_assert(rtcom_el_iter_get_values(it, "free-text", &contents, NULL));

    ck_assert_str_eq("Hello from Dave", contents);

    g_free(contents);

    ck_assert (!rtcom_el_iter_next (it));

    g_object_unref(it);
}
END_TEST

START_TEST(test_like)
{
    RTComElQuery * query = NULL;
    RTComElIter * it = NULL;
    gchar *contents;

    query = rtcom_el_query_new(el);
    if(!rtcom_el_query_prepare(
        query,
        "free-text", "%%AM oNLi%%", RTCOM_EL_OP_STR_LIKE,
        NULL))
    {
        ck_abort_msg("Failed to prepare the query.");
    }

    it = rtcom_el_get_events(el, query);
    g_object_unref(query);

    ck_assert_msg (it != NULL, "Failed to get iterator");
    ck_assert_msg (rtcom_el_iter_first(it), "Failed to start iterator");

    ck_assert(rtcom_el_iter_get_values(it, "free-text", &contents, NULL));

    ck_assert_str_eq("I am online", contents);
    g_free(contents);

    g_object_unref(it);
}
END_TEST

START_TEST(test_delete_events)
{
    RTComElQuery * query = NULL;
    RTComElEvent * ev = NULL;
    RTComElIter *it;
    gint event_id = -1;
    gint count;
    gboolean success;

    /* there are initially only the canned events */
    query = rtcom_el_query_new(el);
    ck_assert (rtcom_el_query_prepare (query,
                NULL));
    it = rtcom_el_get_events (el, query);
    g_object_unref (query);
    ck_assert (it != NULL);
    ck_assert (RTCOM_IS_EL_ITER (it));
    count = iter_count_results (it);
    ck_assert_int_eq (count, num_canned_events ());
    g_object_unref (it);

    ev = event_new_full (time (NULL));
    if(!ev)
    {
        ck_abort_msg("Failed to create event.");
    }

    event_id = rtcom_el_add_event(el, ev, NULL);
    ck_assert_msg (event_id >= 0, "Failed to add event");

    query = rtcom_el_query_new(el);
    if(!rtcom_el_query_prepare(
                query,
                "id", event_id, RTCOM_EL_OP_EQUAL,
                NULL))
    {
        ck_abort_msg("Failed to prepare the query.");
    }

    success = rtcom_el_delete_events(el, query, NULL);
    g_object_unref(query);

    ck_assert_msg (success, "Failed to delete stuff");

    /* check that we deleted only what we wanted to delete */

    rtcom_el_event_free_contents (ev);
    rtcom_el_event_free (ev);

    query = rtcom_el_query_new(el);
    ck_assert (rtcom_el_query_prepare (query,
                "id", event_id, RTCOM_EL_OP_EQUAL,
                NULL));
    it = rtcom_el_get_events (el, query);
    g_object_unref (query);
    ck_assert (it == NULL);
    count = iter_count_results (it);
    ck_assert_int_eq (count, 0);

    query = rtcom_el_query_new(el);
    ck_assert (rtcom_el_query_prepare (query,
                NULL));
    it = rtcom_el_get_events (el, query);
    g_object_unref (query);
    ck_assert (it != NULL);
    ck_assert (RTCOM_IS_EL_ITER (it));
    count = iter_count_results (it);
    ck_assert_int_eq (count, num_canned_events ());
    g_object_unref (it);
}
END_TEST

START_TEST(test_in_strv)
{
    RTComElQuery * query = NULL;
    RTComElIter * it = NULL;
    gchar *contents;
    const gchar * const interesting_people[] = { "Chris", "Dave", NULL };

    query = rtcom_el_query_new(el);
    if(!rtcom_el_query_prepare(
                query,
                "remote-name", &interesting_people, RTCOM_EL_OP_IN_STRV,
                NULL))
    {
        ck_abort_msg("Failed to prepare the query.");
    }

    it = rtcom_el_get_events(el, query);
    g_object_unref(query);

    ck_assert_msg (it != NULL, "Failed to get iterator");
    ck_assert_msg (rtcom_el_iter_first(it), "Failed to start iterator");

    ck_assert(rtcom_el_iter_get_values(it, "free-text", &contents, NULL));

    /* It is an API guarantee that we're sorting by event ID, which ascends
     * as time goes on, so Dave's recent message comes before Chris's older
     * message */

    ck_assert_str_eq("Hello from Dave", contents);

    g_free (contents);

    ck_assert (rtcom_el_iter_next (it));

    ck_assert(rtcom_el_iter_get_values(it, "free-text", &contents, NULL));

    ck_assert_str_eq("Hello from Chris", contents);

    ck_assert (!rtcom_el_iter_next (it));

    g_free (contents);
    g_object_unref(it);
}
END_TEST

START_TEST(test_delete_event)
{
    RTComElQuery * query = NULL;
    RTComElEvent * ev = NULL;
    RTComElIter *it;
    gint event_id = -1;
    gint count;
    gint ret;

    /* there are initially only the canned events */
    query = rtcom_el_query_new(el);
    ck_assert (rtcom_el_query_prepare (query,
                NULL));
    it = rtcom_el_get_events (el, query);
    g_object_unref (query);
    ck_assert (it != NULL);
    ck_assert (RTCOM_IS_EL_ITER (it));
    count = iter_count_results (it);
    ck_assert_int_eq (count, num_canned_events ());
    g_object_unref (it);

    ev = event_new_full (time (NULL));
    if(!ev)
    {
        ck_abort_msg("Failed to create event.");
    }

    event_id = rtcom_el_add_event(el, ev, NULL);
    ck_assert_msg (event_id >= 0, "Failed to add event");

    ret = rtcom_el_delete_event(el, event_id, NULL);
    ck_assert_int_eq (ret, 0);

    /* check that we deleted only what we wanted to delete */

    rtcom_el_event_free_contents (ev);
    rtcom_el_event_free (ev);

    query = rtcom_el_query_new(el);
    ck_assert (rtcom_el_query_prepare (query,
                "id", event_id, RTCOM_EL_OP_EQUAL,
                NULL));
    it = rtcom_el_get_events (el, query);
    g_object_unref (query);
    ck_assert (it == NULL);
    count = iter_count_results (it);
    ck_assert_int_eq (count, 0);

    query = rtcom_el_query_new(el);
    ck_assert (rtcom_el_query_prepare (query,
                NULL));
    it = rtcom_el_get_events (el, query);
    g_object_unref (query);
    ck_assert (it != NULL);
    ck_assert (RTCOM_IS_EL_ITER (it));
    count = iter_count_results (it);
    ck_assert_int_eq (count, num_canned_events ());
    g_object_unref (it);
}
END_TEST

START_TEST(test_string_equals)
{
    RTComElQuery * query = NULL;
    RTComElIter * it = NULL;
    gchar *contents;

    query = rtcom_el_query_new(el);
    if(!rtcom_el_query_prepare(
                query,
                "local-uid", "butterfly/msn/alice", RTCOM_EL_OP_NOT_EQUAL,
                "remote-name", "Bob", RTCOM_EL_OP_EQUAL,
                NULL))
    {
        ck_abort_msg("Failed to prepare the query.");
    }

    it = rtcom_el_get_events(el, query);
    g_object_unref(query);

    ck_assert_msg (it != NULL, "Failed to get iterator");
    ck_assert_msg (rtcom_el_iter_first(it), "Failed to start iterator");

    ck_assert_msg(rtcom_el_iter_get_values(it, "free-text", &contents, NULL),
                  "Failed to get values.");

    /* It is an API guarantee that we're sorting by event ID, which ascends
     * as time goes on, so Bob's recent message comes before his older
     * message */

    ck_assert_str_eq("Are you there?", contents);
    g_free(contents);

    ck_assert (rtcom_el_iter_next (it));

    ck_assert_msg(rtcom_el_iter_get_values(it, "free-text", &contents, NULL),
                  "Failed to get values.");

    ck_assert_str_eq("Hi Alice", contents);
    g_free(contents);

    g_object_unref(it);
}
END_TEST

START_TEST(test_int_ranges)
{
    RTComElQuery * query = NULL;
    RTComElIter * it = NULL;
    gchar *contents;

    query = rtcom_el_query_new(el);
    if(!rtcom_el_query_prepare(
                query,
                "start-time",(time_t)0, RTCOM_EL_OP_GREATER,
                "start-time", (time_t)4000, RTCOM_EL_OP_LESS_EQUAL,
                NULL))
    {
        ck_abort_msg("Failed to prepare the query.");
    }

    it = rtcom_el_get_events(el, query);
    g_object_unref(query);

    ck_assert_msg (it != NULL, "Failed to get iterator");
    ck_assert_msg (rtcom_el_iter_first(it), "Failed to start iterator");

    ck_assert_msg(rtcom_el_iter_get_values(it, "free-text", &contents, NULL),
                  "Failed to get values");

    /* It is an API guarantee that we're sorting by event ID, which ascends
     * as time goes on */

    ck_assert_str_eq("Are you there?", contents);
    g_free(contents);

    ck_assert (rtcom_el_iter_next (it));

    ck_assert_msg (rtcom_el_iter_get_values(it, "free-text", &contents, NULL),
                   "Failed to get values");

    ck_assert_str_eq("Hello from Dave", contents);
    g_free(contents);

    ck_assert (rtcom_el_iter_next (it));

    ck_assert_msg (rtcom_el_iter_get_values(it, "free-text", &contents, NULL),
                   "Failed to get values");

    ck_assert_str_eq("Hello from Chris", contents);
    g_free(contents);

    ck_assert (rtcom_el_iter_next (it));

    ck_assert_msg (rtcom_el_iter_get_values(it, "free-text", &contents, NULL),
                   "Failed to get values");

    ck_assert_str_eq("Hi Alice", contents);
    g_free(contents);

    ck_assert (!rtcom_el_iter_next (it));

    g_object_unref(it);
}
END_TEST

START_TEST(test_group_by_uids)
{
    RTComElQuery * query = NULL;
    RTComElIter * it = NULL;
    gchar *s;

    query = rtcom_el_query_new(el);
    rtcom_el_query_set_group_by (query, RTCOM_EL_QUERY_GROUP_BY_UIDS);
    ck_assert (rtcom_el_query_prepare(query,
               "remote-uid", "f", RTCOM_EL_OP_LESS,
               NULL));

    it = rtcom_el_get_events(el, query);
    g_object_unref(query);

    ck_assert_msg (it != NULL, "Failed to get iterator");

    /* It is an API guarantee that we're sorting by event ID, which ascends
     * as time goes on: so this is the order we'll get */

    ck_assert_msg (rtcom_el_iter_first(it), "Failed to start iterator");
    ck_assert (rtcom_el_iter_get_values (it, "remote-uid", &s, NULL));
    ck_assert_str_eq("bob@example.com", s);
    g_free (s);
    ck_assert (rtcom_el_iter_get_values (it, "local-uid", &s, NULL));
    ck_assert_str_eq("butterfly/msn/alice", s);
    g_free (s);

    ck_assert (rtcom_el_iter_next (it));
    ck_assert (rtcom_el_iter_get_values (it, "remote-uid", &s, NULL));
    ck_assert_str_eq("christine@msn.invalid", s);
    g_free (s);

    ck_assert (rtcom_el_iter_next (it));
    ck_assert (rtcom_el_iter_get_values (it, "remote-uid", &s, NULL));
    ck_assert_str_eq("eve@example.com", s);
    g_free (s);

    ck_assert (rtcom_el_iter_next (it));
    ck_assert (rtcom_el_iter_get_values (it, "remote-uid", &s, NULL));
    ck_assert_str_eq("bob@example.com", s);
    g_free (s);
    ck_assert (rtcom_el_iter_get_values (it, "free-text", &s, NULL));
    ck_assert_str_eq("Are you there?", s);
    g_free (s);

    ck_assert (rtcom_el_iter_next (it));
    ck_assert (rtcom_el_iter_get_values (it, "remote-uid", &s, NULL));
    ck_assert_str_eq("dave@example.com", s);
    g_free (s);

    ck_assert (rtcom_el_iter_next (it));
    ck_assert (rtcom_el_iter_get_values (it, "remote-uid", &s, NULL));
    ck_assert_str_eq("chris@example.com", s);
    g_free (s);

    /* Bob's first message does not appear here, because of the "group by" */

    ck_assert_msg (!rtcom_el_iter_next (it), "Iterator should have expired");

    g_object_unref(it);
}
END_TEST

START_TEST(test_group_by_metacontacts)
{
    RTComElQuery * query = NULL;
    RTComElIter * it = NULL;
    gchar *s;

    query = rtcom_el_query_new(el);
    rtcom_el_query_set_group_by (query, RTCOM_EL_QUERY_GROUP_BY_CONTACT);
    ck_assert(rtcom_el_query_prepare(query,
                "remote-uid", "f", RTCOM_EL_OP_LESS,
                NULL));

    it = rtcom_el_get_events(el, query);
    g_object_unref(query);

    ck_assert_msg (it != NULL, "Failed to get iterator");

    /* It is an API guarantee that we're sorting by event ID, which ascends
     * as time goes on: so this is the order we'll get */

    ck_assert_msg (rtcom_el_iter_first(it), "Failed to start iterator");
    ck_assert (rtcom_el_iter_get_values (it, "remote-uid", &s, NULL));
    ck_assert_str_eq("bob@example.com", s);
    g_free (s);
    ck_assert (rtcom_el_iter_get_values (it, "local-uid", &s, NULL));
    ck_assert_str_eq("butterfly/msn/alice", s);
    g_free (s);

    ck_assert (rtcom_el_iter_next (it));
    ck_assert (rtcom_el_iter_get_values (it, "remote-uid", &s, NULL));
    ck_assert_str_eq("christine@msn.invalid", s);
    g_free (s);

    ck_assert (rtcom_el_iter_next (it));
    ck_assert (rtcom_el_iter_get_values (it, "remote-uid", &s, NULL));
    ck_assert_str_eq("eve@example.com", s);
    g_free (s);

    /* Bob's second message *does* appear here, because in the absence of an
     * unambiguous identifier from the address book, we cannot assume that
     * the MSN user bob@example.com is the same as the XMPP user
     * bob@example.com (this is a bit obscure in protocols with an
     * "@", but becomes more significant in protocols with a flat namespace
     * like AIM, Skype, Myspace, IRC etc.)
     */
    ck_assert (rtcom_el_iter_next (it));
    ck_assert (rtcom_el_iter_get_values (it, "remote-uid", &s, NULL));
    ck_assert_str_eq("bob@example.com", s);
    g_free (s);
    ck_assert (rtcom_el_iter_get_values (it, "local-uid", &s, NULL));
    ck_assert_str_eq("gabble/jabber/alice", s);
    g_free (s);

    ck_assert (rtcom_el_iter_next (it));
    ck_assert (rtcom_el_iter_get_values (it, "remote-uid", &s, NULL));
    ck_assert_str_eq("dave@example.com", s);
    g_free (s);

    /* Bob's first message does not appear here, because it is grouped
     * together with his second (it's the same local UID *and* remote UID).
     *
     * Christine's first message does not appear here either, because her
     * two different remote user IDs are tied together by a metacontact. */

    ck_assert_msg (!rtcom_el_iter_next (it), "Iterator should have expired");

    g_object_unref(it);
}
END_TEST

START_TEST(test_group_by_group)
{
    RTComElQuery * query = NULL;
    RTComElIter * it = NULL;
    gchar *s;

    query = rtcom_el_query_new(el);
    rtcom_el_query_set_group_by (query, RTCOM_EL_QUERY_GROUP_BY_GROUP);
    ck_assert(rtcom_el_query_prepare(query,
                /* This will match Bob, Christine, Dave, Eve and Frank */
                "remote-uid", "b", RTCOM_EL_OP_GREATER_EQUAL,
                "remote-uid", "g", RTCOM_EL_OP_LESS,
                NULL));

    it = rtcom_el_get_events(el, query);
    g_object_unref(query);

    ck_assert_msg (it != NULL, "Failed to get iterator");

    /* It is an API guarantee that we're sorting by event ID, which ascends
     * as time goes on: so this is the order we'll get */

    ck_assert_msg (rtcom_el_iter_first(it), "Failed to start iterator");
    ck_assert (rtcom_el_iter_get_values (it, "remote-uid", &s, NULL));
    ck_assert_str_eq("bob@example.com", s);
    g_free (s);
    ck_assert (rtcom_el_iter_get_values (it, "local-uid", &s, NULL));
    ck_assert_str_eq("butterfly/msn/alice", s);
    g_free (s);
    ck_assert (rtcom_el_iter_get_values (it, "group-uid", &s, NULL));
    ck_assert_str_eq("group(bob)", s);
    g_free (s);

    ck_assert (rtcom_el_iter_next (it));
    ck_assert (rtcom_el_iter_get_values (it, "remote-uid", &s, NULL));
    ck_assert_str_eq("frank@msn.invalid", s);
    g_free (s);
    ck_assert (rtcom_el_iter_get_values (it, "local-uid", &s, NULL));
    ck_assert_str_eq("butterfly/msn/alice", s);
    g_free (s);
    ck_assert (rtcom_el_iter_get_values (it, "group-uid", &s, NULL));
    ck_assert_str_eq("group(chris+frank)", s);
    g_free (s);

    /* Christine's messages do not appear since Frank's is more recent.
     * Dave and Eve's messages, and Bob's XMPP message, do not appear
     * because they aren't from a groupchat. */

    ck_assert_msg (!rtcom_el_iter_next (it), "Iterator should have expired");

    g_object_unref(it);
}
END_TEST

START_TEST(test_update_remote_contact)
{
    RTComElQuery *query_by_abook;
    RTComElQuery *query_by_name;
    RTComElIter *it;
    gint count;

    /* We've put Bob in the address book */
    ck_assert (rtcom_eventlogger_update_remote_contact (el,
                "gabble/jabber/alice", "bob@example.com",
                "abook-bob", "Robert", NULL));

    query_by_abook = rtcom_el_query_new(el);
    ck_assert(rtcom_el_query_prepare(query_by_abook,
                "remote-ebook-uid", "abook-bob", RTCOM_EL_OP_EQUAL,
                NULL));

    /* Now, Bob's two XMPP messages are attached to that uid */
    it = rtcom_el_get_events (el, query_by_abook);
    ck_assert_msg (it != NULL, "Failed to get iterator");
    count = iter_count_results (it);
    ck_assert_int_eq (count, 2);

    /* Now put Bob's other identity in the address book */
    ck_assert (rtcom_eventlogger_update_remote_contact (el,
                "butterfly/msn/alice", "bob@example.com",
                "abook-bob", "Robert", NULL));

    g_object_unref (it);
    it = rtcom_el_get_events (el, query_by_abook);
    ck_assert_msg (it != NULL, "Failed to get iterator");

    /* Bob's MSN message is attached to that uid too */
    count = iter_count_results (it);
    ck_assert_int_eq (count, 3);

    g_object_unref (it);

    /* All three events are now marked as from Robert */
    query_by_name = rtcom_el_query_new(el);
    ck_assert(rtcom_el_query_prepare(query_by_name,
                "remote-name", "Robert", RTCOM_EL_OP_EQUAL,
                NULL));

    it = rtcom_el_get_events (el, query_by_name);
    ck_assert_msg (it != NULL, "Failed to get iterator");
    count = iter_count_results (it);
    ck_assert_int_eq (count, 3);

    /* When Robert is deleted from the address book, the name persists */
    ck_assert (rtcom_eventlogger_update_remote_contact (el,
                "gabble/jabber/alice", "bob@example.com",
                NULL, "Robert", NULL));
    ck_assert (rtcom_eventlogger_update_remote_contact (el,
                "butterfly/msn/alice", "bob@example.com",
                NULL, "Robert", NULL));

    g_object_unref (it);
    it = rtcom_el_get_events (el, query_by_name);
    ck_assert_msg (it != NULL, "Failed to get iterator");

    count = iter_count_results (it);
    ck_assert_int_eq (count, 3);

    g_object_unref (it);

    g_object_unref (query_by_abook);
    g_object_unref (query_by_name);
}
END_TEST

extern void db_extend_el_suite (Suite *s);

Suite *
el_suite(void)
{
    Suite * s = suite_create ("rtcom-eventlogger");

    /* Low-level DB APi test cases */
    db_extend_el_suite(s);

    /* Core test case */
    TCase * tc_core = tcase_create("Core");
    tcase_add_checked_fixture(tc_core, core_setup, core_teardown);
    tcase_add_test(tc_core, test_add_event);
    tcase_add_test(tc_core, test_add_full);
    tcase_add_test(tc_core, test_header);
    tcase_add_test(tc_core, test_attach);
    tcase_add_test(tc_core, test_read);
    tcase_add_test(tc_core, test_flags);
    tcase_add_test(tc_core, test_get);
    tcase_add_test(tc_core, test_unique_remotes);
    tcase_add_test(tc_core, test_get_int);
    tcase_add_test(tc_core, test_get_time);
    tcase_add_test(tc_core, test_get_string);
    tcase_add_test(tc_core, test_ends_with);
    tcase_add_test(tc_core, test_like);
    tcase_add_test(tc_core, test_delete_events);
    tcase_add_test(tc_core, test_delete_event);
    tcase_add_test(tc_core, test_in_strv);
    tcase_add_test(tc_core, test_string_equals);
    tcase_add_test(tc_core, test_int_ranges);
    tcase_add_test(tc_core, test_group_by_uids);
    tcase_add_test(tc_core, test_group_by_metacontacts);
    tcase_add_test(tc_core, test_group_by_group);
    tcase_add_test(tc_core, test_update_remote_contact);

    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite * s = el_suite();
    SRunner * sr = srunner_create(s);

    srunner_set_xml(sr, "/tmp/result.xml");
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free (sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

/* vim: set ai et tw=75 ts=4 sw=4: */

