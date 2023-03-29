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

enum {
  PROP_0,
  N_PROPS
};

G_DEFINE_FINAL_TYPE (DemoPage, demo_page, PANEL_TYPE_WIDGET)

static GParamSpec *properties [N_PROPS];

static void
demo_page_dispose (GObject *object)
{
  G_OBJECT_CLASS (demo_page_parent_class)->dispose (object);
}

static void
demo_page_get_property (GObject    *object,
                        guint       prop_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
  DemoPage *self = DEMO_PAGE (object);

  switch (prop_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
demo_page_set_property (GObject      *object,
                        guint         prop_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  DemoPage *self = DEMO_PAGE (object);

  switch (prop_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
demo_page_class_init (DemoPageClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = demo_page_dispose;
  object_class->get_property = demo_page_get_property;
  object_class->set_property = demo_page_set_property;

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
