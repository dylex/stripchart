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
#include "strip.h"

static void
fmt_node(xmlNodePtr node, const char *key, const char *fmt, double val)
{
  char buf[100];
  sprintf(buf, fmt, val);
  xmlNewChild(node, NULL, key, buf);
}

void
prefs_to_doc(Chart_app *app, xmlDocPtr doc)
{
  xmlNodePtr node = xmlNewChild(xmlDocGetRootElement(doc), NULL, "preferences-list", NULL);

  fmt_node(node, "strip-update", "%.0f", app->strip_param_group->interval);
  fmt_node(node, "strip-smooth", "%.4f", app->strip_param_group->filter);

  fmt_node(node, "ticks-enable", "%.0f", STRIP(app->strip)->show_ticks);
  fmt_node(node, "ticks-minor", "%.0f", STRIP(app->strip)->minor_ticks);
  fmt_node(node, "ticks-major", "%.0f", STRIP(app->strip)->major_ticks);
}

int
prefs_ingest(Chart_app *app, char *fn)
{
  xmlDocPtr doc = xmlParseFile(fn);
  xmlNodePtr list, node;

  if (!doc || doc->type != XML_DOCUMENT_NODE
  ||  !xmlDocGetRootElement(doc) || xmlDocGetRootElement(doc)->type != XML_ELEMENT_NODE
  ||  !streq(xmlDocGetRootElement(doc)->name, "stripchart") )
    {
      error("Can't parse preferences file \"%s\".\n", fn);
      return 1;
    }

  for (list = xmlDocGetRootElement(doc)->xmlChildrenNode; list != NULL; list = list->next)
    if (list->type == XML_ELEMENT_NODE && streq(list->name,"preferences-list"))
      for (node = list->xmlChildrenNode; node; node = node->next)
	if (node->type == XML_ELEMENT_NODE)
	{
	  const char *key = node->name;
	  const char *val = node->xmlChildrenNode->content;

	  if (streq(key, "strip-update"))

	    app->strip_param_group->interval = atoi(val);
	  else if (streq(key, "strip-smooth"))
	    app->strip_param_group->filter = atof(val);
	  else if (streq(key, "ticks-enable"))
	    STRIP(app->strip)->show_ticks = atoi(val);
	  else if (streq(key, "ticks-minor"))
	    STRIP(app->strip)->minor_ticks = atoi(val);
	  else if (streq(key, "ticks-major"))
	    STRIP(app->strip)->major_ticks = atoi(val);
	  else
	    fprintf(stderr,
	      "%s: unrecognized parameter element: \"%s\" (%s)\n",
	      prog_name, key, val);
	}

  return 0;
}

static void
prefs_load(Chart_app *app)
{
  gtk_adjustment_set_value(GTK_ADJUSTMENT(app->prefs->strip_interval), 
    app->strip_param_group->interval / 1000.0);

  gtk_adjustment_set_value(GTK_ADJUSTMENT(app->prefs->strip_filter), 
    1 - app->strip_param_group->filter);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(app->prefs->ticks_button),
    STRIP(app->strip)->show_ticks);

  gtk_adjustment_set_value(GTK_ADJUSTMENT(app->prefs->minor_ticks),
    STRIP(app->strip)->minor_ticks);
  gtk_adjustment_set_value(GTK_ADJUSTMENT(app->prefs->major_ticks),
    STRIP(app->strip)->major_ticks);
}

static void
prefs_apply(Chart_app *app)
{
  Prefs_edit *prefs = app->prefs;

  double strip_interval = GTK_ADJUSTMENT(prefs->strip_interval)->value;
  double strip_filter = GTK_ADJUSTMENT(prefs->strip_filter)->value;

  int ticks = gtk_toggle_button_get_active(
    GTK_TOGGLE_BUTTON(prefs->ticks_button));

  double minor = GTK_ADJUSTMENT(prefs->minor_ticks)->value;
  double major = GTK_ADJUSTMENT(prefs->major_ticks)->value;

  app->strip_param_group->filter = 1 - strip_filter;

  app->strip_param_group->interval = strip_interval * 1000;
  chart_set_interval(CHART(app->strip), app->strip_param_group->interval);

  strip_set_ticks(STRIP(app->strip), ticks, major, minor);
}

static void
on_prefs_click(GtkWidget *w, int button, Chart_app *app)
{
  switch(button)
    {
    case 0: /* Okay */
      prefs_apply(app);
      gnome_dialog_close(GNOME_DIALOG(app->prefs->dialog));
      break;
    case 1: /* Apply */
      prefs_apply(app);
      break;
    case 2: /* Cancel */
      gnome_dialog_close(GNOME_DIALOG(app->prefs->dialog));
      break;
    }
  gtk_widget_queue_draw(app->strip);
}

static void
on_prefs_delete(GtkWidget *win, GdkEvent *event, void *nil)
{
  gnome_dialog_close(GNOME_DIALOG(win));
}

void
on_prefs_edit(GtkWidget *w, Chart_app *app)
{
  Prefs_edit *prefs;
  GtkWidget *vbox, *tick_box;
  GtkWidget *chart_frame, *chart_table;
  GtkWidget *major_spin, *minor_spin;

  if (app->prefs)
    {
      prefs_load(app);
      gtk_widget_show(app->prefs->dialog);
      return;
    }

  prefs = app->prefs = malloc(sizeof(*prefs));
  prefs->dialog = gnome_dialog_new(_("Preferences"),
    GNOME_STOCK_BUTTON_OK, GNOME_STOCK_BUTTON_APPLY,
    GNOME_STOCK_BUTTON_CANCEL, NULL);
  gnome_dialog_close_hides(GNOME_DIALOG(prefs->dialog), TRUE);
/*
  gtk_window_set_position(GTK_WINDOW(prefs->dialog), GTK_WIN_POS_MOUSE);
*/
  gtk_signal_connect(GTK_OBJECT(prefs->dialog),
    "clicked", GTK_SIGNAL_FUNC(on_prefs_click), app);
  gtk_signal_connect(GTK_OBJECT(prefs->dialog),
    "delete-event", (GtkSignalFunc)on_prefs_delete, NULL);
  vbox = GNOME_DIALOG(prefs->dialog)->vbox;

  chart_frame = gtk_frame_new(_("Chart"));
  gtk_box_pack_start(GTK_BOX(vbox), chart_frame, TRUE, TRUE, 0);

  chart_table = gtk_table_new(3, 2, FALSE);
  gtk_container_add(GTK_CONTAINER(chart_frame), chart_table);

  tick_box = gtk_hbox_new(FALSE, 0);
  gtk_table_attach(GTK_TABLE(chart_table),
    tick_box, 1, 2, 2, 3, GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);

  prefs->ticks_button = gtk_check_button_new_with_label(_("enabled"));
  gtk_box_pack_start(GTK_BOX(tick_box), prefs->ticks_button, FALSE, FALSE, 0);

  prefs->minor_ticks = gtk_adjustment_new(1, 0, 100, 1, 10, 10);
  minor_spin = gtk_spin_button_new(GTK_ADJUSTMENT(prefs->minor_ticks), 1, 0);
  gtk_box_pack_start(GTK_BOX(tick_box), minor_spin, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(tick_box),
    gtk_label_new(_("minor")), FALSE, FALSE, 0);

  prefs->major_ticks = gtk_adjustment_new(1, 0, 100, 1, 10, 10);
  major_spin = gtk_spin_button_new(GTK_ADJUSTMENT(prefs->major_ticks), 1, 0);
  gtk_box_pack_start(GTK_BOX(tick_box), major_spin, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(tick_box),
    gtk_label_new(_("major")), FALSE, FALSE, 0);

  prefs->strip_interval = gtk_adjustment_new(5, 1, 60, 1, 0, 0);
  gtk_table_attach(GTK_TABLE(chart_table),
    gtk_hscale_new(GTK_ADJUSTMENT(prefs->strip_interval)),
    1, 2, 0, 1, GTK_FILL, GTK_FILL, 0, 0);

  prefs->strip_filter = gtk_adjustment_new(0.5, 0, 1, 0.01, 0, 0);
  gtk_table_attach(GTK_TABLE(chart_table),
    gtk_hscale_new(GTK_ADJUSTMENT(prefs->strip_filter)),
    1, 2, 1, 2, GTK_FILL, GTK_FILL, 0, 0);

  gtk_table_attach(GTK_TABLE(chart_table),
    gtk_label_new(_("Update")), 0, 1, 0, 1, 0, 0, 8, 0);

  gtk_table_attach(GTK_TABLE(chart_table),
    gtk_label_new(_("Smooth")), 0, 1, 1, 2, 0, 0, 8, 0);

  gtk_table_attach(GTK_TABLE(chart_table), 
    gtk_label_new(_("Ticks")), 0, 1, 2, 3, 0, 0, 8, 0);

  gtk_box_pack_start(GTK_BOX(vbox),
    gtk_hseparator_new(), TRUE, TRUE, 12);

  prefs_load(app);
  gtk_widget_show_all(app->prefs->dialog);
}

