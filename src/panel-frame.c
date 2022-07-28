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
#include "panel-drop-controls-private.h"
#include "panel-frame-private.h"
#include "panel-frame-header.h"
#include "panel-frame-switcher-private.h"
#include "panel-grid-private.h"
#include "panel-grid-column-private.h"
#include "panel-joined-menu-private.h"
#include "panel-paned.h"
#include "panel-save-delegate.h"
#include "panel-save-dialog.h"
#include "panel-scaler-private.h"
#include "panel-widget-private.h"

typedef struct
{
  PanelFrameHeader  *header;
  GtkWidget         *box;
  AdwTabView        *tab_view;
  GtkWidget         *placeholder;
  GtkStack          *stack;
  GMenuModel        *frame_menu;
  GtkOverlay        *overlay;
  GtkOverlay        *controls_overlay;
  GtkWidget         *focus_highlight;
  PanelDropControls *drop_controls;

  guint              closeable : 1;
} PanelFramePrivate;

#define SIZE_AT_END 50

static void buildable_iface_init (GtkBuildableIface *iface);

G_DEFINE_TYPE_WITH_CODE (PanelFrame, panel_frame, GTK_TYPE_WIDGET,
                         G_ADD_PRIVATE (PanelFrame)
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_BUILDABLE, buildable_iface_init)
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_ORIENTABLE, NULL))

enum {
  PROP_0,
  PROP_CLOSEABLE,
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

static void
close_page_or_frame_action (GtkWidget  *widget,
                            const char *action_name,
                            GVariant   *param)
{
  PanelFrame *self = (PanelFrame *)widget;
  PanelFramePrivate *priv = panel_frame_get_instance_private (self);
  PanelWidget *visible_child;
  GtkWidget *grid;

  g_assert (PANEL_IS_FRAME (self));

  if (!(grid = gtk_widget_get_ancestor (GTK_WIDGET (self), PANEL_TYPE_GRID)))
    return;

  if ((visible_child = panel_frame_get_visible_child (self)))
    {
      _panel_frame_request_close (self, visible_child);
    }
  else if (priv->closeable)
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
  PanelFramePrivate *priv = panel_frame_get_instance_private (self);
  GtkWidget *toplevel;
  GtkWidget *dialog;
  guint n_pages;

  g_assert (PANEL_IS_FRAME (self));

  if (!priv->closeable)
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
frame_page_action (GtkWidget  *widget,
                   const char *action_name,
                   GVariant   *param)
{
  PanelFrame *self = (PanelFrame *)widget;
  PanelFramePrivate *priv = panel_frame_get_instance_private (self);
  int n_pages;
  int n;

  g_assert (PANEL_IS_FRAME (self));
  g_assert (g_variant_is_of_type (param, G_VARIANT_TYPE_INT32));

  n = g_variant_get_int32 (param);
  n_pages = panel_frame_get_n_pages (self);

  if (n == -1)
    {
      adw_tab_view_select_previous_page (priv->tab_view);
    }
  else if (n == 0)
    {
      adw_tab_view_select_next_page (priv->tab_view);
    }
  else if (n > 0 && n <= n_pages)
    {
      AdwTabPage *page = adw_tab_view_get_nth_page (priv->tab_view, n-1);

      if (page != NULL)
        adw_tab_view_set_selected_page (priv->tab_view, page);
    }
}

static void
panel_frame_update_actions (PanelFrame *self)
{
  PanelFramePrivate *priv = panel_frame_get_instance_private (self);
  PanelActionMuxer *action_group = NULL;
  PanelWidget *visible_child;
  GtkWidget *grid;

  g_assert (PANEL_IS_FRAME (self));

  grid = gtk_widget_get_ancestor (GTK_WIDGET (self), PANEL_TYPE_GRID);
  visible_child = panel_frame_get_visible_child (self);

  if (visible_child != NULL)
    action_group = _panel_widget_get_action_muxer (visible_child);
  gtk_widget_insert_action_group (GTK_WIDGET (self),
                                  "page",
                                  G_ACTION_GROUP (action_group));

  gtk_widget_action_set_enabled (GTK_WIDGET (self), "page.move-right", grid  && visible_child);
  gtk_widget_action_set_enabled (GTK_WIDGET (self), "page.move-left", grid && visible_child);
  gtk_widget_action_set_enabled (GTK_WIDGET (self), "page.move-down", grid && visible_child);
  gtk_widget_action_set_enabled (GTK_WIDGET (self), "page.move-up", grid && visible_child);
  gtk_widget_action_set_enabled (GTK_WIDGET (self),
                                 "frame.close-page-or-frame",
                                 grid && (visible_child || priv->closeable));
  gtk_widget_action_set_enabled (GTK_WIDGET (self),
                                 "frame.close",
                                 grid && priv->closeable);
}

static void
panel_frame_notify_selected_page_cb (PanelFrame *self,
                                     GParamSpec *pspec,
                                     AdwTabView *tab_view)
{
  PanelFramePrivate *priv = panel_frame_get_instance_private (self);
  PanelWidget *visible_child;

  g_assert (PANEL_IS_FRAME (self));
  g_assert (ADW_IS_TAB_VIEW (tab_view));

  visible_child = panel_frame_get_visible_child (self);

  panel_frame_update_actions (self);

  if (priv->header)
    panel_frame_header_page_changed (priv->header, visible_child);

  if (priv->placeholder && visible_child == NULL)
    gtk_stack_set_visible_child (priv->stack, priv->placeholder);
  else
    gtk_stack_set_visible_child (priv->stack, GTK_WIDGET (priv->tab_view));

  if (visible_child)
    _panel_widget_emit_presented (visible_child);

  g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_VISIBLE_CHILD]);
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
    {
      _panel_grid_reposition (PANEL_GRID (grid),
                              GTK_WIDGET (visible_child),
                              column + 1,
                              row,
                              FALSE);
      panel_widget_raise (visible_child);
      gtk_widget_grab_focus (GTK_WIDGET (visible_child));
    }
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
      panel_widget_raise (visible_child);
      gtk_widget_grab_focus (GTK_WIDGET (visible_child));
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
    {
      _panel_grid_reposition (PANEL_GRID (grid),
                              GTK_WIDGET (visible_child),
                              column,
                              row + 1,
                              TRUE);
      panel_widget_raise (visible_child);
      gtk_widget_grab_focus (GTK_WIDGET (visible_child));
    }
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
      panel_widget_raise (visible_child);
      gtk_widget_grab_focus (GTK_WIDGET (visible_child));
    }
}

static void
panel_frame_update_drop (PanelFrame *self)
{
  PanelFramePrivate *priv = panel_frame_get_instance_private (self);
  PanelDockPosition position;
  GtkWidget *grid;
  GtkWidget *child;

  g_assert (PANEL_IS_FRAME (self));

  if ((grid = gtk_widget_get_ancestor (GTK_WIDGET (self), PANEL_TYPE_GRID)))
    position = PANEL_DOCK_POSITION_CENTER;
  else if ((child = gtk_widget_get_ancestor (GTK_WIDGET (self), PANEL_TYPE_DOCK_CHILD)))
    position = panel_dock_child_get_position (PANEL_DOCK_CHILD (child));
  else
    position = PANEL_DOCK_POSITION_CENTER;

  panel_drop_controls_set_position (priv->drop_controls, position);
}

static void
on_panel_drag_begin_cb (PanelFrame  *self,
                        PanelWidget *widget,
                        PanelDock   *dock)
{
  PanelFramePrivate *priv = panel_frame_get_instance_private (self);

  g_assert (PANEL_IS_FRAME (self));
  g_assert (PANEL_IS_WIDGET (widget));
  g_assert (PANEL_IS_DOCK (dock));

  if (priv->header != NULL &&
      panel_frame_header_can_drop (priv->header, widget))
    gtk_widget_show (GTK_WIDGET (priv->drop_controls));
}

static void
on_panel_drag_end_cb (PanelFrame  *self,
                      PanelWidget *widget,
                      PanelDock   *dock)
{
  PanelFramePrivate *priv = panel_frame_get_instance_private (self);

  g_assert (PANEL_IS_FRAME (self));
  g_assert (PANEL_IS_WIDGET (widget));
  g_assert (PANEL_IS_DOCK (dock));

  gtk_widget_hide (GTK_WIDGET (priv->drop_controls));
}

static void
panel_frame_root (GtkWidget *widget)
{
  GtkWidget *dock;

  g_assert (PANEL_IS_FRAME (widget));

  GTK_WIDGET_CLASS (panel_frame_parent_class)->root (widget);

  if ((dock = gtk_widget_get_ancestor (widget, PANEL_TYPE_DOCK)))
    {
      g_signal_connect_object (dock,
                               "panel-drag-begin",
                               G_CALLBACK (on_panel_drag_begin_cb),
                               widget,
                               G_CONNECT_SWAPPED);
      g_signal_connect_object (dock,
                               "panel-drag-end",
                               G_CALLBACK (on_panel_drag_end_cb),
                               widget,
                               G_CONNECT_SWAPPED);
    }

  panel_frame_update_actions (PANEL_FRAME (widget));
  panel_frame_update_drop (PANEL_FRAME (widget));
}

static void
panel_frame_unroot (GtkWidget *widget)
{
  GtkWidget *grid;
  GtkWidget *dock;

  g_assert (PANEL_IS_FRAME (widget));

  if ((dock = gtk_widget_get_ancestor (widget, PANEL_TYPE_DOCK)))
    {
      g_signal_handlers_disconnect_by_func (dock,
                                            G_CALLBACK (on_panel_drag_begin_cb),
                                            widget);
      g_signal_handlers_disconnect_by_func (dock,
                                            G_CALLBACK (on_panel_drag_end_cb),
                                            widget);
    }

  if ((grid = gtk_widget_get_ancestor (widget, PANEL_TYPE_GRID)))
    _panel_grid_drop_frame_mru (PANEL_GRID (grid), PANEL_FRAME (widget));

  GTK_WIDGET_CLASS (panel_frame_parent_class)->unroot (widget);

  panel_frame_update_actions (PANEL_FRAME (widget));
  panel_frame_update_drop (PANEL_FRAME (widget));
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
panel_frame_compute_expand (GtkWidget *widget,
                            gboolean  *hexpand,
                            gboolean  *vexpand)
{
  g_assert (PANEL_IS_FRAME (widget));

  if (gtk_orientable_get_orientation (GTK_ORIENTABLE (widget)) == GTK_ORIENTATION_HORIZONTAL)
    {
      *hexpand = TRUE;
      *vexpand = FALSE;
    }
  else
    {
      *hexpand = FALSE;
      *vexpand = TRUE;
    }
}

static void
panel_frame_dispose (GObject *object)
{
  PanelFrame *self = (PanelFrame *)object;
  PanelFramePrivate *priv = panel_frame_get_instance_private (self);

  panel_frame_set_header (self, NULL);
  panel_frame_set_placeholder (self, NULL);

  g_clear_pointer ((GtkWidget **)&priv->controls_overlay, gtk_widget_unparent);

  G_OBJECT_CLASS (panel_frame_parent_class)->dispose (object);
}

static void
panel_frame_get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  PanelFrame *self = PANEL_FRAME (object);
  PanelFramePrivate *priv = panel_frame_get_instance_private (self);

  switch (prop_id)
    {
    case PROP_VISIBLE_CHILD:
      g_value_set_object (value, panel_frame_get_visible_child (self));
      break;

    case PROP_CLOSEABLE:
      g_value_set_boolean (value, priv->closeable);
      break;

    case PROP_EMPTY:
      g_value_set_boolean (value, panel_frame_get_empty (self));
      break;

    case PROP_ORIENTATION:
      g_value_set_enum (value, gtk_orientable_get_orientation (GTK_ORIENTABLE (priv->box)));
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
  PanelFramePrivate *priv = panel_frame_get_instance_private (self);

  switch (prop_id)
    {
    case PROP_VISIBLE_CHILD:
      panel_frame_set_visible_child (self, g_value_get_object (value));
      break;

    case PROP_ORIENTATION:
      gtk_orientable_set_orientation (GTK_ORIENTABLE (priv->box), g_value_get_enum (value));
      if (GTK_IS_ORIENTABLE (priv->header))
        gtk_orientable_set_orientation (GTK_ORIENTABLE (priv->header), !g_value_get_enum (value));

      if (g_value_get_enum (value) == GTK_ORIENTATION_HORIZONTAL)
        {
          gtk_widget_set_size_request (priv->focus_highlight, -1, 2);
          gtk_widget_set_halign (priv->focus_highlight, GTK_ALIGN_FILL);
          gtk_widget_set_valign (priv->focus_highlight, GTK_ALIGN_START);
        }
      else
        {
          gtk_widget_set_size_request (priv->focus_highlight, 2, -1);
          gtk_widget_set_halign (priv->focus_highlight, GTK_ALIGN_START);
          gtk_widget_set_valign (priv->focus_highlight, GTK_ALIGN_FILL);
        }

      _panel_dock_update_orientation (GTK_WIDGET (self), g_value_get_enum (value));
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
  widget_class->compute_expand = panel_frame_compute_expand;

  properties [PROP_CLOSEABLE] =
    g_param_spec_boolean ("closeable",
                          "Closeable",
                          "If the frame may be closed",
                          FALSE,
                          (G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

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
                         GTK_TYPE_WIDGET,
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
  gtk_widget_class_bind_template_child_private (widget_class, PanelFrame, box);
  gtk_widget_class_bind_template_child_private (widget_class, PanelFrame, focus_highlight);
  gtk_widget_class_bind_template_child_private (widget_class, PanelFrame, overlay);
  gtk_widget_class_bind_template_child_private (widget_class, PanelFrame, stack);
  gtk_widget_class_bind_template_child_private (widget_class, PanelFrame, tab_view);
  gtk_widget_class_bind_template_child_private (widget_class, PanelFrame, frame_menu);
  gtk_widget_class_bind_template_child_private (widget_class, PanelFrame, drop_controls);
  gtk_widget_class_bind_template_child_private (widget_class, PanelFrame, controls_overlay);
  gtk_widget_class_bind_template_callback (widget_class, setup_menu_cb);

  gtk_widget_class_install_action (widget_class, "page.move-right", NULL, page_move_right_action);
  gtk_widget_class_install_action (widget_class, "page.move-left", NULL, page_move_left_action);
  gtk_widget_class_install_action (widget_class, "page.move-down", NULL, page_move_down_action);
  gtk_widget_class_install_action (widget_class, "page.move-up", NULL, page_move_up_action);
  gtk_widget_class_install_action (widget_class, "frame.close-page-or-frame", NULL, close_page_or_frame_action);
  gtk_widget_class_install_action (widget_class, "frame.close", NULL, close_frame_action);
  gtk_widget_class_install_action (widget_class, "frame.page", "i", frame_page_action);

  gtk_widget_class_add_binding_action (widget_class, GDK_KEY_braceright, GDK_CONTROL_MASK | GDK_SHIFT_MASK, "page.move-right", NULL);
  gtk_widget_class_add_binding_action (widget_class, GDK_KEY_braceleft, GDK_CONTROL_MASK | GDK_SHIFT_MASK, "page.move-left", NULL);

  g_type_ensure (ADW_TYPE_TAB_VIEW);
  g_type_ensure (PANEL_TYPE_DROP_CONTROLS);
}

static void
panel_frame_init (PanelFrame *self)
{
  PanelFramePrivate *priv = panel_frame_get_instance_private (self);
  PanelJoinedMenu *menu;

  gtk_widget_init_template (GTK_WIDGET (self));

  _panel_dock_update_orientation (GTK_WIDGET (self),
                                  gtk_orientable_get_orientation (GTK_ORIENTABLE (self)));

  /* Locate GtkStack within tab view and alter homogeneous
   * values so that we have more flexibility in sizing.
   */
  for (GtkWidget *child = gtk_widget_get_first_child (GTK_WIDGET (priv->tab_view));
       child;
       child = gtk_widget_get_next_sibling (GTK_WIDGET (child)))
    {
      if (GTK_IS_STACK (child))
        {
          gtk_stack_set_hhomogeneous (GTK_STACK (child), FALSE);
          gtk_stack_set_vhomogeneous (GTK_STACK (child), FALSE);
        }
    }

  menu = panel_joined_menu_new ();
  adw_tab_view_set_menu_model (priv->tab_view, G_MENU_MODEL (menu));
  panel_joined_menu_append_menu (menu, priv->frame_menu);
  g_clear_object (&menu);

  g_signal_connect_object (priv->tab_view,
                           "notify::selected-page",
                           G_CALLBACK (panel_frame_notify_selected_page_cb),
                           self,
                           G_CONNECT_AFTER | G_CONNECT_SWAPPED);

  panel_frame_set_header (self, PANEL_FRAME_HEADER (panel_frame_switcher_new ()));

  panel_frame_update_actions (self);
}

static gboolean
modified_to_indicator_icon (GBinding     *binding,
                            const GValue *from_value,
                            GValue       *to_value,
                            gpointer      user_data)
{
  static GIcon *icon;

  if (icon == NULL)
    icon = g_themed_icon_new ("panel-modified-symbolic");

  if (g_value_get_boolean (from_value))
    g_value_set_object (to_value, icon);
  else
    g_value_set_object (to_value, NULL);

  return TRUE;
}

void
panel_frame_add_before (PanelFrame  *self,
                        PanelWidget *panel,
                        PanelWidget *sibling)
{
  PanelFramePrivate *priv = panel_frame_get_instance_private (self);
  AdwTabPage *page;
  GtkWidget *dock = NULL;
  GtkWidget *dock_child;
  GtkWidget *grid;
  gboolean empty;
  int position;

  g_return_if_fail (PANEL_IS_FRAME (self));
  g_return_if_fail (PANEL_IS_WIDGET (panel));
  g_return_if_fail (!sibling || PANEL_IS_WIDGET (sibling));
  g_return_if_fail (!sibling || gtk_widget_get_ancestor (GTK_WIDGET (sibling), PANEL_TYPE_FRAME) == GTK_WIDGET (self));

  if ((dock_child = gtk_widget_get_ancestor (GTK_WIDGET (self), PANEL_TYPE_DOCK_CHILD)))
    dock = gtk_widget_get_ancestor (GTK_WIDGET (self), PANEL_TYPE_DOCK);

  if (sibling != NULL)
    {
      page = adw_tab_view_get_page (priv->tab_view, GTK_WIDGET (sibling));
      position = adw_tab_view_get_page_position (priv->tab_view, page);
    }
  else
    {
      position = adw_tab_view_get_n_pages (priv->tab_view);
    }

  empty = panel_frame_get_empty (self);
  page = adw_tab_view_insert (priv->tab_view, GTK_WIDGET (panel), position);

  g_object_bind_property (panel, "title", page, "title", G_BINDING_SYNC_CREATE);
  g_object_bind_property (panel, "icon", page, "icon", G_BINDING_SYNC_CREATE);
  g_object_bind_property (panel, "needs-attention", page, "needs-attention", G_BINDING_SYNC_CREATE);
  g_object_bind_property (panel, "busy", page, "loading", G_BINDING_SYNC_CREATE);
  g_object_bind_property_full (panel, "modified", page, "indicator-icon", G_BINDING_SYNC_CREATE,
                               modified_to_indicator_icon, NULL, NULL, NULL);

  g_assert (!panel_frame_get_empty (self));

  if ((grid = gtk_widget_get_ancestor (GTK_WIDGET (self), PANEL_TYPE_GRID)))
    _panel_grid_update_closeable (PANEL_GRID (grid));

  panel_frame_update_actions (self);

  if (empty)
    g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_EMPTY]);

  if (dock_child != NULL && dock != NULL)
    {
      PanelDockPosition dockpos = panel_dock_child_get_position (PANEL_DOCK_CHILD (dock_child));

      switch (dockpos)
        {
        case PANEL_DOCK_POSITION_START:
          g_object_notify (G_OBJECT (dock), "can-reveal-start");
          break;

        case PANEL_DOCK_POSITION_END:
          g_object_notify (G_OBJECT (dock), "can-reveal-end");
          break;

        case PANEL_DOCK_POSITION_TOP:
          g_object_notify (G_OBJECT (dock), "can-reveal-top");
          break;

        case PANEL_DOCK_POSITION_BOTTOM:
          g_object_notify (G_OBJECT (dock), "can-reveal-bottom");
          break;

        case PANEL_DOCK_POSITION_CENTER:
        default:
          break;
        }
    }
}

/**
 * panel_frame_add:
 * @self: a #PanelFrame
 * @panel: a widget to add
 *
 * Adds a widget to the frame.
 */
void
panel_frame_add (PanelFrame  *self,
                 PanelWidget *panel)
{
  panel_frame_add_before (self, panel, NULL);
}

/**
 * panel_frame_remove:
 * @self: a #PanelFrame
 * @panel: a widget to add
 *
 * Removes a widget from the frame.
 */
void
panel_frame_remove (PanelFrame  *self,
                    PanelWidget *panel)
{
  PanelFramePrivate *priv = panel_frame_get_instance_private (self);
  GtkWidget *dock_child;
  GtkWidget *grid;
  AdwTabPage *page;

  g_return_if_fail (PANEL_IS_FRAME (self));
  g_return_if_fail (PANEL_IS_WIDGET (panel));

  page = adw_tab_view_get_page (priv->tab_view, GTK_WIDGET (panel));
  adw_tab_view_close_page (priv->tab_view, page);

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
  PanelFramePrivate *priv = panel_frame_get_instance_private (self);

  g_return_val_if_fail (PANEL_IS_FRAME (self), FALSE);

  return adw_tab_view_get_n_pages (priv->tab_view) == 0;
}

/**
 * panel_frame_get_visible_child:
 * @self: a #PanelFrame
 *
 * Gets the widget of the currently visible child.
 *
 * Returns: (transfer none) (nullable): a #PanelWidget or %NULL
 */
PanelWidget *
panel_frame_get_visible_child (PanelFrame *self)
{
  PanelFramePrivate *priv = panel_frame_get_instance_private (self);
  AdwTabPage *page;

  g_return_val_if_fail (PANEL_IS_FRAME (self), NULL);

  page = adw_tab_view_get_selected_page (priv->tab_view);

  return page ? PANEL_WIDGET (adw_tab_page_get_child (page)) : NULL;
}

/**
 * panel_frame_set_visible_child:
 * @self: a #PanelFrame
 * @widget: (not nullable): a #PanelWidget
 *
 * Sets the current page to the child specified in @widget.
 */
void
panel_frame_set_visible_child (PanelFrame  *self,
                               PanelWidget *widget)
{
  PanelFramePrivate *priv = panel_frame_get_instance_private (self);
  AdwTabPage *page;

  g_return_if_fail (PANEL_IS_FRAME (self));
  g_return_if_fail (PANEL_IS_WIDGET (widget));

  if ((page = adw_tab_view_get_page (priv->tab_view, GTK_WIDGET (widget))))
    adw_tab_view_set_selected_page (priv->tab_view, page);
}

AdwTabView *
_panel_frame_get_tab_view (PanelFrame *self)
{
  PanelFramePrivate *priv = panel_frame_get_instance_private (self);

  g_return_val_if_fail (PANEL_IS_FRAME (self), NULL);

  return priv->tab_view;
}

/**
 * panel_frame_get_header:
 * @self: a #PanelFrame
 *
 * Gets the header for the frame.
 *
 * Returns: (nullable) (transfer none): a #PanelFrameHeader or %NULL
 */
PanelFrameHeader *
panel_frame_get_header (PanelFrame *self)
{
  PanelFramePrivate *priv = panel_frame_get_instance_private (self);

  g_return_val_if_fail (PANEL_IS_FRAME (self), NULL);
  g_return_val_if_fail (PANEL_IS_FRAME_HEADER (priv->header), NULL);

  return priv->header;
}

/**
 * panel_frame_set_header:
 * @self: a #PanelFrame
 * @header: (nullable): a #PanelFrameHeader
 *
 * Sets the header for the frame, such as a #PanelFrameSwitcher.
 */
void
panel_frame_set_header (PanelFrame       *self,
                        PanelFrameHeader *header)
{
  PanelFramePrivate *priv = panel_frame_get_instance_private (self);

  g_return_if_fail (PANEL_IS_FRAME (self));
  g_return_if_fail (!header || PANEL_IS_FRAME_HEADER (header));

  if (priv->header == header)
    return;

  if (priv->header != NULL)
    {
      panel_frame_header_page_changed (priv->header, NULL);
      panel_frame_header_set_frame (priv->header, NULL);
      gtk_overlay_set_child (priv->overlay, NULL);
      priv->header = NULL;
    }

  priv->header = header;

  if (priv->header != NULL)
    {
      PanelWidget *visible_child = panel_frame_get_visible_child (self);

      if (GTK_IS_ORIENTABLE (priv->header))
        gtk_orientable_set_orientation (GTK_ORIENTABLE (priv->header),
                                        !gtk_orientable_get_orientation (GTK_ORIENTABLE (priv->box)));
      gtk_overlay_set_child (priv->overlay, GTK_WIDGET (priv->header));

      panel_frame_header_set_frame (priv->header, self);

      if (visible_child)
        panel_frame_header_page_changed (priv->header, visible_child);

      gtk_widget_add_css_class (GTK_WIDGET (priv->header), "header");
    }
}

/**
 * panel_frame_get_pages:
 * @self: a #PanelFrame
 *
 * Get the pages for the frame.
 *
 * Returns: (transfer full): a #GtkSelectionModel
 */
GtkSelectionModel *
panel_frame_get_pages (PanelFrame *self)
{
  PanelFramePrivate *priv = panel_frame_get_instance_private (self);

  g_return_val_if_fail (PANEL_IS_FRAME (self), NULL);

  return adw_tab_view_get_pages (priv->tab_view);
}

void
_panel_frame_transfer (PanelFrame  *self,
                       PanelWidget *widget,
                       PanelFrame  *new_frame,
                       int          position)
{
  PanelFramePrivate *priv = panel_frame_get_instance_private (self);
  PanelFramePrivate *new_priv = panel_frame_get_instance_private (new_frame);
  AdwTabPage *page;
  GtkWidget *grid;
  GtkWidget *window;

  g_return_if_fail (PANEL_IS_FRAME (self));
  g_return_if_fail (PANEL_IS_WIDGET (widget));
  g_return_if_fail (PANEL_IS_FRAME (new_frame));

  /* First clear focus so that we ensure updating current frame */
  if ((window = gtk_widget_get_ancestor (GTK_WIDGET (self), GTK_TYPE_WINDOW)))
    gtk_window_set_focus (GTK_WINDOW (window), NULL);

  if (!(page = adw_tab_view_get_page (priv->tab_view, GTK_WIDGET (widget))))
    g_return_if_reached ();

  if (position < 0)
    position = adw_tab_view_get_n_pages (new_priv->tab_view);

  adw_tab_view_transfer_page (priv->tab_view, page, new_priv->tab_view, position);

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
  PanelFramePrivate *priv = panel_frame_get_instance_private (self);

  g_return_val_if_fail (PANEL_IS_FRAME (self), 0);

  return adw_tab_view_get_n_pages (priv->tab_view);
}

/**
 * panel_frame_get_page:
 * @self: a #PanelFrame
 * @n: the index of the page
 *
 * Gets the page with the given index, if any.
 *
 * Returns: (nullable) (transfer none): a #PanelWidget or %NULL
 */
PanelWidget *
panel_frame_get_page (PanelFrame *self,
                      guint       n)
{
  PanelFramePrivate *priv = panel_frame_get_instance_private (self);
  AdwTabPage *page;

  g_return_val_if_fail (PANEL_IS_FRAME (self), NULL);
  g_return_val_if_fail (n < panel_frame_get_n_pages (self), NULL);

  if ((page = adw_tab_view_get_nth_page (priv->tab_view, n)))
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
  PanelFramePrivate *priv = panel_frame_get_instance_private (self);

  g_return_val_if_fail (PANEL_IS_FRAME (self), NULL);

  return priv->placeholder;
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
  PanelFramePrivate *priv = panel_frame_get_instance_private (self);

  g_return_if_fail (PANEL_IS_FRAME (self));
  g_return_if_fail (!placeholder || GTK_IS_WIDGET (placeholder));

  if (priv->placeholder == placeholder)
    return;

  if (priv->placeholder)
    gtk_stack_remove (priv->stack, priv->placeholder);

  priv->placeholder = placeholder;

  if (priv->placeholder)
    gtk_stack_add_named (priv->stack, priv->placeholder, "placeholder");

  if (priv->placeholder && !panel_frame_get_visible_child (self))
    gtk_stack_set_visible_child (priv->stack, priv->placeholder);
  else
    gtk_stack_set_visible_child (priv->stack, GTK_WIDGET (priv->tab_view));

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
  PanelFramePrivate *priv = panel_frame_get_instance_private (self);
  AdwTabPage *page;

  g_return_val_if_fail (PANEL_IS_FRAME (self), NULL);

  page = adw_tab_view_get_selected_page (priv->tab_view);
  g_signal_emit_by_name (priv->tab_view, "setup-menu", page);
  return adw_tab_view_get_menu_model (priv->tab_view);
}

void
_panel_frame_set_closeable (PanelFrame  *self,
                            gboolean     closeable)
{
  PanelFramePrivate *priv = panel_frame_get_instance_private (self);

  g_return_if_fail (PANEL_IS_FRAME (self));

  closeable = !!closeable;

  if (priv->closeable == closeable)
    return;

  priv->closeable = closeable;
  panel_frame_update_actions (self);
  g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_CLOSEABLE]);
}

void
_panel_frame_set_drop_before (PanelFrame  *self,
                              PanelWidget *widget)
{
  PanelFramePrivate *priv = panel_frame_get_instance_private (self);

  g_return_if_fail (PANEL_IS_FRAME (self));
  g_return_if_fail (!widget || PANEL_IS_WIDGET (widget));

  if (PANEL_IS_FRAME_SWITCHER (priv->header))
    _panel_frame_switcher_set_drop_before (PANEL_FRAME_SWITCHER (priv->header), widget);
}

gboolean
_panel_frame_in_drop (PanelFrame *self)
{
  PanelFramePrivate *priv = panel_frame_get_instance_private (self);

  g_return_val_if_fail (PANEL_IS_FRAME (self), FALSE);

  return panel_drop_controls_in_drop (priv->drop_controls);
}

void
_panel_frame_request_close (PanelFrame  *self,
                            PanelWidget *widget)
{
  PanelFramePrivate *priv = panel_frame_get_instance_private (self);
  AdwTabPage *page;

  g_return_if_fail (PANEL_IS_FRAME (self));
  g_return_if_fail (PANEL_IS_WIDGET (widget));

  if ((page = adw_tab_view_get_page (priv->tab_view, GTK_WIDGET (widget))))
    adw_tab_view_close_page (priv->tab_view, page);
}

gboolean
panel_frame_get_closeable (PanelFrame *self)
{
  PanelFramePrivate *priv = panel_frame_get_instance_private (self);

  g_return_val_if_fail (PANEL_IS_FRAME (self), FALSE);

  return priv->closeable;
}
