/* Stripchart -- the gnome-utils stripchart plotting utility
 * Copyright (C) 2000 John Kodis <kodis@jagunet.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "chart-app.h"
#include <string.h>

char *prog_name;
char *config_fn = NULL;
const char *geometry = NULL;

static
GOptionEntry option_entries[] =
{
  { "geometry",        'g', 0, G_OPTION_ARG_STRING, &geometry, "Geometry string: WxH+X+Y", "GEO" },
  { "config-file",     'f', 0, G_OPTION_ARG_FILENAME, &config_fn, "Configuration file name", "FILE" },
  { NULL }
};

int
main(int argc, char *argv[])
{
  Chart_app *app;
  GError *error = NULL;

  prog_name = argv[0];
  if (strrchr(prog_name, '/'))
    prog_name = strrchr(prog_name, '/') + 1;

  if (!gtk_init_with_args(&argc, &argv, NULL, option_entries, NULL, &error))
  {
	  g_printerr("%s\n", error->message);
	  g_error_free (error);
	  return EXIT_FAILURE;
  }

  app = chart_app_new();
  app->frame = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(app->frame), "Stripchart");
  gtk_window_set_default_size(GTK_WINDOW(app->frame), 360, 50);
  if (geometry)
	  gtk_window_parse_geometry(GTK_WINDOW(app->frame), geometry);
  gtk_widget_show(app->frame);
  g_signal_connect(GTK_OBJECT(app->frame), "destroy", G_CALLBACK(gtk_main_quit), NULL);

  gtk_container_add(GTK_CONTAINER(app->frame), app->hbox);

  gtk_widget_add_events(app->strip, GDK_BUTTON_PRESS_MASK);
  g_signal_connect(GTK_OBJECT(app->frame),
    "button-press-event", G_CALLBACK(on_button_press), app);
  g_signal_connect(GTK_OBJECT(app->frame),
    "popup-menu", G_CALLBACK(on_popup_menu), app);

  gtk_main();
  return EXIT_SUCCESS;
}
