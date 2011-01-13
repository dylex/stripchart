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
  xmlChar buf[100];
  xmlStrPrintf(buf, 100, BAD_CAST fmt, val);
  xmlNewChild(node, NULL, BAD_CAST key, buf);
}

void
prefs_to_doc(Chart_app *app, xmlDocPtr doc)
{
  xmlNodePtr node = xmlNewChild(xmlDocGetRootElement(doc), NULL, BAD_CAST "preferences-list", NULL);

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
  ||  !xmlstreq(xmlDocGetRootElement(doc)->name, "stripchart") )
    {
      xmlFreeDoc(doc);
      error("Can't parse preferences file \"%s\".\n", fn);
      return 1;
    }

  for (list = xmlDocGetRootElement(doc)->xmlChildrenNode; list != NULL; list = list->next)
    if (list->type == XML_ELEMENT_NODE && xmlstreq(list->name,"preferences-list"))
      for (node = list->xmlChildrenNode; node; node = node->next)
	if (node->type == XML_ELEMENT_NODE)
	{
	  const xmlChar *key = node->name;
	  const char *val = (const char *)node->xmlChildrenNode->content;

	  if (xmlstreq(key, "strip-update"))
	    app->strip_param_group->interval = atoi(val);
	  else if (xmlstreq(key, "strip-smooth"))
	    app->strip_param_group->filter = atof(val);
	  else if (xmlstreq(key, "ticks-enable"))
	    STRIP(app->strip)->show_ticks = atoi(val);
	  else if (xmlstreq(key, "ticks-minor"))
	    STRIP(app->strip)->minor_ticks = atoi(val);
	  else if (xmlstreq(key, "ticks-major"))
	    STRIP(app->strip)->major_ticks = atoi(val);
	  else
	    fprintf(stderr,
	      "%s: unrecognized parameter element: \"%s\" (%s)\n",
	      prog_name, key, val);
	}

  xmlFreeDoc(doc);
  return 0;
}

static void
prefs_load(Prefs_edit *prefs, Chart_app *app)
{
  gtk_adjustment_set_value(GTK_ADJUSTMENT(prefs->strip_interval), 
    app->strip_param_group->interval / 1000.0);

  gtk_adjustment_set_value(GTK_ADJUSTMENT(prefs->strip_filter), 
    1 - app->strip_param_group->filter);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(prefs->ticks_button),
    STRIP(app->strip)->show_ticks);

  gtk_adjustment_set_value(GTK_ADJUSTMENT(prefs->minor_ticks),
    STRIP(app->strip)->minor_ticks);
  gtk_adjustment_set_value(GTK_ADJUSTMENT(prefs->major_ticks),
    STRIP(app->strip)->major_ticks);
}

static void
prefs_apply(Prefs_edit *prefs, Chart_app *app)
{
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
on_prefs_click(GtkDialog *w, gint button, Prefs_edit *prefs)
{
  Chart_app *app = prefs->app;
  switch(button)
    {
    case GTK_RESPONSE_OK: /* Okay */
      prefs_apply(prefs, app);
      gtk_widget_destroy(prefs->dialog);
      g_free(prefs);
      break;
    case GTK_RESPONSE_APPLY: /* Apply */
      prefs_apply(prefs, app);
      break;
    case GTK_RESPONSE_CANCEL: /* Cancel */
    case GTK_RESPONSE_DELETE_EVENT:
      gtk_widget_destroy(prefs->dialog);
      g_free(prefs);
      break;
    }
  gtk_widget_queue_draw(app->strip);
}

void
on_prefs_edit(GtkWidget *w, Chart_app *app)
{
  Prefs_edit *prefs;
  GtkWidget *vbox, *tick_box;
  GtkWidget *chart_frame, *chart_table;
  GtkWidget *major_spin, *minor_spin;

  prefs = g_malloc(sizeof(*prefs));
  prefs->app = app;
  prefs->dialog = gtk_dialog_new_with_buttons("Preferences",
		  GTK_WINDOW(app->frame), GTK_DIALOG_DESTROY_WITH_PARENT,
		  GTK_STOCK_OK, GTK_RESPONSE_OK,
		  GTK_STOCK_APPLY, GTK_RESPONSE_APPLY,
		  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		  NULL);
/*
  gtk_window_set_position(GTK_WINDOW(prefs->dialog), GTK_WIN_POS_MOUSE);
*/
  g_signal_connect(prefs->dialog, "response", G_CALLBACK(on_prefs_click), prefs);

  vbox = GTK_DIALOG(prefs->dialog)->vbox;

  chart_frame = gtk_frame_new(("Chart"));
  gtk_box_pack_start(GTK_BOX(vbox), chart_frame, TRUE, TRUE, 0);

  chart_table = gtk_table_new(3, 2, FALSE);
  gtk_container_add(GTK_CONTAINER(chart_frame), chart_table);

  tick_box = gtk_hbox_new(FALSE, 0);
  gtk_table_attach(GTK_TABLE(chart_table),
    tick_box, 1, 2, 2, 3, GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);

  prefs->ticks_button = gtk_check_button_new_with_label(("enabled"));
  gtk_box_pack_start(GTK_BOX(tick_box), prefs->ticks_button, FALSE, FALSE, 0);

  prefs->minor_ticks = gtk_adjustment_new(1, 0, 100, 1, 10, 0);
  minor_spin = gtk_spin_button_new(GTK_ADJUSTMENT(prefs->minor_ticks), 1, 0);
  gtk_box_pack_start(GTK_BOX(tick_box), minor_spin, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(tick_box),
    gtk_label_new(("minor")), FALSE, FALSE, 0);

  prefs->major_ticks = gtk_adjustment_new(1, 0, 100, 1, 10, 0);
  major_spin = gtk_spin_button_new(GTK_ADJUSTMENT(prefs->major_ticks), 1, 0);
  gtk_box_pack_start(GTK_BOX(tick_box), major_spin, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(tick_box),
    gtk_label_new(("major")), FALSE, FALSE, 0);

  prefs->strip_interval = gtk_adjustment_new(5, 1, 60, 1, 5, 0);
  gtk_table_attach(GTK_TABLE(chart_table),
    gtk_hscale_new(GTK_ADJUSTMENT(prefs->strip_interval)),
    1, 2, 0, 1, GTK_FILL, GTK_FILL, 0, 0);

  prefs->strip_filter = gtk_adjustment_new(0.5, 0, 1, 0.01, 0.1, 0);
  gtk_table_attach(GTK_TABLE(chart_table),
    gtk_hscale_new(GTK_ADJUSTMENT(prefs->strip_filter)),
    1, 2, 1, 2, GTK_FILL, GTK_FILL, 0, 0);

  gtk_table_attach(GTK_TABLE(chart_table),
    gtk_label_new(("Update")), 0, 1, 0, 1, 0, 0, 8, 0);

  gtk_table_attach(GTK_TABLE(chart_table),
    gtk_label_new(("Smooth")), 0, 1, 1, 2, 0, 0, 8, 0);

  gtk_table_attach(GTK_TABLE(chart_table), 
    gtk_label_new(("Ticks")), 0, 1, 2, 3, 0, 0, 8, 0);

  gtk_box_pack_start(GTK_BOX(vbox),
    gtk_hseparator_new(), TRUE, TRUE, 12);

  prefs_load(prefs, app);
  gtk_widget_show_all(prefs->dialog);
}

