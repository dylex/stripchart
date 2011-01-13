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

#ifndef CHART_APP_H
#define CHART_APP_H

#include "config.h"

#include "chart.h"
#include "strip.h"

extern char *prog_name;
extern char *config_fn;

typedef struct _Chart_app
{
  struct _Param_group *strip_param_group;

  char *config_fn;
  GtkWidget *frame, *hbox, *strip;
  GtkWidget *text_window, *text_clist, *file_sel, *editor;
  GtkNotebook *notebook;
}
Chart_app;

#include "prefs.h"
#include "params.h"
#include "eval.h"
#include "utils.h"

gboolean on_button_press(GtkWidget *win, GdkEventButton *event, Chart_app *app);
gboolean on_popup_menu(GtkWidget *win, Chart_app *app);
void text_refresh(Chart *chart, Chart_app *app);
Chart_app *chart_app_new(void);

#endif /* CHART_APP_H */
