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

#ifndef __YELP_SEARCH_PROVIDER_SETTINGS_H__
#define __YELP_SEARCH_PROVIDER_SETTINGS_H__

#include <glib-object.h>

#include "yelp-settings.h"

G_BEGIN_DECLS

#define YELP_TYPE_SEARCH_PROVIDER_SETTINGS (yelp_search_provider_settings_get_type ())
G_DECLARE_FINAL_TYPE (YelpSearchProviderSettings, yelp_search_provider_settings, YELP, SEARCH_PROVIDER_SETTINGS, GObject)

struct _YelpSearchProviderSettingsClass {
    GObjectClass parent_class;
};

YelpSearchProviderSettings *yelp_search_provider_settings_new (void);

G_END_DECLS

#endif /* __YELP_SEARCH_PROVIDER_SETTINGS_H__ */
