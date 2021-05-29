/* panel-paned.c
 *
 * Copyright 2021 Christian Hergert <chergert@redhat.com>
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

#include "panel-dock-private.h"
#include "panel-paned-private.h"
#include "panel-resizer-private.h"

struct _PanelPaned
{
  GtkWidget      parent_instance;
  GtkOrientation orientation;
};

G_DEFINE_TYPE_WITH_CODE (PanelPaned, panel_paned, GTK_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_ORIENTABLE, NULL))

enum {
  PROP_0,
  N_PROPS,

  PROP_ORIENTATION,
};

/**
 * panel_paned_new:
 *
 * Create a new #PanelPaned.
 *
 * Returns: (transfer full): a newly created #PanelPaned
 */
GtkWidget *
panel_paned_new (void)
{
  return g_object_new (PANEL_TYPE_PANED, NULL);
}

static void
panel_paned_set_orientation (PanelPaned *self,
                                  GtkOrientation  orientation)
{
  PanelDockPosition dockpos;

  g_assert (PANEL_IS_PANED (self));
  g_assert (orientation == GTK_ORIENTATION_HORIZONTAL ||
            orientation == GTK_ORIENTATION_VERTICAL);

  if (self->orientation == orientation)
    return;

  self->orientation = orientation;

  if (self->orientation == GTK_ORIENTATION_HORIZONTAL)
    dockpos = PANEL_DOCK_POSITION_END;
  else
    dockpos = PANEL_DOCK_POSITION_BOTTOM;

  for (GtkWidget *child = gtk_widget_get_last_child (GTK_WIDGET (self));
       child != NULL;
       child = gtk_widget_get_prev_sibling (child))
    {
      if (PANEL_IS_RESIZER (child))
        panel_resizer_set_position (PANEL_RESIZER (child), dockpos);
    }

  _panel_dock_update_orientation (GTK_WIDGET (self), self->orientation);
  gtk_widget_queue_resize (GTK_WIDGET (self));
  g_object_notify (G_OBJECT (self), "orientation");
}

static void
panel_paned_dispose (GObject *object)
{
  PanelPaned *self = (PanelPaned *)object;
  GtkWidget *child;

  child = gtk_widget_get_first_child (GTK_WIDGET (self));
  while (child)
    {
      GtkWidget *next = gtk_widget_get_next_sibling (child);
      gtk_widget_unparent (child);
      child = next;
    }

  G_OBJECT_CLASS (panel_paned_parent_class)->dispose (object);
}

static void
panel_paned_get_property (GObject    *object,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  PanelPaned *self = PANEL_PANED (object);

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      g_value_set_enum (value, self->orientation);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
panel_paned_set_property (GObject      *object,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  PanelPaned *self = PANEL_PANED (object);

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      panel_paned_set_orientation (self, g_value_get_enum (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
panel_paned_class_init (PanelPanedClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = panel_paned_dispose;
  object_class->get_property = panel_paned_get_property;
  object_class->set_property = panel_paned_set_property;

  g_object_class_override_property (object_class, PROP_ORIENTATION, "orientation");

  gtk_widget_class_set_css_name (widget_class, "panelpaned");
}

static void
panel_paned_init (PanelPaned *self)
{
  self->orientation = GTK_ORIENTATION_HORIZONTAL;
}

void
panel_paned_remove (PanelPaned *self,
                         GtkWidget      *child)
{
  g_return_if_fail (PANEL_IS_PANED (self));
  g_return_if_fail (GTK_IS_WIDGET (child));
  g_return_if_fail (gtk_widget_get_parent (child) == GTK_WIDGET (self));

}

void
panel_paned_insert (PanelPaned *self,
                         int             position,
                         GtkWidget      *child)
{
  PanelDockPosition dockpos;
  GtkWidget *resizer;

  g_return_if_fail (PANEL_IS_PANED (self));
  g_return_if_fail (GTK_IS_WIDGET (child));
  g_return_if_fail (gtk_widget_get_parent (child) == NULL);

  if (self->orientation == GTK_ORIENTATION_HORIZONTAL)
    dockpos = PANEL_DOCK_POSITION_END;
  else
    dockpos = PANEL_DOCK_POSITION_BOTTOM;

  resizer = panel_resizer_new (dockpos);
  panel_resizer_set_child (PANEL_RESIZER (resizer), child);

  if (position < 0)
    gtk_widget_insert_before (GTK_WIDGET (resizer), GTK_WIDGET (self), NULL);
  else if (position == 0)
    gtk_widget_insert_after (GTK_WIDGET (resizer), GTK_WIDGET (self), NULL);
  else
    {
      GtkWidget *sibling = gtk_widget_get_first_child (GTK_WIDGET (self));

      for (int i = position; i > 0 && sibling != NULL; i--)
        sibling = gtk_widget_get_next_sibling (sibling);

      gtk_widget_insert_before (GTK_WIDGET (resizer), GTK_WIDGET (self), sibling);
    }

  gtk_widget_queue_resize (GTK_WIDGET (self));
}

void
panel_paned_append (PanelPaned *self,
                         GtkWidget      *child)
{
  panel_paned_insert (self, -1, child);
}

void
panel_paned_prepend (PanelPaned *self,
                          GtkWidget      *child)
{
  panel_paned_insert (self, 0, child);
}
