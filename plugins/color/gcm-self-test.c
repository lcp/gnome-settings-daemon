/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007-2011 Richard Hughes <richard@hughsie.com>
 *
 * Licensed under the GNU General Public License Version 2
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "config.h"

#include <glib-object.h>
#include <stdlib.h>
#include <gtk/gtk.h>

#include "gcm-edid.h"
#include "gcm-dmi.h"
#include "gcm-tables.h"

static void
gcm_test_tables_func (void)
{
        GcmTables *tables;
        GError *error = NULL;
        gchar *vendor;

        tables = gcm_tables_new ();
        g_assert (tables != NULL);

        vendor = gcm_tables_get_pnp_id (tables, "IBM", &error);
        g_assert_no_error (error);
        g_assert (vendor != NULL);
        g_assert_cmpstr (vendor, ==, "IBM France");
        g_free (vendor);

        vendor = gcm_tables_get_pnp_id (tables, "MIL", &error);
        g_assert_no_error (error);
        g_assert (vendor != NULL);
        g_assert_cmpstr (vendor, ==, "Marconi Instruments Ltd");
        g_free (vendor);

        vendor = gcm_tables_get_pnp_id (tables, "XXX", &error);
        g_assert_error (error, GCM_TABLES_ERROR, GCM_TABLES_ERROR_FAILED);
        g_assert_cmpstr (vendor, ==, NULL);
        g_error_free (error);

        g_object_unref (tables);
}

static void
gcm_test_dmi_func (void)
{
        GcmDmi *dmi;

        dmi = gcm_dmi_new ();
        g_assert (dmi != NULL);
        g_assert (gcm_dmi_get_name (dmi) != NULL);
        g_assert (gcm_dmi_get_vendor (dmi) != NULL);
        g_object_unref (dmi);
}

static void
gcm_test_edid_func (void)
{
        GcmEdid *edid;
        gchar *data;
        gboolean ret;
        GError *error = NULL;
        gsize length = 0;

        edid = gcm_edid_new ();
        g_assert (edid != NULL);

        /* LG 21" LCD panel */
        ret = g_file_get_contents (TESTDATADIR "/LG-L225W-External.bin",
                                   &data, &length, &error);
        g_assert_no_error (error);
        g_assert (ret);
        ret = gcm_edid_parse (edid, (const guint8 *) data, length, &error);
        g_assert_no_error (error);
        g_assert (ret);

        g_assert_cmpstr (gcm_edid_get_monitor_name (edid), ==, "L225W");
        g_assert_cmpstr (gcm_edid_get_vendor_name (edid), ==, "Goldstar Company Ltd");
        g_assert_cmpstr (gcm_edid_get_serial_number (edid), ==, "34398");
        g_assert_cmpstr (gcm_edid_get_eisa_id (edid), ==, NULL);
        g_assert_cmpstr (gcm_edid_get_checksum (edid), ==, "80b7dda4c74b06366abb8fa23e71d645");
        g_assert_cmpstr (gcm_edid_get_pnp_id (edid), ==, "GSM");
        g_assert_cmpint (gcm_edid_get_height (edid), ==, 30);
        g_assert_cmpint (gcm_edid_get_width (edid), ==, 47);
        g_assert_cmpfloat (gcm_edid_get_gamma (edid), >=, 2.2f - 0.01);
        g_assert_cmpfloat (gcm_edid_get_gamma (edid), <, 2.2f + 0.01);
        g_free (data);

        /* Lenovo T61 internal Panel */
        ret = g_file_get_contents (TESTDATADIR "/Lenovo-T61-Internal.bin",
                                   &data, &length, &error);
        g_assert_no_error (error);
        g_assert (ret);
        ret = gcm_edid_parse (edid, (const guint8 *) data, length, &error);
        g_assert_no_error (error);
        g_assert (ret);

        g_assert_cmpstr (gcm_edid_get_monitor_name (edid), ==, NULL);
        g_assert_cmpstr (gcm_edid_get_vendor_name (edid), ==, "IBM France");
        g_assert_cmpstr (gcm_edid_get_serial_number (edid), ==, NULL);
        g_assert_cmpstr (gcm_edid_get_eisa_id (edid), ==, "LTN154P2-L05");
        g_assert_cmpstr (gcm_edid_get_checksum (edid), ==, "c585d9e80adc65c54f0a52597e850f83");
        g_assert_cmpstr (gcm_edid_get_pnp_id (edid), ==, "IBM");
        g_assert_cmpint (gcm_edid_get_height (edid), ==, 21);
        g_assert_cmpint (gcm_edid_get_width (edid), ==, 33);
        g_assert_cmpfloat (gcm_edid_get_gamma (edid), >=, 2.2f - 0.01);
        g_assert_cmpfloat (gcm_edid_get_gamma (edid), <, 2.2f + 0.01);
        g_free (data);

        g_object_unref (edid);
}

int
main (int argc, char **argv)
{
        if (! g_thread_supported ())
                g_thread_init (NULL);
        gtk_init (&argc, &argv);
        g_test_init (&argc, &argv, NULL);

        g_test_add_func ("/color/tables", gcm_test_tables_func);
        g_test_add_func ("/color/dmi", gcm_test_dmi_func);
        g_test_add_func ("/color/edid", gcm_test_edid_func);

        return g_test_run ();
}
