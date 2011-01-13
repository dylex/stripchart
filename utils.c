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
#include <ctype.h>
#include <string.h>

/*
 * streq -- case-blind string comparison returning true on equality.
 */
gboolean
streq(const char *s1, const char *s2)
{
  return s1 && s2 && strcasecmp(s1, s2) == 0;
}

gboolean
xmlstreq(const xmlChar *s1, const char *s2)
{
  return s1 && s2 && xmlStrcasecmp(s1, BAD_CAST s2) == 0;
}

/*
 * error -- error handler used primarily by eval.
 */
const char *
verror(char *msg, va_list args)
{
  static char err_msg[1000];
  vsnprintf(err_msg, 1000, msg, args);

  fflush(stdout);
  fprintf(stderr, "%s: %s\n", prog_name, err_msg);

  GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "%s", err_msg);
  gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);

  return err_msg;
}

const char *
error(char *msg, ...)
{
  va_list args;
  const char *err_msg;

  va_start(args, msg);
  err_msg = verror(msg, args);
  va_end(args);

  return err_msg;
}

ChartPlotStyle
str_to_plot_style(const char *style_name)
{
  if (streq(style_name, "indicator"))
    return chart_plot_indicator;
  if (streq(style_name, "point"))
    return chart_plot_point;
  if (streq(style_name, "line"))
    return chart_plot_line;
  if (streq(style_name, "solid"))
    return chart_plot_solid;
  return chart_plot_line;
}

ChartScaleStyle
str_to_scale_style(const char *style_name)
{
  if (streq(style_name, "linear"))
    return chart_scale_linear;
  if (streq(style_name, "log"))
    return chart_scale_log;
  return chart_scale_linear;
}

gdouble
str_to_gdouble(const char *str, gdouble fallback)
{
  gdouble value;
  char *e = NULL;

  if (!str)
	  return fallback;
  value = g_ascii_strtod(str, &e);
  if (e && *e && !isspace(*e))
	  return fallback;
  return value;
}
