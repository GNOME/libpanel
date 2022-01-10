/* panel-omni-bar.c
 *
 * Copyright 2022 Christian Hergert <chergert@redhat.com>
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

#include "panel-omni-bar.h"

typedef struct
{
  GtkBox *prefix;
  GtkBox *center;
  GtkBox *suffix;
  GtkPopover *popover;
} PanelOmniBarPrivate;

static void buildable_iface_init (GtkBuildableIface *iface);

G_DEFINE_TYPE_WITH_CODE (PanelOmniBar, panel_omni_bar, GTK_TYPE_WIDGET,
                         G_ADD_PRIVATE (PanelOmniBar)
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_BUILDABLE, buildable_iface_init))

enum {
  PROP_0,
  PROP_POPOVER,
  N_PROPS
};

static GParamSpec *properties [N_PROPS];

/**
 * panel_omni_bar_get_popover:
 * @self: a #PanelOmniBar
 *
 * Returns: (transfer none) (nullable): a #GtkPopover or %NULL
 */
GtkPopover *
panel_omni_bar_get_popover (PanelOmniBar *self)
{
  PanelOmniBarPrivate *priv = panel_omni_bar_get_instance_private (self);

  g_return_val_if_fail (PANEL_IS_OMNI_BAR (self), NULL);

  return priv->popover;
}

void
panel_omni_bar_set_popover (PanelOmniBar *self,
                            GtkPopover   *popover)
{
  PanelOmniBarPrivate *priv = panel_omni_bar_get_instance_private (self);

  g_return_if_fail (PANEL_IS_OMNI_BAR (self));
  g_return_if_fail (!popover || GTK_IS_POPOVER (popover));

  if (popover == priv->popover)
    return;

  if (priv->popover)
    gtk_widget_unparent (GTK_WIDGET (priv->popover));

  priv->popover = popover;

  if (popover)
    gtk_widget_set_parent (GTK_WIDGET (popover), GTK_WIDGET (self));

  g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_POPOVER]);
}

/**
 * panel_omni_bar_new:
 *
 * Create a new #PanelOmniBar.
 *
 * Returns: (transfer full): a newly created #PanelOmniBar
 */
GtkWidget *
panel_omni_bar_new (void)
{
  return g_object_new (PANEL_TYPE_OMNI_BAR, NULL);
}

static void
panel_omni_bar_click_pressed_cb (PanelOmniBar    *self,
                                 int              n_pressed,
                                 double           x,
                                 double           y,
                                 GtkGestureClick *click)
{
  PanelOmniBarPrivate *priv = panel_omni_bar_get_instance_private (self);

  g_assert (PANEL_IS_OMNI_BAR (self));
  g_assert (GTK_IS_GESTURE_CLICK (click));

  if (priv->popover == NULL)
    return;

  gtk_popover_popup (priv->popover);
}

static void
panel_omni_bar_size_allocate (GtkWidget *widget,
                              int        width,
                              int        height,
                              int        baseline)
{
  PanelOmniBar *self = (PanelOmniBar *)widget;
  PanelOmniBarPrivate *priv = panel_omni_bar_get_instance_private (self);

  g_assert (PANEL_IS_OMNI_BAR (self));

  if (priv->popover)
    gtk_popover_present (priv->popover);
}

static void
panel_omni_bar_dispose (GObject *object)
{
  PanelOmniBar *self = (PanelOmniBar *)object;
  PanelOmniBarPrivate *priv = panel_omni_bar_get_instance_private (self);

  g_clear_pointer ((GtkWidget **)&priv->prefix, gtk_widget_unparent);
  g_clear_pointer ((GtkWidget **)&priv->center, gtk_widget_unparent);
  g_clear_pointer ((GtkWidget **)&priv->suffix, gtk_widget_unparent);
  g_clear_pointer ((GtkWidget **)&priv->popover, gtk_widget_unparent);

  G_OBJECT_CLASS (panel_omni_bar_parent_class)->dispose (object);
}

static void
panel_omni_bar_get_property (GObject    *object,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  PanelOmniBar *self = PANEL_OMNI_BAR (object);

  switch (prop_id)
    {
    case PROP_POPOVER:
      g_value_set_object (value, panel_omni_bar_get_popover (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
panel_omni_bar_set_property (GObject      *object,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  PanelOmniBar *self = PANEL_OMNI_BAR (object);

  switch (prop_id)
    {
    case PROP_POPOVER:
      panel_omni_bar_set_popover (self, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
panel_omni_bar_class_init (PanelOmniBarClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = panel_omni_bar_dispose;
  object_class->get_property = panel_omni_bar_get_property;
  object_class->set_property = panel_omni_bar_set_property;

  widget_class->size_allocate = panel_omni_bar_size_allocate;

  properties [PROP_POPOVER] =
    g_param_spec_object ("popover",
                         "Popover",
                         "Popover",
                         GTK_TYPE_POPOVER,
                         (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);

  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BOX_LAYOUT);
  gtk_widget_class_set_css_name (widget_class, "panelomnibar");
}

static void
panel_omni_bar_init (PanelOmniBar *self)
{
  PanelOmniBarPrivate *priv = panel_omni_bar_get_instance_private (self);
  GtkGesture *gesture;

  priv->prefix = g_object_new (GTK_TYPE_BOX, NULL);
  priv->center = g_object_new (GTK_TYPE_BOX,
                               "hexpand", TRUE,
                               NULL);
  priv->suffix = g_object_new (GTK_TYPE_BOX, NULL);

  gtk_widget_set_parent (GTK_WIDGET (priv->prefix), GTK_WIDGET (self));
  gtk_widget_set_parent (GTK_WIDGET (priv->center), GTK_WIDGET (self));
  gtk_widget_set_parent (GTK_WIDGET (priv->suffix), GTK_WIDGET (self));

  gesture = gtk_gesture_click_new ();
  g_signal_connect_object (gesture,
                           "pressed",
                           G_CALLBACK (panel_omni_bar_click_pressed_cb),
                           self,
                           G_CONNECT_SWAPPED);
  gtk_widget_add_controller (GTK_WIDGET (self), GTK_EVENT_CONTROLLER (gesture));
}

#define GET_PRIORITY(w)   GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w),"PRIORITY"))
#define SET_PRIORITY(w,i) g_object_set_data(G_OBJECT(w),"PRIORITY",GINT_TO_POINTER(i))

void
panel_omni_bar_add_prefix (PanelOmniBar *self,
                           int           priority,
                           GtkWidget    *widget)
{
  PanelOmniBarPrivate *priv = panel_omni_bar_get_instance_private (self);
  GtkWidget *sibling = NULL;

  g_return_if_fail (PANEL_IS_OMNI_BAR (self));
  g_return_if_fail (GTK_IS_WIDGET (widget));

  SET_PRIORITY (widget, priority);

  for (GtkWidget *child = gtk_widget_get_first_child (GTK_WIDGET (priv->prefix));
       child != NULL;
       child = gtk_widget_get_next_sibling (child))
    {
      if (priority < GET_PRIORITY(child))
        break;
      sibling = child;
    }

  gtk_box_insert_child_after (priv->prefix, widget, sibling);
}

void
panel_omni_bar_add_suffix (PanelOmniBar *self,
                           int           priority,
                           GtkWidget    *widget)
{
  PanelOmniBarPrivate *priv = panel_omni_bar_get_instance_private (self);
  GtkWidget *sibling = NULL;

  g_return_if_fail (PANEL_IS_OMNI_BAR (self));
  g_return_if_fail (GTK_IS_WIDGET (widget));

  SET_PRIORITY (widget, priority);

  for (GtkWidget *child = gtk_widget_get_first_child (GTK_WIDGET (priv->suffix));
       child != NULL;
       child = gtk_widget_get_next_sibling (child))
    {
      if (priority < GET_PRIORITY(child))
        break;
      sibling = child;
    }

  gtk_box_insert_child_after (priv->suffix, widget, sibling);
}

void
panel_omni_bar_remove (PanelOmniBar *self,
                       GtkWidget    *widget)
{
  PanelOmniBarPrivate *priv = panel_omni_bar_get_instance_private (self);
  GtkWidget *parent;

  g_return_if_fail (PANEL_IS_OMNI_BAR (self));
  g_return_if_fail (GTK_IS_WIDGET (widget));

  parent = gtk_widget_get_parent (widget);

  if (parent == GTK_WIDGET (priv->suffix) ||
      parent == GTK_WIDGET (priv->prefix))
    {
      gtk_box_remove (GTK_BOX (parent), widget);
      return;
    }

  /* TODO: Support removing internal things */
}

static void
panel_omni_bar_add_child (GtkBuildable *buildable,
                          GtkBuilder   *builder,
                          GObject      *child,
                          const char   *type)
{
  PanelOmniBar *self = (PanelOmniBar *)buildable;

  g_assert (GTK_IS_BUILDABLE (buildable));
  g_assert (GTK_IS_BUILDER (builder));
  g_assert (G_IS_OBJECT (child));

  if (!GTK_IS_WIDGET (child))
    {
      g_critical ("Attempted to add a non-widget to %s, which is not supported",
                  G_OBJECT_TYPE_NAME (self));
      return;
    }

  if (g_strcmp0 (type, "suffix") == 0)
    panel_omni_bar_add_suffix (self, 0, GTK_WIDGET (child));
  else
    panel_omni_bar_add_prefix (self, 0, GTK_WIDGET (child));
}

static void
buildable_iface_init (GtkBuildableIface *iface)
{
  iface->add_child = panel_omni_bar_add_child;
}
