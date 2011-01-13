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

#ifndef CHART_H
#define CHART_H

#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <gtk/gtkdrawingarea.h>

#define TYPE_CHART			(chart_get_type())
#define CHART(obj) 			(G_TYPE_CHECK_INSTANCE_CAST((obj), TYPE_CHART, Chart))
#define IS_CHART(obj) 			(G_TYPE_CHECK_INSTANCE_TYPE((obj), TYPE_CHART))
#define CHART_CLASS(klass) 		(G_TYPE_CHECK_CLASS_CAST((klass), TYPE_CHART, ChartClass))
#define IS_CHART_CLASS(klass) 		(G_TYPE_CHECK_CLASS_TYPE((klass), TYPE_CHART))

typedef struct _Chart		Chart;
typedef struct _ChartClass	ChartClass;
typedef struct _ChartDatum	ChartDatum;
typedef struct _ChartAdjustment	ChartAdjustment;

typedef enum
{
  chart_scale_linear,
  chart_scale_log
}
ChartScaleStyle;

typedef enum
{
  chart_plot_point,
  chart_plot_line,
  chart_plot_solid,
  chart_plot_indicator
}
ChartPlotStyle; /* FIX THIS: should be in strip.h */

struct _Chart
{
  GtkDrawingArea drawing;
  guint timer;

  gint points_in_view;
  gint default_history_size;

  ChartPlotStyle default_plot_style;
  ChartScaleStyle default_scale_style;

  GdkColormap *colormap;
  GSList *param;
};

struct _ChartClass
{
  GtkDrawingAreaClass parent_class;
  void (*chart_pre_update)  (Chart *chart);
  void (*chart_post_update) (Chart *chart);
  void (*chart_rescale)	    (Chart *chart);
};

struct _ChartAdjustment
{
  gdouble lower, upper;
};

struct _ChartDatum
{
  Chart *chart;

  gint history_size, history_count, active, idle, newest, skip;
  gfloat *history;

  gdouble max, min; /* of current window's worth of history values */
  gdouble top_max, top_min;
  gdouble bot_max, bot_min; /* Adjustment limits */
  gboolean rescale;
  ChartAdjustment *adj; /* Adjustement */

  gdouble (*user_func)(void *user_data);
  void *user_data;

  ChartScaleStyle scale_style;
  ChartPlotStyle plot_style; /* FIX THIS: only strips have plot_styles */

  gchar *color_names;
  gint colors;
  GdkGC **gdk_gc;
  GdkColor *gdk_color;
};

GType chart_get_type(void);

void chart_set_interval(Chart *chart, guint msec);

ChartDatum *chart_parameter_add(Chart *chart,
  gdouble (*func)(), gpointer user_data,
  const gchar *color_name, ChartAdjustment *adj, int pageno,
  gdouble bot_min, gdouble bot_max, gdouble top_min, gdouble top_max);

void chart_parameter_deactivate(Chart *chart, ChartDatum *param);

void chart_set_autorange(ChartDatum *param, gboolean rescale);

void chart_set_top_min(ChartDatum *datum, gdouble top_min);
void chart_set_top_max(ChartDatum *datum, gdouble top_max);
void chart_set_bot_min(ChartDatum *datum, gdouble bot_min);
void chart_set_bot_max(ChartDatum *datum, gdouble bot_max);

void chart_set_plot_style(ChartDatum *datum, ChartPlotStyle plot_style);
void chart_set_scale_style(ChartDatum *datum, ChartScaleStyle scale_style);
void chart_assign_color(Chart *chart, ChartDatum *parm);

gint val2gdk(gdouble val,
  ChartAdjustment *adj, gint height, ChartScaleStyle scale);

#endif /* CHART_H */ 
