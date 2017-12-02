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

#ifndef __YELP_SETTINGS_H__
#define __YELP_SETTINGS_H__

#include <glib-object.h>

G_BEGIN_DECLS

typedef enum {
    YELP_SETTINGS_COLOR_BASE,
    YELP_SETTINGS_COLOR_TEXT,
    YELP_SETTINGS_COLOR_TEXT_LIGHT,
    YELP_SETTINGS_COLOR_LINK,
    YELP_SETTINGS_COLOR_LINK_VISITED,
    YELP_SETTINGS_COLOR_GRAY_BASE,
    YELP_SETTINGS_COLOR_DARK_BASE,
    YELP_SETTINGS_COLOR_GRAY_BORDER,
    YELP_SETTINGS_COLOR_BLUE_BASE,
    YELP_SETTINGS_COLOR_BLUE_BORDER,
    YELP_SETTINGS_COLOR_RED_BASE,
    YELP_SETTINGS_COLOR_RED_BORDER,
    YELP_SETTINGS_COLOR_YELLOW_BASE,
    YELP_SETTINGS_COLOR_YELLOW_BORDER,
    YELP_SETTINGS_NUM_COLORS,
    YELP_SETTINGS_NUM_COLORS_START = 0,
} YelpSettingsColor;

typedef enum {
    YELP_SETTINGS_ICON_START,
    YELP_SETTINGS_ICON_BUG,
    YELP_SETTINGS_ICON_IMPORTANT,
    YELP_SETTINGS_ICON_NOTE,
    YELP_SETTINGS_ICON_TIP,
    YELP_SETTINGS_ICON_WARNING,
    YELP_SETTINGS_NUM_ICONS,
    YELP_SETTINGS_NUM_ICONS_START = 0,
} YelpSettingsIcon;

typedef enum {
  YELP_SETTINGS_TEXT_DIRECTION_RTL,
  YELP_SETTINGS_TEXT_DIRECTION_LTR,
} YelpSettingsTextDirection;

#define YELP_TYPE_SETTINGS         (yelp_settings_get_type ())

G_DECLARE_INTERFACE (YelpSettings, yelp_settings, YELP, SETTINGS, GObject)

struct _YelpSettingsInterface {
    GTypeInterface g_iface;

    gchar **                  (*get_colors)         (YelpSettings *settings);
    gint                      (*get_icon_size)      (YelpSettings *settings);
    gchar **                  (*get_icons)          (YelpSettings *settings);
    gboolean                  (*get_editor_mode)    (YelpSettings *settings);
    gchar **                  (*get_tokens)         (YelpSettings *settings);
    YelpSettingsTextDirection (*get_text_direction) (YelpSettings *settings);
};

gchar **                  yelp_settings_get_colors         (YelpSettings *settings);
gboolean                  yelp_settings_get_editor_mode    (YelpSettings *settings);
gchar **                  yelp_settings_get_all_params     (YelpSettings *settings,
                                                            gsize         extra,
                                                            gsize        *end);
YelpSettingsTextDirection yelp_settings_get_text_direction (YelpSettings *settings);

G_END_DECLS

#endif /* __YELP_SETTINGS_H__ */
