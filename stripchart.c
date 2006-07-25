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
// #include <mcheck.h>

char *prog_name;
char *config_fn = NULL;
const char *geometry = NULL;

static gboolean quit_event(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	gtk_main_quit();
	return FALSE;
}

static int
proc_arg(int opt, const char *arg)
{
  switch (opt)
    {
    case 'f':
      config_fn = strdup(arg);
      break;
    case 'g':
      geometry = arg;
      break;
    }
  return 0;
}

void
popt_arg_extractor(
  poptContext state, enum poptCallbackReason reason,
  const struct poptOption *opt, const char *arg, void *data )
{
  if (proc_arg(opt->val, arg))
    {
      poptPrintUsage(state, stderr, 0);
      exit(EXIT_FAILURE);
    }
}

static struct
poptOption popt_options[] =
{
  { NULL,             '\0', POPT_ARG_CALLBACK, popt_arg_extractor },
  { "geometry",        'g', POPT_ARG_STRING, NULL, 'g',
    N_("Geometry string: WxH+X+Y"), N_("GEO") },
  { "config-file",     'f', POPT_ARG_STRING, NULL, 'f',
    N_("Configuration file name"), N_("FILE") },
  { NULL,             '\0', 0, NULL, 0 }
};

int geometry_w = -1, geometry_h = -1;
int geometry_x = -1, geometry_y = -1;
  
int
main(int argc, char *argv[])
{
	// mtrace();

  Chart_app *app;

  prog_name = argv[0];
  if (strrchr(prog_name, '/'))
    prog_name = strrchr(prog_name, '/') + 1;

  gnome_init_with_popt_table(
    _("Stripchart"), VERSION, argc, argv, popt_options, 0, NULL);

  app = chart_app_new();
  app->frame = gnome_app_new("stripchart", _("Stripchart"));
  gtk_window_set_default_size(GTK_WINDOW(app->frame), 360, 50);
  //gtk_widget_set_uposition(app->frame, geometry_x, geometry_y);
  gtk_widget_show(app->frame);
  gtk_signal_connect(GTK_OBJECT(app->frame), "destroy", GTK_SIGNAL_FUNC(quit_event), NULL);

  gnome_app_set_contents(GNOME_APP(app->frame), app->hbox);

  if (geometry)
	  gtk_window_parse_geometry(GTK_WINDOW(app->frame), geometry);

  gtk_widget_add_events(app->strip, GDK_BUTTON_PRESS_MASK);
  gtk_signal_connect(GTK_OBJECT(app->frame),
    "button-press-event", GTK_SIGNAL_FUNC(on_button_press), app);

  gtk_main();
  return EXIT_SUCCESS;
}
