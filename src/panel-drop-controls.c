/* panel-drop-controls.c
 *
 * Copyright 2022 Christian Hergert <chergert@redhat.com>
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

#include "panel-dock-private.h"
#include "panel-drop-controls-private.h"
#include "panel-enums.h"
#include "panel-grid-private.h"
#include "panel-grid-column-private.h"
#include "panel-paned.h"

struct _PanelDropControls
{
  GtkWidget          parent_instance;

  GtkWidget         *child;

  GtkButton          *bottom;
  GtkButton          *center;
  GtkButton          *left;
  GtkButton          *right;
  GtkButton          *top;

  GtkDropTarget     *bottom_target;
  GtkDropTarget     *center_target;
  GtkDropTarget     *left_target;
  GtkDropTarget     *right_target;
  GtkDropTarget     *top_target;

  PanelDock         *dock;

  PanelDockPosition  position;
};

G_DEFINE_TYPE (PanelDropControls, panel_drop_controls, GTK_TYPE_WIDGET)

enum {
  PROP_0,
  PROP_POSITION,
  N_PROPS
};

static GParamSpec *properties [N_PROPS];

GtkWidget *
panel_drop_controls_new (void)
{
  return g_object_new (PANEL_TYPE_DROP_CONTROLS, NULL);
}

void
panel_drop_controls_set_position (PanelDropControls *self,
                                  PanelDockPosition  position)
{
  g_return_if_fail (PANEL_IS_DROP_CONTROLS (self));
  g_return_if_fail (position <= PANEL_DOCK_POSITION_CENTER);

  self->position = position;

  switch (self->position)
    {
    case PANEL_DOCK_POSITION_START:
    case PANEL_DOCK_POSITION_END:
      gtk_widget_show (GTK_WIDGET (self->top));
      gtk_widget_show (GTK_WIDGET (self->bottom));
      gtk_widget_show (GTK_WIDGET (self->center));
      gtk_widget_hide (GTK_WIDGET (self->left));
      gtk_widget_hide (GTK_WIDGET (self->right));
      break;

    case PANEL_DOCK_POSITION_TOP:
    case PANEL_DOCK_POSITION_BOTTOM:
      gtk_widget_hide (GTK_WIDGET (self->top));
      gtk_widget_hide (GTK_WIDGET (self->bottom));
      gtk_widget_show (GTK_WIDGET (self->center));
      gtk_widget_show (GTK_WIDGET (self->left));
      gtk_widget_show (GTK_WIDGET (self->right));
      break;

    case PANEL_DOCK_POSITION_CENTER:
      gtk_widget_show (GTK_WIDGET (self->center));
      gtk_widget_show (GTK_WIDGET (self->top));
      gtk_widget_show (GTK_WIDGET (self->bottom));
      gtk_widget_show (GTK_WIDGET (self->left));
      gtk_widget_show (GTK_WIDGET (self->right));
      break;

    default:
      g_assert_not_reached ();
    }
}

PanelDockPosition
panel_drop_controls_get_position (PanelDropControls *self)
{
  g_return_val_if_fail (PANEL_IS_DROP_CONTROLS (self), 0);

  return self->position;
}

static gboolean
drop_target_accept_cb (PanelDropControls *self,
                       GdkDrop           *drop,
                       GtkDropTarget     *drop_target)
{
  g_assert (PANEL_IS_DROP_CONTROLS (self));
  g_assert (GDK_IS_DROP (drop));
  g_assert (GTK_IS_DROP_TARGET (drop_target));

  return TRUE;
}

static void
on_drop_target_notify_value_cb (PanelDropControls *self,
                                GParamSpec        *pspec,
                                GtkDropTarget     *drop_target)
{
  PanelFrameHeader *header;
  const GValue *value;
  PanelWidget *panel;
  GtkWidget *frame;

  g_assert (PANEL_IS_DROP_CONTROLS (self));
  g_assert (GTK_IS_DROP_TARGET (drop_target));

  if (!(value = gtk_drop_target_get_value (drop_target)) ||
      !G_VALUE_HOLDS (value, PANEL_TYPE_WIDGET) ||
      !(panel = g_value_get_object (value)) ||
      !(frame = gtk_widget_get_ancestor (GTK_WIDGET (self), PANEL_TYPE_FRAME)) ||
      !(header = panel_frame_get_header (PANEL_FRAME (frame))))
    return;

  /* TODO: Actually handle this based on position */

  if (!panel_widget_get_reorderable (panel) ||
      (!panel_frame_header_can_drop (header, panel)))
    gtk_drop_target_reject (drop_target);
}

static GdkDragAction
on_drop_target_motion_cb (PanelDropControls *self,
                          double             x,
                          double             y,
                          GtkDropTarget     *drop_target)
{
  g_assert (PANEL_IS_DROP_CONTROLS (self));
  g_assert (GTK_IS_DROP_TARGET (drop_target));

  return GDK_ACTION_MOVE;
}

static void
on_drop_target_leave_cb (PanelDropControls *self,
                         GtkDropTarget     *drop_target)
{
  g_assert (PANEL_IS_DROP_CONTROLS (self));
  g_assert (GTK_IS_DROP_TARGET (drop_target));

}

static gboolean
on_drop_target_drop_cb (PanelDropControls *self,
                        const GValue      *value,
                        double             x,
                        double             y,
                        GtkDropTarget     *drop_target)
{
  PanelDockPosition position;
  PanelFrameHeader *header;
  PanelFrame *target;
  PanelGrid *grid;
  GtkWidget *paned;
  GtkWidget *src_paned;
  PanelWidget *panel;
  GtkWidget *frame;
  GtkWidget *button;
  guint column;
  guint row;

  g_assert (PANEL_IS_DROP_CONTROLS (self));
  g_assert (GTK_IS_DROP_TARGET (drop_target));

  target = PANEL_FRAME (gtk_widget_get_ancestor (GTK_WIDGET (self), PANEL_TYPE_FRAME));

  if (!G_VALUE_HOLDS (value, PANEL_TYPE_WIDGET) ||
      !(panel = g_value_get_object (value)) ||
      !(frame = gtk_widget_get_ancestor (GTK_WIDGET (panel), PANEL_TYPE_FRAME)) ||
      !panel_widget_get_reorderable (panel) ||
      !(header = panel_frame_get_header (PANEL_FRAME (target))) ||
      !panel_frame_header_can_drop (header, panel) ||
      !(src_paned = gtk_widget_get_ancestor (GTK_WIDGET (panel), PANEL_TYPE_PANED)) ||
      !(paned = gtk_widget_get_ancestor (GTK_WIDGET (self), PANEL_TYPE_PANED)))
    return FALSE;

  g_assert (PANEL_IS_WIDGET (panel));
  g_assert (PANEL_IS_FRAME_HEADER (header));
  g_assert (PANEL_IS_PANED (src_paned));
  g_assert (PANEL_IS_PANED (paned));
  g_assert (panel_frame_header_can_drop (header, panel));

  button = gtk_event_controller_get_widget (GTK_EVENT_CONTROLLER (drop_target));
  position = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (button), "POSITION"));
  grid = PANEL_GRID (gtk_widget_get_ancestor (GTK_WIDGET (self), PANEL_TYPE_GRID));

  switch (position)
    {
    case PANEL_DOCK_POSITION_CENTER:
      /* Do Nothing */
      break;

    case PANEL_DOCK_POSITION_START:
      if (grid != NULL)
        {
          PanelGridColumn *grid_column;

          _panel_grid_get_position (grid, GTK_WIDGET (target), &column, &row);
          _panel_grid_insert_column (grid, column);

          grid_column = panel_grid_get_column (grid, column);
          target = panel_grid_column_get_most_recent_frame (grid_column);
        }
      break;

    case PANEL_DOCK_POSITION_END:
      if (grid != NULL)
        {
          PanelGridColumn *grid_column;

          _panel_grid_get_position (grid, GTK_WIDGET (target), &column, &row);
          _panel_grid_insert_column (grid, ++column);

          grid_column = panel_grid_get_column (grid, column);
          target = panel_grid_column_get_most_recent_frame (grid_column);
        }
      break;

    case PANEL_DOCK_POSITION_TOP:
      if (grid != NULL)
        {
          PanelGridColumn *grid_column;

          _panel_grid_get_position (grid, GTK_WIDGET (target), &column, &row);
          grid_column = panel_grid_get_column (grid, column);

          if (row == 0)
            {
              _panel_grid_column_prepend_frame (PANEL_GRID_COLUMN (grid_column));
              row++;
            }

          target = panel_grid_column_get_row (grid_column, row - 1);
        }
      break;

    case PANEL_DOCK_POSITION_BOTTOM:
      if (grid != NULL)
        {
          PanelGridColumn *grid_column;

          _panel_grid_get_position (grid, GTK_WIDGET (target), &column, &row);
          grid_column = panel_grid_get_column (grid, column);
          target = panel_grid_column_get_row (grid_column, row + 1);
        }
      break;

    default:
      g_assert_not_reached ();
    }

  if (frame == GTK_WIDGET (target))
    return FALSE;

  g_object_ref (panel);

  panel_frame_remove (PANEL_FRAME (frame), panel);
  panel_frame_add (target, panel);
  panel_frame_set_visible_child (target, panel);

  g_object_unref (panel);

  return TRUE;
}

static void
setup_drop_target (PanelDropControls  *self,
                   GtkButton           *widget,
                   GtkDropTarget     **targetptr,
                   PanelDockPosition   position)
{
  GType types[] = { PANEL_TYPE_WIDGET };

  g_assert (PANEL_IS_DROP_CONTROLS (self));

  g_object_set_data (G_OBJECT (widget),
                     "POSITION",
                     GINT_TO_POINTER (position));

  *targetptr = gtk_drop_target_new (G_TYPE_INVALID, GDK_ACTION_COPY | GDK_ACTION_MOVE);
  gtk_drop_target_set_gtypes (*targetptr, types, G_N_ELEMENTS (types));
  gtk_drop_target_set_preload (*targetptr, TRUE);
  g_signal_connect_object (*targetptr,
                           "accept",
                           G_CALLBACK (drop_target_accept_cb),
                           self,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (*targetptr,
                           "notify::value",
                           G_CALLBACK (on_drop_target_notify_value_cb),
                           self,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (*targetptr,
                           "motion",
                           G_CALLBACK (on_drop_target_motion_cb),
                           self,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (*targetptr,
                           "drop",
                           G_CALLBACK (on_drop_target_drop_cb),
                           self,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (*targetptr,
                           "leave",
                           G_CALLBACK (on_drop_target_leave_cb),
                           self,
                           G_CONNECT_SWAPPED);
  gtk_widget_add_controller (GTK_WIDGET (widget),
                             GTK_EVENT_CONTROLLER (*targetptr));
}

static void
panel_drop_controls_root (GtkWidget *widget)
{
  PanelDropControls *self = (PanelDropControls *)widget;
  GtkWidget *dock;

  g_assert (PANEL_IS_DROP_CONTROLS (self));

  if (!(dock = gtk_widget_get_ancestor (widget, PANEL_TYPE_DOCK)))
    {
      g_warning ("%s added without a dock, this cannot work.",
                 G_OBJECT_TYPE_NAME (self));
      return;
    }

  self->dock = PANEL_DOCK (dock);
}

static void
panel_drop_controls_unroot (GtkWidget *widget)
{
  PanelDropControls *self = (PanelDropControls *)widget;

  g_assert (PANEL_IS_DROP_CONTROLS (self));

  self->dock = NULL;
}

static void
panel_drop_controls_dispose (GObject *object)
{
  PanelDropControls *self = (PanelDropControls *)object;

  g_clear_pointer (&self->child, gtk_widget_unparent);

  G_OBJECT_CLASS (panel_drop_controls_parent_class)->dispose (object);
}

static void
panel_drop_controls_get_property (GObject    *object,
                                  guint       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  PanelDropControls *self = PANEL_DROP_CONTROLS (object);

  switch (prop_id)
    {
    case PROP_POSITION:
      g_value_set_enum (value, panel_drop_controls_get_position (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
panel_drop_controls_set_property (GObject      *object,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  PanelDropControls *self = PANEL_DROP_CONTROLS (object);

  switch (prop_id)
    {
    case PROP_POSITION:
      panel_drop_controls_set_position (self, g_value_get_enum (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
panel_drop_controls_class_init (PanelDropControlsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = panel_drop_controls_dispose;
  object_class->get_property = panel_drop_controls_get_property;
  object_class->set_property = panel_drop_controls_set_property;

  widget_class->root = panel_drop_controls_root;
  widget_class->unroot = panel_drop_controls_unroot;

  properties [PROP_POSITION] =
    g_param_spec_enum ("position",
                       "Position",
                       "The position of the drop controls",
                       PANEL_TYPE_DOCK_POSITION,
                       PANEL_DOCK_POSITION_CENTER,
                       (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);

  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BIN_LAYOUT);
  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/libpanel/panel-drop-controls.ui");
  gtk_widget_class_set_css_name (widget_class, "paneldropcontrols");

  gtk_widget_class_bind_template_child (widget_class, PanelDropControls, child);
  gtk_widget_class_bind_template_child (widget_class, PanelDropControls, left);
  gtk_widget_class_bind_template_child (widget_class, PanelDropControls, right);
  gtk_widget_class_bind_template_child (widget_class, PanelDropControls, top);
  gtk_widget_class_bind_template_child (widget_class, PanelDropControls, bottom);
  gtk_widget_class_bind_template_child (widget_class, PanelDropControls, center);
}

static void
panel_drop_controls_init (PanelDropControls *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  setup_drop_target (self, self->bottom, &self->bottom_target, PANEL_DOCK_POSITION_BOTTOM);
  setup_drop_target (self, self->center, &self->center_target, PANEL_DOCK_POSITION_CENTER);
  setup_drop_target (self, self->left, &self->left_target, PANEL_DOCK_POSITION_START);
  setup_drop_target (self, self->right, &self->right_target, PANEL_DOCK_POSITION_END);
  setup_drop_target (self, self->top, &self->top_target, PANEL_DOCK_POSITION_TOP);
}
