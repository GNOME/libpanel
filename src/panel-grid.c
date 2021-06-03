/* panel-grid.c
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

#include "panel-grid.h"
#include "panel-grid-column-private.h"
#include "panel-paned-private.h"

struct _PanelGrid
{
  GtkWidget   parent_instance;

  PanelPaned *columns;
};

static void buildable_iface_init (GtkBuildableIface *iface);

G_DEFINE_TYPE_WITH_CODE (PanelGrid, panel_grid, GTK_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_BUILDABLE, buildable_iface_init))

/**
 * panel_grid_new:
 *
 * Create a new #PanelGrid.
 *
 * Returns: (transfer full): a newly created #PanelGrid
 */
GtkWidget *
panel_grid_new (void)
{
  return g_object_new (PANEL_TYPE_GRID, NULL);
}

static void
panel_grid_dispose (GObject *object)
{
  PanelGrid *self = (PanelGrid *)object;

  g_clear_pointer ((GtkWidget **)self->columns, gtk_widget_unparent);

  G_OBJECT_CLASS (panel_grid_parent_class)->dispose (object);
}

static void
panel_grid_class_init (PanelGridClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = panel_grid_dispose;

  gtk_widget_class_set_css_name (widget_class, "panelgrid");
  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BIN_LAYOUT);
}

static void
panel_grid_init (PanelGrid *self)
{
  self->columns = PANEL_PANED (panel_paned_new ());
  gtk_orientable_set_orientation (GTK_ORIENTABLE (self->columns), GTK_ORIENTATION_HORIZONTAL);
  gtk_widget_set_parent (GTK_WIDGET (self->columns), GTK_WIDGET (self));
  panel_paned_append (self->columns, _panel_grid_column_new ());
}

static PanelGridColumn *
panel_grid_get_most_recent_column (PanelGrid *self)
{
  /* TODO: actually track w/ MRU */

  return panel_paned_get_nth_child (self->columns, 0);
}

void
panel_grid_add (PanelGrid   *self,
                PanelWidget *widget)
{
  PanelGridColumn *column;

  g_return_if_fail (PANEL_IS_GRID (self));
  g_return_if_fail (PANEL_IS_WIDGET (widget));
  g_return_if_fail (gtk_widget_get_parent (GTK_WIDGET (widget)) == NULL);

  column = panel_grid_get_most_recent_column (self);
  _panel_grid_column_add (column, widget);
}

static void
panel_grid_add_child (GtkBuildable *buildable,
                      GtkBuilder   *builder,
                      GObject      *child,
                      const char   *type)
{
  PanelGrid *self = (PanelGrid *)buildable;

  g_assert (PANEL_IS_GRID (self));
  g_assert (GTK_IS_BUILDER (builder));
  g_assert (G_IS_OBJECT (child));

  if (PANEL_IS_WIDGET (child))
    panel_grid_add (self, PANEL_WIDGET (child));
  else
    g_warning ("%s cannot add child of type %s",
               G_OBJECT_TYPE_NAME (self),
               G_OBJECT_TYPE_NAME (child));
}

static void
buildable_iface_init (GtkBuildableIface *iface)
{
  iface->add_child = panel_grid_add_child;
}
