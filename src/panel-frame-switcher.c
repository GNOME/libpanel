/*
 * Copyright (c) 2013 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "config.h"

#include "panel-binding-group-private.h"
#include "panel-dock-private.h"
#include "panel-frame-private.h"
#include "panel-frame-header.h"
#include "panel-frame-switcher-private.h"
#include "panel-scaler-private.h"
#include "panel-widget.h"

/**
 * PanelFrameSwitcher:
 *
 * The `PanelFrameSwitcher` shows a row of buttons to switch between `GtkStack`
 * pages.
 */

#define TIMEOUT_EXPAND 500
#define INDICATOR_SIZE 16

struct _PanelFrameSwitcher
{
  GtkWidget          parent_instance;

  PanelBindingGroup *bindings;
  GtkCssProvider    *css_provider;
  PanelFrame        *frame;
  GtkSelectionModel *pages;
  GHashTable        *buttons;
  PanelWidget       *drag_panel;
  PanelDock         *drag_dock;

  GtkWidget         *drop_before_button;
  GskRenderNode     *drop_indicator;

  GdkRGBA            foreground_rgba;
  GdkRGBA            background_rgba;

  guint              update_css_handler;

  guint              foreground_rgba_set : 1;
  guint              background_rgba_set : 1;
};

struct _PanelFrameSwitcherClass
{
  GtkWidgetClass parent_class;
};

static void        frame_header_iface_init        (PanelFrameHeaderInterface *iface);
static PanelFrame *panel_frame_switcher_get_frame (PanelFrameSwitcher        *self);
static void        panel_frame_switcher_set_frame (PanelFrameSwitcher        *self,
                                                   PanelFrame                *frame);

G_DEFINE_TYPE_WITH_CODE (PanelFrameSwitcher, panel_frame_switcher, GTK_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_ORIENTABLE, NULL)
                         G_IMPLEMENT_INTERFACE (PANEL_TYPE_FRAME_HEADER, frame_header_iface_init))

enum {
  PROP_0,
  PROP_FOREGROUND_RGBA,
  PROP_BACKGROUND_RGBA,
  N_PROPS,

  PROP_FRAME,
  PROP_ORIENTATION,
};

static GParamSpec *properties [N_PROPS];

static void
ensure_indicator (PanelFrameSwitcher *self)
{
  GtkStyleContext *style_context;
  GtkSnapshot *snapshot;
  GtkOrientation orientation;
  GdkRGBA color;
  cairo_t *cr;

  g_assert (PANEL_IS_FRAME_SWITCHER (self));

  if (self->drop_indicator != NULL)
    return;

  orientation = gtk_orientable_get_orientation (GTK_ORIENTABLE (self));

  style_context = gtk_widget_get_style_context (GTK_WIDGET (self));
  gtk_style_context_save (style_context);
  gtk_style_context_add_class (style_context, "drop-indicator");
  gtk_style_context_get_color (style_context, &color);
  gtk_style_context_restore (style_context);

  snapshot = gtk_snapshot_new ();
  cr = gtk_snapshot_append_cairo (snapshot,
                                  &GRAPHENE_RECT_INIT (0, 0, INDICATOR_SIZE, INDICATOR_SIZE));
  gdk_cairo_set_source_rgba (cr, &color);
  cairo_set_line_width (cr, 1.0);
  cairo_translate (cr, .5, .5);

  if (orientation == GTK_ORIENTATION_HORIZONTAL)
    {
      cairo_move_to (cr, INDICATOR_SIZE/2, 0);
      cairo_line_to (cr, INDICATOR_SIZE/2, INDICATOR_SIZE-3);
      cairo_stroke (cr);
      cairo_arc (cr, INDICATOR_SIZE/2-1, INDICATOR_SIZE-5, 4, 0, 2 * M_PI);
      cairo_fill (cr);
    }
  else
    {
      cairo_move_to (cr, 0, INDICATOR_SIZE/2);
      cairo_line_to (cr, INDICATOR_SIZE-3, INDICATOR_SIZE/2);
      cairo_stroke (cr);
      cairo_arc (cr, INDICATOR_SIZE-5, INDICATOR_SIZE/2, 4, 0, 2 * M_PI);
      cairo_fill (cr);
    }

  cairo_destroy (cr);

  self->drop_indicator = gtk_snapshot_free_to_node (snapshot);
}

static void
panel_frame_switcher_init (PanelFrameSwitcher *switcher)
{
  switcher->buttons = g_hash_table_new_full (g_direct_hash, g_direct_equal, g_object_unref, NULL);

  switcher->bindings = panel_binding_group_new ();
  panel_binding_group_bind (switcher->bindings, "background-rgba", switcher, "background-rgba", 0);
  panel_binding_group_bind (switcher->bindings, "foreground-rgba", switcher, "foreground-rgba", 0);

  switcher->css_provider = gtk_css_provider_new ();
  gtk_style_context_add_provider (gtk_widget_get_style_context (GTK_WIDGET (switcher)),
                                  GTK_STYLE_PROVIDER (switcher->css_provider),
                                  GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  gtk_widget_add_css_class (GTK_WIDGET (switcher), "linked");

  _panel_dock_update_orientation (GTK_WIDGET (switcher), GTK_ORIENTATION_HORIZONTAL);
}

static void
on_button_toggled (GtkWidget        *button,
                   GParamSpec       *pspec,
                   PanelFrameSwitcher *self)
{
  gboolean active;
  guint index;

  active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));
  index = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (button), "child-index"));

  if (active)
    {
      gtk_selection_model_select_item (self->pages, index, TRUE);
    }
  else
    {
      gboolean selected = gtk_selection_model_is_selected (self->pages, index);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), selected);
    }
}

static void
rebuild_child (GtkWidget  *self,
               GIcon      *icon,
               const char *title)
{
  GtkWidget *button_child;

  button_child = NULL;

  if (icon != NULL)
    {
      button_child = gtk_image_new_from_gicon (icon);
      if (title != NULL)
        gtk_widget_set_tooltip_text (GTK_WIDGET (self), title);

      gtk_widget_remove_css_class (self, "text-button");
      gtk_widget_add_css_class (self, "image-button");
    }
  else if (title != NULL)
    {
      button_child = gtk_label_new (title);

      gtk_widget_set_tooltip_text (GTK_WIDGET (self), NULL);

      gtk_widget_remove_css_class (self, "image-button");
      gtk_widget_add_css_class (self, "text-button");
    }

  if (button_child)
    {
      gtk_widget_set_halign (GTK_WIDGET (button_child), GTK_ALIGN_CENTER);
      gtk_button_set_child (GTK_BUTTON (self), button_child);
    }

  gtk_accessible_update_property (GTK_ACCESSIBLE (self),
                                  GTK_ACCESSIBLE_PROPERTY_LABEL, title,
                                  -1);
}

static void
update_button (PanelFrameSwitcher *self,
               AdwTabPage         *page,
               GtkWidget          *button)
{
  char *title = NULL;
  GIcon *icon = NULL;
  gboolean needs_attention = FALSE;

  g_assert (PANEL_IS_FRAME_SWITCHER (self));
  g_assert (ADW_IS_TAB_PAGE (page));
  g_assert (GTK_IS_TOGGLE_BUTTON (button));

  g_object_get (page,
                "title", &title,
                "icon", &icon,
                "needs-attention", &needs_attention,
                NULL);

  rebuild_child (button, icon, title);

  gtk_widget_set_visible (button, (title != NULL || icon != NULL));

  if (needs_attention)
    gtk_widget_add_css_class (button, "needs-attention");
  else
    gtk_widget_remove_css_class (button, "needs-attention");

  g_free (title);
  g_clear_object (&icon);
}

static void
on_page_updated (AdwTabPage       *page,
                 GParamSpec       *pspec,
                 PanelFrameSwitcher *self)
{
  GtkWidget *button;

  button = g_hash_table_lookup (self->buttons, page);
  update_button (self, page, button);
}

static gboolean
panel_frame_switcher_switch_timeout (gpointer data)
{
  GtkWidget *button = data;

  g_object_steal_data (G_OBJECT (button), "-panel-switch-timer");

  if (button)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);

  return G_SOURCE_REMOVE;
}

static void
clear_timer (gpointer data)
{
  if (data)
    g_source_remove (GPOINTER_TO_UINT (data));
}

static void
panel_frame_switcher_drag_enter (GtkDropControllerMotion *motion,
                                 double                   x,
                                 double                   y,
                                 gpointer                 unused)
{
  GtkWidget *button = gtk_event_controller_get_widget (GTK_EVENT_CONTROLLER (motion));

  if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
    {
      guint switch_timer = g_timeout_add (TIMEOUT_EXPAND,
                                          panel_frame_switcher_switch_timeout,
                                          button);
      g_source_set_name_by_id (switch_timer, "[gtk] panel_frame_switcher_switch_timeout");
      g_object_set_data_full (G_OBJECT (button), "-panel-switch-timer", GUINT_TO_POINTER (switch_timer), clear_timer);
    }
}

static void
panel_frame_switcher_drag_leave (GtkDropControllerMotion *motion,
                                 gpointer                 unused)
{
  GtkWidget *button = gtk_event_controller_get_widget (GTK_EVENT_CONTROLLER (motion));
  guint switch_timer;

  switch_timer = GPOINTER_TO_UINT (g_object_steal_data (G_OBJECT (button), "-panel-switch-timer"));
  if (switch_timer)
    g_source_remove (switch_timer);
}

static void
panel_frame_switcher_click_pressed_cb (PanelFrameSwitcher *self,
                                       int                 n_presses,
                                       double              x,
                                       double              y,
                                       GtkGestureClick    *click)
{
  g_assert (PANEL_IS_FRAME_SWITCHER (self));
  g_assert (GTK_IS_GESTURE_CLICK (click));

  if (self->frame == NULL)
    return;

  if (n_presses == 2)
    {
      GtkWidget *child = gtk_widget_pick (GTK_WIDGET (self), x, y, GTK_PICK_DEFAULT);
      g_autoptr(GListModel) pages = NULL;
      AdwTabPage *page;
      guint i = 0;

      if (!GTK_IS_TOGGLE_BUTTON (child) &&
          !(child = gtk_widget_get_ancestor (child, GTK_TYPE_TOGGLE_BUTTON)))
        return;

      for (child = gtk_widget_get_prev_sibling (child);
           child;
           child = gtk_widget_get_prev_sibling (child))
        {
          if (GTK_IS_TOGGLE_BUTTON (child))
            i++;
        }

      pages = G_LIST_MODEL (panel_frame_get_pages (self->frame));
      page = g_list_model_get_item (pages, i);
      child = adw_tab_page_get_child (page);
      g_clear_object (&page);

      if (PANEL_IS_WIDGET (child))
        panel_widget_maximize (PANEL_WIDGET (child));
    }
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

  if (self->frame == NULL)
    return NULL;

  child = gtk_widget_pick (GTK_WIDGET (self), x, y, GTK_PICK_DEFAULT);
  if (!GTK_IS_TOGGLE_BUTTON (child) &&
      !(child = gtk_widget_get_ancestor (child, GTK_TYPE_TOGGLE_BUTTON)))
    return NULL;

  /* Panel must be active so that we can get a snapshot */
  if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (child)))
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (child), TRUE);

  for (child = gtk_widget_get_prev_sibling (child);
       child;
       child = gtk_widget_get_prev_sibling (child))
    {
      if (GTK_IS_TOGGLE_BUTTON (child))
        i++;
    }

  pages = G_LIST_MODEL (panel_frame_get_pages (self->frame));
  page = g_list_model_get_item (pages, i);
  child = gtk_stack_page_get_child (page);
  g_clear_object (&page);

  if (!PANEL_IS_WIDGET (child) ||
      !panel_widget_get_reorderable (PANEL_WIDGET (child)))
    return NULL;

  self->drag_panel = PANEL_WIDGET (child);

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

  if ((paintable = gtk_widget_paintable_new (GTK_WIDGET (self->drag_panel))))
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
    {
      g_set_weak_pointer (&self->drag_dock, PANEL_DOCK (dock));
      _panel_dock_begin_drag (PANEL_DOCK (dock), PANEL_WIDGET (self->drag_panel));
    }
}

static void
panel_frame_switcher_drag_end_cb (PanelFrameSwitcher *self,
                                  GdkDrag            *drag,
                                  gboolean            delete_data,
                                  GtkDragSource      *source)
{
  g_assert (GTK_IS_DRAG_SOURCE (source));
  g_assert (GDK_IS_DRAG (drag));
  g_assert (PANEL_IS_FRAME_SWITCHER (self));

  if (self->drag_dock)
    _panel_dock_end_drag (self->drag_dock, PANEL_WIDGET (self->drag_panel));

  self->drag_panel = NULL;
  g_clear_weak_pointer (&self->drag_dock);
}

static void
add_child (guint               position,
           PanelFrameSwitcher *self)
{
  GtkEventController *controller;
  GtkDragSource *drag;
  GtkWidget *button;
  gboolean selected;
  AdwTabPage *page;

  button = g_object_new (GTK_TYPE_TOGGLE_BUTTON,
                         "accessible-role", GTK_ACCESSIBLE_ROLE_TAB,
                         NULL);
  gtk_widget_set_focus_on_click (button, FALSE);

  if (gtk_orientable_get_orientation (GTK_ORIENTABLE (self)) == GTK_ORIENTATION_HORIZONTAL)
    gtk_widget_set_hexpand (button, TRUE);
  else
    gtk_widget_set_vexpand (button, TRUE);

  controller = gtk_drop_controller_motion_new ();
  g_signal_connect (controller, "enter", G_CALLBACK (panel_frame_switcher_drag_enter), NULL);
  g_signal_connect (controller, "leave", G_CALLBACK (panel_frame_switcher_drag_leave), NULL);
  gtk_widget_add_controller (button, controller);

  controller = GTK_EVENT_CONTROLLER (gtk_gesture_click_new ());
  g_signal_connect_object (controller,
                           "pressed",
                           G_CALLBACK (panel_frame_switcher_click_pressed_cb),
                           self,
                           G_CONNECT_SWAPPED);
  gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (controller), 1);
  gtk_event_controller_set_propagation_phase (controller, GTK_PHASE_CAPTURE);
  gtk_widget_add_controller (GTK_WIDGET (self), controller);

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
  gtk_widget_add_controller (GTK_WIDGET (self),
                             GTK_EVENT_CONTROLLER (drag));

  page = g_list_model_get_item (G_LIST_MODEL (self->pages), position);
  update_button (self, page, button);

  gtk_widget_set_parent (button, GTK_WIDGET (self));

  g_object_set_data (G_OBJECT (button), "child-index", GUINT_TO_POINTER (position));
  selected = gtk_selection_model_is_selected (self->pages, position);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), selected);

  gtk_accessible_update_state (GTK_ACCESSIBLE (button),
                               GTK_ACCESSIBLE_STATE_SELECTED, selected,
                               -1);

#if 0
  gtk_accessible_update_relation (GTK_ACCESSIBLE (button),
                                  GTK_ACCESSIBLE_RELATION_CONTROLS, page, NULL,
                                  -1);
#endif

  g_signal_connect (button, "notify::active", G_CALLBACK (on_button_toggled), self);
  g_signal_connect (page, "notify", G_CALLBACK (on_page_updated), self);

  g_hash_table_insert (self->buttons, g_object_ref (page), button);

  g_object_unref (page);
}

static void
populate_switcher (PanelFrameSwitcher *self)
{
  guint i;

  for (i = 0; i < g_list_model_get_n_items (G_LIST_MODEL (self->pages)); i++)
    add_child (i, self);
}

static void
clear_switcher (PanelFrameSwitcher *self)
{
  GHashTableIter iter;
  GtkWidget *page;
  GtkWidget *button;

  g_hash_table_iter_init (&iter, self->buttons);
  while (g_hash_table_iter_next (&iter, (gpointer *)&page, (gpointer *)&button))
    {
      gtk_widget_unparent (button);
      g_signal_handlers_disconnect_by_func (page, on_page_updated, self);
      g_hash_table_iter_remove (&iter);
    }
}

static void
items_changed_cb (GListModel       *model,
                  guint             position,
                  guint             removed,
                  guint             added,
                  PanelFrameSwitcher *switcher)
{
  clear_switcher (switcher);
  populate_switcher (switcher);
}

static void
selection_changed_cb (GtkSelectionModel *model,
                      guint              position,
                      guint              n_items,
                      PanelFrameSwitcher  *switcher)
{
  guint i;

  for (i = position; i < position + n_items; i++)
    {
      GtkStackPage *page;
      GtkWidget *button;
      gboolean selected;

      page = g_list_model_get_item (G_LIST_MODEL (switcher->pages), i);
      button = g_hash_table_lookup (switcher->buttons, page);
      if (button)
        {
          selected = gtk_selection_model_is_selected (switcher->pages, i);
          gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), selected);

          gtk_accessible_update_state (GTK_ACCESSIBLE (button),
                                       GTK_ACCESSIBLE_STATE_SELECTED, selected,
                                       -1);
        }
      g_object_unref (page);
    }
}

static void
disconnect_frame_signals (PanelFrameSwitcher *switcher)
{
  g_signal_handlers_disconnect_by_func (switcher->pages, items_changed_cb, switcher);
  g_signal_handlers_disconnect_by_func (switcher->pages, selection_changed_cb, switcher);
}

static void
connect_frame_signals (PanelFrameSwitcher *switcher)
{
  g_signal_connect (switcher->pages, "items-changed", G_CALLBACK (items_changed_cb), switcher);
  g_signal_connect (switcher->pages, "selection-changed", G_CALLBACK (selection_changed_cb), switcher);
}

static void
set_frame (PanelFrameSwitcher *switcher,
           PanelFrame         *frame)
{
  if (frame)
    {
      switcher->frame = g_object_ref (frame);
      switcher->pages = panel_frame_get_pages (frame);
      populate_switcher (switcher);
      connect_frame_signals (switcher);
    }
}

static void
unset_frame (PanelFrameSwitcher *switcher)
{
  if (switcher->frame)
    {
      disconnect_frame_signals (switcher);
      clear_switcher (switcher);
      g_clear_object (&switcher->frame);
      g_clear_object (&switcher->pages);
    }
}

static void
panel_frame_switcher_set_frame (PanelFrameSwitcher *switcher,
                                PanelFrame         *frame)
{
  g_return_if_fail (PANEL_IS_FRAME_SWITCHER (switcher));
  g_return_if_fail (!frame || PANEL_IS_FRAME (frame));

  if (switcher->frame == frame)
    return;

  unset_frame (switcher);
  set_frame (switcher, frame);

  gtk_widget_queue_resize (GTK_WIDGET (switcher));

  g_object_notify (G_OBJECT (switcher), "frame");
}

static PanelFrame *
panel_frame_switcher_get_frame (PanelFrameSwitcher *switcher)
{
  g_return_val_if_fail (PANEL_IS_FRAME_SWITCHER (switcher), NULL);

  return switcher->frame;
}

static void
panel_frame_switcher_snapshot (GtkWidget   *widget,
                               GtkSnapshot *snapshot)
{
  PanelFrameSwitcher *self = (PanelFrameSwitcher *)widget;
  GtkOrientation orientation;
  PanelFrame *frame;
  GtkWidget *last;
  gboolean draw_indicator = FALSE;
  int x = -1, y = -1;

  g_assert (PANEL_IS_FRAME_SWITCHER (self));
  g_assert (GTK_IS_SNAPSHOT (snapshot));

  GTK_WIDGET_CLASS (panel_frame_switcher_parent_class)->snapshot (widget, snapshot);

  orientation = gtk_orientable_get_orientation (GTK_ORIENTABLE (self));

  if (self->drop_before_button != NULL)
    {
      GtkAllocation alloc;
      gboolean is_first = gtk_widget_get_prev_sibling (self->drop_before_button) == NULL;

      gtk_widget_get_allocation (self->drop_before_button, &alloc);

      if (orientation == GTK_ORIENTATION_HORIZONTAL)
        {
          x = alloc.x - INDICATOR_SIZE/2;
          y = alloc.y + alloc.height - INDICATOR_SIZE;

          if (is_first)
            x += 4;
        }
      else
        {
          x = alloc.x + alloc.width - INDICATOR_SIZE;
          y = alloc.y - INDICATOR_SIZE/2;

          if (is_first)
            y += 4;
        }

      draw_indicator = TRUE;
    }
  else if ((last = gtk_widget_get_last_child (GTK_WIDGET (self))) &&
           (frame = panel_frame_header_get_frame (PANEL_FRAME_HEADER (self))) &&
           _panel_frame_in_drop (frame))
    {
      GtkAllocation alloc;

      gtk_widget_get_allocation (last, &alloc);

      if (orientation == GTK_ORIENTATION_HORIZONTAL)
        {
          x = alloc.x + alloc.width - INDICATOR_SIZE/2 - 4;
          y = alloc.y + alloc.height - INDICATOR_SIZE;
        }
      else
        {
          x = alloc.x + alloc.width - INDICATOR_SIZE;
          y = alloc.y + alloc.height - INDICATOR_SIZE/2 - 4;
        }

      draw_indicator = TRUE;
    }

  if (draw_indicator)
    {
      ensure_indicator (self);

      gtk_snapshot_save (snapshot);
      gtk_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (x, y));
      gtk_snapshot_append_node (snapshot, self->drop_indicator);
      gtk_snapshot_restore (snapshot);
    }
}

static void
panel_frame_switcher_get_property (GObject    *object,
                                   guint       prop_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  PanelFrameSwitcher *switcher = PANEL_FRAME_SWITCHER (object);
  GtkLayoutManager *box_layout = gtk_widget_get_layout_manager (GTK_WIDGET (switcher));

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      g_value_set_enum (value, gtk_orientable_get_orientation (GTK_ORIENTABLE (box_layout)));
      break;

    case PROP_FRAME:
      g_value_set_object (value, panel_frame_switcher_get_frame (switcher));
      break;

    case PROP_BACKGROUND_RGBA:
      g_value_set_boxed (value, panel_frame_switcher_get_background_rgba (switcher));
      break;

    case PROP_FOREGROUND_RGBA:
      g_value_set_boxed (value, panel_frame_switcher_get_foreground_rgba (switcher));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
panel_frame_switcher_set_property (GObject      *object,
                                   guint         prop_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  PanelFrameSwitcher *switcher = PANEL_FRAME_SWITCHER (object);
  GtkLayoutManager *box_layout = gtk_widget_get_layout_manager (GTK_WIDGET (switcher));

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      {
        GtkOrientation orientation = g_value_get_enum (value);
        if (gtk_orientable_get_orientation (GTK_ORIENTABLE (box_layout)) != orientation)
          {
            gtk_orientable_set_orientation (GTK_ORIENTABLE (box_layout), orientation);
            _panel_dock_update_orientation (GTK_WIDGET (switcher), orientation);
            g_clear_pointer (&switcher->drop_indicator, gsk_render_node_unref);
            g_object_notify_by_pspec (object, pspec);
          }
      }
      break;

    case PROP_FRAME:
      panel_frame_switcher_set_frame (switcher, g_value_get_object (value));
      break;

    case PROP_BACKGROUND_RGBA:
      panel_frame_switcher_set_background_rgba (switcher, g_value_get_boxed (value));
      break;

    case PROP_FOREGROUND_RGBA:
      panel_frame_switcher_set_foreground_rgba (switcher, g_value_get_boxed (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
panel_frame_switcher_css_changed (GtkWidget         *widget,
                                  GtkCssStyleChange *change)
{
  PanelFrameSwitcher *self = (PanelFrameSwitcher *)widget;

  g_assert (GTK_IS_WIDGET (widget));

  GTK_WIDGET_CLASS (panel_frame_switcher_parent_class)->css_changed (widget, change);

  g_clear_pointer (&self->drop_indicator, gsk_render_node_unref);
}

static void
panel_frame_switcher_dispose (GObject *object)
{
  PanelFrameSwitcher *switcher = PANEL_FRAME_SWITCHER (object);

  g_clear_handle_id (&switcher->update_css_handler, g_source_remove);
  g_clear_object (&switcher->css_provider);
  g_clear_object (&switcher->bindings);
  unset_frame (switcher);

  g_clear_pointer (&switcher->drop_indicator, gsk_render_node_unref);

  G_OBJECT_CLASS (panel_frame_switcher_parent_class)->dispose (object);
}

static void
panel_frame_switcher_finalize (GObject *object)
{
  PanelFrameSwitcher *switcher = PANEL_FRAME_SWITCHER (object);

  g_hash_table_destroy (switcher->buttons);

  G_OBJECT_CLASS (panel_frame_switcher_parent_class)->finalize (object);
}

static void
panel_frame_switcher_class_init (PanelFrameSwitcherClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);

  object_class->get_property = panel_frame_switcher_get_property;
  object_class->set_property = panel_frame_switcher_set_property;
  object_class->dispose = panel_frame_switcher_dispose;
  object_class->finalize = panel_frame_switcher_finalize;

  widget_class->snapshot = panel_frame_switcher_snapshot;
  widget_class->css_changed = panel_frame_switcher_css_changed;

  properties [PROP_BACKGROUND_RGBA] =
    g_param_spec_boxed ("background-rgba",
                        "Background RGBA",
                        "Background RGBA",
                        GDK_TYPE_RGBA,
                        (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  properties [PROP_FOREGROUND_RGBA] =
    g_param_spec_boxed ("foreground-rgba",
                        "Foreground RGBA",
                        "Foreground RGBA",
                        GDK_TYPE_RGBA,
                        (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);

  g_object_class_override_property (object_class, PROP_FRAME, "frame");
  g_object_class_override_property (object_class, PROP_ORIENTATION, "orientation");

  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BOX_LAYOUT);
  gtk_widget_class_set_css_name (widget_class, "panelframeswitcher");
  gtk_widget_class_set_accessible_role (widget_class, GTK_ACCESSIBLE_ROLE_TAB_LIST);
}

/**
 * panel_frame_switcher_new:
 *
 * Create a new `PanelFrameSwitcher`.
 *
 * Returns: a new `PanelFrameSwitcher`.
 */
GtkWidget *
panel_frame_switcher_new (void)
{
  return g_object_new (PANEL_TYPE_FRAME_SWITCHER, NULL);
}

static gboolean
panel_frame_switcher_can_drop (PanelFrameHeader *header,
                               PanelWidget      *widget)
{
  const char *kind;

  g_assert (PANEL_IS_FRAME_SWITCHER (header));
  g_assert (PANEL_IS_WIDGET (widget));

  kind = panel_widget_get_kind (widget);

  /* Don't alloc documents here */

  return g_strcmp0 (kind, PANEL_WIDGET_KIND_DOCUMENT) != 0;
}

static gboolean
panel_frame_switcher_update_css (PanelFrameSwitcher *self)
{
  GString *str = NULL;

  g_assert (PANEL_IS_FRAME_SWITCHER (self));
  g_assert (self->css_provider != NULL);
  g_assert (GTK_IS_CSS_PROVIDER (self->css_provider));

  str = g_string_new (NULL);

  /*
   * We set various styles on this provider so that we can update multiple
   * widgets using the same CSS style. That includes ourself, various buttons,
   * labels, and some images.
   */

  if (self->background_rgba_set)
    {
#if 0
      /* FIXME: Make work with updated css style */
      gchar *bgstr = gdk_rgba_to_string (&self->background_rgba);

      g_string_append        (str, "panelframeswitcher {\n");
      g_string_append        (str, "  background: none;\n");
      g_string_append_printf (str, "  background-color: %s;\n", bgstr);
      g_string_append        (str, "}\n");

      g_free (bgstr);
#endif
    }

  /* Use -1 for length so CSS provider knows the string is NULL terminated
   * and there-by avoid a string copy.
   */
  gtk_css_provider_load_from_data (self->css_provider, str->str, -1);

  self->update_css_handler = 0;

  g_string_free (str, TRUE);

  return G_SOURCE_REMOVE;
}

static void
panel_frame_switcher_queue_update_css (PanelFrameSwitcher *self)
{
  g_assert (PANEL_IS_FRAME_SWITCHER (self));

  if (self->update_css_handler == 0)
    self->update_css_handler =
      g_idle_add_full (G_PRIORITY_HIGH,
                       (GSourceFunc)panel_frame_switcher_update_css,
                       g_object_ref (self),
                       g_object_unref);
}

static void
panel_frame_switcher_page_changed (PanelFrameHeader *header,
                                   PanelWidget      *widget)
{
  PanelFrameSwitcher *self = (PanelFrameSwitcher *)header;

  g_assert (PANEL_IS_FRAME_HEADER (header));
  g_assert (!widget || PANEL_IS_WIDGET (widget));

  self->foreground_rgba_set = FALSE;
  self->background_rgba_set = FALSE;
  panel_frame_switcher_queue_update_css (self);

  panel_binding_group_set_source (self->bindings, widget);
}

static void
frame_header_iface_init (PanelFrameHeaderInterface *iface)
{
  iface->can_drop = panel_frame_switcher_can_drop;
  iface->page_changed = panel_frame_switcher_page_changed;
}

/**
 * panel_frame_switcher_get_background_rgba:
 * @self: a #PanelFrameSwitcher
 *
 * Gets the background color of the switcher.
 *
 * Returns: (nullable): the background color
 */
const GdkRGBA *
panel_frame_switcher_get_background_rgba (PanelFrameSwitcher *self)
{
  g_return_val_if_fail (PANEL_IS_FRAME_SWITCHER (self), NULL);

  return self->background_rgba_set ? &self->background_rgba : NULL;
}

/**
 * panel_frame_switcher_set_background_rgba:
 * @self: a #PanelFrameSwitcher
 * @background_rgba: (nullable): the background color
 *
 * Sets the background color of the switcher.
 */
void
panel_frame_switcher_set_background_rgba (PanelFrameSwitcher *self,
                                            const GdkRGBA       *background_rgba)
{
  g_return_if_fail (PANEL_IS_FRAME_SWITCHER (self));

  self->background_rgba_set = background_rgba != NULL;
  if (background_rgba)
    self->background_rgba = *background_rgba;
  panel_frame_switcher_queue_update_css (self);
  g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_BACKGROUND_RGBA]);
}

/**
 * panel_frame_switcher_get_foreground_rgba:
 * @self: a #PanelFrameSwitcher
 *
 * Gets the foreground color of the switcher.
 *
 * Returns: (nullable): the foreground color
 */
const GdkRGBA *
panel_frame_switcher_get_foreground_rgba (PanelFrameSwitcher *self)
{
  g_return_val_if_fail (PANEL_IS_FRAME_SWITCHER (self), NULL);

  return self->foreground_rgba_set ? &self->foreground_rgba : NULL;
}

/**
 * panel_frame_switcher_set_foreground_rgba:
 * @self: a #PanelFrameSwitcher
 * @foreground_rgba: (nullable): the foreground color
 *
 * Sets the foreground color of the switcher.
 */
void
panel_frame_switcher_set_foreground_rgba (PanelFrameSwitcher *self,
                                            const GdkRGBA       *foreground_rgba)
{
  g_return_if_fail (PANEL_IS_FRAME_SWITCHER (self));

  self->foreground_rgba_set = foreground_rgba != NULL;
  if (foreground_rgba)
    self->foreground_rgba = *foreground_rgba;
  panel_frame_switcher_queue_update_css (self);
  g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_FOREGROUND_RGBA]);
}

AdwTabPage *
_panel_frame_switcher_get_page (PanelFrameSwitcher *self,
                                GtkWidget          *button)
{
  GHashTableIter iter;
  AdwTabPage *page;
  GtkWidget *page_button;

  g_return_val_if_fail (PANEL_IS_FRAME_SWITCHER (self), NULL);
  g_return_val_if_fail (GTK_IS_WIDGET (button), NULL);

  g_hash_table_iter_init (&iter, self->buttons);
  while (g_hash_table_iter_next (&iter, (gpointer *)&page, (gpointer *)&page_button))
    {
      if (page_button == button)
        return page;
    }

  return NULL;
}

void
_panel_frame_switcher_set_drop_before (PanelFrameSwitcher *self,
                                       PanelWidget        *widget)
{
  GHashTableIter iter;
  AdwTabPage *page;
  GtkWidget *button;
  GtkWidget *child;

  g_return_if_fail (PANEL_IS_FRAME_SWITCHER (self));
  g_return_if_fail (!widget || PANEL_IS_WIDGET (widget));

  self->drop_before_button = NULL;

  if (widget != NULL)
    {
      g_hash_table_iter_init (&iter, self->buttons);
      while (g_hash_table_iter_next (&iter, (gpointer *)&page, (gpointer *)&button))
        {
          g_assert (ADW_IS_TAB_PAGE (page));
          g_assert (GTK_IS_TOGGLE_BUTTON (button));

          child = adw_tab_page_get_child (page);

          if (child == GTK_WIDGET (widget))
            {
              self->drop_before_button = button;
              break;
            }
        }
    }

  gtk_widget_queue_draw (GTK_WIDGET (self));
}
