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

#ifndef __YELP_GTK_SETTINGS_H__
#define __YELP_GTK_SETTINGS_H__

#include <glib-object.h>

#include "yelp-settings.h"

G_BEGIN_DECLS

#define YELP_TYPE_GTK_SETTINGS (yelp_gtk_settings_get_type ())
G_DECLARE_DERIVABLE_TYPE (YelpGtkSettings, yelp_gtk_settings, YELP, GTK_SETTINGS, GObject)

struct _YelpGtkSettingsClass {
    GObjectClass parent_class;
};

typedef enum {
    YELP_GTK_SETTINGS_FONT_VARIABLE,
    YELP_GTK_SETTINGS_FONT_FIXED,
    YELP_GTK_SETTINGS_NUM_FONTS,
    YELP_GTK_SETTINGS_NUM_FONTS_START = 0,
} YelpGtkSettingsFont;

YelpGtkSettings *   yelp_gtk_settings_get_default          (void);

gchar *             yelp_gtk_settings_get_color            (YelpGtkSettings       *settings,
                                                            YelpSettingsColor   color);
void                yelp_gtk_settings_set_colors           (YelpGtkSettings       *settings,
                                                            YelpSettingsColor   first_color,
                                                            ...);

gchar *             yelp_gtk_settings_get_font             (YelpGtkSettings       *settings,
                                                            YelpGtkSettingsFont    font);
gchar *             yelp_gtk_settings_get_font_family      (YelpGtkSettings       *settings,
                                                            YelpGtkSettingsFont    font);
gint                yelp_gtk_settings_get_font_size        (YelpGtkSettings       *settings,
                                                            YelpGtkSettingsFont    font);
void                yelp_gtk_settings_set_fonts            (YelpGtkSettings       *settings,
                                                            YelpGtkSettingsFont    first_font,
                                                            ...);
gint                yelp_gtk_settings_get_font_adjustment  (YelpGtkSettings       *settings);
void                yelp_gtk_settings_set_font_adjustment  (YelpGtkSettings       *settings,
                                                            gint                adjustment);

gint                yelp_gtk_settings_get_icon_size        (YelpGtkSettings       *settings);
void                yelp_gtk_settings_set_icon_size        (YelpGtkSettings       *settings,
                                                            gint                size);
gchar *             yelp_gtk_settings_get_icon             (YelpGtkSettings       *settings,
                                                            YelpSettingsIcon    icon);
void                yelp_gtk_settings_set_icons            (YelpGtkSettings       *settings,
                                                            YelpSettingsIcon    first_icon,
                                                            ...);

gboolean            yelp_gtk_settings_get_show_text_cursor (YelpGtkSettings       *settings);
void                yelp_gtk_settings_set_show_text_cursor (YelpGtkSettings       *settings,
                                                            gboolean            show);

void                yelp_gtk_settings_set_editor_mode      (YelpGtkSettings       *settings,
                                                            gboolean            editor_mode);

gint                yelp_gtk_settings_cmp_icons            (const gchar        *icon1,
                                                            const gchar        *icon2);

G_END_DECLS

#endif /* __YELP_GTK_SETTINGS_H__ */
