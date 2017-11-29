/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * Copyright (C) 2004-2009 Shaun McCance <shaunm@gnome.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Shaun McCance <shaunm@gnome.org>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n.h>

#include "yelp-settings.h"

G_DEFINE_INTERFACE (YelpSettings, yelp_settings, G_TYPE_OBJECT)

static void
yelp_settings_default_init (YelpSettingsInterface *iface)
{}

static gchar **
get_colors (YelpSettings *settings)
{
    YelpSettingsInterface *iface;

    g_assert (YELP_IS_SETTINGS (settings));

    iface = YELP_SETTINGS_GET_IFACE (settings);
    g_return_val_if_fail (iface->get_colors != NULL, NULL);
    return iface->get_colors (settings);
}

gchar **
yelp_settings_get_colors (YelpSettings *settings)
{
    g_return_val_if_fail (YELP_IS_SETTINGS (settings), NULL);

    return get_colors (settings);
}

static gboolean
get_editor_mode (YelpSettings *settings)
{
    YelpSettingsInterface *iface;

    g_assert (YELP_IS_SETTINGS (settings));

    iface = YELP_SETTINGS_GET_IFACE (settings);
    g_return_val_if_fail (iface->get_editor_mode != NULL, FALSE);
    return iface->get_editor_mode (settings);
}

gboolean
yelp_settings_get_editor_mode (YelpSettings *settings)
{
    g_return_val_if_fail (YELP_IS_SETTINGS (settings), FALSE);

    return get_editor_mode (settings);
}

static const gchar*
get_color_param (YelpSettingsColor color)
{
    static const gchar * const params[YELP_SETTINGS_NUM_COLORS] = {
        "color.background",
        "color.text",
        "color.text_light",
        "color.link",
        "color.link_visted",
        "color.gray_background",
        "color.dark_background",
        "color.gray_border",
        "color.blue_background",
        "color.blue_border",
        "color.red_background",
        "color.red_border",
        "color.yellow_background",
        "color.yellow_border"
    };
    g_assert (color < YELP_SETTINGS_NUM_COLORS);
    return params[color];
}

static const gchar *
get_icon_param (YelpSettingsIcon icon)
{
    static const gchar *params[YELP_SETTINGS_NUM_ICONS] = {
        "icons.note.bug",
        "icons.note.important",
        "icons.note",
        "icons.note.tip",
        "icons.note.warning"
    };
    g_assert (icon < YELP_SETTINGS_NUM_ICONS);
    return params[icon];
}

static gchar **
get_icons (YelpSettings *settings)
{
    YelpSettingsInterface *iface;

    g_assert (YELP_IS_SETTINGS (settings));

    iface = YELP_SETTINGS_GET_IFACE (settings);
    g_return_val_if_fail (iface->get_colors != NULL, NULL);
    return iface->get_icons (settings);
}

static gint
get_icon_size (YelpSettings *settings)
{
    YelpSettingsInterface *iface;

    g_assert (YELP_IS_SETTINGS (settings));

    iface = YELP_SETTINGS_GET_IFACE (settings);
    g_return_val_if_fail (iface->get_icon_size != NULL, 0);
    return iface->get_icon_size (settings);
}

static gchar **
get_tokens (YelpSettings *settings)
{
    YelpSettingsInterface *iface;

    g_assert (YELP_IS_SETTINGS (settings));

    iface = YELP_SETTINGS_GET_IFACE (settings);
    g_return_val_if_fail (iface->get_tokens != NULL, NULL);
    return iface->get_tokens (settings);
}

static void
fill_colors (YelpSettings  *settings,
             gchar        **params,
             gsize         *inout_ix)
{
    gsize i, ix;
    gchar **colors;

    g_assert (YELP_IS_SETTINGS (settings));
    g_assert (params != NULL);
    g_assert (inout_ix != NULL);

    colors = get_colors (settings);
    for (i = 0, ix = *inout_ix;
         i < YELP_SETTINGS_NUM_COLORS;
         i++, ix += 2) {
        params[ix] = g_strdup (get_color_param (i));
        params[ix + 1] = g_strdup_printf ("\"%s\"", colors[i]);
    }
    g_strfreev (colors);
    *inout_ix = ix;
}

static void
fill_icons (YelpSettings  *settings,
             gchar        **params,
             gsize         *inout_ix)
{
    gsize i, ix;
    gchar **icons;

    g_assert (YELP_IS_SETTINGS (settings));
    g_assert (params != NULL);
    g_assert (inout_ix != NULL);

    icons = get_icons (settings);
    for (i = 0, ix = *inout_ix;
         i < YELP_SETTINGS_NUM_ICONS;
         i++, ix += 2) {
        params[ix] = g_strdup (get_icon_param (i));
        params[ix + 1] = g_strdup_printf ("\"%s\"", icons[i]);
    }
    g_strfreev (icons);
    *inout_ix = ix;
}

static void
fill_misc (YelpSettings  *settings,
           gchar        **params,
           gsize         *inout_ix)
{
    gsize ix;

    g_assert (YELP_IS_SETTINGS (settings));
    g_assert (params != NULL);
    g_assert (inout_ix != NULL);

    ix = *inout_ix;
    params[ix++] = g_strdup ("icons.size.note");
    params[ix++] = g_strdup_printf ("%i", get_icon_size (settings));
    params[ix++] = g_strdup ("yelp.editor_mode");
    params[ix++] = g_strdup (get_editor_mode (settings) ? "true()" : "false()");
    *inout_ix = ix;
}

static void
fill_tokens (YelpSettings  *settings,
             gchar        **params,
             gsize         *inout_ix)
{
    gsize ix;
    GString *malstr, *dbstr;
    gchar **tokens, **iter;

    g_assert (YELP_IS_SETTINGS (settings));
    g_assert (params != NULL);
    g_assert (inout_ix != NULL);

    malstr = g_string_new ("'");
    dbstr = g_string_new ("'");
    tokens = get_tokens (settings);
    for (iter = tokens; iter != NULL && *iter != NULL; iter++) {
        g_string_append_c (malstr, ' ');
        g_string_append (malstr, *iter);
        if (g_str_has_prefix (*iter, "platform:")) {
            g_string_append_c (dbstr, ';');
            g_string_append (dbstr, *iter + 9);
        }
    }
    g_strfreev (tokens);
    g_string_append_c (malstr, '\'');
    g_string_append_c (dbstr, '\'');
    ix = *inout_ix;
    params[ix++] = g_strdup ("mal.if.custom");
    params[ix++] = g_string_free (malstr, FALSE);
    params[ix++] = g_strdup ("db.profile.os");
    params[ix++] = g_string_free (dbstr, FALSE);
    *inout_ix = ix;
}

gchar **
yelp_settings_get_all_params (YelpSettings *settings,
                              gsize         extra,
                              gsize        *end)
{
    gchar **params;
    gsize ix;

    g_return_val_if_fail (YELP_IS_SETTINGS (settings), NULL);

    params = g_new0 (gchar *,
                     (2 * YELP_SETTINGS_NUM_COLORS) +
                     (2 * YELP_SETTINGS_NUM_ICONS) +
                     extra +
                     8 /* editor mode, icon size, db.profile.os and mal.if.custom */ +
                     1 /* NULL */);

    fill_colors (settings, params, &ix);
    fill_icons (settings, params, &ix);
    fill_misc (settings, params, &ix);
    fill_tokens (settings, params, &ix);

    if (end != NULL)
        *end = ix;
    return params;
}
