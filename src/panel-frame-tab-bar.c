/* panel-frame-tab-bar.c
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

#include "panel-frame-private.h"
#include "panel-frame-switcher.h"
#include "panel-frame-tab-bar.h"

struct _PanelFrameTabBar
{
  GtkWidget   parent_instance;
  PanelFrame *frame;
  AdwTabBar  *tab_bar;
};

static void frame_header_iface_init (PanelFrameHeaderInterface *iface);

G_DEFINE_TYPE_WITH_CODE (PanelFrameTabBar, panel_frame_tab_bar, GTK_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (PANEL_TYPE_FRAME_HEADER, frame_header_iface_init))

enum {
  PROP_0,
  N_PROPS,

  PROP_FRAME,
};

/**
 * panel_frame_tab_bar_new:
 *
 * Create a new #PanelFrameTabBar.
 *
 * Returns: (transfer full): a newly created #PanelFrameTabBar
 */
GtkWidget *
panel_frame_tab_bar_new (void)
{
  return g_object_new (PANEL_TYPE_FRAME_TAB_BAR, NULL);
}

static void
panel_frame_tab_bar_set_frame (PanelFrameTabBar *self,
                               PanelFrame       *frame)
{
  g_assert (PANEL_IS_FRAME_TAB_BAR (self));
  g_assert (!frame || PANEL_IS_FRAME (frame));

  if (g_set_object (&self->frame, frame))
    {
      AdwTabView *view = NULL;

      view = frame ? _panel_frame_get_tab_view (frame) : NULL;
      adw_tab_bar_set_view (self->tab_bar, view);

      g_object_notify (G_OBJECT (self), "frame");
    }
}

static void
panel_frame_tab_bar_dispose (GObject *object)
{
  PanelFrameTabBar *self = (PanelFrameTabBar *)object;

  g_clear_pointer ((GtkWidget **)&self->tab_bar, gtk_widget_unparent);

  G_OBJECT_CLASS (panel_frame_tab_bar_parent_class)->dispose (object);
}

static void
panel_frame_tab_bar_get_property (GObject    *object,
                                  guint       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  PanelFrameTabBar *self = PANEL_FRAME_TAB_BAR (object);

  switch (prop_id)
    {
    case PROP_FRAME:
      g_value_set_object (value, self->frame);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
panel_frame_tab_bar_set_property (GObject      *object,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  PanelFrameTabBar *self = PANEL_FRAME_TAB_BAR (object);

  switch (prop_id)
    {
    case PROP_FRAME:
      panel_frame_tab_bar_set_frame (self, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
panel_frame_tab_bar_class_init (PanelFrameTabBarClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = panel_frame_tab_bar_dispose;
  object_class->get_property = panel_frame_tab_bar_get_property;
  object_class->set_property = panel_frame_tab_bar_set_property;

  g_object_class_override_property (object_class, PROP_FRAME, "frame");

  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BIN_LAYOUT);
}

static void
panel_frame_tab_bar_init (PanelFrameTabBar *self)
{
  self->tab_bar = ADW_TAB_BAR (adw_tab_bar_new ());
  adw_tab_bar_set_autohide (self->tab_bar, FALSE);
  gtk_widget_set_parent (GTK_WIDGET (self->tab_bar), GTK_WIDGET (self));
}

static gboolean
panel_frame_tab_bar_can_drop (PanelFrameHeader *header,
                              PanelWidget      *widget)
{
  const char *kind;

  g_assert (PANEL_IS_FRAME_TAB_BAR (header));
  g_assert (PANEL_IS_WIDGET (widget));

  kind = panel_widget_get_kind (widget);

  return g_strcmp0 (kind, PANEL_WIDGET_KIND_DOCUMENT) == 0;
}

static void
frame_header_iface_init (PanelFrameHeaderInterface *iface)
{
  iface->can_drop = panel_frame_tab_bar_can_drop;
}
