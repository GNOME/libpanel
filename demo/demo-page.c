/* demo-page.c
 *
 * Copyright 2023 Christian Hergert <chergert@redhat.com>
 *
 * This file is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 3 of the License, or (at your option)
 * any later version.
 *
 * This file is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "config.h"

#include "demo-page.h"

struct _DemoPage
{
  PanelWidget parent_instance;

  GtkTextView *text_view;
};

G_DEFINE_FINAL_TYPE (DemoPage, demo_page, PANEL_TYPE_WIDGET)

static void
demo_page_class_init (DemoPageClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/libpanel/demo/demo-page.ui");
}

static void
demo_page_init (DemoPage *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

DemoPage *
demo_page_new (void)
{
  return g_object_new (DEMO_TYPE_PAGE, NULL);
}
