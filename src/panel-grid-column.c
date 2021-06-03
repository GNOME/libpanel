/* panel-grid-column.c
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

#include "panel-frame.h"
#include "panel-grid-column-private.h"
#include "panel-paned-private.h"

struct _PanelGridColumn
{
  GtkWidget   parent_instance;
  PanelPaned *rows;
};

G_DEFINE_TYPE (PanelGridColumn, panel_grid_column, GTK_TYPE_WIDGET)

GtkWidget *
_panel_grid_column_new (void)
{
  return g_object_new (PANEL_TYPE_GRID_COLUMN, NULL);
}

static void
panel_grid_column_dispose (GObject *object)
{
  PanelGridColumn *self = (PanelGridColumn *)object;

  g_clear_pointer ((GtkWidget **)&self->rows, gtk_widget_unparent);

  G_OBJECT_CLASS (panel_grid_column_parent_class)->dispose (object);
}

static void
panel_grid_column_class_init (PanelGridColumnClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = panel_grid_column_dispose;

  gtk_widget_class_set_css_name (widget_class, "panelgridcolumn");
  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BIN_LAYOUT);
}

static void
panel_grid_column_init (PanelGridColumn *self)
{
  self->rows = PANEL_PANED (panel_paned_new ());
  gtk_orientable_set_orientation (GTK_ORIENTABLE (self->rows), GTK_ORIENTATION_VERTICAL);
  gtk_widget_set_parent (GTK_WIDGET (self->rows), GTK_WIDGET (self));
}

static PanelFrame *
panel_grid_column_get_most_recent_frame (PanelGridColumn *self)
{
  PanelFrame *frame;

  g_assert (PANEL_IS_GRID_COLUMN (self));

  if (!(frame = PANEL_FRAME (panel_paned_get_nth_child (self->rows, 0))))
    {
      frame = PANEL_FRAME (panel_frame_new ());
      panel_paned_append (self->rows, GTK_WIDGET (frame));
    }

  return frame;
}

void
_panel_grid_column_add (PanelGridColumn *self,
                        PanelWidget     *widget)
{
  PanelFrame *frame;

  g_return_if_fail (PANEL_IS_GRID_COLUMN (self));
  g_return_if_fail (PANEL_IS_WIDGET (widget));

  frame = panel_grid_column_get_most_recent_frame (self);
  panel_frame_add (frame, widget);
}
