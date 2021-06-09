/* panel-frame-header-bar.c
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

#include <adwaita.h>

#include "panel-binding-group-private.h"
#include "panel-frame-header-bar.h"
#include "panel-frame-header-bar-row-private.h"
#include "panel-frame-private.h"
#include "panel-joined-menu-private.h"

struct _PanelFrameHeaderBar
{
  GtkWidget          parent_instance;

  PanelBindingGroup *bindings;
  PanelFrame        *frame;
  GMenuModel        *menu_model;
  PanelWidget       *visible_child;
  PanelJoinedMenu   *joined_menu;

  GMenuModel        *frame_menu;
  GtkBox            *box;
  GtkBox            *start_area;
  GtkBox            *end_area;
  GtkBox            *controls;
  GtkMenuButton     *menu_button;
  GtkPopover        *pages_popover;
  GtkListView       *list_view;
  GtkMenuButton     *title_button;
  GtkLabel          *title;
  GtkLabel          *modified;
  GtkImage          *image;
};

static void frame_header_iface_init (PanelFrameHeaderInterface *iface);

G_DEFINE_TYPE_WITH_CODE (PanelFrameHeaderBar, panel_frame_header_bar, GTK_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (PANEL_TYPE_FRAME_HEADER, frame_header_iface_init))

enum {
  PROP_0,
  PROP_MENU_MODEL,
  PROP_START_CHILD,
  PROP_END_CHILD,
  N_PROPS,

  PROP_FRAME,
};

static GParamSpec *properties [N_PROPS];

/**
 * panel_frame_header_bar_new:
 *
 * Create a new #PanelFrameHeaderBar.
 *
 * Returns: (transfer full): a newly created #PanelFrameHeaderBar
 */
GtkWidget *
panel_frame_header_bar_new (void)
{
  return g_object_new (PANEL_TYPE_FRAME_HEADER_BAR, NULL);
}

static void
setup_row_cb (GtkSignalListItemFactory *factory,
              GtkListItem              *list_item,
              PanelFrameHeaderBar      *self)
{
  g_assert (GTK_IS_SIGNAL_LIST_ITEM_FACTORY (factory));
  g_assert (GTK_IS_LIST_ITEM (list_item));
  g_assert (PANEL_IS_FRAME_HEADER_BAR (self));

  gtk_list_item_set_child (list_item, panel_frame_header_bar_row_new ());
}


static void
bind_row_cb (GtkSignalListItemFactory *factory,
             GtkListItem              *list_item,
             PanelFrameHeaderBar      *self)
{
  AdwTabPage *item;
  GtkWidget *row;

  g_assert (GTK_IS_SIGNAL_LIST_ITEM_FACTORY (factory));
  g_assert (GTK_IS_LIST_ITEM (list_item));
  g_assert (PANEL_IS_FRAME_HEADER_BAR (self));

  item = gtk_list_item_get_item (list_item);
  row = gtk_list_item_get_child (list_item);

  g_assert (ADW_IS_TAB_PAGE (item));
  g_assert (PANEL_IS_FRAME_HEADER_BAR_ROW (row));

  panel_frame_header_bar_row_set_page (PANEL_FRAME_HEADER_BAR_ROW (row), item);
}

static void
unbind_row_cb (GtkSignalListItemFactory *factory,
               GtkListItem              *list_item,
               PanelFrameHeaderBar      *self)
{
  GtkWidget *row;

  g_assert (GTK_IS_SIGNAL_LIST_ITEM_FACTORY (factory));
  g_assert (GTK_IS_LIST_ITEM (list_item));
  g_assert (PANEL_IS_FRAME_HEADER_BAR (self));

  row = gtk_list_item_get_child (list_item);
  panel_frame_header_bar_row_set_page (PANEL_FRAME_HEADER_BAR_ROW (row), NULL);
}

static void
panel_frame_header_bar_set_visible_child (PanelFrameHeaderBar *self,
                                          PanelWidget         *widget)
{
  g_return_if_fail (PANEL_IS_FRAME_HEADER_BAR (self));
  g_return_if_fail (!widget || PANEL_IS_WIDGET (widget));

  if (g_set_object (&self->visible_child, widget))
    panel_binding_group_set_source (self->bindings, widget);
}

static void
notify_visible_child_cb (PanelFrameHeaderBar *self,
                         GParamSpec          *pspec,
                         PanelFrame          *frame)
{
  PanelWidget *widget;

  g_assert (PANEL_IS_FRAME_HEADER_BAR (self));
  g_assert (PANEL_IS_FRAME (frame));

  widget = panel_frame_get_visible_child (frame);
  panel_frame_header_bar_set_visible_child (self, widget);

  if (widget == NULL)
    gtk_menu_button_popdown (self->title_button);

  gtk_widget_action_set_enabled (GTK_WIDGET (self), "page.close", widget != NULL);
  gtk_widget_set_sensitive (GTK_WIDGET (self->title_button), widget != NULL);
}

static void
panel_frame_header_bar_set_frame (PanelFrameHeaderBar *self,
                                  PanelFrame          *frame)
{
  g_assert (PANEL_IS_FRAME_HEADER_BAR (self));
  g_assert (!frame || PANEL_IS_FRAME (frame));

  if (self->frame == frame)
    return;

  if (self->frame)
    {
      panel_frame_header_bar_set_visible_child (self, NULL);
      g_signal_handlers_disconnect_by_func (self->frame,
                                            G_CALLBACK (notify_visible_child_cb),
                                            self);
      gtk_list_view_set_model (self->list_view, NULL);
      g_clear_object (&self->frame);
    }

  g_set_object (&self->frame, frame);

  if (self->frame)
    {
      g_signal_connect_object (self->frame,
                               "notify::visible-child",
                               G_CALLBACK (notify_visible_child_cb),
                               self,
                               G_CONNECT_SWAPPED);
      GtkSelectionModel *pages = panel_frame_get_pages (self->frame);
      gtk_list_view_set_model (self->list_view, pages);
      panel_frame_header_bar_set_visible_child (self,
                                                panel_frame_get_visible_child (self->frame));
    }

  g_object_notify (G_OBJECT (self), "frame");
}

static void
page_close_action (GtkWidget  *widget,
                   const char *action_name,
                   GVariant   *param)
{
  PanelFrameHeaderBar *self = (PanelFrameHeaderBar *)widget;
  AdwTabView *tab_view;

  g_assert (PANEL_IS_FRAME_HEADER (self));

  if (self->frame != NULL &&
      (tab_view = _panel_frame_get_tab_view (self->frame)))
    adw_tab_view_close_page (tab_view, adw_tab_view_get_selected_page (tab_view));
}

static void
panel_frame_header_bar_dispose (GObject *object)
{
  PanelFrameHeaderBar *self = (PanelFrameHeaderBar *)object;

  panel_frame_header_bar_set_frame (self, NULL);

  g_clear_pointer ((GtkWidget **)&self->box, gtk_widget_unparent);

  g_clear_object (&self->visible_child);
  g_clear_object (&self->frame);
  g_clear_object (&self->menu_model);
  g_clear_object (&self->bindings);
  g_clear_object (&self->joined_menu);

  G_OBJECT_CLASS (panel_frame_header_bar_parent_class)->dispose (object);
}

static void
panel_frame_header_bar_get_property (GObject    *object,
                                     guint       prop_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  PanelFrameHeaderBar *self = PANEL_FRAME_HEADER_BAR (object);

  switch (prop_id)
    {
    case PROP_FRAME:
      g_value_set_object (value, self->frame);
      break;

    case PROP_MENU_MODEL:
      g_value_set_object (value, panel_frame_header_bar_get_menu_model (self));
      break;

    case PROP_START_CHILD:
      g_value_set_object (value, panel_frame_header_bar_get_start_child (self));
      break;

    case PROP_END_CHILD:
      g_value_set_object (value, panel_frame_header_bar_get_end_child (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
panel_frame_header_bar_set_property (GObject      *object,
                                     guint         prop_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  PanelFrameHeaderBar *self = PANEL_FRAME_HEADER_BAR (object);

  switch (prop_id)
    {
    case PROP_FRAME:
      panel_frame_header_bar_set_frame (self, g_value_get_object (value));
      break;

    case PROP_MENU_MODEL:
      panel_frame_header_bar_set_menu_model (self, g_value_get_object (value));
      break;

    case PROP_START_CHILD:
      panel_frame_header_bar_set_start_child (self, g_value_get_object (value));
      break;

    case PROP_END_CHILD:
      panel_frame_header_bar_set_end_child (self, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
panel_frame_header_bar_class_init (PanelFrameHeaderBarClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = panel_frame_header_bar_dispose;
  object_class->get_property = panel_frame_header_bar_get_property;
  object_class->set_property = panel_frame_header_bar_set_property;

  properties [PROP_MENU_MODEL] =
    g_param_spec_object ("menu-model",
                         "Menu Model",
                         "Menu Model",
                         G_TYPE_MENU_MODEL,
                         (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  properties [PROP_START_CHILD] =
    g_param_spec_object ("start-child",
                         "Start Child",
                         "Start Child",
                         GTK_TYPE_WIDGET,
                         (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  properties [PROP_END_CHILD] =
    g_param_spec_object ("end-child",
                         "End Child",
                         "End Child",
                         GTK_TYPE_WIDGET,
                         (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  g_object_class_override_property (object_class, PROP_FRAME, "frame");

  g_object_class_install_properties (object_class, N_PROPS, properties);

  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/libpanel/panel-frame-header-bar.ui");
  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BIN_LAYOUT);
  gtk_widget_class_set_css_name (widget_class, "panelframeheaderbar");
  gtk_widget_class_bind_template_child (widget_class, PanelFrameHeaderBar, box);
  gtk_widget_class_bind_template_child (widget_class, PanelFrameHeaderBar, controls);
  gtk_widget_class_bind_template_child (widget_class, PanelFrameHeaderBar, end_area);
  gtk_widget_class_bind_template_child (widget_class, PanelFrameHeaderBar, frame_menu);
  gtk_widget_class_bind_template_child (widget_class, PanelFrameHeaderBar, list_view);
  gtk_widget_class_bind_template_child (widget_class, PanelFrameHeaderBar, menu_button);
  gtk_widget_class_bind_template_child (widget_class, PanelFrameHeaderBar, pages_popover);
  gtk_widget_class_bind_template_child (widget_class, PanelFrameHeaderBar, start_area);
  gtk_widget_class_bind_template_child (widget_class, PanelFrameHeaderBar, title_button);
  gtk_widget_class_bind_template_callback (widget_class, setup_row_cb);
  gtk_widget_class_bind_template_callback (widget_class, bind_row_cb);
  gtk_widget_class_bind_template_callback (widget_class, unbind_row_cb);

  gtk_widget_class_install_action (widget_class, "page.close", NULL, page_close_action);
}

static void
panel_frame_header_bar_init (PanelFrameHeaderBar *self)
{
  GtkWidget *button;
  GtkWidget *box;

  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_widget_action_set_enabled (GTK_WIDGET (self), "page.close", FALSE);

  /* Because GtkMenuButton does not allow us to specify children within
   * the label, we have to dive into it and modify it directly.
   */
  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_widget_set_halign (box, GTK_ALIGN_CENTER);
  self->image = GTK_IMAGE (gtk_image_new ());
  gtk_widget_set_valign (GTK_WIDGET (self->image), GTK_ALIGN_BASELINE);
  gtk_box_append (GTK_BOX (box), GTK_WIDGET (self->image));
  self->title = g_object_new (GTK_TYPE_LABEL,
                              "valign", GTK_ALIGN_BASELINE,
                              "xalign", 0.0f,
                              "ellipsize", PANGO_ELLIPSIZE_MIDDLE,
                              "width-chars", 5,
                              NULL);
  gtk_box_append (GTK_BOX (box), GTK_WIDGET (self->title));
  self->modified = g_object_new (GTK_TYPE_LABEL,
                                 "valign", GTK_ALIGN_BASELINE,
                                 "xalign", 0.0f,
                                 "single-line-mode", TRUE,
                                 "width-chars", 1,
                                 "max-width-chars", 1,
                                 NULL);
  gtk_box_append (GTK_BOX (box), GTK_WIDGET (self->modified));
  button = gtk_widget_get_first_child (GTK_WIDGET (self->title_button));
  gtk_button_set_child (GTK_BUTTON (button), box);

  self->joined_menu = panel_joined_menu_new ();
  panel_joined_menu_append_menu (self->joined_menu, self->frame_menu);
  gtk_menu_button_set_menu_model (self->menu_button,
                                  G_MENU_MODEL (self->joined_menu));

  self->bindings = panel_binding_group_new ();
  panel_binding_group_bind (self->bindings, "title", self->title, "label", 0);
  panel_binding_group_bind (self->bindings, "modified", self->modified, "visible", 0);
  panel_binding_group_bind (self->bindings, "icon", self->image, "gicon", 0);
}

static gboolean
panel_frame_header_bar_can_drop (PanelFrameHeader *header,
                                 PanelWidget      *widget)
{
  const char *kind;

  g_assert (PANEL_IS_FRAME_HEADER_BAR (header));
  g_assert (PANEL_IS_WIDGET (widget));

  kind = panel_widget_get_kind (widget);

  return g_strcmp0 (kind, PANEL_WIDGET_KIND_DOCUMENT) == 0;
}

static void
panel_frame_header_bar_page_changed (PanelFrameHeader *header,
                                     PanelWidget      *page)
{
  PanelFrameHeaderBar *self = (PanelFrameHeaderBar *)header;

  g_assert (PANEL_IS_FRAME_HEADER_BAR (self));
  g_assert (!page || PANEL_IS_WIDGET (page));

  while (panel_joined_menu_get_n_joined (self->joined_menu) > 1)
    panel_joined_menu_remove_index (self->joined_menu, 0);

  if (page != NULL)
    {
      GMenuModel *menu_model = panel_widget_get_menu_model (page);

      if (menu_model != NULL)
        panel_joined_menu_prepend_menu (self->joined_menu, menu_model);
    }

  gtk_label_set_label (self->title, NULL);
  gtk_image_clear (self->image);
}

static void
frame_header_iface_init (PanelFrameHeaderInterface *iface)
{
  iface->can_drop = panel_frame_header_bar_can_drop;
  iface->page_changed = panel_frame_header_bar_page_changed;
}

GMenuModel *
panel_frame_header_bar_get_menu_model (PanelFrameHeaderBar *self)
{
  g_return_val_if_fail (PANEL_IS_FRAME_HEADER_BAR (self), NULL);

  return self->menu_model;
}

void
panel_frame_header_bar_set_menu_model (PanelFrameHeaderBar *self,
                                       GMenuModel          *menu_model)
{
  g_return_if_fail (PANEL_IS_FRAME_HEADER_BAR (self));
  g_return_if_fail (!menu_model || G_IS_MENU_MODEL (menu_model));

  if (self->menu_model == menu_model)
    return;

  if (self->menu_model)
    panel_joined_menu_remove_menu (self->joined_menu, self->menu_model);

  g_set_object (&self->menu_model, menu_model);

  if (self->menu_model)
    panel_joined_menu_prepend_menu (self->joined_menu, self->menu_model);

  g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_MENU_MODEL]);
}

GtkPopoverMenu *
panel_frame_header_bar_get_menu_popover (PanelFrameHeaderBar *self)
{
  g_return_val_if_fail (PANEL_IS_FRAME_HEADER_BAR (self), NULL);

  return GTK_POPOVER_MENU (gtk_menu_button_get_popover (self->menu_button));
}

/**
 * panel_frame_header_bar_get_start_child:
 * @self: a #PanelFrameHeaderBar
 *
 * Gets the #PanelFrameHeaderBar:start-child property.
 *
 * This is the child that is placed at the beginning of the header bar.
 *
 * Returns: (transfer none) (nullable): a #GtkWidget or %NULL.
 */
GtkWidget *
panel_frame_header_bar_get_start_child (PanelFrameHeaderBar *self)
{
  g_return_val_if_fail (PANEL_IS_FRAME_HEADER_BAR (self), NULL);

  return gtk_widget_get_first_child (GTK_WIDGET (self->start_area));
}

void
panel_frame_header_bar_set_start_child (PanelFrameHeaderBar *self,
                                        GtkWidget           *start_child)
{
  GtkWidget *prev;

  g_return_if_fail (PANEL_IS_FRAME_HEADER_BAR (self));
  g_return_if_fail (!start_child || GTK_IS_WIDGET (start_child));
  g_return_if_fail (gtk_widget_get_parent (start_child) == NULL);

  prev = panel_frame_header_bar_get_start_child (self);

  if (prev == start_child)
    return;

  if (prev != NULL)
    gtk_box_remove (self->start_area, prev);

  if (start_child)
    gtk_box_append (self->start_area, start_child);

  g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_START_CHILD]);
}

/**
 * panel_frame_header_bar_get_end_child:
 * @self: a #PanelFrameHeaderBar
 *
 * Gets the #PanelFrameHeaderBar:end-child property.
 *
 * This is the child that is placed at the end of the header bar
 * but before the menu/close controls.
 *
 * Returns: (transfer none) (nullable): a #GtkWidget or %NULL.
 */
GtkWidget *
panel_frame_header_bar_get_end_child (PanelFrameHeaderBar *self)
{
  g_return_val_if_fail (PANEL_IS_FRAME_HEADER_BAR (self), NULL);

  return gtk_widget_get_first_child (GTK_WIDGET (self->end_area));
}

void
panel_frame_header_bar_set_end_child (PanelFrameHeaderBar *self,
                                      GtkWidget           *end_child)
{
  GtkWidget *prev;

  g_return_if_fail (PANEL_IS_FRAME_HEADER_BAR (self));
  g_return_if_fail (!end_child || GTK_IS_WIDGET (end_child));
  g_return_if_fail (gtk_widget_get_parent (end_child) == NULL);

  prev = panel_frame_header_bar_get_end_child (self);

  if (prev == end_child)
    return;

  if (prev != NULL)
    gtk_box_remove (self->end_area, prev);

  if (end_child)
    gtk_box_append (self->end_area, end_child);

  g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_END_CHILD]);
}
