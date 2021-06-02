/* panel-frame.c
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
#include "panel-dock-child-private.h"
#include "panel-frame-private.h"
#include "panel-paned-private.h"
#include "panel-widget.h"

struct _PanelFrame
{
  GtkWidget parent_instance;

  GtkWidget *box;
  GtkWidget *stack;
  GtkWidget *switcher;
  GtkWidget *drag_panel;

  guint      disposed : 1;
};

#define SIZE_AT_END 50

G_DEFINE_TYPE_WITH_CODE (PanelFrame, panel_frame, GTK_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_ORIENTABLE, NULL))

enum {
  PROP_0,
  PROP_EMPTY,
  N_PROPS,

  PROP_ORIENTATION,
};

static GParamSpec *properties [N_PROPS];

GtkWidget *
panel_frame_new (void)
{
  return g_object_new (PANEL_TYPE_FRAME, NULL);
}

static void
panel_frame_notify_value_cb (PanelFrame    *self,
                             GParamSpec    *pspec,
                             GtkDropTarget *drop_target)
{
  const GValue *value;
  PanelWidget *panel;

  g_assert (PANEL_IS_FRAME (self));
  g_assert (GTK_IS_DROP_TARGET (drop_target));

  if (!(value = gtk_drop_target_get_value (drop_target)) ||
      !G_VALUE_HOLDS (value, PANEL_TYPE_WIDGET) ||
      !(panel = g_value_get_object (value)))
    return;

  if (!panel_widget_get_reorderable (panel))
    gtk_drop_target_reject (drop_target);
}

static gboolean
panel_frame_drop_accept_cb (PanelFrame    *self,
                            GdkDrop       *drop,
                            GtkDropTarget *drop_target)
{
  g_assert (PANEL_IS_FRAME (self));
  g_assert (GDK_IS_DROP (drop));
  g_assert (GTK_IS_DROP_TARGET (drop_target));

  return TRUE;
}

static GdkDragAction
panel_frame_drop_motion_cb (PanelFrame    *self,
                            double         x,
                            double         y,
                            GtkDropTarget *drop_target)
{
  GtkOrientation orientation;
  GtkAllocation alloc;

  g_assert (PANEL_IS_FRAME (self));
  g_assert (GTK_IS_DROP_TARGET (drop_target));

  gtk_widget_get_allocation (GTK_WIDGET (self), &alloc);

  orientation = gtk_orientable_get_orientation (GTK_ORIENTABLE (self));

  if (orientation == GTK_ORIENTATION_HORIZONTAL)
    {
      if (x + SIZE_AT_END >= alloc.width)
        gtk_widget_add_css_class (GTK_WIDGET (self), "drop-after");
      else
        gtk_widget_remove_css_class (GTK_WIDGET (self), "drop-after");
    }
  else
    {
      if (y + SIZE_AT_END >= alloc.height)
        gtk_widget_add_css_class (GTK_WIDGET (self), "drop-after");
      else
        gtk_widget_remove_css_class (GTK_WIDGET (self), "drop-after");
    }

  return GDK_ACTION_MOVE;
}

static void
panel_frame_drop_leave_cb (PanelFrame    *self,
                           GtkDropTarget *drop_target)
{
  g_assert (PANEL_IS_FRAME (self));
  g_assert (GTK_IS_DROP_TARGET (drop_target));

  gtk_widget_remove_css_class (GTK_WIDGET (self), "drop-after");
}

static gboolean
panel_frame_drop_cb (PanelFrame    *self,
                     const GValue  *value,
                     double         x,
                     double         y,
                     GtkDropTarget *drop_target)
{
  PanelFrame *target = self;
  GtkWidget *paned;
  GtkWidget *src_paned;
  PanelWidget *panel;
  GtkWidget *frame;
  GtkAllocation alloc;
  GtkOrientation orientation;
  gboolean is_after = FALSE;

  g_assert (PANEL_IS_FRAME (self));
  g_assert (GTK_IS_DROP_TARGET (drop_target));

  gtk_widget_remove_css_class (GTK_WIDGET (self), "drop-after");

  if (!G_VALUE_HOLDS (value, PANEL_TYPE_WIDGET) ||
      !(panel = g_value_get_object (value)) ||
      !(frame = gtk_widget_get_ancestor (GTK_WIDGET (panel), PANEL_TYPE_FRAME)))
    return FALSE;

  orientation = gtk_orientable_get_orientation (GTK_ORIENTABLE (self));
  gtk_widget_get_allocation (GTK_WIDGET (self), &alloc);

  if (orientation == GTK_ORIENTATION_HORIZONTAL)
    is_after = x + SIZE_AT_END >= alloc.width;
  else
    is_after = y + SIZE_AT_END >= alloc.height;

  if (frame == GTK_WIDGET (self) && !is_after)
    return FALSE;

  if (!(src_paned = gtk_widget_get_ancestor (GTK_WIDGET (panel), PANEL_TYPE_PANED)))
    return FALSE;

  if (!(paned = gtk_widget_get_ancestor (GTK_WIDGET (self), PANEL_TYPE_PANED)))
    return FALSE;

  if (is_after)
    {
      GtkWidget *new_frame;

      new_frame = panel_frame_new ();
      gtk_orientable_set_orientation (GTK_ORIENTABLE (new_frame), orientation);
      panel_paned_insert_after (PANEL_PANED (paned), new_frame, GTK_WIDGET (self));
      target = PANEL_FRAME (new_frame);
    }

  g_object_ref (panel);

  panel_frame_remove (PANEL_FRAME (frame), panel);
  panel_frame_add (target, panel);

  if (panel_frame_get_empty (PANEL_FRAME (frame)) &&
      panel_paned_get_n_children (PANEL_PANED (src_paned)) > 1)
    panel_paned_remove (PANEL_PANED (src_paned), frame);

  g_object_unref (panel);

  return TRUE;
}

static GdkContentProvider *
panel_frame_drag_prepare_cb (PanelFrame    *self,
                             double         x,
                             double         y,
                             GtkDragSource *source)
{
  GtkStackPage *page = NULL;
  GListModel *pages;
  GtkWidget *child;
  guint i = 0;

  g_assert (PANEL_IS_FRAME (self));
  g_assert (GTK_IS_DRAG_SOURCE (source));

  child = gtk_widget_pick (GTK_WIDGET (self->switcher), x, y, GTK_PICK_DEFAULT);
  if (!GTK_IS_TOGGLE_BUTTON (child) &&
      !(child = gtk_widget_get_ancestor (child, GTK_TYPE_TOGGLE_BUTTON)))
    return NULL;

  /* Only allow dragging the current panel */
  if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (child)))
    return NULL;

  for (child = gtk_widget_get_prev_sibling (child);
       child;
       child = gtk_widget_get_prev_sibling (child))
    i++;

  pages = G_LIST_MODEL (gtk_stack_get_pages (GTK_STACK (self->stack)));
  page = g_list_model_get_item (pages, i);
  child = gtk_stack_page_get_child (page);
  g_clear_object (&page);

  if (!PANEL_IS_WIDGET (child) ||
      !panel_widget_get_reorderable (PANEL_WIDGET (child)))
    return NULL;

  self->drag_panel = child;

  return gdk_content_provider_new_typed (PANEL_TYPE_WIDGET, child);
}

static void
panel_frame_drag_begin_cb (PanelFrame    *self,
                           GdkDrag       *drag,
                           GtkDragSource *source)
{
  g_autoptr(GdkPaintable) paintable = NULL;
  GtkWidget *dock;

  g_assert (PANEL_IS_FRAME (self));
  g_assert (GTK_IS_DRAG_SOURCE (source));
  g_assert (GDK_IS_DRAG (drag));

  paintable = gtk_widget_paintable_new (self->drag_panel);
  gtk_drag_source_set_icon (source, paintable, 0, 0);

  if ((dock = gtk_widget_get_ancestor (GTK_WIDGET (self), PANEL_TYPE_DOCK)))
    _panel_dock_begin_drag (PANEL_DOCK (dock), PANEL_WIDGET (self->drag_panel));
}

static void
panel_frame_drag_end_cb (PanelFrame    *self,
                         GdkDrag       *drag,
                         gboolean       delete_data,
                         GtkDragSource *source)
{
  GtkWidget *dock;

  g_assert (GTK_IS_DRAG_SOURCE (source));
  g_assert (GDK_IS_DRAG (drag));
  g_assert (PANEL_IS_FRAME (self));

  if ((dock = gtk_widget_get_ancestor (GTK_WIDGET (self), PANEL_TYPE_DOCK)))
    _panel_dock_end_drag (PANEL_DOCK (dock), PANEL_WIDGET (self->drag_panel));

  self->drag_panel = NULL;
}

static void
panel_frame_items_changed_cb (PanelFrame *self,
                              guint       position,
                              guint       removed,
                              guint       added,
                              GListModel *model)
{
  GtkOrientation orientation;
  gboolean hexpand;
  gboolean vexpand;

  g_assert (PANEL_IS_FRAME (self));
  g_assert (G_IS_LIST_MODEL (model));

  if (self->disposed)
    return;

  orientation = gtk_orientable_get_orientation (GTK_ORIENTABLE (self));
  hexpand = orientation == GTK_ORIENTATION_VERTICAL;
  vexpand = orientation == GTK_ORIENTATION_HORIZONTAL;

  for (GtkWidget *child = gtk_widget_get_first_child (self->switcher);
       child != NULL;
       child = gtk_widget_get_next_sibling (child))
    {
      gtk_widget_set_vexpand (child, vexpand);
      gtk_widget_set_hexpand (child, hexpand);
    }
}

static void
panel_frame_dispose (GObject *object)
{
  PanelFrame *self = (PanelFrame *)object;

  self->disposed = TRUE;

  if (self->switcher)
    gtk_stack_switcher_set_stack (GTK_STACK_SWITCHER (self->switcher), NULL);

  if (self->stack)
    {
      GtkSelectionModel *pages = gtk_stack_get_pages (GTK_STACK (self->stack));
      g_signal_handlers_disconnect_by_func (pages,
                                            G_CALLBACK (panel_frame_items_changed_cb),
                                            self);
    }

  g_clear_pointer (&self->box, gtk_widget_unparent);

  G_OBJECT_CLASS (panel_frame_parent_class)->dispose (object);
}

static void
panel_frame_get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  PanelFrame *self = PANEL_FRAME (object);

  switch (prop_id)
    {
    case PROP_EMPTY:
      g_value_set_boolean (value, panel_frame_get_empty (self));
      break;

    case PROP_ORIENTATION:
      g_value_set_enum (value, gtk_orientable_get_orientation (GTK_ORIENTABLE (self->box)));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
panel_frame_set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  PanelFrame *self = PANEL_FRAME (object);

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      gtk_orientable_set_orientation (GTK_ORIENTABLE (self->box), g_value_get_enum (value));
#if GTK_CHECK_VERSION(4, 4, 0)
      gtk_orientable_set_orientation (GTK_ORIENTABLE (self->switcher), g_value_get_enum (value));
#else
      {
        GtkLayoutManager *layout = gtk_widget_get_layout_manager (GTK_WIDGET (self->switcher));
        gtk_orientable_set_orientation (GTK_ORIENTABLE (layout), !g_value_get_enum (value));
        _panel_dock_update_orientation (GTK_WIDGET (self->switcher), !g_value_get_enum (value));
      }
#endif
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
panel_frame_class_init (PanelFrameClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = panel_frame_dispose;
  object_class->get_property = panel_frame_get_property;
  object_class->set_property = panel_frame_set_property;

  properties [PROP_EMPTY] =
    g_param_spec_boolean ("empty",
                          "Empty",
                          "If there are any panels added",
                          TRUE,
                          (G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);

  g_object_class_override_property (object_class, PROP_ORIENTATION, "orientation");

  gtk_widget_class_set_css_name (widget_class, "panelframe");
  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BIN_LAYOUT);
  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/libpanel/panel-frame.ui");
  gtk_widget_class_bind_template_child (widget_class, PanelFrame, box);
  gtk_widget_class_bind_template_child (widget_class, PanelFrame, stack);
  gtk_widget_class_bind_template_child (widget_class, PanelFrame, switcher);
}

static void
panel_frame_init (PanelFrame *self)
{
  GtkSelectionModel *pages;
  GtkDragSource *drag;
  GtkDropTarget *drop_target;
  GType types[] = { PANEL_TYPE_WIDGET };

  gtk_widget_init_template (GTK_WIDGET (self));

  drag = gtk_drag_source_new ();
  gtk_drag_source_set_actions (drag, GDK_ACTION_COPY | GDK_ACTION_MOVE);
  g_signal_connect_object (drag,
                           "prepare",
                           G_CALLBACK (panel_frame_drag_prepare_cb),
                           self,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (drag,
                           "drag-begin",
                           G_CALLBACK (panel_frame_drag_begin_cb),
                           self,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (drag,
                           "drag-end",
                           G_CALLBACK (panel_frame_drag_end_cb),
                           self,
                           G_CONNECT_SWAPPED);
  gtk_event_controller_set_propagation_phase (GTK_EVENT_CONTROLLER (drag),
                                              GTK_PHASE_CAPTURE);
  gtk_widget_add_controller (GTK_WIDGET (self->switcher),
                             GTK_EVENT_CONTROLLER (drag));

  drop_target = gtk_drop_target_new (G_TYPE_INVALID, GDK_ACTION_COPY | GDK_ACTION_MOVE);
  gtk_drop_target_set_gtypes (drop_target, types, G_N_ELEMENTS (types));
  gtk_drop_target_set_preload (drop_target, TRUE);
  g_signal_connect_object (drop_target,
                           "accept",
                           G_CALLBACK (panel_frame_drop_accept_cb),
                           self,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (drop_target,
                           "notify::value",
                           G_CALLBACK (panel_frame_notify_value_cb),
                           self,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (drop_target,
                           "drop",
                           G_CALLBACK (panel_frame_drop_cb),
                           self,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (drop_target,
                           "motion",
                           G_CALLBACK (panel_frame_drop_motion_cb),
                           self,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (drop_target,
                           "leave",
                           G_CALLBACK (panel_frame_drop_leave_cb),
                           self,
                           G_CONNECT_SWAPPED);
  gtk_widget_add_controller (GTK_WIDGET (self), GTK_EVENT_CONTROLLER (drop_target));

  pages = gtk_stack_get_pages (GTK_STACK (self->stack));
  g_signal_connect_object (pages,
                           "items-changed",
                           G_CALLBACK (panel_frame_items_changed_cb),
                           self,
                           G_CONNECT_SWAPPED | G_CONNECT_AFTER);
  panel_frame_items_changed_cb (self,
                                0,
                                0,
                                g_list_model_get_n_items (G_LIST_MODEL (pages)),
                                G_LIST_MODEL (pages));
}

void
panel_frame_add (PanelFrame  *self,
                 PanelWidget *panel)
{
  GtkStackPage *page;
  gboolean empty;

  g_return_if_fail (PANEL_IS_FRAME (self));
  g_return_if_fail (PANEL_IS_WIDGET (panel));

  empty = panel_frame_get_empty (self);
  page = gtk_stack_add_child (GTK_STACK (self->stack), GTK_WIDGET (panel));

  g_object_bind_property (panel, "title", page, "title", G_BINDING_SYNC_CREATE);
  g_object_bind_property (panel, "icon-name", page, "icon-name", G_BINDING_SYNC_CREATE);

  g_assert (!panel_frame_get_empty (self));

  if (empty)
    g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_EMPTY]);
}

void
panel_frame_remove (PanelFrame  *self,
                    PanelWidget *panel)
{
  GtkWidget *new_child;
  GtkWidget *dock_child;

  g_return_if_fail (PANEL_IS_FRAME (self));
  g_return_if_fail (PANEL_IS_WIDGET (panel));

  if (!(new_child = gtk_widget_get_prev_sibling (GTK_WIDGET (panel))))
    new_child = gtk_widget_get_next_sibling (GTK_WIDGET (panel));

  gtk_stack_remove (GTK_STACK (self->stack), GTK_WIDGET (panel));

  if (new_child)
    gtk_stack_set_visible_child (GTK_STACK (self->stack), new_child);

  if (panel_frame_get_empty (self))
    {
      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_EMPTY]);

      if ((dock_child = gtk_widget_get_ancestor (GTK_WIDGET (self), PANEL_TYPE_DOCK_CHILD)))
        {
          if (gtk_widget_get_first_child (dock_child) == gtk_widget_get_last_child (dock_child))
            g_object_notify (G_OBJECT (dock_child), "empty");
        }
    }
}

gboolean
panel_frame_get_empty (PanelFrame *self)
{
  g_return_val_if_fail (PANEL_IS_FRAME (self), FALSE);

  return gtk_widget_get_first_child (GTK_WIDGET (self->stack)) == NULL;
}
