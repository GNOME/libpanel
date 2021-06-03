/* panel-widget.c
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
#include "panel-widget.h"

typedef struct
{
  GtkWidget *child;
  char      *title;
  char      *icon_name;

  GtkWidget *maximize_frame;
  GtkWidget *maximize_dock_child;

  guint      reorderable : 1;
  guint      can_maximize : 1;
  guint      maximized : 1;
} PanelWidgetPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (PanelWidget, panel_widget, GTK_TYPE_WIDGET)

enum {
  PROP_0,
  PROP_CAN_MAXIMIZE,
  PROP_CHILD,
  PROP_ICON_NAME,
  PROP_REORDERABLE,
  PROP_TITLE,
  N_PROPS
};

static GParamSpec *properties [N_PROPS];

/**
 * panel_widget_new:
 *
 * Create a new #PanelWidget.
 *
 * Returns: (transfer full): a newly created #PanelWidget
 */
GtkWidget *
panel_widget_new (void)
{
  return g_object_new (PANEL_TYPE_WIDGET, NULL);
}

static void
panel_widget_dispose (GObject *object)
{
  PanelWidget *self = (PanelWidget *)object;
  PanelWidgetPrivate *priv = panel_widget_get_instance_private (self);

  g_clear_pointer (&priv->icon_name, g_free);
  g_clear_pointer (&priv->title, g_free);
  g_clear_pointer (&priv->child, gtk_widget_unparent);

  G_OBJECT_CLASS (panel_widget_parent_class)->dispose (object);
}

static void
panel_widget_get_property (GObject    *object,
                           guint       prop_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  PanelWidget *self = PANEL_WIDGET (object);

  switch (prop_id)
    {
    case PROP_CAN_MAXIMIZE:
      g_value_set_boolean (value, panel_widget_get_can_maximize (self));
      break;

    case PROP_ICON_NAME:
      g_value_set_string (value, panel_widget_get_icon_name (self));
      break;

    case PROP_TITLE:
      g_value_set_string (value, panel_widget_get_title (self));
      break;

    case PROP_CHILD:
      g_value_set_object (value, panel_widget_get_child (self));
      break;

    case PROP_REORDERABLE:
      g_value_set_boolean (value, panel_widget_get_reorderable (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
panel_widget_set_property (GObject      *object,
                           guint         prop_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  PanelWidget *self = PANEL_WIDGET (object);

  switch (prop_id)
    {
    case PROP_CAN_MAXIMIZE:
      panel_widget_set_can_maximize (self, g_value_get_boolean (value));
      break;

    case PROP_ICON_NAME:
      panel_widget_set_icon_name (self, g_value_get_string (value));
      break;

    case PROP_TITLE:
      panel_widget_set_title (self, g_value_get_string (value));
      break;

    case PROP_CHILD:
      panel_widget_set_child (self, g_value_get_object (value));
      break;

    case PROP_REORDERABLE:
      panel_widget_set_reorderable (self, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
panel_widget_class_init (PanelWidgetClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = panel_widget_dispose;
  object_class->get_property = panel_widget_get_property;
  object_class->set_property = panel_widget_set_property;

  properties [PROP_CAN_MAXIMIZE] =
    g_param_spec_boolean ("can-maximize",
                          "Can Maximize",
                          "Can Maximize",
                          FALSE,
                          (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  properties [PROP_ICON_NAME] =
    g_param_spec_string ("icon-name",
                         "Icon Name",
                         "Icon Name",
                         NULL,
                         (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  properties [PROP_TITLE] =
    g_param_spec_string ("title",
                         "Title",
                         "Title",
                         NULL,
                         (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  properties [PROP_CHILD] =
    g_param_spec_object ("child",
                         "Child",
                         "Child",
                         GTK_TYPE_WIDGET,
                         (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  properties [PROP_REORDERABLE] =
    g_param_spec_boolean ("reorderable",
                          "Reorderable",
                          "If the panel may be reordered",
                          TRUE,
                          (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);

  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BIN_LAYOUT);
  gtk_widget_class_set_css_name (widget_class, "panelwidget");
}

static void
panel_widget_init (PanelWidget *self)
{
  PanelWidgetPrivate *priv = panel_widget_get_instance_private (self);

  priv->reorderable = TRUE;
}

const char *
panel_widget_get_icon_name (PanelWidget *self)
{
  PanelWidgetPrivate *priv = panel_widget_get_instance_private (self);

  g_return_val_if_fail (PANEL_IS_WIDGET (self), NULL);

  return priv->icon_name;
}

void
panel_widget_set_icon_name (PanelWidget *self,
                            const char  *icon_name)
{
  PanelWidgetPrivate *priv = panel_widget_get_instance_private (self);

  g_return_if_fail (PANEL_IS_WIDGET (self));

  if (g_strcmp0 (priv->icon_name, icon_name) != 0)
    {
      g_free (priv->icon_name);
      priv->icon_name = g_strdup (icon_name);
      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_ICON_NAME]);
    }
}

const char *
panel_widget_get_title (PanelWidget *self)
{
  PanelWidgetPrivate *priv = panel_widget_get_instance_private (self);

  g_return_val_if_fail (PANEL_IS_WIDGET (self), NULL);

  return priv->title;
}

void
panel_widget_set_title (PanelWidget *self,
                        const char  *title)
{
  PanelWidgetPrivate *priv = panel_widget_get_instance_private (self);

  g_return_if_fail (PANEL_IS_WIDGET (self));

  if (g_strcmp0 (priv->title, title) != 0)
    {
      g_free (priv->title);
      priv->title = g_strdup (title);
      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_TITLE]);
    }
}

/**
 * panel_widget_get_child:
 * @self: a #PanelWidget
 *
 * Gets the child widget of the panel.
 *
 * Returns: (transfer none) (nullable): a #GtkWidget or %NULL
 */
GtkWidget *
panel_widget_get_child (PanelWidget *self)
{
  PanelWidgetPrivate *priv = panel_widget_get_instance_private (self);

  g_return_val_if_fail (PANEL_IS_WIDGET (self), NULL);

  return priv->child;
}

void
panel_widget_set_child (PanelWidget *self,
                        GtkWidget   *child)
{
  PanelWidgetPrivate *priv = panel_widget_get_instance_private (self);

  g_return_if_fail (PANEL_IS_WIDGET (self));
  g_return_if_fail (!child || GTK_IS_WIDGET (child));

  if (priv->child == child)
    return;

  g_clear_pointer (&priv->child, gtk_widget_unparent);
  priv->child = child;
  if (priv->child != NULL)
    gtk_widget_set_parent (priv->child, GTK_WIDGET (self));
}

gboolean
panel_widget_get_reorderable (PanelWidget *self)
{
  PanelWidgetPrivate *priv = panel_widget_get_instance_private (self);

  g_return_val_if_fail (PANEL_IS_WIDGET (self), FALSE);

  return priv->reorderable;
}

void
panel_widget_set_reorderable (PanelWidget *self,
                              gboolean     reorderable)
{
  PanelWidgetPrivate *priv = panel_widget_get_instance_private (self);

  g_return_if_fail (PANEL_IS_WIDGET (self));

  reorderable = !!reorderable;

  if (reorderable != priv->reorderable)
    {
      priv->reorderable = reorderable;
      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_REORDERABLE]);
    }
}

gboolean
panel_widget_get_can_maximize (PanelWidget *self)
{
  PanelWidgetPrivate *priv = panel_widget_get_instance_private (self);

  g_return_val_if_fail (PANEL_IS_WIDGET (self), FALSE);

  return priv->can_maximize;
}

void
panel_widget_set_can_maximize (PanelWidget *self,
                               gboolean     can_maximize)
{
  PanelWidgetPrivate *priv = panel_widget_get_instance_private (self);

  g_return_if_fail (PANEL_IS_WIDGET (self));

  can_maximize = !!can_maximize;

  if (priv->can_maximize != can_maximize)
    {
      priv->can_maximize = can_maximize;
      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_CAN_MAXIMIZE]);
    }
}

void
panel_widget_maximize (PanelWidget *self)
{
  PanelWidgetPrivate *priv = panel_widget_get_instance_private (self);
  GtkWidget *dock;
  GtkWidget *dock_child;
  GtkWidget *frame;

  g_return_if_fail (PANEL_IS_WIDGET (self));

  if (priv->maximized)
    return;

  if (!panel_widget_get_can_maximize (self))
    return;

  if (!(frame = gtk_widget_get_ancestor (GTK_WIDGET (self), PANEL_TYPE_FRAME)) ||
      !(dock_child = gtk_widget_get_ancestor (frame, PANEL_TYPE_DOCK_CHILD)) ||
      !(dock = gtk_widget_get_ancestor (dock_child, PANEL_TYPE_DOCK)))
    return;

  priv->maximized = TRUE;

  g_object_ref (self);

  g_set_weak_pointer (&priv->maximize_frame, frame);
  g_set_weak_pointer (&priv->maximize_dock_child, frame);

  panel_frame_remove (PANEL_FRAME (frame), self);

  _panel_dock_set_maximized (PANEL_DOCK (dock), self);

  g_object_unref (self);
}

void
panel_widget_unmaximize (PanelWidget *self)
{
  PanelWidgetPrivate *priv = panel_widget_get_instance_private (self);
  GtkWidget *dock;

  g_return_if_fail (PANEL_IS_WIDGET (self));

  if (!priv->maximized)
    return;

  if (!(dock = gtk_widget_get_ancestor (GTK_WIDGET (self), PANEL_TYPE_DOCK)))
    return;

  g_object_ref (self);

  _panel_dock_set_maximized (PANEL_DOCK (dock), NULL);
  _panel_dock_add_widget (PANEL_DOCK (dock),
                          PANEL_DOCK_CHILD (priv->maximize_dock_child),
                          PANEL_FRAME (priv->maximize_frame),
                          self);

  g_clear_weak_pointer (&priv->maximize_frame);
  g_clear_weak_pointer (&priv->maximize_dock_child);

  g_object_unref (self);
}
