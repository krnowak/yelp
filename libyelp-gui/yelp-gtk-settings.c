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

#include <stdarg.h>

#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include "yelp-gtk-settings.h"

typedef struct {
    GMutex        mutex;

    gchar         colors[YELP_SETTINGS_NUM_COLORS][8];
    gchar        *setfonts[YELP_GTK_SETTINGS_NUM_FONTS];
    gchar        *fonts[YELP_GTK_SETTINGS_NUM_FONTS];
    gchar        *icons[YELP_SETTINGS_NUM_ICONS];
    gint          icon_size;

    GtkSettings  *gtk_settings;
    GtkIconTheme *gtk_icon_theme;

    gint          font_adjustment;

    gulong        gtk_theme_changed;
    gulong        gtk_font_changed;
    gulong        icon_theme_changed;

    gboolean      show_text_cursor;

    gboolean      editor_mode;

    GHashTable   *tokens;
} YelpGtkSettingsPrivate;

enum {
    COLORS_CHANGED,
    FONTS_CHANGED,
    ICONS_CHANGED,
    LAST_SIGNAL
};
static guint settings_signals[LAST_SIGNAL] = {0,};

enum {
  PROP_0,
  PROP_GTK_SETTINGS,
  PROP_GTK_ICON_THEME,
  PROP_FONT_ADJUSTMENT,
  PROP_SHOW_TEXT_CURSOR,
  PROP_EDITOR_MODE
};

static const gchar *icon_names[YELP_SETTINGS_NUM_ICONS];

static void
yelp_gtk_settings_yelp_settings_init (YelpSettingsInterface *iface);

G_DEFINE_TYPE_WITH_CODE (YelpGtkSettings,
                         yelp_gtk_settings,
                         G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (YELP_TYPE_SETTINGS,
                                                yelp_gtk_settings_yelp_settings_init)
                         G_ADD_PRIVATE (YelpGtkSettings))
#define GET_PRIV(object) yelp_gtk_settings_get_instance_private (object)

static void           yelp_gtk_settings_constructed  (GObject              *object);
static void           yelp_gtk_settings_finalize     (GObject              *object);
static void           yelp_gtk_settings_get_property (GObject              *object,
                                                      guint                 prop_id,
                                                      GValue               *value,
                                                      GParamSpec           *pspec);
static void           yelp_gtk_settings_set_property (GObject              *object,
                                                      guint                 prop_id,
                                                      const GValue         *value,
                                                      GParamSpec           *pspec);
static void           yelp_gtk_settings_set_if_token (YelpGtkSettings         *settings,
                                                      const gchar          *token);

static void           gtk_theme_changed          (GtkSettings          *gtk_settings,
                                                  GParamSpec           *pspec,
                                                  YelpGtkSettings      *settings);
static void           gtk_font_changed           (GtkSettings          *gtk_settings,
                                                  GParamSpec           *pspec,
                                                  YelpGtkSettings      *settings);
static void           icon_theme_changed         (GtkIconTheme         *theme,
                                                  YelpGtkSettings      *settings);

static void           rgb_to_hsv                 (GdkRGBA  color,
                                                  gdouble *h,
                                                  gdouble *s,
                                                  gdouble *v);
static void           hsv_to_hex                 (gdouble  h,
                                                  gdouble  s,
                                                  gdouble  v,
                                                  gchar    *str);

/******************************************************************************/

static void
yelp_gtk_settings_class_init (YelpGtkSettingsClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    gint i;

    object_class->constructed  = yelp_gtk_settings_constructed;
    object_class->finalize = yelp_gtk_settings_finalize;
    object_class->get_property = yelp_gtk_settings_get_property;
    object_class->set_property = yelp_gtk_settings_set_property;

    for (i = YELP_SETTINGS_NUM_ICONS_START;
         i < YELP_SETTINGS_NUM_ICONS;
         i++) {
        switch (i) {
        case YELP_SETTINGS_ICON_BUG:
            icon_names[i] = "yelp-note-bug";
            break;
        case YELP_SETTINGS_ICON_IMPORTANT:
            icon_names[i] = "yelp-note-important";
            break;
        case YELP_SETTINGS_ICON_NOTE:
            icon_names[i] = "yelp-note";
            break;
        case YELP_SETTINGS_ICON_TIP:
            icon_names[i] = "yelp-note-tip";
            break;
        case YELP_SETTINGS_ICON_WARNING:
            icon_names[i] = "yelp-note-warning";
            break;
        default:
            g_assert_not_reached ();
        }
    }

    g_object_class_install_property (object_class,
                                     PROP_GTK_SETTINGS,
                                     g_param_spec_object ("gtk-settings",
                                                          "GtkSettings",
                                                          "A GtkSettings object to get settings from",
                                                          GTK_TYPE_SETTINGS,
                                                          G_PARAM_READWRITE | G_PARAM_STATIC_NAME |
                                                          G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB));

    g_object_class_install_property (object_class,
                                     PROP_GTK_ICON_THEME,
                                     g_param_spec_object ("gtk-icon-theme",
                                                          "GtkIconTheme",
                                                          "A GtkIconTheme object to get icons from",
                                                          GTK_TYPE_ICON_THEME,
                                                          G_PARAM_READWRITE | G_PARAM_STATIC_NAME |
                                                          G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB));

    g_object_class_install_property (object_class,
                                     PROP_FONT_ADJUSTMENT,
                                     g_param_spec_int ("font-adjustment",
                                                       "Font Adjustment",
                                                       "A size adjustment to add to font sizes",
                                                       -3, 10, 0,
                                                       G_PARAM_READWRITE | G_PARAM_STATIC_NAME |
                                                       G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB));

    g_object_class_install_property (object_class,
                                     PROP_SHOW_TEXT_CURSOR,
                                     g_param_spec_boolean ("show-text-cursor",
                                                           "Show Text Cursor",
                                                           "Show the text cursor or caret for accessible navigation",
                                                           FALSE,
                                                           G_PARAM_READWRITE | G_PARAM_STATIC_NAME |
                                                           G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB));

    g_object_class_install_property (object_class,
                                     PROP_EDITOR_MODE,
                                     g_param_spec_boolean ("editor-mode",
                                                           "Editor Mode",
                                                           "Enable features useful to editors",
                                                           FALSE,
                                                           G_PARAM_READWRITE | G_PARAM_STATIC_NAME |
                                                           G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB));

    settings_signals[COLORS_CHANGED] =
        g_signal_new ("colors-changed",
                      G_OBJECT_CLASS_TYPE (klass),
                      G_SIGNAL_RUN_LAST,
                      0, NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);

    settings_signals[FONTS_CHANGED] =
        g_signal_new ("fonts-changed",
                      G_OBJECT_CLASS_TYPE (klass),
                      G_SIGNAL_RUN_LAST,
                      0, NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);

    settings_signals[ICONS_CHANGED] =
        g_signal_new ("icons-changed",
                      G_OBJECT_CLASS_TYPE (klass),
                      G_SIGNAL_RUN_LAST,
                      0, NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);
}

static void
yelp_gtk_settings_init (YelpGtkSettings *settings)
{
    gint i;
    YelpGtkSettingsPrivate *priv;

    priv = GET_PRIV (settings);
    g_mutex_init (&priv->mutex);
    priv->icon_size = 24;

    for (i = 0; i < YELP_SETTINGS_NUM_ICONS; i++)
        priv->icons[i] = NULL;
    for (i = YELP_GTK_SETTINGS_NUM_FONTS_START;
         i < YELP_GTK_SETTINGS_NUM_FONTS;
         i++) {
        priv->setfonts[i] = NULL;
        priv->fonts[i] = NULL;
    }

    priv->tokens = g_hash_table_new_full (g_str_hash, g_str_equal,
                                          g_free, NULL);
}

static void
yelp_gtk_settings_constructed (GObject *object)
{
    YelpGtkSettings *settings = YELP_GTK_SETTINGS (object);
    gboolean skip_dbus_checks = FALSE;
    gchar *os_release = NULL;
    const gchar *desktop;

    yelp_gtk_settings_set_if_token (settings, "action:install");

    g_file_get_contents ("/etc/os-release", &os_release, NULL, NULL);
    if (os_release == NULL)
        g_file_get_contents ("/usr/lib/os-release", &os_release, NULL, NULL);

    if (os_release != NULL) {
        gint i;
        gchar **lines = g_strsplit(os_release, "\n", -1);
        gchar *osid = NULL, *osversion = NULL, *end;
        g_free (os_release);

        for (i = 0; lines[i] != NULL; i++) {
            if (g_str_has_prefix (lines[i], "ID=")) {
                if (lines[i][3] == '"') {
                    end = strchr (lines[i] + 4, '"');
                    if (end != NULL)
                        osid = g_strndup (lines[i] + 4, end - lines[i] - 4);
                }
                else if (lines[i][3] == '\'') {
                    end = strchr (lines[i] + 4, '\'');
                    if (end != NULL)
                        osid = g_strndup (lines[i] + 4, end - lines[i] - 4);
                }
                else {
                    osid = g_strdup (lines[i] + 3);
                }
            }
            else if (g_str_has_prefix (lines[i], "VERSION_ID=")) {
                if (lines[i][11] == '"') {
                    end = strchr (lines[i] + 12, '"');
                    if (end != NULL)
                        osversion = g_strndup (lines[i] + 12, end - lines[i] - 12);
                }
                else if (lines[i][11] == '\'') {
                    end = strchr (lines[i] + 12, '\'');
                    if (end != NULL)
                        osversion = g_strndup (lines[i] + 12, end - lines[i] - 12);
                }
                else {
                    osversion = g_strdup (lines[i] + 11);
                }
            }
        }

        if (osid) {
            gchar *token = g_strconcat("platform:", osid, NULL);
            yelp_gtk_settings_set_if_token (settings, token);
            g_free (token);
            if (osversion) {
                token = g_strconcat("platform:", osid, "-", osversion, NULL);
                yelp_gtk_settings_set_if_token (settings, token);
                g_free (token);
                g_free (osversion);
            }
            g_free (osid);
        }

        g_strfreev(lines);
    }

    desktop = g_getenv ("XDG_CURRENT_DESKTOP");
    if (desktop != NULL) {
        gchar **desktops = g_strsplit (desktop, ":", -1);
        gint i;
        gboolean xdg_gnome = FALSE, xdg_gnome_classic = FALSE;
        for (i = 0; desktops[i]; i++) {
            if (!g_ascii_strcasecmp (desktops[i], "gnome")) {
                xdg_gnome = TRUE;
            }
            else if (!g_ascii_strcasecmp (desktops[i], "gnome-classic")) {
                xdg_gnome_classic = TRUE;
            }
            else if (!g_ascii_strcasecmp (desktops[i], "kde")) {
                yelp_gtk_settings_set_if_token (settings, "platform:kde");
                skip_dbus_checks = TRUE;
                break;
            }
            else if (!g_ascii_strcasecmp (desktops[i], "mate")) {
                yelp_gtk_settings_set_if_token (settings, "platform:mate");
                yelp_gtk_settings_set_if_token (settings, "platform:gnome-panel");
                skip_dbus_checks = TRUE;
                break;
            }
            else if (!g_ascii_strcasecmp (desktops[i], "pantheon")) {
                yelp_gtk_settings_set_if_token (settings, "platform:pantheon");
                yelp_gtk_settings_set_if_token (settings, "platform:gnome-shell");
                skip_dbus_checks = TRUE;
                break;
            }
            else if (!g_ascii_strcasecmp (desktops[i], "unity")) {
                yelp_gtk_settings_set_if_token (settings, "platform:unity");
                skip_dbus_checks = TRUE;
                break;
            }
            else if (!g_ascii_strcasecmp (desktops[i], "x-cinnamon")) {
                yelp_gtk_settings_set_if_token (settings, "platform:cinnamon");
                yelp_gtk_settings_set_if_token (settings, "platform:gnome-shell");
                skip_dbus_checks = TRUE;
                break;
            }
            else if (!g_ascii_strcasecmp (desktops[i], "xfce")) {
                yelp_gtk_settings_set_if_token (settings, "platform:xfce");
                skip_dbus_checks = TRUE;
                break;
            }
        }
        if (xdg_gnome) {
            yelp_gtk_settings_set_if_token (settings, "platform:gnome-shell");
            if (!xdg_gnome_classic)
                yelp_gtk_settings_set_if_token (settings, "platform:gnome-3");
            skip_dbus_checks = TRUE;
        }
        if (xdg_gnome_classic) {
            yelp_gtk_settings_set_if_token (settings, "platform:gnome-classic");
            yelp_gtk_settings_set_if_token (settings, "platform:gnome-shell");
            skip_dbus_checks = TRUE;
        }
        g_strfreev (desktops);
    }

    if (!skip_dbus_checks) {
        GDBusConnection *connection;
        GVariant *ret, *names;
        GVariantIter iter;
        gchar *name;
        gboolean env_shell, env_classic, env_panel, env_unity, env_xfce;
        GError *error = NULL;

        connection = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error);
        if (connection == NULL) {
            g_warning ("Unable to connect to dbus: %s", error->message);
            g_error_free (error);
            return;
        }

        ret = g_dbus_connection_call_sync (connection,
                                           "org.freedesktop.DBus",
                                           "/org/freedesktop/DBus",
                                           "org.freedesktop.DBus",
                                           "ListNames",
                                           NULL,
                                           G_VARIANT_TYPE ("(as)"),
                                           G_DBUS_CALL_FLAGS_NONE,
                                           -1, NULL, &error);
        if (ret == NULL) {
            g_warning ("Unable to query dbus: %s", error->message);
            g_error_free (error);
            return;
        }
        env_shell = env_classic = env_panel = env_unity = env_xfce = FALSE;
        names = g_variant_get_child_value (ret, 0);
        g_variant_iter_init (&iter, names);
        while (g_variant_iter_loop (&iter, "&s", &name)) {
            if (g_str_equal (name, "org.gnome.Panel"))
                env_panel = TRUE;
            else if (g_str_equal (name, "org.gnome.Shell"))
                env_shell = TRUE;
            else if (g_str_equal (name, "com.canonical.Unity"))
                env_unity = TRUE;
            else if (g_str_equal (name, "org.xfce.Panel"))
                env_xfce = TRUE;
        }
        g_variant_unref (names);
        g_variant_unref (ret);
        if (env_shell) {
            ret = g_dbus_connection_call_sync (connection,
                                                   "org.gnome.Shell",
                                                   "/org/gnome/Shell",
                                                   "org.freedesktop.DBus.Properties",
                                                   "Get",
                                                   g_variant_new ("(ss)",
                                                                  "org.gnome.Shell",
                                                                  "Mode"),
                                                   G_VARIANT_TYPE ("(v)"),
                                                   G_DBUS_CALL_FLAGS_NONE,
                                                   -1, NULL, &error);
            if (ret == NULL) {
                g_warning ("Failed to get GNOME shell mode: %s", error->message);
                g_error_free (error);
            } else {
                GVariant *v;
                g_variant_get (ret, "(v)", &v);
                if (g_variant_is_of_type (v, G_VARIANT_TYPE_STRING) &&
                    g_str_equal (g_variant_get_string (v, NULL), "classic")) {
                    env_classic = TRUE;
                }
                g_variant_unref (v);
                g_variant_unref (ret);
            }
        }

        if (env_classic)
            yelp_gtk_settings_set_if_token (settings, "platform:gnome-classic");

        /* order is important:
           gnome-shell also provides org.gnome.Panel
           unity also provides org.gnome.Shell
         */
        if (env_unity)
            yelp_gtk_settings_set_if_token (settings, "platform:unity");
        else if (env_shell)
            yelp_gtk_settings_set_if_token (settings, "platform:gnome-shell");
        else if (env_xfce)
            yelp_gtk_settings_set_if_token (settings, "platform:xfce");
        else if (env_panel)
            yelp_gtk_settings_set_if_token (settings, "platform:gnome-panel");
    }
}

static void
yelp_gtk_settings_finalize (GObject *object)
{
    YelpGtkSettings *settings = YELP_GTK_SETTINGS (object);
    YelpGtkSettingsPrivate *priv = GET_PRIV (settings);

    g_mutex_clear (&priv->mutex);

    g_hash_table_destroy (priv->tokens);

    G_OBJECT_CLASS (yelp_gtk_settings_parent_class)->finalize (object);
}

static void
yelp_gtk_settings_get_property (GObject    *object,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
    YelpGtkSettings *settings = YELP_GTK_SETTINGS (object);
    YelpGtkSettingsPrivate *priv = GET_PRIV (settings);

    switch (prop_id) {
    case PROP_GTK_SETTINGS:
        g_value_set_object (value, priv->gtk_settings);
        break;
    case PROP_GTK_ICON_THEME:
        g_value_set_object (value, priv->gtk_icon_theme);
        break;
    case PROP_FONT_ADJUSTMENT:
        g_value_set_int (value, priv->font_adjustment);
        break;
    case PROP_SHOW_TEXT_CURSOR:
        g_value_set_boolean (value, priv->show_text_cursor);
        break;
    case PROP_EDITOR_MODE:
        g_value_set_boolean (value, priv->editor_mode);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
yelp_gtk_settings_set_property (GObject      *object,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
    YelpGtkSettings *settings = YELP_GTK_SETTINGS (object);
    YelpGtkSettingsPrivate *priv = GET_PRIV (settings);

    switch (prop_id) {
    case PROP_GTK_SETTINGS:
        if (priv->gtk_settings) {
            g_signal_handler_disconnect (priv->gtk_settings,
                                         priv->gtk_theme_changed);
            g_signal_handler_disconnect (priv->gtk_settings,
                                         priv->gtk_font_changed);
            g_object_unref (priv->gtk_settings);
        }
        priv->gtk_settings = g_value_get_object (value);
        if (priv->gtk_settings != NULL) {
            g_object_ref (priv->gtk_settings);
            priv->gtk_theme_changed =
                g_signal_connect (priv->gtk_settings,
                                  "notify::gtk-theme-name",
                                  (GCallback) gtk_theme_changed,
                                  settings);
            priv->gtk_font_changed =
                g_signal_connect (priv->gtk_settings,
                                  "notify::gtk-font-name",
                                  (GCallback) gtk_font_changed,
                                  settings);
            gtk_theme_changed (priv->gtk_settings, NULL, settings);
            gtk_font_changed (priv->gtk_settings, NULL, settings);
        }
        else {
            priv->gtk_theme_changed = 0;
            priv->gtk_font_changed = 0;
        }
        break;
    case PROP_GTK_ICON_THEME:
        if (priv->gtk_icon_theme) {
            g_signal_handler_disconnect (priv->gtk_icon_theme,
                                         priv->icon_theme_changed);
            g_object_unref (priv->gtk_icon_theme);
        }
        priv->gtk_icon_theme = g_value_get_object (value);
        if (priv->gtk_icon_theme != NULL) {
            gchar **search_path;
            gint search_path_len, i;
            gboolean append_search_path = TRUE;
            gtk_icon_theme_get_search_path (priv->gtk_icon_theme,
                                            &search_path,
                                            &search_path_len);
            for (i = search_path_len - 1; i >= 0; i--)
                if (g_str_equal (search_path[i], YELP_ICON_PATH)) {
                    append_search_path = FALSE;
                    break;
                }
            if (append_search_path)
                gtk_icon_theme_append_search_path (priv->gtk_icon_theme,
                                                   YELP_ICON_PATH);
            append_search_path = TRUE;
            for (i = search_path_len - 1; i >= 0; i--)
                if (g_str_equal (search_path[i], DATADIR"/yelp/icons")) {
                    append_search_path = FALSE;
                    break;
                }
            if (append_search_path)
                gtk_icon_theme_append_search_path (priv->gtk_icon_theme,
                                                   DATADIR"/yelp/icons");
            g_strfreev (search_path);
            g_object_ref (priv->gtk_icon_theme);
            priv->icon_theme_changed =
                g_signal_connect (priv->gtk_icon_theme,
                                  "changed",
                                  (GCallback) icon_theme_changed,
                                  settings);
            icon_theme_changed (priv->gtk_icon_theme, settings);
        }
        else {
            priv->icon_theme_changed = 0;
        }
        break;
    case PROP_FONT_ADJUSTMENT:
        priv->font_adjustment = g_value_get_int (value);
        gtk_font_changed (priv->gtk_settings, NULL, settings);
        break;
    case PROP_SHOW_TEXT_CURSOR:
        priv->show_text_cursor = g_value_get_boolean (value);
        break;
    case PROP_EDITOR_MODE:
        priv->editor_mode = g_value_get_boolean (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

/******************************************************************************/

YelpGtkSettings *
yelp_gtk_settings_get_default (void)
{
    static GMutex mutex;
    static YelpGtkSettings *settings = NULL;
    g_mutex_lock (&mutex);
    if (settings == NULL) {
        const gchar * yelp_skip_gui = g_getenv("YELP_SKIP_GUI");
        if (yelp_skip_gui) {
            settings = g_object_new (YELP_TYPE_SETTINGS, NULL);
        }
        else {
            settings = g_object_new (YELP_TYPE_GTK_SETTINGS,
                                     "gtk-settings", gtk_settings_get_default (),
                                     "gtk-icon-theme", gtk_icon_theme_get_default (),
                                     NULL);
        }
    }
    g_mutex_unlock (&mutex);
    return settings;
}

/******************************************************************************/

static gchar *
get_color_locked (YelpGtkSettingsPrivate *priv,
                  YelpSettingsColor       color)
{
    g_assert (priv != NULL);
    g_assert (color < YELP_SETTINGS_NUM_COLORS);

    return g_strdup (priv->colors[color]);
}

gchar *
yelp_gtk_settings_get_color (YelpGtkSettings    *settings,
                             YelpSettingsColor   color)
{
    gchar *colorstr;
    YelpGtkSettingsPrivate *priv;

    g_return_val_if_fail (YELP_IS_GTK_SETTINGS (settings), NULL);
    g_return_val_if_fail (color < YELP_SETTINGS_NUM_COLORS, NULL);

    priv = GET_PRIV (settings);
    g_mutex_lock (&priv->mutex);
    colorstr = get_color_locked (priv, color);
    g_mutex_unlock (&priv->mutex);

    return colorstr;
}

void
yelp_gtk_settings_set_colors (YelpGtkSettings      *settings,
                              YelpSettingsColor  first_color,
                              ...)
{
    YelpGtkSettingsPrivate *priv = GET_PRIV (settings);
    YelpSettingsColor color;
    va_list args;

    g_mutex_lock (&priv->mutex);
    va_start (args, first_color);

    color = first_color;
    while ((gint) color >= 0) {
        gchar *colorstr = va_arg (args, gchar *);
        gint i;
        for (i = 0; i < 7; i++) {
            priv->colors[color][i] = colorstr[i];
            if (colorstr[i] == '\0')
                break;
        }
        priv->colors[color][7] = '\0';
        color = va_arg (args, YelpSettingsColor);
    }

    va_end (args);
    g_mutex_unlock (&priv->mutex);

    g_signal_emit (settings, settings_signals[COLORS_CHANGED], 0);
}

/******************************************************************************/

gchar *
yelp_gtk_settings_get_font (YelpGtkSettings     *settings,
                            YelpGtkSettingsFont  font)
{
    gchar *ret;
    YelpGtkSettingsPrivate *priv;

    g_return_val_if_fail (font < YELP_GTK_SETTINGS_NUM_FONTS, NULL);

    priv = GET_PRIV (settings);
    g_mutex_lock (&priv->mutex);
    if (priv->setfonts[font])
        ret = g_strdup (priv->setfonts[font]);
    else
        ret = g_strdup (priv->fonts[font]);
    g_mutex_unlock (&priv->mutex);

    return ret;
}

gchar *
yelp_gtk_settings_get_font_family (YelpGtkSettings     *settings,
                                   YelpGtkSettingsFont  font)
{
    const gchar *def = (font == YELP_GTK_SETTINGS_FONT_VARIABLE) ? "Sans" : "Monospace";
    gchar *desc, *ret, *c; /* do not free */
    YelpGtkSettingsPrivate *priv;

    g_return_val_if_fail (font < YELP_GTK_SETTINGS_NUM_FONTS, NULL);

    priv = GET_PRIV (settings);
    g_mutex_lock (&priv->mutex);

    if (priv->setfonts[font])
        desc = g_strdup (priv->setfonts[font]);
    else
        desc = g_strdup (priv->fonts[font]);

    if (desc == NULL) {
        ret = g_strdup (def);
        goto done;
    }

    c = strrchr (desc, ' ');
    if (c == NULL) {
        g_warning ("Cannot parse font: %s", desc);
        ret = g_strdup (def);
        goto done;
    }

    ret = g_strndup (desc, c - desc);

 done:
    g_mutex_unlock (&priv->mutex);
    return ret;
}

gint
yelp_gtk_settings_get_font_size (YelpGtkSettings     *settings,
                                 YelpGtkSettingsFont  font)
{
    gchar *desc, *c; /* do not free */
    gint ret;
    YelpGtkSettingsPrivate *priv;
    g_return_val_if_fail (font < YELP_GTK_SETTINGS_NUM_FONTS, 0);

    priv = GET_PRIV (settings);
    g_mutex_lock (&priv->mutex);

    if (priv->setfonts[font])
        desc = g_strdup (priv->setfonts[font]);
    else
        desc = g_strdup (priv->fonts[font]);

    if (desc == NULL) {
        ret = 10;
        goto done;
    }

    c = strrchr (desc, ' ');
    if (c == NULL) {
        g_warning ("Cannot parse font %s", desc);
        ret = 10;
        goto done;
    }

    ret = g_ascii_strtod (c, NULL);

 done:
    g_mutex_unlock (&priv->mutex);
    ret += priv->font_adjustment;
    ret = (ret < 5) ? 5 : ret;
    return ret;
}

void
yelp_gtk_settings_set_fonts (YelpGtkSettings     *settings,
                             YelpGtkSettingsFont  first_font,
                             ...)
{
    YelpGtkSettingsFont font;
    va_list args;
    YelpGtkSettingsPrivate *priv = GET_PRIV (settings);

    g_mutex_lock (&priv->mutex);
    va_start (args, first_font);

    font = first_font;
    while ((gint) font >= 0) {
        gchar *fontname = va_arg (args, gchar *);
        if (priv->setfonts[font] != NULL)
            g_free (priv->setfonts[font]);
        priv->setfonts[font] = g_strdup (fontname);
        font = va_arg (args, YelpGtkSettingsFont);
    }

    va_end (args);
    g_mutex_unlock (&priv->mutex);

    g_signal_emit (settings, settings_signals[FONTS_CHANGED], 0);
}

gint
yelp_gtk_settings_get_font_adjustment (YelpGtkSettings *settings)
{
    YelpGtkSettingsPrivate *priv = GET_PRIV (settings);
    return priv->font_adjustment;
}

void
yelp_gtk_settings_set_font_adjustment (YelpGtkSettings *settings,
                                   gint          adjustment)
{
    g_object_set (settings, "font-adjustment", adjustment, NULL);
}

/******************************************************************************/

gint
yelp_gtk_settings_get_icon_size (YelpGtkSettings *settings)
{
    YelpGtkSettingsPrivate *priv = GET_PRIV (settings);
    return priv->icon_size;
}

void
yelp_gtk_settings_set_icon_size (YelpGtkSettings *settings,
                                 gint          size)
{
    YelpGtkSettingsPrivate *priv = GET_PRIV (settings);
    priv->icon_size = size;
    if (priv->gtk_icon_theme != NULL)
        icon_theme_changed (priv->gtk_icon_theme, settings);
}

static gchar *
get_icon_locked (YelpGtkSettingsPrivate *priv,
                 YelpSettingsIcon        icon)
{
    g_assert (priv != NULL);
    g_assert (icon < YELP_SETTINGS_NUM_ICONS);

    return g_strdup (priv->icons[icon]);
}

gchar *
yelp_gtk_settings_get_icon (YelpGtkSettings  *settings,
                            YelpSettingsIcon  icon)
{
    gchar *ret;
    YelpGtkSettingsPrivate *priv;

    g_return_val_if_fail (YELP_IS_GTK_SETTINGS (settings), NULL);
    g_return_val_if_fail (icon < YELP_SETTINGS_NUM_ICONS, NULL);

    priv = GET_PRIV (settings);
    g_mutex_lock (&priv->mutex);
    ret = get_icon_locked (priv, icon);
    g_mutex_unlock (&priv->mutex);

    return ret;
}

void
yelp_gtk_settings_set_icons (YelpGtkSettings     *settings,
                             YelpSettingsIcon  first_icon,
                             ...)
{
    YelpSettingsIcon icon;
    va_list args;
    YelpGtkSettingsPrivate *priv = GET_PRIV (settings);

    g_mutex_lock (&priv->mutex);
    va_start (args, first_icon);

    icon = first_icon;
    while ((gint) icon >= 0) {
        gchar *filename = va_arg (args, gchar *);
        if (priv->icons[icon] != NULL)
            g_free (priv->icons[icon]);
        priv->icons[icon] = g_filename_to_uri (filename, NULL, NULL);
        icon = va_arg (args, YelpSettingsIcon);
    }

    va_end (args);
    g_mutex_unlock (&priv->mutex);

    g_signal_emit (settings, settings_signals[ICONS_CHANGED], 0);
}

/******************************************************************************/

gboolean
yelp_gtk_settings_get_show_text_cursor (YelpGtkSettings *settings)
{
    YelpGtkSettingsPrivate *priv = GET_PRIV (settings);
    return priv->show_text_cursor;
}

void
yelp_gtk_settings_set_show_text_cursor (YelpGtkSettings *settings,
                                    gboolean      show)
{
    g_object_set (settings, "show-text-cursor", show, NULL);
}

void
yelp_gtk_settings_set_editor_mode (YelpGtkSettings *settings,
                               gboolean      editor_mode)
{
    g_object_set (settings, "editor-mode", editor_mode, NULL);
}

/******************************************************************************/

static void
yelp_gtk_settings_set_if_token (YelpGtkSettings *settings,
                                const gchar  *token)
{
    YelpGtkSettingsPrivate *priv = GET_PRIV (settings);
    if (!g_hash_table_contains (priv->tokens, token))
        g_hash_table_add (priv->tokens, g_strdup (token));
}

/******************************************************************************/

static void
gtk_theme_changed (GtkSettings     *gtk_settings,
                   GParamSpec      *pspec,
                   YelpGtkSettings *settings)
{
    GtkStyleContext *context, *linkcontext;
    GtkWidget *tmpwin, *tmpbox, *tmpview, *tmplink;
    GdkRGBA base, text, link;
    gdouble    base_h, base_s, base_v;
    gdouble    text_h, text_s, text_v;
    YelpGtkSettingsPrivate *priv = GET_PRIV (settings);

    g_mutex_lock (&priv->mutex);

    tmpwin = gtk_offscreen_window_new ();
    tmpbox = gtk_grid_new ();
    tmpview = gtk_text_view_new ();
    tmplink = gtk_link_button_new ("http://projectmallard.org/");
    gtk_container_add (GTK_CONTAINER (tmpwin), tmpbox);
    gtk_container_add (GTK_CONTAINER (tmpbox), tmpview);
    gtk_container_add (GTK_CONTAINER (tmpbox), tmplink);
    gtk_widget_show_all (tmpwin);

    context = gtk_widget_get_style_context (tmpview);
    gtk_style_context_save (context);

    gtk_style_context_set_state (context, GTK_STATE_FLAG_NORMAL);
    gtk_style_context_add_class (context, GTK_STYLE_CLASS_VIEW);
    gtk_style_context_get_color (context, GTK_STATE_FLAG_NORMAL, &text);
    gtk_style_context_get_background_color (context, GTK_STATE_FLAG_NORMAL, &base);

    gtk_style_context_restore (context);

    rgb_to_hsv (text, &text_h, &text_s, &text_v);
    rgb_to_hsv (base, &base_h, &base_s, &base_v);

    /* YELP_SETTINGS_COLOR_BASE */
    g_snprintf (priv->colors[YELP_SETTINGS_COLOR_BASE], 8, "#%02X%02X%02X",
                (guint) (base.red * 255), (guint) (base.green * 255), (guint) (base.blue * 255));

    /* YELP_SETTINGS_COLOR_TEXT */
    g_snprintf (priv->colors[YELP_SETTINGS_COLOR_TEXT], 8, "#%02X%02X%02X",
                (guint) (text.red * 255), (guint) (text.green * 255), (guint) (text.blue * 255));

    linkcontext = gtk_widget_get_style_context (tmplink);
    gtk_style_context_save (linkcontext);

    /* YELP_SETTINGS_COLOR_LINK */
    gtk_style_context_set_state (linkcontext, GTK_STATE_FLAG_LINK);
    gtk_style_context_get_color (linkcontext, GTK_STATE_FLAG_LINK, &link);
    g_snprintf (priv->colors[YELP_SETTINGS_COLOR_LINK], 8, "#%02X%02X%02X",
                (guint) (link.red * 255), (guint) (link.green * 255), (guint) (link.blue * 255));

    /* YELP_SETTINGS_COLOR_LINK_VISITED */
    gtk_style_context_set_state (linkcontext, GTK_STATE_FLAG_VISITED);
    gtk_style_context_get_color (linkcontext, GTK_STATE_FLAG_VISITED, &link);
    g_snprintf (priv->colors[YELP_SETTINGS_COLOR_LINK_VISITED], 8, "#%02X%02X%02X",
                (guint) (link.red * 255), (guint) (link.green * 255), (guint) (link.blue * 255));


    gtk_style_context_restore (linkcontext);

    /* YELP_SETTINGS_COLOR_TEXT_LIGHT */
    hsv_to_hex (text_h, text_s, text_v - ((text_v - base_v) * 0.25),
                priv->colors[YELP_SETTINGS_COLOR_TEXT_LIGHT]);

    /* YELP_SETTINGS_COLOR_GRAY */
    hsv_to_hex (base_h, base_s,
                base_v - ((base_v - text_v) * 0.05),
                priv->colors[YELP_SETTINGS_COLOR_GRAY_BASE]);
    hsv_to_hex (base_h, base_s,
                base_v - ((base_v - text_v) * 0.1),
                priv->colors[YELP_SETTINGS_COLOR_DARK_BASE]);
    hsv_to_hex (base_h, base_s,
                base_v - ((base_v - text_v) * 0.26),
                priv->colors[YELP_SETTINGS_COLOR_GRAY_BORDER]);

    /* YELP_SETTINGS_COLOR_BLUE */
    hsv_to_hex (211, 0.1,
                base_v - ((base_v - text_v) * 0.01),
                priv->colors[YELP_SETTINGS_COLOR_BLUE_BASE]);
    hsv_to_hex (211, 0.45,
                base_v - ((base_v - text_v) * 0.19),
                priv->colors[YELP_SETTINGS_COLOR_BLUE_BORDER]);

    /* YELP_SETTINGS_COLOR_RED */
    hsv_to_hex (0, 0.13,
                base_v - ((base_v - text_v) * 0.01),
                priv->colors[YELP_SETTINGS_COLOR_RED_BASE]);
    hsv_to_hex (0, 0.83,
                base_v - ((base_v - text_v) * 0.06),
                priv->colors[YELP_SETTINGS_COLOR_RED_BORDER]);

    /* YELP_SETTINGS_COLOR_YELLOW */
    hsv_to_hex (60, 0.25,
                base_v - ((base_v - text_v) * 0.01),
                priv->colors[YELP_SETTINGS_COLOR_YELLOW_BASE]);
    hsv_to_hex (60, 1.0,
                base_v - ((base_v - text_v) * 0.07),
                priv->colors[YELP_SETTINGS_COLOR_YELLOW_BORDER]);

    gtk_widget_destroy (tmpwin);

    g_mutex_unlock (&priv->mutex);

    g_signal_emit (settings, settings_signals[COLORS_CHANGED], 0);
}

static void
gtk_font_changed (GtkSettings     *gtk_settings,
                  GParamSpec      *pspec,
                  YelpGtkSettings *settings)
{
    gchar *font, *c;
    YelpGtkSettingsPrivate *priv = GET_PRIV (settings);

    /* This happens when font_adjustment is set during init */
    if (gtk_settings == NULL)
        return;

    g_free (priv->fonts[YELP_GTK_SETTINGS_FONT_VARIABLE]);
    g_object_get (gtk_settings, "gtk-font-name", &font, NULL);
    priv->fonts[YELP_GTK_SETTINGS_FONT_VARIABLE] = font;

    c = strrchr (font, ' ');
    if (c == NULL) {
        g_warning ("Cannot parse font: %s", font);
        font = g_strdup ("Monospace 10");
    }
    else {
        font = g_strconcat ("Monospace", c, NULL);
    }

    g_free (priv->fonts[YELP_GTK_SETTINGS_FONT_FIXED]);
    priv->fonts[YELP_GTK_SETTINGS_FONT_FIXED] = font;

    g_signal_emit (settings, settings_signals[FONTS_CHANGED], 0);
}

static void
icon_theme_changed (GtkIconTheme    *theme,
                    YelpGtkSettings *settings)
{
    GtkIconInfo *info;
    gint i;
    YelpGtkSettingsPrivate *priv = GET_PRIV (settings);

    g_mutex_lock (&priv->mutex);

    for (i = 0; i < YELP_SETTINGS_NUM_ICONS; i++) {
        if (priv->icons[i] != NULL)
            g_free (priv->icons[i]);
        info = gtk_icon_theme_lookup_icon (theme,
                                           icon_names[i],
                                           priv->icon_size,
                                           GTK_ICON_LOOKUP_NO_SVG);
        if (info != NULL) {
            priv->icons[i] = g_filename_to_uri (gtk_icon_info_get_filename (info),
                                                          NULL, NULL);
            g_object_unref (info);
        }
        else {
            priv->icons[i] = NULL;
        }
    }

    g_mutex_unlock (&priv->mutex);

    g_signal_emit (settings, settings_signals[ICONS_CHANGED], 0);
}

gint
yelp_gtk_settings_cmp_icons (const gchar *icon1,
                             const gchar *icon2)
{
    static const gchar *icons[] = {
        "yelp-page-search-symbolic",
        "yelp-page-video-symbolic",
        "yelp-page-task-symbolic",
        "yelp-page-tip-symbolic",
        "yelp-page-problem-symbolic",
        "yelp-page-ui-symbolic",
        "yelp-page-symbolic",
        NULL
    };
    gint i;
    for (i = 0; icons[i] != NULL; i++) {
        gboolean eq1 = icon1 ? g_str_has_prefix (icon1, icons[i]) : FALSE;
        gboolean eq2 = icon2 ? g_str_has_prefix (icon2, icons[i]) : FALSE;
        if (eq1 && eq2)
            return 0;
        else if (eq1)
            return -1;
        else if (eq2)
            return 1;
    }
    if (icon1 == NULL && icon2 == NULL)
        return 0;
    else if (icon2 == NULL)
        return -1;
    else if (icon1 == NULL)
        return 1;
    else
        return strcmp (icon1, icon2);
}

/******************************************************************************/

static void
rgb_to_hsv (GdkRGBA color, gdouble *h, gdouble *s, gdouble *v)
{
    gdouble min, max, delta;

    max = (color.red > color.green) ? color.red : color.green;
    max = (max > color.blue) ? max : color.blue;
    min = (color.red < color.green) ? color.red : color.green;
    min = (min < color.blue) ? min : color.blue;

    delta = max - min;

    *v = max;
    *s = 0;
    *h = 0;

    if (max != min) {
        *s = delta / *v;

        if (color.red == max)
            *h = (color.green - color.blue) / delta;
        else if (color.green == max)
            *h = 2 + (color.blue - color.red) / delta;
        else if (color.blue == max)
            *h = 4 + (color.red - color.green) / delta;

        *h *= 60;
        if (*h < 0.0)
            *h += 360;
    }
}

static void
hsv_to_hex (gdouble h, gdouble s, gdouble v, gchar *str)
{
    gint hue;
    gdouble m1, m2, m3;
    gdouble r, g, b;
    guint red, green, blue;

    h /= 60;
    hue = (int) h;
    m1 = v * (1 - s);
    m2 = v * (1 - s * (h - hue));
    m3 = v * (1 - s * (-h + hue + 1));

    r = g = b = v;
    switch (hue) {
    case 0:
        b = m1; g = m3; break;
    case 1:
        b = m1; r = m2; break;
    case 2:
        r = m1; b = m3; break;
    case 3:
        r = m1; g = m2; break;
    case 4:
        g = m1; r = m3; break;
    case 5:
        g = m1; b = m2; break;
    default:
        g_assert_not_reached (); break;
    }

    red = r * 255;
    green = g * 255;
    blue = b * 255;
    g_snprintf (str, 8, "#%02X%02X%02X", red, green, blue);
}

static gchar **
get_colors (YelpSettings *settings)
{
    YelpGtkSettingsPrivate *priv;
    gchar **colors;
    YelpSettingsColor color;

    g_assert (YELP_IS_GTK_SETTINGS (settings));

    priv = GET_PRIV (YELP_GTK_SETTINGS (settings));
    colors = g_new0 (gchar *, YELP_SETTINGS_NUM_COLORS + 1);
    g_mutex_lock (&priv->mutex);
    for (color = YELP_SETTINGS_NUM_COLORS_START;
         color < YELP_SETTINGS_NUM_COLORS;
         color++)
        colors[color] = get_color_locked (priv, color);
    g_mutex_unlock (&priv->mutex);

    return colors;
}

static gint
get_icon_size (YelpSettings *settings)
{
    YelpGtkSettingsPrivate *priv;
    g_assert (YELP_IS_GTK_SETTINGS (settings));
    priv = GET_PRIV (YELP_GTK_SETTINGS (settings));
    return priv->icon_size;
}

static gchar **
get_icons (YelpSettings *settings)
{
    YelpGtkSettingsPrivate *priv;
    gchar **icons;
    YelpSettingsIcon icon;

    g_assert (YELP_IS_GTK_SETTINGS (settings));

    priv = GET_PRIV (YELP_GTK_SETTINGS (settings));
    icons = g_new0 (gchar *, YELP_SETTINGS_NUM_ICONS + 1);
    g_mutex_lock (&priv->mutex);
    for (icon = YELP_SETTINGS_NUM_ICONS_START;
         icon < YELP_SETTINGS_NUM_ICONS;
         icon++)
        icons[icon] = get_icon_locked (priv, icon);
    g_mutex_unlock (&priv->mutex);

    return icons;
}

static gboolean
get_editor_mode (YelpSettings *settings)
{
    YelpGtkSettingsPrivate *priv;
    g_assert (YELP_IS_GTK_SETTINGS (settings));
    priv = GET_PRIV (YELP_GTK_SETTINGS (settings));
    return priv->editor_mode;
}

static gchar **
get_tokens (YelpSettings *settings)
{
    YelpGtkSettingsPrivate *priv;
    gchar **tokens;
    GHashTableIter iter;
    const gchar *token;
    guint idx;

    g_assert (YELP_IS_GTK_SETTINGS (settings));

    priv = GET_PRIV (YELP_GTK_SETTINGS (settings));

    tokens = g_new0 (gchar *, g_hash_table_size (priv->tokens) + 1);
    idx = 0;
    g_hash_table_iter_init (&iter, priv->tokens);
    while (g_hash_table_iter_next (&iter, (gpointer *)&token, NULL)) {
        tokens[idx] = g_strdup (token);
        idx++;
    }
    return tokens;
}

static YelpSettingsTextDirection
get_text_direction (YelpSettings *settings)
{
    return (gtk_widget_get_default_direction() == GTK_TEXT_DIR_RTL) ?
        YELP_SETTINGS_TEXT_DIRECTION_RTL :
        YELP_SETTINGS_TEXT_DIRECTION_LTR;
}

static gchar *
get_uri_for_gicon (YelpSettings *settings,
                   GIcon        *icon)
{
    YelpGtkSettingsPrivate *priv;
    GtkIconInfo *info;
    const gchar *icon_file;
    gchar *icon_uri;

    g_assert (YELP_IS_GTK_SETTINGS (settings));
    g_assert (G_IS_ICON (icon));

    priv = GET_PRIV (YELP_GTK_SETTINGS (settings));
    info = gtk_icon_theme_lookup_by_gicon (priv->gtk_icon_theme, icon, 22, GTK_ICON_LOOKUP_NO_SVG);

    if (info == NULL)
        return NULL;
    icon_file = gtk_icon_info_get_filename (info);
    if (icon_file != NULL)
        icon_uri = g_filename_to_uri (icon_file, NULL, NULL);
    else
        icon_uri = NULL;

    g_object_unref (info);
    return icon_uri;
}

static void
yelp_gtk_settings_yelp_settings_init (YelpSettingsInterface *iface)
{
  iface->get_colors = get_colors;
  iface->get_icon_size = get_icon_size;
  iface->get_icons = get_icons;
  iface->get_editor_mode = get_editor_mode;
  iface->get_tokens = get_tokens;
  iface->get_text_direction = get_text_direction;
  iface->get_uri_for_gicon = get_uri_for_gicon;
}
