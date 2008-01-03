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

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "chart.h"

enum { PRE_UPDATE, POST_UPDATE, RESCALE, SIGNAL_COUNT };
static gint chart_signals[SIGNAL_COUNT] = { 0 };

static void 
chart_class_init(ChartClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS(klass);

  chart_signals[PRE_UPDATE] = 
    g_signal_new("chart_pre_update", 
		    G_TYPE_FROM_CLASS(object_class), G_SIGNAL_RUN_FIRST, 
		    G_STRUCT_OFFSET(ChartClass, chart_pre_update),
		    NULL, NULL,
		    g_cclosure_marshal_VOID__VOID, GTK_TYPE_NONE, 0);

  chart_signals[POST_UPDATE] = 
    g_signal_new("chart_post_update", 
		    G_TYPE_FROM_CLASS(object_class), G_SIGNAL_RUN_FIRST, 
		    G_STRUCT_OFFSET(ChartClass, chart_post_update),
		    NULL, NULL,
		    g_cclosure_marshal_VOID__VOID, GTK_TYPE_NONE, 0);

  chart_signals[RESCALE] = 
    g_signal_new("chart_rescale", 
		    G_TYPE_FROM_CLASS(object_class), G_SIGNAL_RUN_FIRST, 
		    G_STRUCT_OFFSET(ChartClass, chart_rescale),
		    NULL, NULL,
		    g_cclosure_marshal_VOID__VOID, GTK_TYPE_NONE, 0);

  // gtk_object_class_add_signals(object_class, chart_signals, SIGNAL_COUNT);
  klass->chart_rescale = NULL;
}

static void
chart_configure(GtkWidget *widget, GdkEvent *event, void *nil)
{
  CHART(widget)->points_in_view = widget->allocation.width;

  if (!CHART(widget)->colormap)
    CHART(widget)->colormap = gdk_window_get_colormap(widget->window);
}

static void
chart_object_init(Chart *chart)
{
  chart->param = NULL;
  chart->default_history_size = 1;
  chart->default_plot_style = chart_plot_line;
  chart->default_scale_style = chart_scale_linear;

  chart_set_interval(chart, 1000);

  g_signal_connect(chart, "configure_event", G_CALLBACK(chart_configure), NULL);
}

GType
chart_get_type(void)
{
  static GType type = 0;

  if (type)
	  return type;
  static const GTypeInfo info = {
	  .class_size = sizeof(ChartClass),
	  .class_init = (GClassInitFunc)chart_class_init,
	  .instance_size = sizeof(Chart),
	  .instance_init = (GInstanceInitFunc)chart_object_init
  };
  type = g_type_register_static(GTK_TYPE_DRAWING_AREA, "Chart", &info, 0);
  return type;
}

static guint chart_range(gdouble *bound, gdouble val, gdouble min, gdouble max, gboolean side)
{
	gdouble in = side ? max : min;
	gdouble out = side ? min : max;
	gdouble b;
	if (side ? val >= in : val <= in)
		b = in;
	else if (side ? val <= out : val >= out)
		b = out;
	else if (min < 0 && max > 0)
		b = val;
	else if (val == 0)
		b = 0;
	else
	{
		gboolean dir = side ^ (val < 0);

		double l = log10(fabs(in)); l -= floor(l);
		if (l == 0 || l == log10(2) || l == log10(5))
		{
			l = log10(fabs(val));
			double i = floor(l);
			l -= i;
			if (l <= 0) 		l = 0;
			else if (l < log10(2)) 	l = dir ? 0 : log10(2);
			else if (l <= log10(2)) l = log10(2);
			else if (l < log10(5)) 	l = dir ? log10(2) : log10(5);
			else if (l <= log10(5)) l = log10(5);
			else 			l = dir ? log10(5) : 1;
			b = exp10(i+l);
		}
		else
		{
			l = log2(fabs(val));
			double i = floor(l);
			if (l > i && !dir) i++;
			b = exp2(i);
		}

		b = copysign(b, val);
		if (side ? b <= out : b >= out)
			b = out;
	}

	if (b != *bound)
	{
		*bound = b;
		return 1;
	}
	return 0;
}

static guint chart_rescale_range(ChartDatum *datum)
{
	guint changed = 0;
	changed += chart_range(&datum->adj->upper, datum->max, datum->top_min, datum->top_max, 0);
	changed += chart_range(&datum->adj->lower, datum->min, datum->bot_min, datum->bot_max, 1);
	return changed;
}

void
chart_parameter_deactivate(Chart *chart, ChartDatum *param)
{
  if (param)
    param->active = FALSE;
}

static gint
chart_timer(Chart *chart)
{
  gint rescale = 0;
  GSList *list, *next;

  gtk_signal_emit_by_name(GTK_OBJECT(chart), "chart_pre_update", NULL);

  for (list = chart->param; list != NULL; list = g_slist_next(list))
    {
      gdouble val;
      ChartDatum *datum = list->data;

      if (!datum->active)
	{
	  datum->idle++;
	  if (datum->history_size <= datum->history_count + datum->idle)
	    datum->history_count--;
	  if (datum->history_count == 0)
	    {
	      #ifdef DEBUG
	      printf("timer: deleting: chart %p, param %p, datum %p\n",
		chart, chart->param, datum);
	      #endif
	      next = g_slist_next(list);
	      chart->param = g_slist_remove(chart->param, datum);
	      g_free(datum);
	      list = next;
	    }
	  continue;
	}

      val = datum->user_func(datum->user_data);

      if (datum->skip)
	{
	  datum->skip--;
	  continue;
	}

#ifdef DEBUG
/* printf("timer: value %p = %f\n", datum, val); */
#endif

      if (datum->history_count < datum->history_size)
	datum->history_count++;
      if (++datum->newest >= datum->history_size)
	datum->newest = 0;

      datum->history[datum->newest] = val;

      if (datum->rescale)
	{
	  gint view = chart->points_in_view;
	  gint idle = datum->idle;
	  gint data = datum->history_count;
	  gint n = MIN(data, view - idle);

	  gint h = datum->newest;
	  datum->min = datum->max = datum->history[h];

	  while (n-- > 0)
	    {
	      gfloat x = datum->history[h];
	      if (x < datum->min)
		datum->min = x;
	      else if (datum->max < x)
		datum->max = x;
	      if (--h < 0)
		h = datum->history_size - 1;
	    }
	  rescale += chart_rescale_range(datum);
	}
    }

  if (rescale)
    gtk_signal_emit_by_name(GTK_OBJECT(chart), "chart_rescale", NULL);
  gtk_signal_emit_by_name(GTK_OBJECT(chart), "chart_post_update", NULL);

  return TRUE;
}

void
chart_set_interval(Chart *chart, guint msec)
{
  if (chart->timer)
    gtk_timeout_remove(chart->timer);
  chart->timer = gtk_timeout_add(msec, (GtkFunction)chart_timer, chart);
}

ChartDatum *
chart_parameter_add(Chart *chart,
  gdouble (*user_func)(), gpointer user_data,
  gchar *color_names, ChartAdjustment *adj, int pageno,
  gdouble bot_min, gdouble bot_max, gdouble top_min, gdouble top_max)
{
  ChartDatum *datum = g_malloc(sizeof(*datum));

  datum->chart = chart;
  datum->user_func = user_func;
  datum->user_data = user_data;
  datum->rescale = FALSE;
  datum->active = TRUE;
  datum->idle = datum->newest = datum->history_count = 0;

  datum->top_max = top_max;
  datum->bot_max = bot_max;
  datum->top_min = top_min;
  datum->bot_min = bot_min;

  if (adj == NULL)
  {
    adj = g_malloc(sizeof(*adj));
    adj->upper = datum->top_min;
    adj->lower = datum->bot_max;
  }
  datum->adj = adj;

#ifdef DEBUG
  printf("Initial bounds %d: %lf - %lf | %lf - %lf (%f...%f)\n", pageno,
    datum->bot_min, datum->bot_max, datum->top_min, datum->top_max, datum->adj->lower, datum->adj->upper);
#endif

  datum->colors = 0;
  datum->color_names = g_strdup(color_names);
  datum->gdk_gc = NULL;
  datum->gdk_color = NULL;
  chart->param = g_slist_insert(chart->param, datum, pageno);

  datum->plot_style = chart->default_plot_style;
  datum->scale_style = chart->default_scale_style;

  datum->skip = 2;
  datum->history_size = chart->default_history_size;
  datum->history = g_malloc(datum->history_size * sizeof(*datum->history));

  return datum;
}

void
chart_set_top_max(ChartDatum *datum, double top_max)
{
  datum->top_max = top_max;
}

void
chart_set_top_min(ChartDatum *datum, double top_min)
{
  datum->top_min = top_min;
}

void
chart_set_bot_max(ChartDatum *datum, double bot_max)
{
  datum->bot_max = bot_max;
}

void
chart_set_bot_min(ChartDatum *datum, double bot_min)
{
  datum->bot_min = bot_min;
}

void
chart_set_autorange(ChartDatum *datum, gboolean rescale)
{
  datum->rescale = rescale;
}

void
chart_set_scale_style(ChartDatum *datum, ChartScaleStyle scale_style)
{
  datum->scale_style = scale_style;
}

void
chart_set_plot_style(ChartDatum *datum, ChartPlotStyle plot_style)
{
  datum->plot_style = plot_style;
}

void
chart_assign_color(Chart *chart, ChartDatum *datum)
{
  gchar *names, *color = NULL, *whitespace = " \t\r\n";
  GdkColor default_fg = GTK_WIDGET(chart)->style->fg[GTK_WIDGET_STATE(chart)];

  datum->colors = 0;
  datum->gdk_gc = NULL;
  datum->gdk_color = NULL;

  names = g_strdup(datum->color_names);
  do
    {
    color = strtok(color ? NULL : names, whitespace);
      gint cnum = datum->colors++;
      datum->gdk_color = g_realloc(datum->gdk_color,
	datum->colors * sizeof(*datum->gdk_color));
      datum->gdk_gc = g_realloc(datum->gdk_gc,
	datum->colors * sizeof(*datum->gdk_gc));

      if (!color || !gdk_color_parse(color, &datum->gdk_color[cnum]))
	{
	  g_warning("Unknown color: (%s); Using default fg color.", color);
	  datum->gdk_color[cnum] = default_fg;
	}
      gdk_color_alloc(chart->colormap, &datum->gdk_color[cnum]);
      datum->gdk_gc[cnum] = gdk_gc_new(GTK_WIDGET(chart)->window);
      gdk_gc_set_foreground(datum->gdk_gc[cnum], &datum->gdk_color[cnum]);
    }
  while (color != NULL);
  g_free(names);
}

/*
 * val2y -- scales a parameter value into a y coordinate value.
 */
gint
val2gdk(gdouble val, ChartAdjustment *adj, gint height, ChartScaleStyle scale)
{
  gdouble y, delta = adj->upper - adj->lower;

  height--;
  if (scale == chart_scale_log)
    y = height - (height * log1p(val - adj->lower) / log1p(delta));
  else
    y = height - height * (val - adj->lower) / delta;

  if (isnan(y))
    y = height;
  else if (y < 0)
    y = 0;
  else if (height < y)
    y = height;

  return y + 0.5;
}

