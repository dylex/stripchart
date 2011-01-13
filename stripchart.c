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
#include <gnome.h>

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
  GnomeProgram *stripchart;
  GOptionContext *option_context;
  Chart_app *app;

  prog_name = argv[0];
  if (strrchr(prog_name, '/'))
    prog_name = strrchr(prog_name, '/') + 1;

  option_context = g_option_context_new (PACKAGE);
  g_option_context_add_main_entries (option_context, option_entries, NULL);
  stripchart = gnome_program_init(PACKAGE, VERSION, LIBGNOMEUI_MODULE, argc, argv, 
		  GNOME_PARAM_GOPTION_CONTEXT, option_context, 
		  GNOME_PARAM_NONE);

  app = chart_app_new();
  app->frame = gnome_app_new("stripchart", ("Stripchart"));
  gtk_window_set_default_size(GTK_WINDOW(app->frame), 360, 50);
  if (geometry)
	  gtk_window_parse_geometry(GTK_WINDOW(app->frame), geometry);
  gtk_widget_show(app->frame);
  g_signal_connect(GTK_OBJECT(app->frame), "destroy", G_CALLBACK(gtk_main_quit), NULL);

  gnome_app_set_contents(GNOME_APP(app->frame), app->hbox);

  gtk_widget_add_events(app->strip, GDK_BUTTON_PRESS_MASK);
  g_signal_connect(GTK_OBJECT(app->frame),
    "button-press-event", G_CALLBACK(on_button_press), app);
  g_signal_connect(GTK_OBJECT(app->frame),
    "popup-menu", G_CALLBACK(on_popup_menu), app);

  gtk_main();
  return EXIT_SUCCESS;
}
