/* panel-statusbar.c
 *
 * Copyright 2021 Christian Hergert <chergert@redhat.com>
 *
 * This file is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 3 of the
 * License, or (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "config.h"

#include "panel-statusbar.h"

struct _PanelStatusbar
{
  GtkWidget parent_instance;
};

G_DEFINE_TYPE (PanelStatusbar, panel_statusbar, GTK_TYPE_WIDGET)

enum {
  PROP_0,
  N_PROPS
};

static GParamSpec *properties [N_PROPS];

GtkWidget *
panel_statusbar_new (void)
{
  return g_object_new (PANEL_TYPE_STATUSBAR, NULL);
}

static void
panel_statusbar_dispose (GObject *object)
{
  PanelStatusbar *self = (PanelStatusbar *)object;
  GtkWidget *child;

  while ((child = gtk_widget_get_first_child (GTK_WIDGET (self))))
    gtk_widget_unparent (child);

  G_OBJECT_CLASS (panel_statusbar_parent_class)->dispose (object);
}

static void
panel_statusbar_get_property (GObject    *object,
                              guint       prop_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  PanelStatusbar *self = PANEL_STATUSBAR (object);

  switch (prop_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
panel_statusbar_set_property (GObject      *object,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  PanelStatusbar *self = PANEL_STATUSBAR (object);

  switch (prop_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
panel_statusbar_class_init (PanelStatusbarClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = panel_statusbar_dispose;
  object_class->get_property = panel_statusbar_get_property;
  object_class->set_property = panel_statusbar_set_property;

  gtk_widget_class_set_css_name (widget_class, "panelstatusbar");
  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BOX_LAYOUT);
}

static void
panel_statusbar_init (PanelStatusbar *self)
{
}
