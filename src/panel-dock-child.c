/* panel-dock-child.c
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

#include "panel-dock.h"
#include "panel-dock-child-private.h"
#include "panel-enums.h"
#include "panel-frame-private.h"
#include "panel-grid-private.h"
#include "panel-paned.h"
#include "panel-resizer-private.h"
#include "panel-widget.h"

#define MIN_SIZE_DURING_DRAG 32

struct _PanelDockChild
{
  GtkWidget     parent_instance;
  GtkRevealer  *revealer;
  PanelResizer *resizer;
  GtkWidget    *left_edge;
  GtkWidget    *right_edge;
  GtkWidget    *top_edge;
  GtkWidget    *bottom_edge;
  PanelArea     area : 4;
  guint         dragging : 1;
};

G_DEFINE_TYPE (PanelDockChild, panel_dock_child, GTK_TYPE_WIDGET)

enum {
  PROP_0,
  PROP_CHILD,
  PROP_EMPTY,
  PROP_AREA,
  PROP_REVEAL_CHILD,
  PROP_BOTTOM_EDGE,
  PROP_LEFT_EDGE,
  PROP_RIGHT_EDGE,
  PROP_TOP_EDGE,
  N_PROPS
};

static GParamSpec *properties[N_PROPS];

static void
panel_dock_child_set_edge (PanelDockChild  *self,
                           GtkWidget       *child,
                           GtkWidget      **childptr,
                           guint            prop_id)
{
  g_return_if_fail (PANEL_IS_DOCK_CHILD (self));
  g_return_if_fail (!child || GTK_IS_WIDGET (child));
  g_return_if_fail (gtk_widget_get_parent (child) == NULL);
  g_return_if_fail (prop_id > 0);
  g_return_if_fail (prop_id < N_PROPS);

  if (child == *childptr)
    return;

  g_clear_pointer (childptr, gtk_widget_unparent);
  *childptr = child;
  if (child)
    gtk_widget_set_parent (child, GTK_WIDGET (self));
  g_object_notify_by_pspec (G_OBJECT (self), properties[prop_id]);
  gtk_widget_queue_resize (GTK_WIDGET (self));
}

static void
panel_dock_child_measure (GtkWidget      *widget,
                          GtkOrientation  orientation,
                          int             for_size,
                          int            *minimum,
                          int            *natural,
                          int            *minimum_baseline,
                          int            *natural_baseline)
{
  PanelDockChild *self = PANEL_DOCK_CHILD (widget);
  int min = 0, nat = 0;

  *minimum = 0;
  *natural = 0;
  *minimum_baseline = -1;
  *natural_baseline = -1;

  if (orientation == GTK_ORIENTATION_VERTICAL)
    {
      if (self->revealer != NULL)
        {
          gtk_widget_measure (GTK_WIDGET (self->revealer), orientation, for_size, &min, &nat, NULL, NULL);
          *minimum += min;
          *natural += nat;
        }

      if (self->left_edge != NULL)
        {
          gtk_widget_measure (self->left_edge, orientation, for_size, &min, &nat, NULL, NULL);
          *minimum = MAX (*minimum, min);
          *natural = MAX (*natural, nat);
        }

      if (self->right_edge != NULL)
        {
          gtk_widget_measure (self->right_edge, orientation, for_size, &min, &nat, NULL, NULL);
          *minimum = MAX (*minimum, min);
          *natural = MAX (*natural, nat);
        }

      if (self->top_edge != NULL)
        {
          gtk_widget_measure (self->top_edge, orientation, for_size, &min, &nat, NULL, NULL);
          *minimum += min;
          *natural += nat;
        }

      if (self->bottom_edge != NULL)
        {
          gtk_widget_measure (self->bottom_edge, orientation, for_size, &min, &nat, NULL, NULL);
          *minimum += min;
          *natural += nat;
        }
    }
  else
    {
      if (self->revealer != NULL)
        {
          gtk_widget_measure (GTK_WIDGET (self->revealer), orientation, for_size, &min, &nat, NULL, NULL);
          *minimum += min;
          *natural += nat;
        }

      if (self->left_edge != NULL)
        {
          gtk_widget_measure (self->left_edge, orientation, for_size, &min, &nat, NULL, NULL);
          *minimum += min;
          *natural += nat;
        }

      if (self->right_edge != NULL)
        {
          gtk_widget_measure (self->right_edge, orientation, for_size, &min, &nat, NULL, NULL);
          *minimum += min;
          *natural += nat;
        }

      if (self->top_edge != NULL)
        {
          gtk_widget_measure (self->top_edge, orientation, for_size, &min, &nat, NULL, NULL);
          *minimum = MAX (*minimum, min);
          *natural = MAX (*natural, nat);
        }

      if (self->bottom_edge != NULL)
        {
          gtk_widget_measure (self->bottom_edge, orientation, for_size, &min, &nat, NULL, NULL);
          *minimum = MAX (*minimum, min);
          *natural = MAX (*natural, nat);
        }
    }
}

static void
panel_dock_child_size_allocate (GtkWidget *widget,
                                int        width,
                                int        height,
                                int        baseline)
{
  PanelDockChild *self = PANEL_DOCK_CHILD (widget);
  GtkRequisition min, nat;
  int y = 0;
  int x = 0;

  if (self->top_edge)
    {
      gtk_widget_get_preferred_size (self->top_edge, &min, &nat);
      gtk_widget_size_allocate (self->top_edge,
                                &(GtkAllocation) { 0, 0, width, min.height },
                                -1);
      y += min.height;
      height -= min.height;
    }

  if (self->bottom_edge)
    {
      gtk_widget_get_preferred_size (self->bottom_edge, &min, &nat);
      gtk_widget_size_allocate (self->bottom_edge,
                                &(GtkAllocation) { 0, y+height-min.height, width, min.height },
                                -1);
      height -= min.height;
    }

  if (self->left_edge)
    {
      gtk_widget_get_preferred_size (self->left_edge, &min, &nat);
      gtk_widget_size_allocate (self->left_edge,
                                &(GtkAllocation) { 0, y, min.width, height },
                                -1);
      x += min.width;
      width -= min.width;
    }

  if (self->right_edge)
    {
      gtk_widget_get_preferred_size (self->right_edge, &min, &nat);
      gtk_widget_size_allocate (self->right_edge,
                                &(GtkAllocation) { x+width-min.width, y, min.width, height },
                                -1);
      width -= min.width;
    }

  gtk_widget_size_allocate (GTK_WIDGET (self->revealer),
                            &(GtkAllocation) { x, y, width, height },
                            -1);
}

static gboolean
panel_dock_child_grab_focus (GtkWidget *widget)
{
  return gtk_widget_grab_focus (GTK_WIDGET (PANEL_DOCK_CHILD (widget)->resizer));
}

static void
panel_dock_child_dispose (GObject *object)
{
  PanelDockChild *self = (PanelDockChild *)object;

  g_clear_pointer ((GtkWidget **)&self->revealer, gtk_widget_unparent);
  g_clear_pointer ((GtkWidget **)&self->top_edge, gtk_widget_unparent);
  g_clear_pointer ((GtkWidget **)&self->bottom_edge, gtk_widget_unparent);
  g_clear_pointer ((GtkWidget **)&self->left_edge, gtk_widget_unparent);
  g_clear_pointer ((GtkWidget **)&self->right_edge, gtk_widget_unparent);

  G_OBJECT_CLASS (panel_dock_child_parent_class)->dispose (object);
}

static void
panel_dock_child_get_property (GObject    *object,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  PanelDockChild *self = PANEL_DOCK_CHILD (object);

  switch (prop_id)
    {
    case PROP_CHILD:
      g_value_set_object (value, panel_dock_child_get_child (self));
      break;

    case PROP_EMPTY:
      g_value_set_boolean (value, panel_dock_child_get_empty (self));
      break;

    case PROP_REVEAL_CHILD:
      g_value_set_boolean (value, panel_dock_child_get_reveal_child (self));
      break;

    case PROP_AREA:
      g_value_set_enum (value, panel_dock_child_get_area (self));
      break;

    case PROP_TOP_EDGE:
      g_value_set_object (value, self->top_edge);
      break;

    case PROP_BOTTOM_EDGE:
      g_value_set_object (value, self->bottom_edge);
      break;

    case PROP_LEFT_EDGE:
      g_value_set_object (value, self->left_edge);
      break;

    case PROP_RIGHT_EDGE:
      g_value_set_object (value, self->right_edge);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
panel_dock_child_set_property (GObject      *object,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  PanelDockChild *self = PANEL_DOCK_CHILD (object);

  switch (prop_id)
    {
    case PROP_CHILD:
      panel_dock_child_set_child (self, g_value_get_object (value));
      break;

    case PROP_REVEAL_CHILD:
      panel_dock_child_set_reveal_child (self, g_value_get_boolean (value));
      break;

    case PROP_TOP_EDGE:
      panel_dock_child_set_edge (self, g_value_get_object (value), &self->top_edge, prop_id);
      break;

    case PROP_BOTTOM_EDGE:
      panel_dock_child_set_edge (self, g_value_get_object (value), &self->bottom_edge, prop_id);
      break;

    case PROP_LEFT_EDGE:
      panel_dock_child_set_edge (self, g_value_get_object (value), &self->left_edge, prop_id);
      break;

    case PROP_RIGHT_EDGE:
      panel_dock_child_set_edge (self, g_value_get_object (value), &self->right_edge, prop_id);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
panel_dock_child_class_init (PanelDockChildClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = panel_dock_child_dispose;
  object_class->get_property = panel_dock_child_get_property;
  object_class->set_property = panel_dock_child_set_property;

  widget_class->grab_focus = panel_dock_child_grab_focus;
  widget_class->measure = panel_dock_child_measure;
  widget_class->size_allocate = panel_dock_child_size_allocate;

  properties [PROP_REVEAL_CHILD] =
    g_param_spec_boolean ("reveal-child",
                          "Reveal Child",
                          "Reveal Child",
                          FALSE,
                          (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  properties [PROP_CHILD] =
    g_param_spec_object ("child",
                         "Child",
                         "Child",
                         GTK_TYPE_WIDGET,
                         (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  properties [PROP_TOP_EDGE] =
    g_param_spec_object ("top-edge", NULL, NULL,
                         GTK_TYPE_WIDGET,
                         (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  properties [PROP_BOTTOM_EDGE] =
    g_param_spec_object ("bottom-edge", NULL, NULL,
                         GTK_TYPE_WIDGET,
                         (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  properties [PROP_LEFT_EDGE] =
    g_param_spec_object ("left-edge", NULL, NULL,
                         GTK_TYPE_WIDGET,
                         (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  properties [PROP_RIGHT_EDGE] =
    g_param_spec_object ("right-edge", NULL, NULL,
                         GTK_TYPE_WIDGET,
                         (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  properties [PROP_EMPTY] =
    g_param_spec_boolean ("empty",
                          "Empty",
                          "If the dock child is empty",
                          FALSE,
                          (G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  properties [PROP_AREA] =
    g_param_spec_enum ("area", NULL, NULL,
                       PANEL_TYPE_AREA,
                       PANEL_AREA_START,
                       (G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);

  gtk_widget_class_set_css_name (widget_class, "paneldockchild");
}

static void
panel_dock_child_init (PanelDockChild *self)
{
}

GtkWidget *
panel_dock_child_new (PanelArea area)
{
  PanelDockChild *self;

  self = g_object_new (PANEL_TYPE_DOCK_CHILD, NULL);
  self->area = area;

  self->revealer = GTK_REVEALER (gtk_revealer_new ());
  gtk_revealer_set_reveal_child (GTK_REVEALER (self->revealer), TRUE);
  gtk_widget_set_parent (GTK_WIDGET (self->revealer), GTK_WIDGET (self));

  self->resizer = PANEL_RESIZER (panel_resizer_new (area));
  gtk_revealer_set_child (self->revealer, GTK_WIDGET (self->resizer));

  switch (area)
    {
    case PANEL_AREA_TOP:
      gtk_widget_set_hexpand (GTK_WIDGET (self), TRUE);
      gtk_widget_set_vexpand (GTK_WIDGET (self), FALSE);
      gtk_revealer_set_transition_type (GTK_REVEALER (self->revealer),
                                        GTK_REVEALER_TRANSITION_TYPE_SLIDE_DOWN);
      gtk_widget_add_css_class (GTK_WIDGET (self), "top");
      break;

    case PANEL_AREA_BOTTOM:
      gtk_widget_set_hexpand (GTK_WIDGET (self), TRUE);
      gtk_widget_set_vexpand (GTK_WIDGET (self), FALSE);
      gtk_revealer_set_transition_type (GTK_REVEALER (self->revealer),
                                        GTK_REVEALER_TRANSITION_TYPE_SLIDE_UP);
      gtk_widget_add_css_class (GTK_WIDGET (self), "bottom");
      break;

    case PANEL_AREA_START:
      gtk_widget_set_hexpand (GTK_WIDGET (self), FALSE);
      gtk_widget_set_vexpand (GTK_WIDGET (self), TRUE);
      gtk_revealer_set_transition_type (GTK_REVEALER (self->revealer),
                                        GTK_REVEALER_TRANSITION_TYPE_SLIDE_RIGHT);
      gtk_widget_add_css_class (GTK_WIDGET (self), "start");
      break;

    case PANEL_AREA_END:
      gtk_widget_set_hexpand (GTK_WIDGET (self), FALSE);
      gtk_widget_set_vexpand (GTK_WIDGET (self), TRUE);
      gtk_revealer_set_transition_type (GTK_REVEALER (self->revealer),
                                        GTK_REVEALER_TRANSITION_TYPE_SLIDE_LEFT);
      gtk_widget_add_css_class (GTK_WIDGET (self), "end");
      break;

    default:
    case PANEL_AREA_CENTER:
      gtk_widget_set_hexpand (GTK_WIDGET (self), TRUE);
      gtk_widget_set_vexpand (GTK_WIDGET (self), TRUE);
      gtk_revealer_set_transition_type (GTK_REVEALER (self->revealer),
                                        GTK_REVEALER_TRANSITION_TYPE_NONE);
      gtk_widget_add_css_class (GTK_WIDGET (self), "center");
      break;
    }

  return GTK_WIDGET (self);
}

PanelArea
panel_dock_child_get_area (PanelDockChild *self)
{
  g_return_val_if_fail (PANEL_IS_DOCK_CHILD (self), 0);

  return self->area;
}

static void
panel_dock_child_notify_empty_cb (PanelDockChild *self,
                                  GParamSpec     *pspec,
                                  GtkWidget      *child)
{
  g_assert (PANEL_IS_DOCK_CHILD (self));

  g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_EMPTY]);
}

void
panel_dock_child_set_child (PanelDockChild *self,
                            GtkWidget      *child)
{
  g_return_if_fail (PANEL_IS_DOCK_CHILD (self));
  g_return_if_fail (!child || GTK_IS_WIDGET (child));

  if (child == panel_dock_child_get_child (self))
    return;

  if (PANEL_IS_FRAME (child))
    g_signal_connect_object (child,
                             "notify::empty",
                             G_CALLBACK (panel_dock_child_notify_empty_cb),
                             self,
                             G_CONNECT_SWAPPED);

  panel_resizer_set_child (self->resizer, child);
  g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_CHILD]);
  g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_EMPTY]);
}

/**
 * panel_dock_child_get_child:
 * @self: a #PanelDockChild
 *
 * Gets the child widget.
 *
 * Returns: (transfer none) (nullable): a #GtkWidget
 */
GtkWidget *
panel_dock_child_get_child (PanelDockChild *self)
{
  g_return_val_if_fail (PANEL_IS_DOCK_CHILD (self), NULL);

  return panel_resizer_get_child (self->resizer);
}

gboolean
panel_dock_child_get_reveal_child (PanelDockChild *self)
{
  g_return_val_if_fail (PANEL_IS_DOCK_CHILD (self), FALSE);

  return gtk_revealer_get_reveal_child (GTK_REVEALER (self->revealer));
}

void
panel_dock_child_set_reveal_child (PanelDockChild *self,
                                   gboolean        reveal_child)
{
  g_return_if_fail (PANEL_IS_DOCK_CHILD (self));

  reveal_child = !!reveal_child;

  if (reveal_child != gtk_revealer_get_reveal_child (GTK_REVEALER (self->revealer)))
    {
      gtk_revealer_set_reveal_child (GTK_REVEALER (self->revealer), reveal_child);
      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_REVEAL_CHILD]);
    }
}

gboolean
panel_dock_child_get_empty (PanelDockChild *self)
{
  GtkWidget *child;

  g_return_val_if_fail (PANEL_IS_DOCK_CHILD (self), FALSE);

  if (!(child = panel_dock_child_get_child (self)))
    return TRUE;

  if (ADW_IS_TOOLBAR_VIEW (child))
    child = adw_toolbar_view_get_content (ADW_TOOLBAR_VIEW (child));

  if (PANEL_IS_PANED (child))
    {
      guint n_children = panel_paned_get_n_children (PANEL_PANED (child));

      if (n_children > 1)
        return FALSE;

      child = panel_paned_get_nth_child (PANEL_PANED (child), 0);
    }

  if (PANEL_IS_FRAME (child))
    return panel_frame_get_empty (PANEL_FRAME (child));

  return FALSE;
}

gboolean
panel_dock_child_get_dragging (PanelDockChild *self)
{
  g_return_val_if_fail (PANEL_IS_DOCK_CHILD (self), FALSE);

  return self->dragging;
}

void
panel_dock_child_set_dragging (PanelDockChild *self,
                               gboolean        dragging)
{
  GtkWidget *child;

  g_return_if_fail (PANEL_IS_DOCK_CHILD (self));

  self->dragging = !!dragging;

  child = panel_dock_child_get_child (self);

  if (PANEL_IS_PANED (child))
    {
      if (dragging)
        {
          if (gtk_orientable_get_orientation (GTK_ORIENTABLE (child)) == GTK_ORIENTATION_HORIZONTAL)
            gtk_widget_set_size_request (child, -1, MIN_SIZE_DURING_DRAG);
          else
            gtk_widget_set_size_request (child, MIN_SIZE_DURING_DRAG, -1);
        }
      else
        {
          gtk_widget_set_size_request (child, -1, -1);
        }
    }
}

void
panel_dock_child_foreach_frame (PanelDockChild     *self,
                                PanelFrameCallback  callback,
                                gpointer            user_data)
{
  GtkWidget *child;

  g_return_if_fail (PANEL_IS_DOCK_CHILD (self));
  g_return_if_fail (callback != NULL);

  if (!(child = panel_resizer_get_child (self->resizer)))
    return;

  if (PANEL_IS_PANED (child))
    {
      for (GtkWidget *desc = gtk_widget_get_first_child (child);
           desc != NULL;
           desc = gtk_widget_get_next_sibling (desc))
        {
          if (PANEL_IS_RESIZER (desc))
            {
              GtkWidget *rchild = panel_resizer_get_child (PANEL_RESIZER (desc));

              if (PANEL_IS_FRAME (rchild))
                callback (PANEL_FRAME (rchild), user_data);
            }
        }
    }
  else if (PANEL_IS_GRID (child))
    {
      _panel_grid_foreach_frame (PANEL_GRID (child), callback, user_data);
    }
}

int
panel_dock_child_get_drag_position (PanelDockChild *self)
{
  g_return_val_if_fail (PANEL_IS_DOCK_CHILD (self), -1);

  return panel_resizer_get_drag_position (self->resizer);
}

void
panel_dock_child_set_drag_position (PanelDockChild *self,
                                    int             drag_position)
{
  g_return_if_fail (PANEL_IS_DOCK_CHILD (self));

  panel_resizer_set_drag_position (self->resizer, drag_position);
}
