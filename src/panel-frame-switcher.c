/* panel-frame-switcher.c
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

#include "panel-dock-private.h"
#include "panel-frame-header-private.h"
#include "panel-frame-switcher.h"
#include "panel-frame-private.h"
#include "panel-scaler-private.h"

struct _PanelFrameSwitcher
{
  GtkWidget         parent_instance;

  GtkStackSwitcher *switcher;

  /* Unowned references */
  GtkWidget        *drag_panel;
  GtkStack         *stack;
  GListModel       *pages;

  GtkOrientation    orientation : 1;
  guint             disposed : 1;
};

enum {
  PROP_0,
  N_PROPS,

  PROP_ORIENTATION,
};

static void frame_header_iface_init (PanelFrameHeaderInterface *iface);

G_DEFINE_TYPE_WITH_CODE (PanelFrameSwitcher, panel_frame_switcher, GTK_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (PANEL_TYPE_FRAME_HEADER, frame_header_iface_init)
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_ORIENTABLE, NULL))

/**
 * panel_frame_switcher_new:
 *
 * Create a new #PanelFrameSwitcher.
 *
 * Returns: (transfer full): a newly created #PanelFrameSwitcher
 */
GtkWidget *
panel_frame_switcher_new (void)
{
  return g_object_new (PANEL_TYPE_FRAME_SWITCHER, NULL);
}

static GdkContentProvider *
panel_frame_switcher_drag_prepare_cb (PanelFrameSwitcher *self,
                                      double              x,
                                      double              y,
                                      GtkDragSource      *source)
{
  GtkStackPage *page = NULL;
  GListModel *pages;
  GtkWidget *child;
  guint i = 0;

  g_assert (PANEL_IS_FRAME_SWITCHER (self));
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

#define MAX_WIDTH  250.0
#define MAX_HEIGHT 250.0

static void
panel_frame_switcher_drag_begin_cb (PanelFrameSwitcher *self,
                                    GdkDrag            *drag,
                                    GtkDragSource      *source)
{
  g_autoptr(GdkPaintable) paintable = NULL;
  GtkWidget *dock;

  g_assert (PANEL_IS_FRAME_SWITCHER (self));
  g_assert (GTK_IS_DRAG_SOURCE (source));
  g_assert (GDK_IS_DRAG (drag));
  g_assert (PANEL_IS_WIDGET (self->drag_panel));

  if ((paintable = gtk_widget_paintable_new (self->drag_panel)))
    {
      int width = gdk_paintable_get_intrinsic_width (paintable);
      int height = gdk_paintable_get_intrinsic_height (paintable);
      double ratio;

      if (width <= MAX_WIDTH && height <= MAX_HEIGHT)
        ratio = 1.0;
      else if (width > height)
        ratio = width / MAX_WIDTH;
      else
        ratio = height / MAX_HEIGHT;

      if (ratio != 1.0)
        {
          GdkPaintable *tmp = paintable;
          paintable = panel_scaler_new (paintable, ratio);
          g_clear_object (&tmp);
        }
    }
  else
    {
      GtkIconTheme *icon_theme;
      const char *icon_name;
      int scale;

      icon_theme = gtk_icon_theme_get_for_display (gtk_widget_get_display (GTK_WIDGET (self)));
      icon_name = panel_widget_get_icon_name (PANEL_WIDGET (self->drag_panel));
      scale = gtk_widget_get_scale_factor (GTK_WIDGET (self));

      if (icon_name)
        paintable = GDK_PAINTABLE (gtk_icon_theme_lookup_icon (icon_theme, icon_name, NULL, 32, scale, GTK_TEXT_DIR_NONE,  0));
    }

  if (paintable != NULL)
    gtk_drag_source_set_icon (source, paintable, 0, 0);

  if ((dock = gtk_widget_get_ancestor (GTK_WIDGET (self), PANEL_TYPE_DOCK)))
    _panel_dock_begin_drag (PANEL_DOCK (dock), PANEL_WIDGET (self->drag_panel));
}

static void
panel_frame_switcher_drag_end_cb (PanelFrameSwitcher *self,
                                  GdkDrag            *drag,
                                  gboolean            delete_data,
                                  GtkDragSource      *source)
{
  GtkWidget *dock;

  g_assert (GTK_IS_DRAG_SOURCE (source));
  g_assert (GDK_IS_DRAG (drag));
  g_assert (PANEL_IS_FRAME_SWITCHER (self));

  if ((dock = gtk_widget_get_ancestor (GTK_WIDGET (self), PANEL_TYPE_DOCK)))
    _panel_dock_end_drag (PANEL_DOCK (dock), PANEL_WIDGET (self->drag_panel));

  self->drag_panel = NULL;
}

static void
panel_frame_switcher_items_changed_cb (PanelFrameSwitcher *self,
                                       guint               position,
                                       guint               removed,
                                       guint               added,
                                       GListModel         *model)
{
  gboolean hexpand;
  gboolean vexpand;

  g_assert (PANEL_IS_FRAME_SWITCHER (self));
  g_assert (G_IS_LIST_MODEL (model));

  if (self->disposed)
    return;

  hexpand = self->orientation == GTK_ORIENTATION_HORIZONTAL;
  vexpand = self->orientation == GTK_ORIENTATION_VERTICAL;

  for (GtkWidget *child = gtk_widget_get_first_child (GTK_WIDGET (self->switcher));
       child != NULL;
       child = gtk_widget_get_next_sibling (child))
    {
      gtk_widget_set_vexpand (child, vexpand);
      gtk_widget_set_hexpand (child, hexpand);
    }
}

static void
panel_frame_switcher_set_orientation (PanelFrameSwitcher *self,
                                      GtkOrientation      orientation)
{
  g_assert (PANEL_IS_FRAME_SWITCHER (self));

  if (self->orientation == orientation)
    return;

  self->orientation = orientation;

#if GTK_CHECK_VERSION(4, 4, 0)
  gtk_orientable_set_orientation (GTK_ORIENTABLE (self->switcher), orientation);
#else
  {
    GtkLayoutManager *layout = gtk_widget_get_layout_manager (GTK_WIDGET (self->switcher));
    gtk_orientable_set_orientation (GTK_ORIENTABLE (layout), orientation);
    _panel_dock_update_orientation (GTK_WIDGET (self->switcher), orientation);
  }
#endif

  g_object_notify (G_OBJECT (self), "orientation");
}

static void
panel_frame_switcher_dispose (GObject *object)
{
  PanelFrameSwitcher *self = (PanelFrameSwitcher *)object;

  self->disposed = TRUE;

  if (self->stack)
    _panel_frame_header_disconnect (PANEL_FRAME_HEADER (self));

  g_clear_pointer ((GtkWidget **)&self->switcher, gtk_widget_unparent);

  G_OBJECT_CLASS (panel_frame_switcher_parent_class)->dispose (object);
}

static void
panel_frame_switcher_get_property (GObject    *object,
                                   guint       prop_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  PanelFrameSwitcher *self = PANEL_FRAME_SWITCHER (object);

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
panel_frame_switcher_set_property (GObject      *object,
                                   guint         prop_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  PanelFrameSwitcher *self = PANEL_FRAME_SWITCHER (object);

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      panel_frame_switcher_set_orientation (self, g_value_get_enum (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
panel_frame_switcher_class_init (PanelFrameSwitcherClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = panel_frame_switcher_dispose;
  object_class->get_property = panel_frame_switcher_get_property;
  object_class->set_property = panel_frame_switcher_set_property;

  g_object_class_override_property (object_class, PROP_ORIENTATION, "orientation");

  gtk_widget_class_set_css_name (widget_class, "panelframeswitcher");
  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BIN_LAYOUT);
  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/libpanel/panel-frame-switcher.ui");
  gtk_widget_class_bind_template_child (widget_class, PanelFrameSwitcher, switcher);
}

static void
panel_frame_switcher_init (PanelFrameSwitcher *self)
{
  GtkDragSource *drag;

  gtk_widget_init_template (GTK_WIDGET (self));

  drag = gtk_drag_source_new ();
  gtk_drag_source_set_actions (drag, GDK_ACTION_COPY | GDK_ACTION_MOVE);
  g_signal_connect_object (drag,
                           "prepare",
                           G_CALLBACK (panel_frame_switcher_drag_prepare_cb),
                           self,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (drag,
                           "drag-begin",
                           G_CALLBACK (panel_frame_switcher_drag_begin_cb),
                           self,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (drag,
                           "drag-end",
                           G_CALLBACK (panel_frame_switcher_drag_end_cb),
                           self,
                           G_CONNECT_SWAPPED);
  gtk_event_controller_set_propagation_phase (GTK_EVENT_CONTROLLER (drag),
                                              GTK_PHASE_CAPTURE);
  gtk_widget_add_controller (GTK_WIDGET (self->switcher),
                             GTK_EVENT_CONTROLLER (drag));
}

static void
panel_frame_switcher_connect (PanelFrameHeader *header,
                              PanelFrame       *frame)
{
  PanelFrameSwitcher *self = (PanelFrameSwitcher *)header;
  guint n_items;

  g_assert (PANEL_IS_FRAME_SWITCHER (self));
  g_assert (PANEL_IS_FRAME (frame));

  self->stack = _panel_frame_get_stack (frame);
  self->pages = G_LIST_MODEL (gtk_stack_get_pages (GTK_STACK (self->stack)));
  n_items = g_list_model_get_n_items (self->pages);

  gtk_stack_switcher_set_stack (self->switcher, self->stack);

  g_signal_connect_object (self->pages,
                           "items-changed",
                           G_CALLBACK (panel_frame_switcher_items_changed_cb),
                           self,
                           G_CONNECT_SWAPPED | G_CONNECT_AFTER);

  if (n_items > 0)
    panel_frame_switcher_items_changed_cb (self, 0, 0, n_items, self->pages);
}

static void
panel_frame_switcher_disconnect (PanelFrameHeader *header)
{
  PanelFrameSwitcher *self = (PanelFrameSwitcher *)header;

  g_assert (PANEL_IS_FRAME_SWITCHER (self));

  if (self->pages)
    {
      g_signal_handlers_disconnect_by_func (self->pages,
                                            G_CALLBACK (panel_frame_switcher_items_changed_cb),
                                            self);
      self->pages = NULL;
    }

  gtk_stack_switcher_set_stack (self->switcher, NULL);
  self->stack = NULL;
}

static void
frame_header_iface_init (PanelFrameHeaderInterface *iface)
{
  iface->connect = panel_frame_switcher_connect;
  iface->disconnect = panel_frame_switcher_disconnect;
}
