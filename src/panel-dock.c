/* panel-dock.c
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
#include "panel-dock-private.h"
#include "panel-dock-child-private.h"
#include "panel-frame-private.h"
#include "panel-maximized-controls-private.h"
#include "panel-paned-private.h"
#include "panel-resizer-private.h"
#include "panel-widget.h"

typedef struct
{
  GtkOverlay *overlay;
  GtkGrid *grid;
  PanelMaximizedControls *controls;

  PanelWidget *maximized;

  guint reveal_start : 1;
  guint reveal_end : 1;
  guint reveal_top : 1;
  guint reveal_bottom : 1;
} PanelDockPrivate;

static void buildable_iface_init (GtkBuildableIface *iface);

G_DEFINE_TYPE_WITH_CODE (PanelDock, panel_dock, GTK_TYPE_WIDGET,
                         G_ADD_PRIVATE (PanelDock)
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_BUILDABLE, buildable_iface_init))

enum {
  PROP_0,
  PROP_REVEAL_BOTTOM,
  PROP_REVEAL_END,
  PROP_REVEAL_START,
  PROP_REVEAL_TOP,
  PROP_CAN_REVEAL_BOTTOM,
  PROP_CAN_REVEAL_END,
  PROP_CAN_REVEAL_START,
  PROP_CAN_REVEAL_TOP,
  N_PROPS
};

enum {
  PANEL_DRAG_BEGIN,
  PANEL_DRAG_END,
  N_SIGNALS
};

static GParamSpec *properties [N_PROPS];
static guint signals [N_SIGNALS];

GtkWidget *
panel_dock_new (void)
{
  return g_object_new (PANEL_TYPE_DOCK, NULL);
}

static void
get_grid_positions (PanelDockPosition  position,
                    int               *left,
                    int               *top,
                    int               *width,
                    int               *height,
                    GtkOrientation    *orientation)
{

  switch (position)
    {
    case PANEL_DOCK_POSITION_START:
      *left = 0, *top = 0, *width = 1, *height = 3;
      *orientation = GTK_ORIENTATION_VERTICAL;
      break;

    case PANEL_DOCK_POSITION_END:
      *left = 2, *top = 0, *width = 1, *height = 3;
      *orientation = GTK_ORIENTATION_VERTICAL;
      break;

    case PANEL_DOCK_POSITION_TOP:
      *left = 1, *top = 0, *width = 1, *height = 1;
      *orientation = GTK_ORIENTATION_HORIZONTAL;
      break;

    case PANEL_DOCK_POSITION_BOTTOM:
      *left = 1, *top = 2, *width = 1, *height = 1;
      *orientation = GTK_ORIENTATION_HORIZONTAL;
      break;

    default:
    case PANEL_DOCK_POSITION_CENTER:
      *left = 1, *top = 1, *width = 1, *height = 1;
      *orientation = GTK_ORIENTATION_HORIZONTAL;
      break;
    }
}

static gboolean
set_reveal (PanelDock         *self,
            PanelDockPosition  position,
            gboolean           value)
{
  PanelDockPrivate *priv = panel_dock_get_instance_private (self);

  g_return_val_if_fail (PANEL_IS_DOCK (self), FALSE);

  for (GtkWidget *child = gtk_widget_get_first_child (GTK_WIDGET (priv->grid));
       child != NULL;
       child = gtk_widget_get_next_sibling (child))
    {
      if (!PANEL_IS_DOCK_CHILD (child))
        continue;

      if (panel_dock_child_get_position (PANEL_DOCK_CHILD (child)) == position)
        {
          if (value != panel_dock_child_get_reveal_child (PANEL_DOCK_CHILD (child)))
            {
              panel_dock_child_set_reveal_child (PANEL_DOCK_CHILD (child), value);
              return TRUE;
            }
        }
    }

  return FALSE;
}

static gboolean
panel_dock_get_child_position_cb (PanelDock     *self,
                                  GtkWidget     *child,
                                  GtkAllocation *allocation,
                                  GtkOverlay    *overlay)
{
  GtkRequisition min, nat;

  g_assert (PANEL_IS_DOCK (self));
  g_assert (GTK_IS_WIDGET (child));
  g_assert (allocation != NULL);
  g_assert (GTK_IS_OVERLAY (overlay));

  if (PANEL_IS_MAXIMIZED_CONTROLS (child))
    return FALSE;

  /* Just use the whole section for now and rely on styling to
   * adjust the margin/padding/etc.
   */
  gtk_widget_get_preferred_size (child, &min, &nat);
  gtk_widget_get_allocation (GTK_WIDGET (self), allocation);
  allocation->x = 0;
  allocation->y = 0;

  return TRUE;
}

static void
panel_dock_controls_close_cb (PanelDock              *self,
                              PanelMaximizedControls *controls)
{
  PanelDockPrivate *priv = panel_dock_get_instance_private (self);

  g_assert (PANEL_IS_DOCK (self));
  g_assert (PANEL_IS_MAXIMIZED_CONTROLS (controls));

  if (priv->maximized != NULL)
    panel_widget_unmaximize (priv->maximized);
}

static void
panel_dock_dispose (GObject *object)
{
  PanelDock *self = (PanelDock *)object;
  PanelDockPrivate *priv = panel_dock_get_instance_private (self);

  _panel_dock_set_maximized (self, NULL);
  g_clear_pointer ((GtkWidget **)&priv->overlay, gtk_widget_unparent);

  G_OBJECT_CLASS (panel_dock_parent_class)->dispose (object);
}

static void
panel_dock_get_property (GObject    *object,
                         guint       prop_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
  PanelDock *self = PANEL_DOCK (object);

  switch (prop_id)
    {
    case PROP_REVEAL_BOTTOM:
      g_value_set_boolean (value, panel_dock_get_reveal_bottom (self));
      break;

    case PROP_REVEAL_END:
      g_value_set_boolean (value, panel_dock_get_reveal_end (self));
      break;

    case PROP_REVEAL_START:
      g_value_set_boolean (value, panel_dock_get_reveal_start (self));
      break;

    case PROP_REVEAL_TOP:
      g_value_set_boolean (value, panel_dock_get_reveal_top (self));
      break;

    case PROP_CAN_REVEAL_BOTTOM:
      g_value_set_boolean (value, panel_dock_get_can_reveal_bottom (self));
      break;

    case PROP_CAN_REVEAL_END:
      g_value_set_boolean (value, panel_dock_get_can_reveal_end (self));
      break;

    case PROP_CAN_REVEAL_START:
      g_value_set_boolean (value, panel_dock_get_can_reveal_start (self));
      break;

    case PROP_CAN_REVEAL_TOP:
      g_value_set_boolean (value, panel_dock_get_can_reveal_top (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
panel_dock_set_property (GObject      *object,
                         guint         prop_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  PanelDock *self = PANEL_DOCK (object);

  switch (prop_id)
    {
    case PROP_REVEAL_BOTTOM:
      panel_dock_set_reveal_bottom (self, g_value_get_boolean (value));
      break;

    case PROP_REVEAL_END:
      panel_dock_set_reveal_end (self, g_value_get_boolean (value));
      break;

    case PROP_REVEAL_START:
      panel_dock_set_reveal_start (self, g_value_get_boolean (value));
      break;

    case PROP_REVEAL_TOP:
      panel_dock_set_reveal_top (self, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
panel_dock_class_init (PanelDockClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = panel_dock_dispose;
  object_class->get_property = panel_dock_get_property;
  object_class->set_property = panel_dock_set_property;

  properties [PROP_REVEAL_BOTTOM] =
    g_param_spec_boolean ("reveal-bottom",
                          "Reveal bottom",
                          "Reveal bottom",
                          FALSE,
                          (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  properties [PROP_REVEAL_TOP] =
    g_param_spec_boolean ("reveal-top",
                          "Reveal top",
                          "Reveal top",
                          FALSE,
                          (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  properties [PROP_REVEAL_START] =
    g_param_spec_boolean ("reveal-start",
                          "Reveal start",
                          "Reveal start",
                          FALSE,
                          (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  properties [PROP_REVEAL_END] =
    g_param_spec_boolean ("reveal-end",
                          "Reveal end",
                          "Reveal end",
                          FALSE,
                          (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  properties [PROP_CAN_REVEAL_BOTTOM] =
    g_param_spec_boolean ("can-reveal-bottom",
                          "Can reveal bottom",
                          "Can reveal bottom",
                          FALSE,
                          (G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  properties [PROP_CAN_REVEAL_TOP] =
    g_param_spec_boolean ("can-reveal-top",
                          "Can reveal top",
                          "Can reveal top",
                          FALSE,
                          (G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  properties [PROP_CAN_REVEAL_START] =
    g_param_spec_boolean ("can-reveal-start",
                          "Can reveal start",
                          "Can reveal start",
                          FALSE,
                          (G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  properties [PROP_CAN_REVEAL_END] =
    g_param_spec_boolean ("can-reveal-end",
                          "Can reveal end",
                          "Can reveal end",
                          FALSE,
                          (G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);

  /**
   * PanelDock::panel-drag-begin:
   * @self: a #PanelDock
   * @panel: a #PanelWidget
   *
   * This signal is emitted when dragging of a panel begins.
   */
  signals [PANEL_DRAG_BEGIN] =
    g_signal_new ("panel-drag-begin",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (PanelDockClass, panel_drag_begin),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1, PANEL_TYPE_WIDGET);

  /**
   * PanelDock::panel-drag-end:
   * @self: a #PanelDock
   * @panel: a #PanelWidget
   *
   * This signal is emitted when dragging of a panel either
   * completes or was cancelled.
   */
  signals [PANEL_DRAG_END] =
    g_signal_new ("panel-drag-end",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (PanelDockClass, panel_drag_end),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1, PANEL_TYPE_WIDGET);

  gtk_widget_class_set_css_name (widget_class, "paneldock");
  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BIN_LAYOUT);
}

static void
panel_dock_init (PanelDock *self)
{
  PanelDockPrivate *priv = panel_dock_get_instance_private (self);

  priv->overlay = GTK_OVERLAY (gtk_overlay_new ());
  g_signal_connect_object (priv->overlay,
                           "get-child-position",
                           G_CALLBACK (panel_dock_get_child_position_cb),
                           self,
                           G_CONNECT_SWAPPED);
  gtk_widget_set_parent (GTK_WIDGET (priv->overlay), GTK_WIDGET (self));

  priv->grid = GTK_GRID (gtk_grid_new ());
  gtk_overlay_set_child (priv->overlay, GTK_WIDGET (priv->grid));

  priv->controls = PANEL_MAXIMIZED_CONTROLS (panel_maximized_controls_new ());
  gtk_widget_set_halign (GTK_WIDGET (priv->controls), GTK_ALIGN_END);
  gtk_widget_set_valign (GTK_WIDGET (priv->controls), GTK_ALIGN_START);
  gtk_widget_hide (GTK_WIDGET (priv->controls));
  g_signal_connect_object (priv->controls,
                           "close",
                           G_CALLBACK (panel_dock_controls_close_cb),
                           self,
                           G_CONNECT_SWAPPED);
  gtk_overlay_add_overlay (priv->overlay, GTK_WIDGET (priv->controls));
}

static void
panel_dock_notify_empty_cb (PanelDock      *self,
                            GParamSpec     *pspec,
                            PanelDockChild *child)
{
  PanelDockPosition position;

  g_assert (PANEL_IS_DOCK (self));
  g_assert (PANEL_IS_DOCK_CHILD (child));

  position = panel_dock_child_get_position (child);
  if (position == PANEL_DOCK_POSITION_CENTER)
    return;

  if (panel_dock_child_get_empty (child))
    panel_dock_child_set_reveal_child (child, FALSE);

  switch (panel_dock_child_get_position (child))
    {
    case PANEL_DOCK_POSITION_START:
      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_CAN_REVEAL_START]);
      break;

    case PANEL_DOCK_POSITION_END:
      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_CAN_REVEAL_END]);
      break;

    case PANEL_DOCK_POSITION_TOP:
      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_CAN_REVEAL_TOP]);
      break;

    case PANEL_DOCK_POSITION_BOTTOM:
      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_CAN_REVEAL_BOTTOM]);
      break;

    case PANEL_DOCK_POSITION_CENTER:
    default:
      break;
    }
}

static GtkWidget *
get_or_create_dock_child (PanelDock         *self,
                          PanelDockPosition  position,
                          int                left,
                          int                top,
                          int                width,
                          int                height)
{
  PanelDockPrivate *priv = panel_dock_get_instance_private (self);
  GtkWidget *child;

  g_assert (PANEL_IS_DOCK (self));

  for (child = gtk_widget_get_first_child (GTK_WIDGET (priv->grid));
       child != NULL;
       child = gtk_widget_get_next_sibling (child))
    {
      if (PANEL_IS_DOCK_CHILD (child))
        {
          if (position == panel_dock_child_get_position (PANEL_DOCK_CHILD (child)))
            return child;
        }
    }

  child = panel_dock_child_new (position);
  panel_dock_child_set_reveal_child (PANEL_DOCK_CHILD (child), FALSE);
  g_signal_connect_object (child,
                           "notify::empty",
                           G_CALLBACK (panel_dock_notify_empty_cb),
                           self,
                           G_CONNECT_SWAPPED);
  gtk_grid_attach (priv->grid, child, left, top, width, height);

  return child;
}

static GtkWidget *
find_first_frame (GtkWidget *parent)
{
  for (GtkWidget *child = gtk_widget_get_first_child (parent);
       child != NULL;
       child = gtk_widget_get_next_sibling (child))
    {
      if (PANEL_IS_FRAME (child))
        return child;

      if (PANEL_IS_RESIZER (child))
        {
          GtkWidget *resizer_child = panel_resizer_get_child (PANEL_RESIZER (child));

          if (PANEL_IS_FRAME (resizer_child))
            return resizer_child;
        }
    }

  return NULL;
}

static void
panel_dock_add_child (GtkBuildable *buildable,
                      GtkBuilder   *builder,
                      GObject      *object,
                      const char   *type)
{
  PanelDock *self = (PanelDock *)buildable;
  PanelDockPrivate *priv = panel_dock_get_instance_private (self);
  PanelDockPosition position = 0;
  GtkOrientation orientation = 0;
  gboolean reveal;
  int left;
  int top;
  int width;
  int height;

  g_assert (PANEL_IS_DOCK (self));
  g_assert (GTK_IS_BUILDER (builder));
  g_assert (G_IS_OBJECT (object));

  if (!GTK_IS_WIDGET (object))
    return;

  if (g_strcmp0 (type, "start") == 0)
    {
      position = PANEL_DOCK_POSITION_START;
      reveal = priv->reveal_start;
    }
  else if (g_strcmp0 (type, "end") == 0)
    {
      position = PANEL_DOCK_POSITION_END;
      reveal = priv->reveal_end;
    }
  else if (g_strcmp0 (type, "top") == 0)
    {
      position = PANEL_DOCK_POSITION_TOP;
      reveal = priv->reveal_top;
    }
  else if (g_strcmp0 (type, "bottom") == 0)
    {
      position = PANEL_DOCK_POSITION_BOTTOM;
      reveal = priv->reveal_bottom;
    }
  else
    {
      position = PANEL_DOCK_POSITION_CENTER;
      reveal = TRUE;
    }

  get_grid_positions (position, &left, &top, &width, &height, &orientation);

  if (!PANEL_IS_DOCK_CHILD (object))
    {
      GtkWidget *dock_child = get_or_create_dock_child (self, position, left, top, width, height);

      if (position != PANEL_DOCK_POSITION_CENTER && PANEL_IS_WIDGET (object))
        {
          GtkWidget *paned = panel_dock_child_get_child (PANEL_DOCK_CHILD (dock_child));
          GtkWidget *frame;

          if (paned == NULL)
            {
              paned = panel_paned_new ();
              gtk_orientable_set_orientation (GTK_ORIENTABLE (paned), orientation);
              panel_dock_child_set_child (PANEL_DOCK_CHILD (dock_child), paned);
            }

          if (!(frame = find_first_frame (paned)))
            {
              frame = panel_frame_new ();
              gtk_orientable_set_orientation (GTK_ORIENTABLE (frame), orientation);
              panel_paned_append (PANEL_PANED (paned), frame);
            }

          panel_frame_add (PANEL_FRAME (frame), PANEL_WIDGET (object));
        }
      else
        {
          panel_dock_child_set_child (PANEL_DOCK_CHILD (dock_child), GTK_WIDGET (object));
        }
    }
  else
    {
      gtk_grid_attach (priv->grid, GTK_WIDGET (object), left, top, width, height);
    }

  set_reveal (self, position, reveal);
}

static void
buildable_iface_init (GtkBuildableIface *iface)
{
  iface->add_child = panel_dock_add_child;
}

gboolean
panel_dock_get_reveal_bottom (PanelDock *self)
{
  PanelDockPrivate *priv = panel_dock_get_instance_private (self);
  g_return_val_if_fail (PANEL_IS_DOCK (self), FALSE);
  return priv->reveal_bottom;
}

gboolean
panel_dock_get_reveal_end (PanelDock *self)
{
  PanelDockPrivate *priv = panel_dock_get_instance_private (self);
  g_return_val_if_fail (PANEL_IS_DOCK (self), FALSE);
  return priv->reveal_end;
}

gboolean
panel_dock_get_reveal_start (PanelDock *self)
{
  PanelDockPrivate *priv = panel_dock_get_instance_private (self);
  g_return_val_if_fail (PANEL_IS_DOCK (self), FALSE);
  return priv->reveal_start;
}

gboolean
panel_dock_get_reveal_top (PanelDock *self)
{
  PanelDockPrivate *priv = panel_dock_get_instance_private (self);
  g_return_val_if_fail (PANEL_IS_DOCK (self), FALSE);
  return priv->reveal_top;
}

void
panel_dock_set_reveal_bottom (PanelDock *self,
                              gboolean   reveal_bottom)
{
  PanelDockPrivate *priv = panel_dock_get_instance_private (self);

  g_return_if_fail (PANEL_IS_DOCK (self));

  priv->reveal_bottom = !!reveal_bottom;
  if (set_reveal (self, PANEL_DOCK_POSITION_BOTTOM, reveal_bottom))
    g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_REVEAL_BOTTOM]);
}

void
panel_dock_set_reveal_end (PanelDock *self,
                           gboolean   reveal_end)
{
  PanelDockPrivate *priv = panel_dock_get_instance_private (self);

  g_return_if_fail (PANEL_IS_DOCK (self));

  priv->reveal_end = !!reveal_end;
  if (set_reveal (self, PANEL_DOCK_POSITION_END, reveal_end))
    g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_REVEAL_END]);
}

void
panel_dock_set_reveal_start (PanelDock *self,
                             gboolean   reveal_start)
{
  PanelDockPrivate *priv = panel_dock_get_instance_private (self);

  g_return_if_fail (PANEL_IS_DOCK (self));

  priv->reveal_start = !!reveal_start;
  if (set_reveal (self, PANEL_DOCK_POSITION_START, reveal_start))
    g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_REVEAL_START]);
}

void
panel_dock_set_reveal_top (PanelDock *self,
                           gboolean   reveal_top)
{
  PanelDockPrivate *priv = panel_dock_get_instance_private (self);

  g_return_if_fail (PANEL_IS_DOCK (self));

  priv->reveal_top = !!reveal_top;
  if (set_reveal (self, PANEL_DOCK_POSITION_TOP, reveal_top))
    g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_REVEAL_TOP]);
}

static GtkWidget *
panel_dock_get_child_at_position (PanelDock         *self,
                                  PanelDockPosition  position)
{
  PanelDockPrivate *priv = panel_dock_get_instance_private (self);

  g_return_val_if_fail (PANEL_IS_DOCK (self), NULL);

  for (GtkWidget *child = gtk_widget_get_first_child (GTK_WIDGET (priv->grid));
       child != NULL;
       child = gtk_widget_get_next_sibling (child))
    {
      if (!PANEL_IS_DOCK_CHILD (child))
        continue;

      if (panel_dock_child_get_position (PANEL_DOCK_CHILD (child)) == position)
        return child;
    }

  return NULL;
}

GtkWidget *
_panel_dock_get_top_child (PanelDock *self)
{
  return panel_dock_get_child_at_position (self, PANEL_DOCK_POSITION_TOP);
}

GtkWidget *
_panel_dock_get_bottom_child (PanelDock *self)
{
  return panel_dock_get_child_at_position (self, PANEL_DOCK_POSITION_BOTTOM);
}

GtkWidget *
_panel_dock_get_start_child (PanelDock *self)
{
  return panel_dock_get_child_at_position (self, PANEL_DOCK_POSITION_START);
}

GtkWidget *
_panel_dock_get_end_child (PanelDock *self)
{
  return panel_dock_get_child_at_position (self, PANEL_DOCK_POSITION_END);
}

static gboolean
panel_dock_get_can_reveal (PanelDock         *self,
                           PanelDockPosition  position)
{
  GtkWidget *child;

  g_return_val_if_fail (PANEL_IS_DOCK (self), FALSE);

  if (!(child = panel_dock_get_child_at_position (self, position)))
    return FALSE;

  return !panel_dock_child_get_empty (PANEL_DOCK_CHILD (child));
}

gboolean
panel_dock_get_can_reveal_bottom (PanelDock *self)
{
  return panel_dock_get_can_reveal (self, PANEL_DOCK_POSITION_BOTTOM);
}

gboolean
panel_dock_get_can_reveal_top (PanelDock *self)
{
  return panel_dock_get_can_reveal (self, PANEL_DOCK_POSITION_TOP);
}

gboolean
panel_dock_get_can_reveal_start (PanelDock *self)
{
  return panel_dock_get_can_reveal (self, PANEL_DOCK_POSITION_START);
}

gboolean
panel_dock_get_can_reveal_end (PanelDock *self)
{
  return panel_dock_get_can_reveal (self, PANEL_DOCK_POSITION_END);
}

static void
prepare_for_drag (PanelDock         *self,
                  PanelDockPosition  position)
{
  GtkWidget *child;
  GtkWidget *paned;

  g_assert (PANEL_IS_DOCK (self));

  if (!(child = panel_dock_get_child_at_position (self, position)))
    {
      GtkOrientation orientation;
      int left, top, width, height;

      /* TODO: Add policy to disable creating some panels (like top). */

      get_grid_positions (position, &left, &top, &width, &height, &orientation);
      child = get_or_create_dock_child (self, position, left, top, width, height);
      paned = panel_dock_child_get_child (PANEL_DOCK_CHILD (child));

      if (paned == NULL)
        {
          GtkWidget *frame;

          paned = panel_paned_new ();
          gtk_orientable_set_orientation (GTK_ORIENTABLE (paned), orientation);
          panel_dock_child_set_child (PANEL_DOCK_CHILD (child), paned);

          frame = panel_frame_new ();
          gtk_orientable_set_orientation (GTK_ORIENTABLE (frame), orientation);
          panel_paned_append (PANEL_PANED (paned), frame);
        }
    }

  panel_dock_child_set_dragging (PANEL_DOCK_CHILD (child), TRUE);
}

static void
unprepare_from_drag (PanelDock         *self,
                     PanelDockPosition  position)
{
  GtkWidget *child;

  g_assert (PANEL_IS_DOCK (self));

  if ((child = panel_dock_get_child_at_position (self, position)))
    panel_dock_child_set_dragging (PANEL_DOCK_CHILD (child), FALSE);
}

void
_panel_dock_begin_drag (PanelDock   *self,
                        PanelWidget *panel)
{
  g_return_if_fail (PANEL_IS_DOCK (self));
  g_return_if_fail (PANEL_IS_WIDGET (panel));

  /* For each of the edges that policy does not prohibit it,
   * make sure that there is a child there that we can expand
   * if necessary.
   */
  prepare_for_drag (self, PANEL_DOCK_POSITION_START);
  prepare_for_drag (self, PANEL_DOCK_POSITION_END);
  prepare_for_drag (self, PANEL_DOCK_POSITION_TOP);
  prepare_for_drag (self, PANEL_DOCK_POSITION_BOTTOM);

  g_signal_emit (self, signals [PANEL_DRAG_BEGIN], 0, panel);
}

void
_panel_dock_end_drag (PanelDock   *self,
                      PanelWidget *panel)
{
  g_return_if_fail (PANEL_IS_DOCK (self));
  g_return_if_fail (PANEL_IS_WIDGET (panel));

  g_signal_emit (self, signals [PANEL_DRAG_END], 0, panel);

  unprepare_from_drag (self, PANEL_DOCK_POSITION_START);
  unprepare_from_drag (self, PANEL_DOCK_POSITION_END);
  unprepare_from_drag (self, PANEL_DOCK_POSITION_TOP);
  unprepare_from_drag (self, PANEL_DOCK_POSITION_BOTTOM);
}

void
_panel_dock_update_orientation (GtkWidget      *widget,
                                GtkOrientation  orientation)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  if (orientation == GTK_ORIENTATION_HORIZONTAL)
    {
      gtk_widget_remove_css_class (widget, "vertical");
      gtk_widget_add_css_class (widget, "horizontal");
    }
  else
    {
      gtk_widget_remove_css_class (widget, "horizontal");
      gtk_widget_add_css_class (widget, "vertical");
    }

  gtk_accessible_update_property (GTK_ACCESSIBLE (widget),
                                  GTK_ACCESSIBLE_PROPERTY_ORIENTATION, orientation,
                                  -1);
}

void
_panel_dock_set_maximized (PanelDock   *self,
                           PanelWidget *widget)
{
  PanelDockPrivate *priv = panel_dock_get_instance_private (self);

  g_return_if_fail (PANEL_IS_DOCK (self));
  g_return_if_fail (!widget || PANEL_IS_WIDGET (widget));
  g_return_if_fail (!widget || gtk_widget_get_parent (GTK_WIDGET (widget)) == NULL);

  if (priv->maximized == widget)
    return;

  if (priv->maximized)
    {
      gtk_widget_remove_css_class (GTK_WIDGET (priv->maximized), "maximized");
      gtk_overlay_remove_overlay (priv->overlay, GTK_WIDGET (priv->maximized));
      gtk_widget_hide (GTK_WIDGET (priv->controls));
      priv->maximized = NULL;
    }

  priv->maximized = widget;

  if (priv->maximized)
    {
      gtk_widget_add_css_class (GTK_WIDGET (priv->maximized), "maximized");
      gtk_overlay_add_overlay (priv->overlay, GTK_WIDGET (priv->maximized));

      /* Move the controls to the top */
      g_object_ref (priv->controls);
      gtk_overlay_remove_overlay (priv->overlay, GTK_WIDGET (priv->controls));
      gtk_overlay_add_overlay (priv->overlay, GTK_WIDGET (priv->controls));
      gtk_widget_show (GTK_WIDGET (priv->controls));
      gtk_widget_grab_focus (GTK_WIDGET (priv->controls));
      g_object_unref (priv->controls);
    }
}

void
_panel_dock_add_widget (PanelDock      *self,
                        PanelDockChild *dock_child,
                        PanelFrame     *frame,
                        PanelWidget    *widget)
{
  PanelDockPrivate *priv = panel_dock_get_instance_private (self);

  g_return_if_fail (PANEL_IS_DOCK (self));
  g_return_if_fail (!dock_child || PANEL_IS_DOCK_CHILD (dock_child));
  g_return_if_fail (!frame || PANEL_IS_FRAME (frame));
  g_return_if_fail (PANEL_IS_WIDGET (widget));

  if (dock_child == NULL)
    {
      if (!(dock_child = PANEL_DOCK_CHILD (_panel_dock_get_start_child (self))))
        {
          int left, top, width, height;
          GtkOrientation orientation;

          get_grid_positions (PANEL_DOCK_POSITION_START, &left, &top, &width, &height, &orientation);

          dock_child = PANEL_DOCK_CHILD (panel_dock_child_new (PANEL_DOCK_POSITION_START));
          gtk_orientable_set_orientation (GTK_ORIENTABLE (dock_child), orientation);
          gtk_grid_attach (priv->grid, GTK_WIDGET (dock_child), left, top, width, height);
        }

      frame = NULL;
    }

  if (frame == NULL)
    {
      PanelDockPosition position = panel_dock_child_get_position (dock_child);
      GtkOrientation orientation;

      if (position == PANEL_DOCK_POSITION_START ||
          position == PANEL_DOCK_POSITION_END)
        orientation = GTK_ORIENTATION_VERTICAL;
      else
        orientation = GTK_ORIENTATION_HORIZONTAL;

      frame = PANEL_FRAME (panel_frame_new ());
      gtk_orientable_set_orientation (GTK_ORIENTABLE (dock_child), orientation);
      panel_dock_child_set_child (dock_child, GTK_WIDGET (frame));

      panel_dock_child_set_reveal_child (dock_child, TRUE);
    }

  g_assert (PANEL_IS_DOCK_CHILD (dock_child));
  g_assert (PANEL_IS_FRAME (frame));

  panel_frame_add (frame, widget);
}
