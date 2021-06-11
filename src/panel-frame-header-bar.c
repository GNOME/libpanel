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
#include <glib/gi18n.h>

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
  GtkCssProvider    *css_provider;

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

  GdkRGBA            background_rgba;
  GdkRGBA            foreground_rgba;

  guint              update_css_handler;

  guint              background_rgba_set : 1;
  guint              foreground_rgba_set : 1;
};

static void frame_header_iface_init (PanelFrameHeaderInterface *iface);

G_DEFINE_TYPE_WITH_CODE (PanelFrameHeaderBar, panel_frame_header_bar, GTK_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (PANEL_TYPE_FRAME_HEADER, frame_header_iface_init))

enum {
  PROP_0,
  PROP_BACKGROUND_RGBA,
  PROP_FOREGROUND_RGBA,
  N_PROPS,

  PROP_FRAME,
};

static GParamSpec *properties [N_PROPS];
static GQuark css_quark;

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
update_css_providers_recurse (GtkWidget           *widget,
                              PanelFrameHeaderBar *self)
{
  /* Stop at popovers */
  if (GTK_IS_NATIVE (widget))
    return;

  if (!g_object_get_qdata (G_OBJECT (widget), css_quark))
    {
      GtkStyleContext *style_context = gtk_widget_get_style_context (widget);
      gtk_style_context_add_provider (style_context,
                                      GTK_STYLE_PROVIDER (self->css_provider),
                                      GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
      g_object_set_qdata (G_OBJECT (widget), css_quark, self->css_provider);
    }

  for (GtkWidget *child = gtk_widget_get_first_child (widget);
       child != NULL;
       child = gtk_widget_get_next_sibling (child))
    update_css_providers_recurse (child, self);
}

static gboolean
panel_frame_header_bar_update_css (PanelFrameHeaderBar *self)
{
  GString *str = NULL;

  g_assert (PANEL_IS_FRAME_HEADER_BAR (self));
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
      gchar *bgstr = gdk_rgba_to_string (&self->background_rgba);

      g_string_append        (str, "panelframeheaderbar {\n");
      g_string_append        (str, "  background: none;\n");
      g_string_append_printf (str, "  background-color: %s;\n", bgstr);
      g_string_append        (str, "  transition: background-color 400ms;\n");
      g_string_append        (str, "  transition-timing-function: ease;\n");
      g_string_append_printf (str, "  border-bottom: 1px solid shade(%s,0.9);\n", bgstr);
      g_string_append        (str, "}\n");
      g_string_append        (str, "button { background: transparent; }\n");
      g_string_append        (str, "button:hover, button:checked {\n");
      g_string_append_printf (str, "  background: none; background-color: shade(%s,.85); }\n", bgstr);

      /* only use foreground when background is set */
      if (self->foreground_rgba_set)
        {
          static const gchar *names[] = { "image", "label" };
          gchar *fgstr = gdk_rgba_to_string (&self->foreground_rgba);

          for (guint i = 0; i < G_N_ELEMENTS (names); i++)
            {
              g_string_append_printf (str, "%s {\n", names[i]);
              g_string_append        (str, "  -gtk-icon-shadow: none;\n");
              g_string_append        (str, "  text-shadow: none;\n");
              g_string_append_printf (str, "  text-shadow: 0 -1px alpha(%s,0.05);\n", fgstr);
              g_string_append_printf (str, "  color: %s;\n", fgstr);
              g_string_append        (str, "}\n");
            }

          g_free (fgstr);
        }

      g_free (bgstr);
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
panel_frame_header_bar_queue_update_css (PanelFrameHeaderBar *self)
{
  g_assert (PANEL_IS_FRAME_HEADER_BAR (self));

  if (self->update_css_handler == 0)
    self->update_css_handler =
      g_idle_add_full (G_PRIORITY_HIGH,
                       (GSourceFunc)panel_frame_header_bar_update_css,
                       g_object_ref (self),
                       g_object_unref);
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
      gtk_list_view_set_model (self->list_view, NULL);
      g_clear_object (&self->frame);
    }

  g_set_object (&self->frame, frame);

  if (self->frame)
    {
      GtkSelectionModel *pages = panel_frame_get_pages (self->frame);
      gtk_list_view_set_model (self->list_view, pages);
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
menu_clicked_cb (GtkGesture          *gesture,
                 int                  n_press,
                 double               x,
                 double               y,
                 PanelFrameHeaderBar *self)
{
  g_assert (GTK_IS_GESTURE_CLICK (gesture));
  g_assert (PANEL_IS_FRAME_HEADER_BAR (self));

  if (self->frame)
    {
      GMenuModel *menu_model = _panel_frame_get_tab_menu (self->frame);
      gtk_menu_button_set_menu_model (self->menu_button, menu_model);
    }
}

static void
panel_frame_header_bar_dispose (GObject *object)
{
  PanelFrameHeaderBar *self = (PanelFrameHeaderBar *)object;

  g_clear_handle_id (&self->update_css_handler, g_source_remove);

  panel_frame_header_bar_set_frame (self, NULL);

  g_clear_pointer ((GtkWidget **)&self->box, gtk_widget_unparent);

  g_clear_object (&self->visible_child);
  g_clear_object (&self->frame);
  g_clear_object (&self->menu_model);
  g_clear_object (&self->css_provider);
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
    case PROP_BACKGROUND_RGBA:
      g_value_set_boxed (value, panel_frame_header_bar_get_background_rgba (self));
      break;

    case PROP_FOREGROUND_RGBA:
      g_value_set_boxed (value, panel_frame_header_bar_get_foreground_rgba (self));
      break;

    case PROP_FRAME:
      g_value_set_object (value, self->frame);
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
    case PROP_BACKGROUND_RGBA:
      panel_frame_header_bar_set_background_rgba (self, g_value_get_boxed (value));
      break;

    case PROP_FOREGROUND_RGBA:
      panel_frame_header_bar_set_foreground_rgba (self, g_value_get_boxed (value));
      break;

    case PROP_FRAME:
      panel_frame_header_bar_set_frame (self, g_value_get_object (value));
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

  /**
   * PanelFrameHeaderBar:background-rgba:
   *
   * The "background-rgba" property can be used to set the background
   * color of the header. This should be set to the
   * #PanelWidget:background-rgba of the active view.
   *
   * Set to %NULL to unset the background-rgba.
   */
  properties [PROP_BACKGROUND_RGBA] =
    g_param_spec_boxed ("background-rgba",
                        "Background RGBA",
                        "The background color to use for the header",
                        GDK_TYPE_RGBA,
                        (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  /**
   * PanelFrameHeaderBar:foreground-rgba:
   *
   * Sets the foreground color to use when
   * #PanelFrameHeaderBar:background-rgba is used for the background.
   */
  properties [PROP_FOREGROUND_RGBA] =
    g_param_spec_boxed ("foreground-rgba",
                        "Foreground RGBA",
                        "The foreground color to use with background-rgba",
                        GDK_TYPE_RGBA,
                        (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  g_object_class_override_property (object_class, PROP_FRAME, "frame");
  g_object_class_install_properties (object_class, N_PROPS, properties);

  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/libpanel/panel-frame-header-bar.ui");
  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BIN_LAYOUT);
  gtk_widget_class_set_css_name (widget_class, "panelframeheaderbar");
  gtk_widget_class_bind_template_child (widget_class, PanelFrameHeaderBar, box);
  gtk_widget_class_bind_template_child (widget_class, PanelFrameHeaderBar, controls);
  gtk_widget_class_bind_template_child (widget_class, PanelFrameHeaderBar, end_area);
  gtk_widget_class_bind_template_child (widget_class, PanelFrameHeaderBar, list_view);
  gtk_widget_class_bind_template_child (widget_class, PanelFrameHeaderBar, menu_button);
  gtk_widget_class_bind_template_child (widget_class, PanelFrameHeaderBar, pages_popover);
  gtk_widget_class_bind_template_child (widget_class, PanelFrameHeaderBar, start_area);
  gtk_widget_class_bind_template_child (widget_class, PanelFrameHeaderBar, title_button);
  gtk_widget_class_bind_template_callback (widget_class, setup_row_cb);
  gtk_widget_class_bind_template_callback (widget_class, bind_row_cb);
  gtk_widget_class_bind_template_callback (widget_class, unbind_row_cb);
  gtk_widget_class_bind_template_callback (widget_class, menu_clicked_cb);

  gtk_widget_class_install_action (widget_class, "page.close", NULL, page_close_action);

  css_quark = g_quark_from_static_string ("css-provider");
}

static void
panel_frame_header_bar_init (PanelFrameHeaderBar *self)
{
  GtkWidget *button;
  GtkWidget *box;

  self->css_provider = gtk_css_provider_new ();

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
                              "label", _("No Open Pages"),
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

  self->bindings = panel_binding_group_new ();
  panel_binding_group_bind (self->bindings, "title", self->title, "label", 0);
  panel_binding_group_bind (self->bindings, "modified", self->modified, "visible", 0);
  panel_binding_group_bind (self->bindings, "icon", self->image, "gicon", 0);
  panel_binding_group_bind (self->bindings, "background-rgba", self, "background-rgba", 0);
  panel_binding_group_bind (self->bindings, "foreground-rgba", self, "foreground-rgba", 0);

  update_css_providers_recurse (GTK_WIDGET (self), self);
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

  if (page == NULL)
    {
      gtk_label_set_label (self->title, _("No Open Pages"));
      gtk_image_clear (self->image);
      gtk_menu_button_popdown (self->title_button);
    }

  self->foreground_rgba_set = FALSE;
  self->background_rgba_set = FALSE;
  panel_frame_header_bar_queue_update_css (self);

  panel_binding_group_set_source (self->bindings, page);

  gtk_widget_action_set_enabled (GTK_WIDGET (self), "page.close", page != NULL);
  gtk_widget_set_sensitive (GTK_WIDGET (self->title_button), page != NULL);

  if (self->frame)
    {
      GMenuModel *menu_model = _panel_frame_get_tab_menu (self->frame);
      gtk_menu_button_set_menu_model (self->menu_button, menu_model);
    }
}

#define GET_PRIORITY(w)   GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w),"PRIORITY"))
#define SET_PRIORITY(w,i) g_object_set_data(G_OBJECT(w),"PRIORITY",GINT_TO_POINTER(i))

static void
panel_frame_header_bar_pack_start (PanelFrameHeader *header,
                                   int               priority,
                                   GtkWidget        *widget)
{
  PanelFrameHeaderBar *self = (PanelFrameHeaderBar *)header;
  GtkWidget *sibling = NULL;

  g_assert (PANEL_IS_FRAME_HEADER_BAR (self));

  SET_PRIORITY (widget, priority);

  for (GtkWidget *child = gtk_widget_get_first_child (GTK_WIDGET (self->start_area));
       child != NULL;
       child = gtk_widget_get_next_sibling (child))
    {
      if (priority < GET_PRIORITY(child))
        break;
      sibling = child;
    }

  gtk_box_insert_child_after (self->start_area, widget, sibling);

  update_css_providers_recurse (widget, self);
}

static void
panel_frame_header_bar_pack_end (PanelFrameHeader *header,
                                 int               priority,
                                 GtkWidget        *widget)
{
  PanelFrameHeaderBar *self = (PanelFrameHeaderBar *)header;
  GtkWidget *sibling = NULL;

  g_assert (PANEL_IS_FRAME_HEADER_BAR (self));

  SET_PRIORITY (widget, priority);

  for (GtkWidget *child = gtk_widget_get_first_child (GTK_WIDGET (self->end_area));
       child != NULL;
       child = gtk_widget_get_next_sibling (child))
    {
      if (priority < GET_PRIORITY(child))
        break;
      sibling = child;
    }

  gtk_box_insert_child_after (self->end_area, widget, sibling);

  update_css_providers_recurse (widget, self);
}

static void
frame_header_iface_init (PanelFrameHeaderInterface *iface)
{
  iface->can_drop = panel_frame_header_bar_can_drop;
  iface->page_changed = panel_frame_header_bar_page_changed;
  iface->pack_start = panel_frame_header_bar_pack_start;
  iface->pack_end = panel_frame_header_bar_pack_end;
}

GtkPopoverMenu *
panel_frame_header_bar_get_menu_popover (PanelFrameHeaderBar *self)
{
  g_return_val_if_fail (PANEL_IS_FRAME_HEADER_BAR (self), NULL);

  return GTK_POPOVER_MENU (gtk_menu_button_get_popover (self->menu_button));
}

const GdkRGBA *
panel_frame_header_bar_get_background_rgba (PanelFrameHeaderBar *self)
{
  g_return_val_if_fail (PANEL_IS_FRAME_HEADER_BAR (self), NULL);

  return self->background_rgba_set ? &self->background_rgba : NULL;
}

void
panel_frame_header_bar_set_background_rgba (PanelFrameHeaderBar *self,
                                            const GdkRGBA       *background_rgba)
{
  g_return_if_fail (PANEL_IS_FRAME_HEADER_BAR (self));

  self->background_rgba_set = background_rgba != NULL;
  if (background_rgba)
    self->background_rgba = *background_rgba;
  panel_frame_header_bar_queue_update_css (self);
  g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_BACKGROUND_RGBA]);
}

const GdkRGBA *
panel_frame_header_bar_get_foreground_rgba (PanelFrameHeaderBar *self)
{
  g_return_val_if_fail (PANEL_IS_FRAME_HEADER_BAR (self), NULL);

  return self->foreground_rgba_set ? &self->foreground_rgba : NULL;
}

void
panel_frame_header_bar_set_foreground_rgba (PanelFrameHeaderBar *self,
                                            const GdkRGBA       *foreground_rgba)
{
  g_return_if_fail (PANEL_IS_FRAME_HEADER_BAR (self));

  self->foreground_rgba_set = foreground_rgba != NULL;
  if (foreground_rgba)
    self->foreground_rgba = *foreground_rgba;
  panel_frame_header_bar_queue_update_css (self);
  g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_FOREGROUND_RGBA]);
}
