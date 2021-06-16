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
#include "panel-frame-header.h"
#include "panel-frame-switcher.h"
#include "panel-grid-private.h"
#include "panel-grid-column-private.h"
#include "panel-joined-menu-private.h"
#include "panel-paned-private.h"
#include "panel-save-delegate.h"
#include "panel-save-dialog.h"
#include "panel-scaler-private.h"
#include "panel-widget-private.h"

struct _PanelFrame
{
  GtkWidget         parent_instance;

  PanelFrameHeader *header;
  GtkWidget        *box;
  AdwTabView       *tab_view;
  GtkWidget        *placeholder;
  GtkStack         *stack;
  GMenuModel       *frame_menu;
  GtkOverlay       *overlay;
  GtkWidget        *focus_highlight;

  guint             closeable : 1;
};

#define SIZE_AT_END 50

static void buildable_iface_init (GtkBuildableIface *iface);

G_DEFINE_TYPE_WITH_CODE (PanelFrame, panel_frame, GTK_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_BUILDABLE, buildable_iface_init)
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_ORIENTABLE, NULL))

enum {
  PROP_0,
  PROP_EMPTY,
  PROP_PLACEHOLDER,
  PROP_VISIBLE_CHILD,
  N_PROPS,

  PROP_ORIENTATION,
};

static GParamSpec *properties [N_PROPS];
static GtkBuildableIface *parent_buildable;

GtkWidget *
panel_frame_new (void)
{
  return g_object_new (PANEL_TYPE_FRAME, NULL);
}

static GtkWidget *
create_frame (PanelFrame *self)
{
  GtkWidget *grid;

  g_assert (PANEL_IS_FRAME (self));

  if ((grid = gtk_widget_get_ancestor (GTK_WIDGET (self), PANEL_TYPE_GRID)))
    return GTK_WIDGET (_panel_grid_create_frame (PANEL_GRID (grid)));
  else
    return panel_frame_new ();
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

  if (!panel_widget_get_reorderable (panel) ||
      (self->header && !panel_frame_header_can_drop (self->header, panel)))
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
      !(frame = gtk_widget_get_ancestor (GTK_WIDGET (panel), PANEL_TYPE_FRAME)) ||
      !panel_widget_get_reorderable (panel) ||
      (self->header && !panel_frame_header_can_drop (self->header, panel)))
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

      new_frame = create_frame (self);
      gtk_orientable_set_orientation (GTK_ORIENTABLE (new_frame), orientation);
      panel_paned_insert_after (PANEL_PANED (paned), new_frame, GTK_WIDGET (self));
      target = PANEL_FRAME (new_frame);
    }

  g_object_ref (panel);

  panel_frame_remove (PANEL_FRAME (frame), panel);
  panel_frame_add (target, panel);
  panel_frame_set_visible_child (target, panel);

  if (panel_frame_get_empty (PANEL_FRAME (frame)) &&
      panel_paned_get_n_children (PANEL_PANED (src_paned)) > 1)
    panel_paned_remove (PANEL_PANED (src_paned), frame);

  g_object_unref (panel);

  return TRUE;
}

static void
page_save_action (GtkWidget  *widget,
                  const char *action_name,
                  GVariant   *param)
{
  PanelFrame *self = (PanelFrame *)widget;
  PanelWidget *visible_child;
  PanelSaveDelegate *save_delegate;

  g_assert (PANEL_IS_FRAME (self));

  if (!(visible_child = panel_frame_get_visible_child (self)) ||
      !_panel_widget_can_save (visible_child) ||
      !(save_delegate = panel_widget_get_save_delegate (visible_child)))
    g_return_if_reached ();

}

static void
close_page_or_frame_action (GtkWidget  *widget,
                            const char *action_name,
                            GVariant   *param)
{
  PanelFrame *self = (PanelFrame *)widget;
  PanelWidget *visible_child;
  GtkWidget *grid;

  g_assert (PANEL_IS_FRAME (self));

  if (!(grid = gtk_widget_get_ancestor (GTK_WIDGET (self), PANEL_TYPE_GRID)))
    return;

  if ((visible_child = panel_frame_get_visible_child (self)))
    {
      AdwTabPage *page;

      page = adw_tab_view_get_page (self->tab_view, GTK_WIDGET (visible_child));
      adw_tab_view_close_page (self->tab_view, page);
    }
  else if (self->closeable)
    {
      GtkWidget *dock = gtk_widget_get_ancestor (grid, PANEL_TYPE_DOCK);

      _panel_dock_remove_frame (PANEL_DOCK (dock), self);
    }
}

static void
panel_frame_save_cb (GObject      *object,
                     GAsyncResult *result,
                     gpointer      user_data)
{
  PanelSaveDialog *dialog = (PanelSaveDialog *)object;
  g_autoptr(PanelFrame) self = user_data;
  g_autoptr(GError) error = NULL;
  GtkWidget *dock;
  GtkWidget *grid;

  g_assert (PANEL_IS_SAVE_DIALOG (dialog));
  g_assert (G_IS_ASYNC_RESULT (result));
  g_assert (PANEL_IS_FRAME (self));

  if (!panel_save_dialog_run_finish (dialog, result, &error))
    {
      if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        g_warning ("%s", error->message);
      return;
    }

  if (!(grid = gtk_widget_get_ancestor (GTK_WIDGET (self), PANEL_TYPE_GRID)) ||
      !(dock = gtk_widget_get_ancestor (grid, PANEL_TYPE_DOCK)))
    g_return_if_reached ();

  _panel_dock_remove_frame (PANEL_DOCK (dock), self);
}

static void
close_frame_action (GtkWidget  *widget,
                    const char *action_name,
                    GVariant   *param)
{
  PanelFrame *self = (PanelFrame *)widget;
  GtkWidget *toplevel;
  GtkWidget *dialog;
  guint n_pages;

  g_assert (PANEL_IS_FRAME (self));

  if (!self->closeable)
    g_return_if_reached ();

  toplevel = gtk_widget_get_ancestor (widget, GTK_TYPE_WINDOW);

  dialog = panel_save_dialog_new ();
  gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (toplevel));
  gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);

  n_pages = panel_frame_get_n_pages (self);

  for (guint i = 0; i < n_pages; i++)
    {
      PanelWidget *page = panel_frame_get_page (self, i);

      if (_panel_widget_can_save (page))
        panel_save_dialog_add_delegate (PANEL_SAVE_DIALOG (dialog),
                                        panel_widget_get_save_delegate (page));
    }

  panel_save_dialog_run_async (PANEL_SAVE_DIALOG (dialog),
                               NULL,
                               panel_frame_save_cb,
                               g_object_ref (self));
}

static void
panel_frame_update_actions (PanelFrame *self)
{
  GtkWidget *grid;
  PanelWidget *visible_child;

  g_assert (PANEL_IS_FRAME (self));

  grid = gtk_widget_get_ancestor (GTK_WIDGET (self), PANEL_TYPE_GRID);
  visible_child = panel_frame_get_visible_child (self);

  gtk_widget_action_set_enabled (GTK_WIDGET (self), "page.move-right", grid  && visible_child);
  gtk_widget_action_set_enabled (GTK_WIDGET (self), "page.move-left", grid && visible_child);
  gtk_widget_action_set_enabled (GTK_WIDGET (self), "page.move-down", grid && visible_child);
  gtk_widget_action_set_enabled (GTK_WIDGET (self), "page.move-up", grid && visible_child);
  gtk_widget_action_set_enabled (GTK_WIDGET (self), "page.maximize",
                                 grid && visible_child && panel_widget_get_can_maximize (visible_child));
  gtk_widget_action_set_enabled (GTK_WIDGET (self), "page.save",
                                 visible_child && _panel_widget_can_save (visible_child));
  gtk_widget_action_set_enabled (GTK_WIDGET (self),
                                 "frame.close-page-or-frame",
                                 grid && (visible_child || self->closeable));
  gtk_widget_action_set_enabled (GTK_WIDGET (self),
                                 "frame.close",
                                 grid && self->closeable);
}

static void
panel_frame_notify_selected_page_cb (PanelFrame *self,
                                     GParamSpec *pspec,
                                     AdwTabView *tab_view)
{
  PanelWidget *visible_child;

  g_assert (PANEL_IS_FRAME (self));
  g_assert (ADW_IS_TAB_VIEW (tab_view));

  visible_child = panel_frame_get_visible_child (self);

  panel_frame_update_actions (self);

  if (self->header)
    panel_frame_header_page_changed (self->header, visible_child);

  if (self->placeholder && visible_child == NULL)
    gtk_stack_set_visible_child (self->stack, self->placeholder);
  else
    gtk_stack_set_visible_child (self->stack, GTK_WIDGET (self->tab_view));

  g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_VISIBLE_CHILD]);
}

static void
page_maximize_action (GtkWidget  *widget,
                      const char *action_name,
                      GVariant   *param)
{
  PanelWidget *visible_child;

  g_assert (PANEL_IS_FRAME (widget));

  if (!(visible_child = panel_frame_get_visible_child (PANEL_FRAME (widget))))
    g_return_if_reached ();

  panel_widget_maximize (visible_child);
}

static void
page_move_right_action (GtkWidget  *widget,
                        const char *action_name,
                        GVariant   *param)
{
  PanelWidget *visible_child;
  GtkWidget *grid;
  guint column;
  guint row;

  g_assert (PANEL_IS_FRAME (widget));

  if (!(visible_child = panel_frame_get_visible_child (PANEL_FRAME (widget))))
    g_return_if_reached ();

  if ((grid = gtk_widget_get_ancestor (widget, PANEL_TYPE_GRID)) &&
      _panel_grid_get_position (PANEL_GRID (grid), widget, &column, &row))
    _panel_grid_reposition (PANEL_GRID (grid),
                            GTK_WIDGET (visible_child),
                            column + 1,
                            row,
                            FALSE);
}

static void
page_move_left_action (GtkWidget  *widget,
                       const char *action_name,
                       GVariant   *param)
{
  PanelWidget *visible_child;
  GtkWidget *grid;
  guint column;
  guint row;

  g_assert (PANEL_IS_FRAME (widget));

  if (!(visible_child = panel_frame_get_visible_child (PANEL_FRAME (widget))))
    g_return_if_reached ();

  if ((grid = gtk_widget_get_ancestor (widget, PANEL_TYPE_GRID)) &&
      _panel_grid_get_position (PANEL_GRID (grid), widget, &column, &row))
    {
      if (column == 0)
        {
          _panel_grid_prepend_column (PANEL_GRID (grid));
          column = 1;
        }

      _panel_grid_reposition (PANEL_GRID (grid),
                              GTK_WIDGET (visible_child),
                              column - 1,
                              row,
                              FALSE);
    }
}

static void
page_move_down_action (GtkWidget  *widget,
                       const char *action_name,
                       GVariant   *param)
{
  PanelWidget *visible_child;
  GtkWidget *grid;
  guint column;
  guint row;

  g_assert (PANEL_IS_FRAME (widget));

  if (!(visible_child = panel_frame_get_visible_child (PANEL_FRAME (widget))))
    g_return_if_reached ();

  if ((grid = gtk_widget_get_ancestor (widget, PANEL_TYPE_GRID)) &&
      _panel_grid_get_position (PANEL_GRID (grid), widget, &column, &row))
    _panel_grid_reposition (PANEL_GRID (grid),
                            GTK_WIDGET (visible_child),
                            column,
                            row + 1,
                            TRUE);
}

static void
page_move_up_action (GtkWidget  *widget,
                     const char *action_name,
                     GVariant   *param)
{
  PanelWidget *visible_child;
  GtkWidget *grid;
  GtkWidget *grid_column;
  guint column;
  guint row;

  g_assert (PANEL_IS_FRAME (widget));

  if (!(visible_child = panel_frame_get_visible_child (PANEL_FRAME (widget))))
    g_return_if_reached ();

  if ((grid_column = gtk_widget_get_ancestor (widget, PANEL_TYPE_GRID_COLUMN)) &&
      (grid = gtk_widget_get_ancestor (grid_column, PANEL_TYPE_GRID)) &&
      _panel_grid_get_position (PANEL_GRID (grid), widget, &column, &row))
    {
      if (row == 0)
        {
          _panel_grid_column_prepend_frame (PANEL_GRID_COLUMN (grid_column));
          row++;
        }

      _panel_grid_reposition (PANEL_GRID (grid),
                              GTK_WIDGET (visible_child),
                              column,
                              row - 1,
                              TRUE);
    }
}

static void
panel_frame_root (GtkWidget *widget)
{
  g_assert (PANEL_IS_FRAME (widget));

  GTK_WIDGET_CLASS (panel_frame_parent_class)->root (widget);

  panel_frame_update_actions (PANEL_FRAME (widget));
}

static void
panel_frame_unroot (GtkWidget *widget)
{
  GtkWidget *grid;

  g_assert (PANEL_IS_FRAME (widget));

  if ((grid = gtk_widget_get_ancestor (widget, PANEL_TYPE_GRID)))
    _panel_grid_drop_frame_mru (PANEL_GRID (grid), PANEL_FRAME (widget));

  GTK_WIDGET_CLASS (panel_frame_parent_class)->unroot (widget);

  panel_frame_update_actions (PANEL_FRAME (widget));
}

static void
setup_menu_cb (AdwTabView *tab_view,
               AdwTabPage *page)
{
  GMenuModel *menu_model = NULL;
  PanelJoinedMenu *joined;

  g_assert (ADW_IS_TAB_VIEW (tab_view));
  g_assert (!page || ADW_IS_TAB_PAGE (page));

  joined = PANEL_JOINED_MENU (adw_tab_view_get_menu_model (tab_view));

  /* First remove everything but the last menu (which is our frame menu) */
  while (panel_joined_menu_get_n_joined (joined) > 1)
    panel_joined_menu_remove_index (joined, 0);

  if (page != NULL)
    {
      GtkWidget *child = adw_tab_page_get_child (page);

      if (PANEL_IS_WIDGET (child))
        menu_model = panel_widget_get_menu_model (PANEL_WIDGET (child));
    }

  if (menu_model)
    panel_joined_menu_prepend_menu (joined, menu_model);
}

static void
panel_frame_dispose (GObject *object)
{
  PanelFrame *self = (PanelFrame *)object;

  panel_frame_set_header (self, NULL);
  panel_frame_set_placeholder (self, NULL);

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
    case PROP_VISIBLE_CHILD:
      g_value_set_object (value, panel_frame_get_visible_child (self));
      break;

    case PROP_EMPTY:
      g_value_set_boolean (value, panel_frame_get_empty (self));
      break;

    case PROP_ORIENTATION:
      g_value_set_enum (value, gtk_orientable_get_orientation (GTK_ORIENTABLE (self->box)));
      break;

    case PROP_PLACEHOLDER:
      g_value_set_object (value, panel_frame_get_placeholder (self));
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
    case PROP_VISIBLE_CHILD:
      panel_frame_set_visible_child (self, g_value_get_object (value));
      break;

    case PROP_ORIENTATION:
      gtk_orientable_set_orientation (GTK_ORIENTABLE (self->box), g_value_get_enum (value));
      if (GTK_IS_ORIENTABLE (self->header))
        gtk_orientable_set_orientation (GTK_ORIENTABLE (self->header), !g_value_get_enum (value));

      if (g_value_get_enum (value) == GTK_ORIENTATION_HORIZONTAL)
        {
          gtk_widget_set_size_request (self->focus_highlight, -1, 2);
          gtk_widget_set_halign (self->focus_highlight, GTK_ALIGN_FILL);
          gtk_widget_set_valign (self->focus_highlight, GTK_ALIGN_START);
        }
      else
        {
          gtk_widget_set_size_request (self->focus_highlight, 2, -1);
          gtk_widget_set_halign (self->focus_highlight, GTK_ALIGN_START);
          gtk_widget_set_valign (self->focus_highlight, GTK_ALIGN_FILL);
        }
      break;

    case PROP_PLACEHOLDER:
      panel_frame_set_placeholder (self, g_value_get_object (value));
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

  widget_class->root = panel_frame_root;
  widget_class->unroot = panel_frame_unroot;

  properties [PROP_EMPTY] =
    g_param_spec_boolean ("empty",
                          "Empty",
                          "If there are any panels added",
                          TRUE,
                          (G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  properties [PROP_PLACEHOLDER] =
    g_param_spec_object ("placeholder",
                         "Placeholder",
                         "Placeholder",
                         PANEL_TYPE_WIDGET,
                         (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  properties [PROP_VISIBLE_CHILD] =
    g_param_spec_object ("visible-child",
                         "Visible Child",
                         "Visible Child",
                         PANEL_TYPE_WIDGET,
                         (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);

  g_object_class_override_property (object_class, PROP_ORIENTATION, "orientation");

  gtk_widget_class_set_css_name (widget_class, "panelframe");
  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BIN_LAYOUT);
  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/libpanel/panel-frame.ui");
  gtk_widget_class_bind_template_child (widget_class, PanelFrame, box);
  gtk_widget_class_bind_template_child (widget_class, PanelFrame, focus_highlight);
  gtk_widget_class_bind_template_child (widget_class, PanelFrame, overlay);
  gtk_widget_class_bind_template_child (widget_class, PanelFrame, stack);
  gtk_widget_class_bind_template_child (widget_class, PanelFrame, tab_view);
  gtk_widget_class_bind_template_child (widget_class, PanelFrame, frame_menu);
  gtk_widget_class_bind_template_callback (widget_class, setup_menu_cb);

  gtk_widget_class_install_action (widget_class, "page.move-right", NULL, page_move_right_action);
  gtk_widget_class_install_action (widget_class, "page.move-left", NULL, page_move_left_action);
  gtk_widget_class_install_action (widget_class, "page.move-down", NULL, page_move_down_action);
  gtk_widget_class_install_action (widget_class, "page.move-up", NULL, page_move_up_action);
  gtk_widget_class_install_action (widget_class, "page.maximize", NULL, page_maximize_action);
  gtk_widget_class_install_action (widget_class, "page.save", NULL, page_save_action);
  gtk_widget_class_install_action (widget_class, "frame.close-page-or-frame", NULL, close_page_or_frame_action);
  gtk_widget_class_install_action (widget_class, "frame.close", NULL, close_frame_action);

  gtk_widget_class_add_binding_action (widget_class, GDK_KEY_braceright, GDK_CONTROL_MASK | GDK_SHIFT_MASK, "page.move-right", NULL);
  gtk_widget_class_add_binding_action (widget_class, GDK_KEY_braceleft, GDK_CONTROL_MASK | GDK_SHIFT_MASK, "page.move-left", NULL);
  gtk_widget_class_add_binding_action (widget_class, GDK_KEY_F11, GDK_SHIFT_MASK, "page.maximize", NULL);

  g_type_ensure (ADW_TYPE_TAB_VIEW);
}

static void
panel_frame_init (PanelFrame *self)
{
  PanelJoinedMenu *menu;
  GtkDropTarget *drop_target;
  GType types[] = { PANEL_TYPE_WIDGET };

  gtk_widget_init_template (GTK_WIDGET (self));

  menu = panel_joined_menu_new ();
  adw_tab_view_set_menu_model (self->tab_view, G_MENU_MODEL (menu));
  panel_joined_menu_append_menu (menu, self->frame_menu);
  g_clear_object (&menu);

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

  g_signal_connect_object (self->tab_view,
                           "notify::selected-page",
                           G_CALLBACK (panel_frame_notify_selected_page_cb),
                           self,
                           G_CONNECT_AFTER | G_CONNECT_SWAPPED);

  panel_frame_set_header (self, PANEL_FRAME_HEADER (panel_frame_switcher_new ()));

  panel_frame_update_actions (self);
}

void
panel_frame_add (PanelFrame  *self,
                 PanelWidget *panel)
{
  AdwTabPage *page;
  GtkWidget *grid;
  gboolean empty;

  g_return_if_fail (PANEL_IS_FRAME (self));
  g_return_if_fail (PANEL_IS_WIDGET (panel));

  empty = panel_frame_get_empty (self);
  page = adw_tab_view_add_page (self->tab_view, GTK_WIDGET (panel), NULL);

  g_object_bind_property (panel, "title", page, "title", G_BINDING_SYNC_CREATE);
  g_object_bind_property (panel, "icon", page, "icon", G_BINDING_SYNC_CREATE);
  g_object_bind_property (panel, "needs-attention", page, "needs-attention", G_BINDING_SYNC_CREATE);
  g_object_bind_property (panel, "busy", page, "loading", G_BINDING_SYNC_CREATE);

  g_assert (!panel_frame_get_empty (self));

  if ((grid = gtk_widget_get_ancestor (GTK_WIDGET (self), PANEL_TYPE_GRID)))
    _panel_grid_update_closeable (PANEL_GRID (grid));

  panel_frame_update_actions (self);

  if (empty)
    g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_EMPTY]);
}

void
panel_frame_remove (PanelFrame  *self,
                    PanelWidget *panel)
{
  GtkWidget *dock_child;
  GtkWidget *grid;
  AdwTabPage *page;

  g_return_if_fail (PANEL_IS_FRAME (self));
  g_return_if_fail (PANEL_IS_WIDGET (panel));

  page = adw_tab_view_get_page (self->tab_view, GTK_WIDGET (panel));
  adw_tab_view_close_page (self->tab_view, page);

  if (panel_frame_get_empty (self))
    {
      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_EMPTY]);

      if ((dock_child = gtk_widget_get_ancestor (GTK_WIDGET (self), PANEL_TYPE_DOCK_CHILD)))
        {
          if (gtk_widget_get_first_child (dock_child) == gtk_widget_get_last_child (dock_child))
            g_object_notify (G_OBJECT (dock_child), "empty");
        }
    }

  if ((grid = gtk_widget_get_ancestor (GTK_WIDGET (self), PANEL_TYPE_GRID)))
    _panel_grid_update_closeable (PANEL_GRID (grid));

  panel_frame_update_actions (self);
}

gboolean
panel_frame_get_empty (PanelFrame *self)
{
  g_return_val_if_fail (PANEL_IS_FRAME (self), FALSE);

  return adw_tab_view_get_selected_page (self->tab_view) == NULL;
}

PanelWidget *
panel_frame_get_visible_child (PanelFrame *self)
{
  AdwTabPage *page;

  g_return_val_if_fail (PANEL_IS_FRAME (self), NULL);

  page = adw_tab_view_get_selected_page (self->tab_view);

  return page ? PANEL_WIDGET (adw_tab_page_get_child (page)) : NULL;
}

void
panel_frame_set_visible_child (PanelFrame  *self,
                               PanelWidget *widget)
{
  AdwTabPage *page;

  g_return_if_fail (PANEL_IS_FRAME (self));
  g_return_if_fail (PANEL_IS_WIDGET (widget));

  if ((page = adw_tab_view_get_page (self->tab_view, GTK_WIDGET (widget))))
    adw_tab_view_set_selected_page (self->tab_view, page);
}

AdwTabView *
_panel_frame_get_tab_view (PanelFrame *self)
{
  g_return_val_if_fail (PANEL_IS_FRAME (self), NULL);

  return self->tab_view;
}

/**
 * panel_frame_get_header:
 * @self: a #PanelFrame
 *
 * Gets the header for the frame.
 *
 * Returns: (transfer none): a #PanelFrameHeader
 */
PanelFrameHeader *
panel_frame_get_header (PanelFrame *self)
{
  g_return_val_if_fail (PANEL_IS_FRAME (self), NULL);
  g_return_val_if_fail (PANEL_IS_FRAME_HEADER (self->header), NULL);

  return self->header;
}

/**
 * panel_frame_set_header:
 * @self: a #PanelFrame
 * @header: a #PanelFrameHeader
 *
 * Sets the header for the frame, such as a #PanelFrameSwitcher.
 */
void
panel_frame_set_header (PanelFrame       *self,
                        PanelFrameHeader *header)
{
  g_return_if_fail (PANEL_IS_FRAME (self));
  g_return_if_fail (!header || PANEL_IS_FRAME_HEADER (header));

  if (self->header == header)
    return;

  if (self->header != NULL)
    {
      panel_frame_header_page_changed (self->header, NULL);
      panel_frame_header_set_frame (self->header, NULL);
      gtk_overlay_set_child (self->overlay, NULL);
      self->header = NULL;
    }

  self->header = header;

  if (self->header != NULL)
    {
      PanelWidget *visible_child = panel_frame_get_visible_child (self);

      if (GTK_IS_ORIENTABLE (self->header))
        gtk_orientable_set_orientation (GTK_ORIENTABLE (self->header),
                                        !gtk_orientable_get_orientation (GTK_ORIENTABLE (self->box)));
      gtk_overlay_set_child (self->overlay, GTK_WIDGET (self->header));

      panel_frame_header_set_frame (self->header, self);

      if (visible_child)
        panel_frame_header_page_changed (self->header, visible_child);

      gtk_widget_add_css_class (GTK_WIDGET (self->header), "header");
    }
}

GtkSelectionModel *
panel_frame_get_pages (PanelFrame *self)
{
  g_return_val_if_fail (PANEL_IS_FRAME (self), NULL);

  return adw_tab_view_get_pages (self->tab_view);
}

void
_panel_frame_transfer (PanelFrame  *self,
                       PanelWidget *widget,
                       PanelFrame  *new_frame,
                       int          position)
{
  AdwTabPage *page;
  GtkWidget *grid;
  GtkWidget *window;

  g_return_if_fail (PANEL_IS_FRAME (self));
  g_return_if_fail (PANEL_IS_WIDGET (widget));
  g_return_if_fail (PANEL_IS_FRAME (new_frame));

  /* First clear focus so that we ensure updating current frame */
  if ((window = gtk_widget_get_ancestor (GTK_WIDGET (self), GTK_TYPE_WINDOW)))
    gtk_window_set_focus (GTK_WINDOW (window), NULL);

  if (!(page = adw_tab_view_get_page (self->tab_view, GTK_WIDGET (widget))))
    g_return_if_reached ();

  if (position < 0)
    position = adw_tab_view_get_n_pages (new_frame->tab_view);

  adw_tab_view_transfer_page (self->tab_view, page, new_frame->tab_view, position);

  if ((grid = gtk_widget_get_ancestor (GTK_WIDGET (self), PANEL_TYPE_GRID)))
    _panel_grid_update_closeable (PANEL_GRID (grid));

  panel_frame_update_actions (self);

  panel_widget_raise (widget);
  panel_widget_focus_default (widget);

  if (grid)
    _panel_grid_update_focus (PANEL_GRID (grid));
}

guint
panel_frame_get_n_pages (PanelFrame *self)
{
  g_return_val_if_fail (PANEL_IS_FRAME (self), 0);

  return adw_tab_view_get_n_pages (self->tab_view);
}

PanelWidget *
panel_frame_get_page (PanelFrame *self,
                      guint       n)
{
  AdwTabPage *page;

  g_return_val_if_fail (PANEL_IS_FRAME (self), NULL);
  g_return_val_if_fail (n < panel_frame_get_n_pages (self), NULL);

  if ((page = adw_tab_view_get_nth_page (self->tab_view, n)))
    return PANEL_WIDGET (adw_tab_page_get_child (page));

  return NULL;
}

/**
 * panel_frame_get_placeholder:
 * @self: a #PanelFrame
 *
 * Gets the placeholder widget, if any.
 *
 * Returns: (nullable) (transfer none): a #GtkWidget or %NULL
 */
GtkWidget *
panel_frame_get_placeholder (PanelFrame *self)
{
  g_return_val_if_fail (PANEL_IS_FRAME (self), NULL);

  return self->placeholder;
}

/**
 * panel_frame_set_placeholder:
 * @self: a #PanelFrame
 * @placeholder: (nullable): a #GtkWidget or %NULL
 *
 * Sets the placeholder widget for the frame.
 *
 * The placeholder widget is displayed when there are no pages
 * to display in the frame.
 */
void
panel_frame_set_placeholder (PanelFrame *self,
                             GtkWidget  *placeholder)
{
  g_return_if_fail (PANEL_IS_FRAME (self));
  g_return_if_fail (!placeholder || GTK_IS_WIDGET (placeholder));

  if (self->placeholder == placeholder)
    return;

  if (self->placeholder)
    gtk_stack_remove (self->stack, self->placeholder);

  self->placeholder = placeholder;

  if (self->placeholder)
    gtk_stack_add_named (self->stack, self->placeholder, "placeholder");

  if (self->placeholder && !panel_frame_get_visible_child (self))
    gtk_stack_set_visible_child (self->stack, self->placeholder);
  else
    gtk_stack_set_visible_child (self->stack, GTK_WIDGET (self->tab_view));

  g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_PLACEHOLDER]);
}

static void
panel_frame_add_child (GtkBuildable *buildable,
                       GtkBuilder   *builder,
                       GObject      *child,
                       const char   *type)
{
  if (PANEL_IS_WIDGET (child))
    panel_frame_add (PANEL_FRAME (buildable), PANEL_WIDGET (child));
  else
    parent_buildable->add_child (buildable, builder, child, type);
}

static void
buildable_iface_init (GtkBuildableIface *iface)
{
  parent_buildable = g_type_interface_peek_parent (iface);
  iface->add_child = panel_frame_add_child;
}

GMenuModel *
_panel_frame_get_tab_menu (PanelFrame *self)
{
  AdwTabPage *page;

  g_return_val_if_fail (PANEL_IS_FRAME (self), NULL);

  page = adw_tab_view_get_selected_page (self->tab_view);
  g_signal_emit_by_name (self->tab_view, "setup-menu", page);
  return adw_tab_view_get_menu_model (self->tab_view);
}

void
_panel_frame_set_closeable (PanelFrame  *self,
                            gboolean     closeable)
{
  g_return_if_fail (PANEL_IS_FRAME (self));

  self->closeable = !!closeable;

  panel_frame_update_actions (self);
}
