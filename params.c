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
#include <libxml/parser.h>	/* XML input routines */
#include <libxml/tree.h>	/* XML output routines */
#include <gtk/gtk.h>


static const char *whitespace = " \t\r\n";

static void
set_entry(GtkWidget *entry, const char *text)
{
  gtk_entry_set_text(GTK_ENTRY(entry), text? text: "");
}

static void
set_color(ChartDatum *datum, const GdkColor *color, int cnum)
{
  Chart *chart = datum->chart;

  datum->gdk_color[cnum] = *color;
  gdk_colormap_alloc_color(chart->colormap, &datum->gdk_color[cnum], FALSE, TRUE);
  datum->gdk_gc[cnum] = gdk_gc_new(GTK_WIDGET(chart)->window);
  gdk_gc_set_foreground(datum->gdk_gc[cnum], &datum->gdk_color[cnum]);
}

static void
on_color_set(GtkColorButton *picker,
  Param_page *page)
{
  GdkColor color;
  gtk_color_button_get_color(picker, &color);
  int cnum;

  for (cnum = 0; cnum <= page->colors; cnum++)
    if (page->color[cnum] == GTK_WIDGET(picker))
      break;

  if (cnum < page->colors && page->strip_data)
      set_color(page->strip_data, &color, cnum);
}

static int
add_color(Param_page *page)
{
  if (page->colors <= page->shown)
    {
      page->color = realloc(page->color,
	(page->colors + 1) * sizeof(*page->color));
      page->color[page->colors] = gtk_color_button_new();
      gtk_box_pack_start(GTK_BOX(page->color_hbox),
	page->color[page->colors], FALSE, FALSE, 0);
      g_signal_connect(page->color[page->colors],
	"color-set", G_CALLBACK(on_color_set), page);
      page->colors++;
    }
  gtk_widget_show(page->color[page->shown]);
  return page->shown++;
}

static void
param_page_set_from_desc(Param_page *page, const Param_desc *desc)
{
  char *names, *color;

  set_entry(page->name, desc? desc->name: NULL);

  gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(page->desc)),
		  desc ? desc->desc : "", -1);

  set_entry(page->eqn, desc? desc->eqn: NULL);
  set_entry(page->fn, desc? desc->fn: NULL);
  set_entry(page->pattern, desc? desc->pattern: NULL);
  set_entry(page->top_min, desc? desc->top_min: NULL);
  set_entry(page->top_max, desc? desc->top_max: NULL);
  set_entry(page->bot_min, desc? desc->bot_min: NULL);
  set_entry(page->bot_max, desc? desc->bot_max: NULL);

  if (desc == NULL)
    goto RETURN;

  switch (str_to_scale_style(desc->scale))
    {
    case chart_scale_log:
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(page->log), TRUE);
      break;
    default:
    case chart_scale_linear:
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(page->linear), TRUE);
      break;
    }

  switch (str_to_plot_style(desc->plot))
    {
    case chart_plot_point:
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(page->point), TRUE);
      break;
    default:
    case chart_plot_line:
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(page->line), TRUE);
      break;
    case chart_plot_solid:
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(page->solid), TRUE);
      break;
    case chart_plot_indicator:
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(page->indicator), TRUE);
      break;
    }

  names = g_strdup(desc->color_names);
  color = strtok(names, whitespace);
  while (color)
    {
      GdkColor rgb;
      int c = add_color(page);
      if (gdk_color_parse(color, &rgb))
	gtk_color_button_set_color(GTK_COLOR_BUTTON(page->color[c]), &rgb);
      color = strtok(NULL, whitespace);
    }
  g_free(names);

 RETURN:
  page->changed = FALSE; /* Only manual changes count. */
}

static Param_page *
get_current_page_param(GtkNotebook *notebook)
{
  GtkWidget *page;
  int pageno = gtk_notebook_get_current_page(notebook);

  if (pageno < 0)
    return NULL;

  page = gtk_notebook_get_nth_page(notebook, pageno);
  return (Param_page *)g_object_get_data(G_OBJECT(page), "page");
}

/*
 * param_opt_ingest -- reads an XML parameter file into a
 * null-terminated array of Param_desc objects. 
 */
Param_desc **
param_desc_ingest(const char *fn)
{
  int item_count = 0;
  Param_desc **desc_ptr = g_malloc(sizeof(*desc_ptr));
  xmlDocPtr doc = xmlParseFile(fn);
  xmlNodePtr list, param, elem;

  if (!doc || doc->type != XML_DOCUMENT_NODE
  ||  !xmlDocGetRootElement(doc) || xmlDocGetRootElement(doc)->type != XML_ELEMENT_NODE
  ||  !xmlstreq(xmlDocGetRootElement(doc)->name, "stripchart") )
    {
      static int complaints;
      if (complaints++ == 0)
	error("Can't parse parameter file \"%s\".\n", fn);
      xmlFreeDoc(doc);
      return NULL;
    }

  for (list = xmlDocGetRootElement(doc)->xmlChildrenNode; list != NULL; list = list->next)
    if (list->type == XML_ELEMENT_NODE && xmlstreq(list->name, "parameter-list"))
      for (param = list->xmlChildrenNode; param; param = param->next)
	if (param->type == XML_ELEMENT_NODE && xmlstreq(param->name, "parameter"))
	  {
	    Param_desc *desc = g_malloc0(sizeof(*desc));
	    for (elem = param->xmlChildrenNode; elem; elem = elem->next)
	      if (elem->type == XML_ELEMENT_NODE)
	      {
		const xmlChar *key = elem->name;
		char *val = g_strdup((const char *)elem->xmlChildrenNode->content);

		if (xmlstreq(key, "name"))
		  desc->name = val;
		else if (xmlstreq(key, "description"))
		  desc->desc = val;
		else if (xmlstreq(key, "equation"))
		  desc->eqn = val;
		else if (xmlstreq(key, "filename"))
		  desc->fn = val;
		else if (xmlstreq(key, "pattern"))
		  desc->pattern = val;
		else if (xmlstreq(key, "top-min"))
		  desc->top_min = val;
		else if (xmlstreq(key, "top-max"))
		  desc->top_max = val;
		else if (xmlstreq(key, "bot-min"))
		  desc->bot_min = val;
		else if (xmlstreq(key, "bot-max"))
		  desc->bot_max = val;
		else if (xmlstreq(key, "scale"))
		  desc->scale = val;
		else if (xmlstreq(key, "plot"))
		  desc->plot = val;
		else if (xmlstreq(key, "color"))
		  desc->color_names = val;
		else
		{
		  fprintf(stderr,
		    "%s: file %s: unrecognized tag \"%s\" containing \"%s\"\n",
		    prog_name, fn, key, val);
		  g_free(val);
		}
	      }
	    desc_ptr = g_realloc(desc_ptr,
	      (item_count + 2) * sizeof(*desc_ptr));
	    desc_ptr[item_count++] = desc;
	  }

  xmlFreeDoc(doc);
  desc_ptr[item_count] = NULL;
  return desc_ptr;
}

static char *
edit_str(GtkWidget *entry)
{
  return gtk_editable_get_chars(GTK_EDITABLE(entry), 0, -1);
}

static void
page_to_desc(Param_page *page, Param_desc *desc)
{
  int c, len;

  desc->name = edit_str(page->name);
  GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(page->desc));
  GtkTextIter start, end;
  gtk_text_buffer_get_bounds(buf, &start, &end);
  desc->desc = gtk_text_buffer_get_text(buf, &start, &end, FALSE);
  desc->eqn = edit_str(page->eqn);
  desc->fn = edit_str(page->fn);
  desc->pattern = edit_str(page->pattern);
  desc->top_min = edit_str(page->top_min);
  desc->top_max = edit_str(page->top_max);
  desc->bot_min = edit_str(page->bot_min);
  desc->bot_max = edit_str(page->bot_max);

  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(page->log)))
    desc->scale = strdup("log");
  else
    desc->scale = strdup("linear");

  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(page->indicator)))
    desc->plot = strdup("indicator");
  else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(page->point)))
    desc->plot = strdup("point");
  else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(page->solid)))
    desc->plot = strdup("solid");
  else
    desc->plot = strdup("line");

  desc->color_names = g_malloc(page->shown * 16);
  desc->color_names[0] = 0;
  for (c = len = 0; c < page->shown; c++)
    {
      GdkColor rgb;
      gtk_color_button_get_color(GTK_COLOR_BUTTON(page->color[c]), &rgb);
      len += sprintf(desc->color_names + len, "%s ", gdk_color_to_string(&rgb));
    }
  if (len)
    desc->color_names[len - 1] = '\0';
}

static void 
clear_desc(Param_desc *desc)
{
	g_free(desc->name);
	g_free(desc->desc);
	g_free(desc->eqn);
	g_free(desc->fn);
	g_free(desc->pattern);
	g_free(desc->top_min);
	g_free(desc->top_max);
	g_free(desc->bot_min);
	g_free(desc->bot_max);
	g_free(desc->scale);
	g_free(desc->plot);
	g_free(desc->color_names);
}

static void
add_node(xmlNodePtr node, const char *key, const char *val)
{
  if (val && *val)
    xmlNewTextChild(node, NULL, BAD_CAST key, BAD_CAST val);
}

int
opts_to_file(Chart_app *app, char *fn)
{
  int p = 0, stat;
  xmlDocPtr doc;
  xmlNodePtr list, node;
  GtkWidget *nb_page;

  doc = xmlNewDoc(BAD_CAST "1.0");
  xmlDocSetRootElement(doc, xmlNewDocNode(doc, NULL, BAD_CAST "stripchart", NULL));

  prefs_to_doc(app, doc);

  list = xmlNewChild(xmlDocGetRootElement(doc), NULL, BAD_CAST "parameter-list", NULL);

  while ((nb_page = gtk_notebook_get_nth_page(app->notebook, p)) != NULL)
    {
      Param_page *page = g_object_get_data(G_OBJECT(nb_page), "page");
      if (!page->strip_data || page->strip_data->active)
	{
	  Param_desc desc;
	  page_to_desc(page, &desc);
	  node = xmlNewChild(list, NULL, BAD_CAST "parameter", NULL);

	  add_node(node, "name", desc.name);
	  add_node(node, "description", desc.desc);
	  add_node(node, "equation", desc.eqn);
	  add_node(node, "filename", desc.fn);
	  add_node(node, "pattern", desc.pattern);
	  add_node(node, "top-min", desc.top_min);
	  add_node(node, "top-max", desc.top_max);
	  add_node(node, "bot-min", desc.bot_min);
	  add_node(node, "bot-max", desc.bot_max);
	  add_node(node, "scale", desc.scale);
	  add_node(node, "plot", desc.plot);
	  add_node(node, "color", desc.color_names);
	  clear_desc(&desc);
	}
      p++;
    }
  stat = xmlSaveFile(fn, doc);
#ifdef DEBUG
  printf("opts to file: wrote %d bytes to %s\n", stat, fn);
#endif
  xmlFreeDoc(doc);
  return stat;
}

static void
on_save(GtkAction *act, Chart_app *app)
{
  if (opts_to_file(app, app->config_fn) <= 0)
    error("can't save parameters to file \"%s\".", app->config_fn);
}

static void
on_save_as_response(GtkWidget *fs, gint resp, Chart_app *app)
{
      if (resp == GTK_RESPONSE_ACCEPT)
      {
	      g_free(app->config_fn);
	      app->config_fn = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(fs));
	      on_save(NULL, app);
      }
      gtk_widget_destroy(fs);
}


static void
on_save_as(GtkAction *act, Chart_app *app)
{
      GtkWidget *fs = gtk_file_chooser_dialog_new("Save As...", GTK_WINDOW(app->editor), 
		      GTK_FILE_CHOOSER_ACTION_SAVE,
		      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		      GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
		      NULL);
      gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(fs), app->config_fn);
      g_signal_connect(fs, "response", G_CALLBACK(on_save_as_response), app);
      gtk_widget_show(fs);
}

static void
on_close(GtkAction *act, Chart_app *app)
{
  gtk_widget_hide(GTK_WIDGET(app->editor));
}

static void
on_add_color(GtkAction *act, Chart_app *app)
{
  Param_page *page = get_current_page_param(app->notebook);
  add_color(page);
}

static void
on_delete_color(GtkAction *act, Chart_app *app)
{
  Param_page *page = get_current_page_param(app->notebook);

  if (page->shown > 1)
    gtk_widget_hide(page->color[--page->shown]);
}

static void
on_notebook_switch_page(GtkNotebook *notebook,
  GtkNotebookPage *page, gint page_num, Chart_app *app)
{
  #ifdef DEBUG
  int n = gtk_notebook_get_current_page(app->notebook);
  printf("switch page: %p, %d, %d\n", app, n, page_num);
  #endif
}

static void
on_relabel(GtkWidget *widget, Param_page *page)
{
  gtk_notebook_set_tab_label_text(GTK_NOTEBOOK(page->notebook),
    page->table, gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1));
}

static void
on_altered(GtkWidget *unused, Param_page *page)
{
  Param_desc desc;
  ChartPlotStyle plot_style;
  ChartScaleStyle scale_style;

  if (page->strip_data == NULL)
    return;

  #ifdef DEBUG
  printf("on altered: page=%p\n", page);
  #endif
  page_to_desc(page, &desc);

  plot_style = str_to_plot_style(desc.plot);
  chart_set_plot_style(page->strip_data, plot_style);

  scale_style = str_to_scale_style(desc.scale);
  chart_set_scale_style(page->strip_data, scale_style);

  clear_desc(&desc);
}

static void
on_change(GtkWidget *unused, Param_page *page)
{
  page->changed = TRUE;
}

static void
on_top_max(GtkWidget *widget, Param_page *page)
{
  char *s = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);

  if (page->strip_data == NULL)
    return;

  #ifdef DEBUG
  printf("top max: %p, %p, %p, \"%s\", %f\n",
    widget, page, page->strip_data, s, atof(s));
  #endif
  chart_set_top_max(page->strip_data, atof(s));
}

static void
on_top_min(GtkWidget *widget, Param_page *page)
{
  char *s = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);

  if (page->strip_data == NULL)
    return;

  #ifdef DEBUG
  printf("top min: %p, %p, %p, \"%s\", %f\n", 
    widget, page, page->strip_data, s, atof(s));
  #endif
  chart_set_top_min(page->strip_data, atof(s));
}

static void
on_bot_max(GtkWidget *widget, Param_page *page)
{
  char *s = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);

  if (page->strip_data == NULL)
    return;

  #ifdef DEBUG
  printf("bot max: %p, %p, %p, \"%s\", %f\n", 
    widget, page, page->strip_data, s, atof(s));
  #endif
  chart_set_bot_max(page->strip_data, atof(s));
}

static void
on_bot_min(GtkWidget *widget, Param_page *page)
{
  char *s = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);

  if (page->strip_data == NULL)
    return;

  #ifdef DEBUG
  printf("bot min: %p, %p, %p, \"%s\", %f\n", 
    widget, page, page->strip_data, s, atof(s));
  #endif
  chart_set_bot_min(page->strip_data, atof(s));
}

static void
on_type_toggle(GtkToggleButton *type, Param_page *page)
{
  int c;
  if (gtk_toggle_button_get_active(type))
    for (c = 1; c < page->colors; c++)
      gtk_widget_hide(page->color[c]);
  else
    for (c = 1; c < page->shown; c++)
      gtk_widget_show(page->color[c]);

  on_change(GTK_WIDGET(type), page);
}

static void
create_param_page(Chart_app *app, Param_page *page, Param_desc *desc)
{
  GtkWidget *label, *desc_scroller;
  GtkWidget *scale_hbox, *type_hbox, *top_hbox, *bot_hbox;
  GSList *scale_hbox_group = NULL, *type_hbox_group = NULL;

  page->notebook = GTK_WIDGET(app->notebook);
  page->table = gtk_table_new(10, 2, FALSE);
  gtk_widget_show(page->table);

  label = gtk_label_new(("Parameter"));
  gtk_widget_show(label);
  gtk_table_attach(GTK_TABLE(page->table),
    label, 0, 1, 0, 1, 0, 0, 0, 0);

  page->name = gtk_entry_new();
  gtk_widget_show(page->name);
  g_signal_connect(page->name, "changed", G_CALLBACK(on_relabel), page);
  gtk_table_attach(GTK_TABLE(page->table),
    page->name, 1, 2, 0, 1, 0, 0, 0, 0);

  label = gtk_label_new(("Description"));
  gtk_widget_show(label);
  gtk_table_attach(GTK_TABLE(page->table),
    label, 0, 1, 1, 2, 0, 0, 0, 0);

  desc_scroller = gtk_scrolled_window_new(NULL, NULL);
  gtk_widget_show(desc_scroller);
  gtk_table_attach(GTK_TABLE(page->table),
    desc_scroller, 1, 2, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(desc_scroller),
    GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
  page->desc = gtk_text_view_new();
  gtk_widget_show(page->desc);
  gtk_text_view_set_editable(GTK_TEXT_VIEW(page->desc), TRUE);
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(page->desc), GTK_WRAP_WORD_CHAR);
  gtk_container_add(GTK_CONTAINER(desc_scroller), page->desc);

  label = gtk_label_new(("Equation"));
  gtk_widget_show(label);
  gtk_table_attach(GTK_TABLE(page->table),
    label, 0, 1, 2, 3, 0, 0, 0, 0);

  page->eqn = gtk_entry_new();
  gtk_widget_show(page->eqn);
  g_signal_connect(page->eqn, "changed", G_CALLBACK(on_change), page);
  gtk_table_attach(GTK_TABLE(page->table),
    page->eqn, 1, 2, 2, 3, (GTK_EXPAND | GTK_FILL), 0, 0, 0);

  label = gtk_label_new(("Filename"));
  gtk_widget_show(label);
  gtk_table_attach(GTK_TABLE(page->table),
    label, 0, 1, 3, 4, 0, 0, 0, 0);

  page->fn = gtk_entry_new();
  gtk_widget_show(page->fn);
  g_signal_connect(page->fn, "changed", G_CALLBACK(on_change), page);
  gtk_table_attach(GTK_TABLE(page->table),
    page->fn, 1, 2, 3, 4, (GTK_EXPAND | GTK_FILL), 0, 0, 0);

  label = gtk_label_new(("Pattern"));
  gtk_widget_show(label);
  gtk_table_attach(GTK_TABLE(page->table),
    label, 0, 1, 4, 5, 0, 0, 0, 0);

  page->pattern = gtk_entry_new();
  gtk_widget_show(page->pattern);
  g_signal_connect(page->pattern, "changed", G_CALLBACK(on_change), page);
  gtk_table_attach(GTK_TABLE(page->table),
    page->pattern, 1, 2, 4, 5, (GTK_EXPAND | GTK_FILL), 0, 0, 0);

  label = gtk_label_new(("Top"));
  gtk_widget_show(label);
  gtk_table_attach(GTK_TABLE(page->table),
    label, 0, 1, 5, 6, 0, 0, 0, 0);

  top_hbox = gtk_hbox_new(FALSE, 0);
  gtk_widget_show(top_hbox);
  gtk_table_attach(GTK_TABLE(page->table),
    top_hbox, 1, 2, 5, 6, 0, 0, 0, 0);

  label = gtk_label_new(("Min"));
  gtk_widget_show(label);
  gtk_box_pack_start(GTK_BOX(top_hbox), label, FALSE, FALSE, 0);

  page->top_min = gtk_entry_new();
  gtk_widget_show(page->top_min);
  g_signal_connect(page->top_min, "changed", G_CALLBACK(on_top_min), page);
  gtk_box_pack_start(GTK_BOX(top_hbox), page->top_min, FALSE, FALSE, 0);

  label = gtk_label_new(("Max"));
  gtk_widget_show(label);
  gtk_box_pack_start(GTK_BOX(top_hbox), label, FALSE, FALSE, 0);

  page->top_max = gtk_entry_new();
  gtk_widget_show(page->top_max);
  g_signal_connect(page->top_min, "changed", G_CALLBACK(on_top_max), page);
  gtk_box_pack_start(GTK_BOX(top_hbox), page->top_max, FALSE, FALSE, 0);

  label = gtk_label_new(("Bottom"));
  gtk_widget_show(label);
  gtk_table_attach(GTK_TABLE(page->table),
    label, 0, 1, 6, 7, 0, 0, 0, 0);

  bot_hbox = gtk_hbox_new(FALSE, 0);
  gtk_widget_show(bot_hbox);
  gtk_table_attach(GTK_TABLE(page->table),
    bot_hbox, 1, 2, 6, 7, 0, 0, 0, 0);

  label = gtk_label_new(("Min"));
  gtk_widget_show(label);
  gtk_box_pack_start(GTK_BOX(bot_hbox), label, FALSE, FALSE, 0);

  page->bot_min = gtk_entry_new();
  gtk_widget_show(page->bot_min);
  g_signal_connect(G_OBJECT(page->bot_min), "changed", G_CALLBACK(on_bot_min), page);
  gtk_box_pack_start(GTK_BOX(bot_hbox), page->bot_min, FALSE, FALSE, 0);

  label = gtk_label_new(("Max"));
  gtk_widget_show(label);
  gtk_box_pack_start(GTK_BOX(bot_hbox), label, FALSE, FALSE, 0);

  page->bot_max = gtk_entry_new();
  gtk_widget_show(page->bot_max);
  g_signal_connect(G_OBJECT(page->bot_min), "changed", G_CALLBACK(on_bot_max), page);
  gtk_box_pack_start(GTK_BOX(bot_hbox), page->bot_max, FALSE, FALSE, 0);

  label = gtk_label_new(("Scale"));
  gtk_widget_show(label);
  gtk_table_attach(GTK_TABLE(page->table),
    label, 0, 1, 7, 8, 0, 0, 0, 0);

  scale_hbox = gtk_hbox_new(FALSE, 0);
  gtk_widget_show(scale_hbox);
  gtk_table_attach(GTK_TABLE(page->table),
    scale_hbox, 1, 2, 7, 8, GTK_FILL, GTK_FILL, 0, 0);

  page->linear =
    gtk_radio_button_new_with_label(scale_hbox_group, ("Linear"));
  scale_hbox_group =
    gtk_radio_button_get_group(GTK_RADIO_BUTTON(page->linear));
  g_signal_connect(G_OBJECT(page->linear), "toggled", G_CALLBACK(on_altered), page);
  gtk_widget_show(page->linear);
  gtk_box_pack_start(GTK_BOX(scale_hbox), page->linear, FALSE, FALSE, 0);

  page->log =
    gtk_radio_button_new_with_label(scale_hbox_group, ("Logarithmic"));
  scale_hbox_group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(page->log));
  gtk_widget_show(page->log);
  gtk_box_pack_start(GTK_BOX(scale_hbox), page->log, FALSE, FALSE, 0);

  label = gtk_label_new(("Type"));
  gtk_widget_show(label);
  gtk_table_attach(GTK_TABLE(page->table),
    label, 0, 1, 8, 9, 0, 0, 0, 0);

  type_hbox = gtk_hbox_new(FALSE, 0);
  gtk_widget_show(type_hbox);
  gtk_table_attach(GTK_TABLE(page->table),
    type_hbox, 1, 2, 8, 9, GTK_FILL, GTK_FILL, 0, 0);

  page->indicator =
    gtk_radio_button_new_with_label(type_hbox_group, ("Indicator"));
  type_hbox_group =
    gtk_radio_button_get_group(GTK_RADIO_BUTTON(page->indicator));
  gtk_widget_show(page->indicator);
  gtk_box_pack_start(GTK_BOX(type_hbox), page->indicator, FALSE, FALSE, 0);

  page->line =
    gtk_radio_button_new_with_label(type_hbox_group, ("Line"));
  type_hbox_group =
    gtk_radio_button_get_group(GTK_RADIO_BUTTON(page->line));
  gtk_widget_show(page->line);
  gtk_box_pack_start(GTK_BOX(type_hbox), page->line, FALSE, FALSE, 0);

  page->point =
    gtk_radio_button_new_with_label(type_hbox_group, ("Point"));
  type_hbox_group =
    gtk_radio_button_get_group(GTK_RADIO_BUTTON(page->point));
  gtk_widget_show(page->point);
  gtk_box_pack_start(GTK_BOX(type_hbox), page->point, FALSE, FALSE, 0);

  page->solid =
    gtk_radio_button_new_with_label(type_hbox_group, ("Solid"));
  type_hbox_group =
    gtk_radio_button_get_group(GTK_RADIO_BUTTON(page->solid));
  gtk_widget_show(page->solid);
  gtk_box_pack_start(GTK_BOX(type_hbox), page->solid, FALSE, FALSE, 0);

  g_signal_connect(G_OBJECT(page->indicator), "toggled", G_CALLBACK(on_type_toggle), page);
  g_signal_connect(G_OBJECT(page->line), "toggled", G_CALLBACK(on_type_toggle), page);
  g_signal_connect(G_OBJECT(page->point), "toggled", G_CALLBACK(on_type_toggle), page);
  g_signal_connect(G_OBJECT(page->solid), "toggled", G_CALLBACK(on_type_toggle), page);

  label = gtk_label_new(("Color"));
  gtk_widget_show(label);
  gtk_table_attach(GTK_TABLE(page->table),
    label, 0, 1, 9, 10, 0, 0, 0, 0);

  page->color_hbox = gtk_hbox_new(FALSE, 0);
  gtk_widget_show(page->color_hbox);
  gtk_table_attach(GTK_TABLE(page->table),
    page->color_hbox, 1, 2, 9, 10, GTK_FILL, GTK_FILL, 0, 0);

  param_page_set_from_desc(page, desc);
}

Param_page *
add_page_before(Chart_app *app, int n, Param_desc *desc)
{
  static int pno;
  char pno_str[100];
  Param_page *page = g_malloc0(sizeof(*page));

  if (desc && desc->name)
    strcpy(pno_str, desc->name);
  else
    sprintf(pno_str, "Param %d", pno++);

  create_param_page(app, page, desc);

  gtk_notebook_insert_page(app->notebook,
    page->table, gtk_label_new(pno_str), n);
  gtk_notebook_set_current_page(app->notebook, n);
  g_object_set_data(G_OBJECT(page->table), "page", page);

#ifdef DEBUG
  printf("add_param: n %d, page %p\n", n, page);
#endif /* DEBUG */
  return page;
}

static void
on_add_before_param(GtkAction *act, Chart_app *app)
{
  int n = gtk_notebook_get_current_page(app->notebook);
  add_page_before(app, n+0, NULL);
}

static void
on_add_after_param(GtkAction *act, Chart_app *app)
{
  int n = gtk_notebook_get_current_page(app->notebook);
  add_page_before(app, n+1, NULL);
}

static void
on_apply(GtkAction *act, Chart_app *app)
{
  int p = 0;
  GtkWidget *nb_page;

  while ((nb_page = gtk_notebook_get_nth_page(app->notebook, p)) != NULL)
    {
      Param_page *page = g_object_get_data(G_OBJECT(nb_page), "page");
      if (page->changed)
	{
	  ChartDatum *strip_datum = NULL;
	  Param_desc desc;
	  page_to_desc(page, &desc);

	  strip_datum = chart_equation_add(CHART(app->strip),
	    app->strip_param_group, &desc, NULL, p,
	    str_to_plot_style(desc.plot) != chart_plot_indicator);

	  if (strip_datum)
	    {
	      if (page->strip_data && page->strip_data->active)
		chart_parameter_deactivate(CHART(app->strip),page->strip_data);
	      page->strip_data = strip_datum;
	    }

	  clear_desc(&desc);
	}
      p++;
    }
}

static void
on_delete_param(GtkAction *act, Chart_app *app)
{
  int n = gtk_notebook_get_current_page(app->notebook);
  GtkWidget *nb_page = gtk_notebook_get_nth_page(app->notebook, n);
  Param_page *page = g_object_get_data(G_OBJECT(nb_page), "page");

  chart_parameter_deactivate(CHART(app->strip), page->strip_data);

  gtk_notebook_remove_page(app->notebook, n);
  if (app->notebook->children == NULL)
    add_page_before(app, 0, NULL);
}

static const GtkActionEntry menu_entries[] = {
	{ "FileMenu", NULL, "_File" },
	{ "EditMenu", NULL, "_Edit" },
	{ "Save", GTK_STOCK_SAVE, "_Save", "<control>S", "Save configuration", G_CALLBACK(on_save) },
	{ "SaveAs", GTK_STOCK_SAVE_AS, "Save _As...", NULL, "Save configuration in a different file", G_CALLBACK(on_save_as) },
	{ "Close", GTK_STOCK_CLOSE, "_Close", "<control>C", "Close parameter editor", G_CALLBACK(on_close) },
	{ "AddBefore", GTK_STOCK_ADD, "Add _Before", NULL, "Add a new parameter before the current one", G_CALLBACK(on_add_before_param) },
	{ "AddAfter", GTK_STOCK_ADD, "Add _After", NULL, "Add a new parameter after the current one", G_CALLBACK(on_add_after_param) },
	{ "Apply", GTK_STOCK_APPLY, "A_pply", "<control>A", "Apply changes", G_CALLBACK(on_apply) },
	{ "Delete", GTK_STOCK_DELETE, "_Delete", NULL, "Delete parameter", G_CALLBACK(on_delete_param) },
	{ "AddColor", GTK_STOCK_COLOR_PICKER, "Add _Color", NULL, "Provide a color for the current parameter", G_CALLBACK(on_add_color) },
	{ "DeleteColor", GTK_STOCK_REMOVE, "Delete Color", NULL, "Remove color from the current parameter", G_CALLBACK(on_delete_color) },
};

static const char *menu_desc = 
	"<ui><menubar name='EditorMenu'>"
		"<menu action='FileMenu'>"
			"<menuitem action='Save'/>"
			"<menuitem action='SaveAs'/>"
			"<menuitem action='Close'/>"
		"</menu>"
		"<menu action='EditMenu'>"
			"<menuitem action='Apply'/>"
			"<menuitem action='AddBefore'/>"
			"<menuitem action='AddAfter'/>"
			"<menuitem action='Delete'/>"
			"<separator/>"
			"<menuitem action='AddColor'/>"
			"<menuitem action='DeleteColor'/>"
		"</menu>"
	"</menubar></ui>";


static void
on_edit_menu(GtkWidget *item, Chart_app *app)
{
  Param_page *page = get_current_page_param(app->notebook);
  GtkToggleButton *indy = GTK_TOGGLE_BUTTON(page->indicator);
  int is_indicator = gtk_toggle_button_get_active(indy);

  gtk_widget_set_sensitive(app->edit_apply, page->changed);
  gtk_widget_set_sensitive(app->edit_addcolor, is_indicator);
  gtk_widget_set_sensitive(app->edit_rmcolor,
    is_indicator && page->shown > 1);
}

static void
on_editor_delete(GtkWidget *win, GdkEvent *event, void *nil)
{
  gtk_widget_hide(win);
}

void
on_param_edit(GtkWidget *unused, GtkWidget *editor)
{
  if (! GTK_WIDGET_VISIBLE(editor))
    gtk_widget_show(editor);
}

void
create_editor(Chart_app *app)
{
  GtkWidget *edit_vbox, *edit_menubar;
  GtkActionGroup *action_group;
  GtkUIManager *ui_manager;

  app->editor = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(app->editor), ("Parameter Editor"));
  gtk_window_set_position(GTK_WINDOW(app->editor), GTK_WIN_POS_NONE);
  g_signal_connect(G_OBJECT(app->editor),
    "delete-event", G_CALLBACK(on_editor_delete), NULL);

  edit_vbox = gtk_vbox_new(FALSE, 0);
  gtk_widget_show(edit_vbox);
  gtk_container_add(GTK_CONTAINER(app->editor), edit_vbox);

  action_group = gtk_action_group_new("MenuActions");
  gtk_action_group_add_actions(action_group, menu_entries, G_N_ELEMENTS(menu_entries), app);
  ui_manager = gtk_ui_manager_new();
  gtk_ui_manager_insert_action_group(ui_manager, action_group, 0);
  gtk_ui_manager_add_ui_from_string(ui_manager, menu_desc, -1, NULL);

  edit_menubar = gtk_ui_manager_get_widget(ui_manager, "/EditorMenu");
  app->edit_apply = gtk_ui_manager_get_widget(ui_manager, "/EditorMenu/EditMenu/Apply");
  app->edit_addcolor = gtk_ui_manager_get_widget(ui_manager, "/EditorMenu/EditMenu/AddColor");
  app->edit_rmcolor = gtk_ui_manager_get_widget(ui_manager, "/EditorMenu/EditMenu/DeleteColor");
  gtk_widget_show(edit_menubar);
  gtk_box_pack_start(GTK_BOX(edit_vbox), edit_menubar, FALSE, FALSE, 0);

  app->notebook = GTK_NOTEBOOK(gtk_notebook_new());
  gtk_widget_show(GTK_WIDGET(app->notebook));
  gtk_box_pack_start(GTK_BOX(edit_vbox),
    GTK_WIDGET(app->notebook), TRUE, TRUE, 0);
  gtk_notebook_set_tab_pos(app->notebook, GTK_POS_BOTTOM);

  g_signal_connect(G_OBJECT(app->notebook),
    "switch_page", G_CALLBACK(on_notebook_switch_page), app);
  g_signal_connect(G_OBJECT(gtk_ui_manager_get_widget(ui_manager, "/EditorMenu/EditMenu")),
    "activate", G_CALLBACK(on_edit_menu), app);
}
