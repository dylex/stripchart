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

#include <sys/stat.h>
#include <sys/time.h>

/*
 * digit_shift -- a hi_lo_fmt helper function.
 */
static void
digit_shift(char *str, char *digits, int width, int dpos)
{
  int w;
  if (++dpos <= 0)
    {
      *str++ = '.';
      for (w = 1; w < width; w++)
        *str++ = (++dpos <= 0) ? '0' : *digits++;
    }
  else
    for (w = 0; w < width; w++)
      {
        *str++ = *digits++;
        if (--dpos == 0)
          {
            *str++ = '.';
            w++;
          }
      }
}

/*
 * hi_lo_fmt -- formats a hi and lo value to the same magnitude.
 */
static void
val_fmt(double v, char *vs)
{
  int e, p, t;
  char s[50];

  sprintf(s, "% 22.16e", v);
  e = atoi(s+20);
  s[2] = s[1]; s[1] = '0';
  t = ((e - (e < 0)) / 3);
  p = e - 3*t;
  vs[0] = s[0];
  digit_shift(vs+1, s+2, 4, p);
  vs[6] = '\0';
  if (-4 <= t && t <= 5)
    vs[5] = "pnum\0KMGTP"[t+4];
  else
    sprintf(vs+5, "e%d", 3*t);
}

static char *
param_type_str(Param_page *page)
{
  int led = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(page->indicator));
  int log = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(page->log));

  return led ? "*" : (log ? "log" : "-");
}

enum {
	TEXT_COLUMN_PARAM,
	TEXT_COLUMN_CURRENT,
	TEXT_COLUMN_SCALE,
	TEXT_COLUMN_BOT,
	TEXT_COLUMN_TOP,
	TEXT_COLUMN_COLOR,
	TEXT_COLUMNS
};

/*
 * text_load_tree -- fills a tree with chart ident, current, and top values.
 */
static void
text_load_tree(Chart_app *app)
{
  int p = 0;
  GtkWidget *nb_page;
  GtkTreeIter iter;
  char val_str[50], top_str[50], bot_str[50];

  g_object_freeze_notify(G_OBJECT(app->text_store));
  gtk_list_store_clear(app->text_store);

  for (p = 0; (nb_page = gtk_notebook_get_nth_page(app->notebook, p)) != NULL; p ++)
    {
      Param_page *page = g_object_get_data(G_OBJECT(nb_page), "page");
      ChartDatum *datum = page->strip_data;

      if (datum && datum->active)
	{
	  char *name = gtk_editable_get_chars(GTK_EDITABLE(page->name), 0,-1);
	  val_fmt(datum->history[datum->newest], val_str);
	  val_fmt(datum->adj->lower, bot_str);
	  val_fmt(datum->adj->upper, top_str);
	  gtk_list_store_insert_with_values(app->text_store, &iter, -1,
			  TEXT_COLUMN_PARAM, name,
			  TEXT_COLUMN_CURRENT, val_str,
			  TEXT_COLUMN_SCALE, param_type_str(page),
			  TEXT_COLUMN_BOT, bot_str,
			  TEXT_COLUMN_TOP, top_str,
			  TEXT_COLUMN_COLOR, &page->strip_data->gdk_color[0],
			  -1);

	  g_free(name);
	}
    }

  g_object_thaw_notify(G_OBJECT(app->text_store));
}

/*
 * text_refresh -- if the text window is up, refresh the values.
 */
void
text_refresh(Chart *chart, Chart_app *app)
{
  if (app->text_window && GTK_WIDGET_VISIBLE(app->text_window))
    text_load_tree(app);
}

/*
 * on_show_values -- pops up a top level textual display of current
 * parameter values in response to a mouse click.  Closes this window
 * if it's already displayed.
 */
static void
on_show_values(GtkWidget *unused, Chart_app *app)
{
  if (app->text_window == NULL)
    {
      const struct {
	      const char *title;
	      GType type;
	      gboolean show;
      } text_column_info[] = {
	      { "Param", 	G_TYPE_STRING, TRUE },
	      { "Current", 	G_TYPE_STRING, TRUE },
	      { "Scale", 	G_TYPE_STRING, TRUE },
	      { "Bot",		G_TYPE_STRING, TRUE },
	      { "Top",		G_TYPE_STRING, TRUE },
	      { "Color",	GDK_TYPE_COLOR, FALSE }
      };

      int i;
      GType types[TEXT_COLUMNS];

      for (i = 0; i < TEXT_COLUMNS; i ++)
	      types[i] = text_column_info[i].type;

      app->text_store = gtk_list_store_newv(TEXT_COLUMNS, types);
      GtkWidget *text_tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(app->text_store));
      GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
      g_object_set(G_OBJECT(renderer), "background-gdk", &app->frame->style->bg[0], NULL);
      
      for (i = 0; i < TEXT_COLUMNS; i++)
      {
	      if (!text_column_info[i].show)
		      continue;
	      GtkTreeViewColumn *col = gtk_tree_view_column_new_with_attributes(text_column_info[i].title, renderer, 
			      "text", i,
			      "foreground-gdk", TEXT_COLUMN_COLOR,
			      NULL);
	      gtk_tree_view_column_set_sizing(col, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	      gtk_tree_view_append_column(GTK_TREE_VIEW(text_tree), col);
      }
      gtk_widget_show(text_tree);

      app->text_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
      gtk_widget_set_style(GTK_WIDGET(app->text_window), app->frame->style);
      gtk_container_add(GTK_CONTAINER(app->text_window), text_tree);
      gtk_window_set_transient_for(GTK_WINDOW(app->text_window), GTK_WINDOW(app->frame));
      gtk_widget_show(app->text_window);

      /* Load the initial batch of parameter values after showing the
         toplevel window.  This odd sequence is required to get the
         auto-sizing right. */
      text_load_tree(app);

      g_signal_connect(G_OBJECT(app->text_window),
	"delete-event", G_CALLBACK(gtk_widget_hide), app->text_window);
    }
  else if (GTK_WIDGET_VISIBLE(app->text_window))
    gtk_widget_hide(app->text_window);
  else
    gtk_widget_show(app->text_window);
}

static void
menu_popup(GtkWidget *widget, GdkEventButton *event, Chart_app *app)
{
  static GtkWidget *menu;

  if (menu == NULL)
    {
      GtkWidget *menu_item, *ed = app->editor;

      menu = gtk_menu_new();

      menu_item = gtk_menu_item_new_with_label(("Show values"));
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
      g_signal_connect(G_OBJECT(menu_item),
	"activate", G_CALLBACK(on_show_values), app);

      gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

      menu_item = gtk_menu_item_new_with_label(("Edit Prefs..."));
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
      g_signal_connect(G_OBJECT(menu_item),
	"activate", G_CALLBACK(on_prefs_edit), app);

      menu_item = gtk_menu_item_new_with_label(("Edit Params..."));
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
      g_signal_connect(G_OBJECT(menu_item),
	"activate", G_CALLBACK(on_param_edit), ed);

      gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

      menu_item = gtk_menu_item_new_with_label(("Exit"));
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
      g_signal_connect_swapped(G_OBJECT(menu_item),
	"activate", G_CALLBACK(gtk_main_quit), NULL);

      gtk_widget_show_all(menu);
      gtk_menu_attach_to_widget (GTK_MENU (menu), widget, NULL);
    }

  gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, 
    event ? event->button : 0, event ? event->time : 0);
}

gboolean
on_button_press(GtkWidget *win, GdkEventButton *event, Chart_app *app)
{
  int button = event->button;
#ifdef DEBUG
  printf("on button press: %p, %p, %p\n", win, event, app);
#endif
  if (event->type == GDK_BUTTON_PRESS)
  switch (button)
    {
    case 1: on_show_values(NULL, app); return TRUE;
    case 3: menu_popup(win, event, app); return TRUE;
    }
  return FALSE;
}

gboolean
on_popup_menu(GtkWidget *win, Chart_app *app)
{
	menu_popup(win, NULL, app);
	return TRUE;
}

Chart_app *
chart_app_new(void)
{
  int p;
  char *config_path[20];
  struct stat stat_buf;
  Param_desc **param_desc = NULL;
  Chart_app *app = g_malloc(sizeof(*app));

  app->strip_param_group = g_malloc0(sizeof(*app->strip_param_group));
  app->text_window = NULL;

  app->hbox = gtk_hbox_new(/*homo*/0, /*pad*/0);
  gtk_widget_show(app->hbox);

  app->strip = strip_new();
  gtk_widget_show(app->strip);
  gtk_box_pack_start(GTK_BOX(app->hbox),
    app->strip, /*expand*/1, /*fill*/1, /*pad*/0);
  g_signal_connect(G_OBJECT(app->strip),
    "chart_pre_update", G_CALLBACK(chart_start), app->strip_param_group);

  g_signal_connect(G_OBJECT(app->strip),
    "chart_post_update", G_CALLBACK(text_refresh), app);
  strip_set_ticks(STRIP(app->strip), 1, 0, 0);

  app->strip_param_group->filter = 0.5;
  app->strip_param_group->interval = 5000;
  gettimeofday(&app->strip_param_group->t_now, NULL);

  p = 0;
  app->config_fn = g_strdup_printf("%s/.stripchart.conf", getenv("HOME"));
  if (config_fn != NULL)
    config_path[p++] = config_fn;
  else
    {
      config_path[p++] = app->config_fn;
      config_path[p++] = g_strdup("stripchart.conf");
    }
  config_path[p] = NULL;

  for (p = 0; config_path[p] != NULL; p++)
    if (stat(config_path[p], &stat_buf) == 0)
      {
	param_desc = param_desc_ingest(config_path[p]);
	prefs_ingest(app, config_path[p]);
	break;
      }
  if (config_path[p] == NULL)
    error("no config file found, proceeding anyway\n");

  strip_set_default_history_size(STRIP(app->strip), gdk_screen_width());
  chart_set_interval(CHART(app->strip), app->strip_param_group->interval);

  create_editor(app);

  if (param_desc != NULL)
    for (p = 0; param_desc[p]; p++)
      {
	Param_page *page = add_page_before(app, p, param_desc[p]);

	page->strip_data = chart_equation_add(CHART(app->strip),
	  app->strip_param_group, param_desc[p], NULL, 0,
	  str_to_plot_style(param_desc[p]->plot) != chart_plot_indicator);
      }

  return app;
}
