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

#include "yelp-search-provider-settings.h"

struct _YelpSearchProviderSettings {
    GObject parent;
};

typedef struct {
} YelpSearchProviderSettingsPrivate;

static void
yelp_search_provider_settings_yelp_settings_init (YelpSettingsInterface *iface);

G_DEFINE_TYPE_WITH_CODE (YelpSearchProviderSettings,
                         yelp_search_provider_settings,
                         G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (YELP_TYPE_SETTINGS,
                                                yelp_search_provider_settings_yelp_settings_init)
                         G_ADD_PRIVATE (YelpSearchProviderSettings))
#define GET_PRIV(object) yelp_search_provider_settings_get_instance_private (object)

static void
yelp_search_provider_settings_class_init (YelpSearchProviderSettingsClass *settings_class)
{}

static void
yelp_search_provider_settings_init (YelpSearchProviderSettings *settings)
{}

YelpSearchProviderSettings *
yelp_search_provider_settings_new (void)
{
    return g_object_new (YELP_TYPE_SEARCH_PROVIDER_SETTINGS,
                         NULL);
}

static gchar **
get_colors (YelpSettings *settings)
{
    static gchar hex[] = "#000000";
    gchar **colors;
    YelpSettingsColor color;

    colors = g_new0 (gchar *, YELP_SETTINGS_NUM_COLORS + 1);
    for (color = YELP_SETTINGS_NUM_COLORS_START;
         color < YELP_SETTINGS_NUM_COLORS;
         color++)
        colors[color] = g_memdup (hex, sizeof hex);

    return colors;
}

static gint
get_icon_size (YelpSettings *settings)
{
    return 16;
}

static gchar **
get_icons (YelpSettings *settings)
{
    static char url[] = "file:///foo";
    gchar **icons;
    YelpSettingsIcon icon;

    icons = g_new0 (gchar *, YELP_SETTINGS_NUM_ICONS + 1);
    for (icon = YELP_SETTINGS_NUM_ICONS_START;
         icon < YELP_SETTINGS_NUM_ICONS;
         icon++)
        icons[icon] = g_memdup (url, sizeof url);

    return icons;
}

static gboolean
get_editor_mode (YelpSettings *settings)
{
    /* Depend on some env var? */
    return FALSE;
}

static gchar **
get_tokens (YelpSettings *settings)
{
    return g_new0 (gchar *, 1);
}

static YelpSettingsTextDirection
get_text_direction (YelpSettings *settings)
{
    return YELP_SETTINGS_TEXT_DIRECTION_RTL;
}

static gchar *
get_uri_for_gicon (YelpSettings *settings,
                   GIcon        *icon)
{
    static gchar url[] = "file:///foo";

    return g_memdup (url, sizeof url);
}

static void
yelp_search_provider_settings_yelp_settings_init (YelpSettingsInterface *iface)
{
  iface->get_colors = get_colors;
  iface->get_icon_size = get_icon_size;
  iface->get_icons = get_icons;
  iface->get_editor_mode = get_editor_mode;
  iface->get_tokens = get_tokens;
  iface->get_text_direction = get_text_direction;
  iface->get_uri_for_gicon = get_uri_for_gicon;
}
