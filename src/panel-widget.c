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
#include "panel-widget-private.h"

typedef struct
{
  GtkWidget         *child;
  GQuark             kind;
  char              *title;
  char              *icon_name;
  GIcon             *icon;
  GMenuModel        *menu_model;
  PanelSaveDelegate *save_delegate;

  GtkWidget         *maximize_frame;
  GtkWidget         *maximize_dock_child;

  GdkRGBA           foreground_rgba;
  GdkRGBA           background_rgba;

  guint             busy_count;

  guint             background_rgba_set : 1;
  guint             foreground_rgba_set : 1;
  guint             reorderable : 1;
  guint             can_maximize : 1;
  guint             maximized : 1;
  guint             modified : 1;
  guint             needs_attention : 1;
} PanelWidgetPrivate;

static void buildable_iface_init (GtkBuildableIface *iface);

G_DEFINE_TYPE_WITH_CODE (PanelWidget, panel_widget, GTK_TYPE_WIDGET,
                         G_ADD_PRIVATE  (PanelWidget)
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_BUILDABLE, buildable_iface_init))

enum {
  PROP_0,
  PROP_BACKGROUND_RGBA,
  PROP_BUSY,
  PROP_CAN_MAXIMIZE,
  PROP_CHILD,
  PROP_FOREGROUND_RGBA,
  PROP_ICON,
  PROP_ICON_NAME,
  PROP_MENU_MODEL,
  PROP_MODIFIED,
  PROP_NEEDS_ATTENTION,
  PROP_KIND,
  PROP_REORDERABLE,
  PROP_SAVE_DELEGATE,
  PROP_TITLE,
  N_PROPS
};

enum {
  GET_DEFAULT_FOCUS,
  N_SIGNALS
};

static GParamSpec *properties [N_PROPS];
static guint signals [N_SIGNALS];
static const GdkRGBA transparent;

static void
panel_widget_update_actions (PanelWidget *self)
{
  PanelWidgetPrivate *priv = panel_widget_get_instance_private (self);

  g_assert (PANEL_IS_WIDGET (self));

  gtk_widget_action_set_enabled (GTK_WIDGET (self),
                                 "page.maximize",
                                 !priv->maximized && panel_widget_get_can_maximize (self));
}

static void
panel_widget_maximize_action (GtkWidget  *widget,
                              const char *action_name,
                              GVariant   *param)
{
  panel_widget_maximize (PANEL_WIDGET (widget));
}

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
  g_clear_object (&priv->icon);
  g_clear_object (&priv->menu_model);
  g_clear_object (&priv->save_delegate);

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
    case PROP_BACKGROUND_RGBA:
      g_value_set_boxed (value, panel_widget_get_background_rgba (self));
      break;

    case PROP_FOREGROUND_RGBA:
      g_value_set_boxed (value, panel_widget_get_foreground_rgba (self));
      break;

    case PROP_BUSY:
      g_value_set_boolean (value, panel_widget_get_busy (self));
      break;

    case PROP_CAN_MAXIMIZE:
      g_value_set_boolean (value, panel_widget_get_can_maximize (self));
      break;

    case PROP_KIND:
      g_value_set_string (value, panel_widget_get_kind (self));
      break;

    case PROP_ICON:
      g_value_set_object (value, panel_widget_get_icon (self));
      break;

    case PROP_ICON_NAME:
      g_value_set_string (value, panel_widget_get_icon_name (self));
      break;

    case PROP_MENU_MODEL:
      g_value_set_object (value, panel_widget_get_menu_model (self));
      break;

    case PROP_MODIFIED:
      g_value_set_boolean (value, panel_widget_get_modified (self));
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

    case PROP_NEEDS_ATTENTION:
      g_value_set_boolean (value, panel_widget_get_needs_attention (self));
      break;

    case PROP_SAVE_DELEGATE:
      g_value_set_object (value, panel_widget_get_save_delegate (self));
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
    case PROP_BACKGROUND_RGBA:
      panel_widget_set_background_rgba (self, g_value_get_boxed (value));
      break;

    case PROP_FOREGROUND_RGBA:
      panel_widget_set_foreground_rgba (self, g_value_get_boxed (value));
      break;

    case PROP_CAN_MAXIMIZE:
      panel_widget_set_can_maximize (self, g_value_get_boolean (value));
      break;

    case PROP_KIND:
      panel_widget_set_kind (self, g_value_get_string (value));
      break;

    case PROP_ICON:
      panel_widget_set_icon (self, g_value_get_object (value));
      break;

    case PROP_ICON_NAME:
      panel_widget_set_icon_name (self, g_value_get_string (value));
      break;

    case PROP_MENU_MODEL:
      panel_widget_set_menu_model (self, g_value_get_object (value));
      break;

    case PROP_MODIFIED:
      panel_widget_set_modified (self, g_value_get_boolean (value));
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

    case PROP_NEEDS_ATTENTION:
      panel_widget_set_needs_attention (self, g_value_get_boolean (value));
      break;

    case PROP_SAVE_DELEGATE:
      panel_widget_set_save_delegate (self, g_value_get_object (value));
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

  /**
   * PanelWidget:background-rgba:
   *
   * The "background-rgba" property denotes a background rgba color that is
   * similar to the widget's contents.
   *
   * This can be used by #PanelFrameHeader implementations to more closely
   * match the content with accent colors. #PanelFrameHeaderBar uses this
   * to match the background of the bar with the content.
   */
  properties [PROP_BACKGROUND_RGBA] =
    g_param_spec_boxed ("background-rgba",
                        "Background RGBA",
                        "Background RGBA",
                        GDK_TYPE_RGBA,
                        (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  /**
   * PanelWidget:foreground-rgba:
   *
   * The "foreground-rgba" property denotes a foreground rgba color that is
   * similar to the widget's contents.
   *
   * This can be used by #PanelFrameHeader implementations to more closely
   * match the content with accent colors. #PanelFrameHeaderBar uses this
   * to match the foreground of the bar with the content.
   */
  properties [PROP_FOREGROUND_RGBA] =
    g_param_spec_boxed ("foreground-rgba",
                        "Foreground RGBA",
                        "Foreground RGBA",
                        GDK_TYPE_RGBA,
                        (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  properties [PROP_BUSY] =
    g_param_spec_boolean ("busy",
                          "Busy",
                          "If the widget is busy, such as loading or saving a file",
                          FALSE,
                          (G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  properties [PROP_CAN_MAXIMIZE] =
    g_param_spec_boolean ("can-maximize",
                          "Can Maximize",
                          "Can Maximize",
                          FALSE,
                          (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  /**
   * PanelWidget:icon:
   *
   * The icon for this widget.
   */
  properties [PROP_ICON] =
    g_param_spec_object ("icon",
                         "Icon",
                         "A GIcon for the panel",
                         G_TYPE_ICON,
                         (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY |G_PARAM_STATIC_STRINGS));

  /**
   * PanelWidget:icon-name:
   *
   * The icon name for this widget.
   */
  properties [PROP_ICON_NAME] =
    g_param_spec_string ("icon-name",
                         "Icon Name",
                         "Icon Name",
                         NULL,
                         (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  properties [PROP_KIND] =
    g_param_spec_string ("kind",
                         "Kind",
                         "The kind of panel widget",
                         "unknown",
                         (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  /**
   * PanelWidget:menu-model:
   *
   * A menu model to display additional options for the page to the user via
   * menus.
   */
  properties [PROP_MENU_MODEL] =
    g_param_spec_object ("menu-model",
                         "Menu Model",
                         "Menu Model",
                         G_TYPE_MENU_MODEL,
                         (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  properties [PROP_MODIFIED] =
    g_param_spec_boolean ("modified",
                          "Modified",
                          "If the widget contains unsaved state",
                          FALSE,
                          (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  /**
   * PanelWidget:title:
   *
   * The title for this widget.
   */
  properties [PROP_TITLE] =
    g_param_spec_string ("title",
                         "Title",
                         "Title",
                         NULL,
                         (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  /**
   * PanelWidget:child:
   *
   * The child inside this widget.
   */
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

  properties [PROP_NEEDS_ATTENTION] =
    g_param_spec_boolean ("needs-attention",
                          "Needs Attention",
                          "Needs Attention",
                          FALSE,
                          (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  /**
   * PanelWidget:save-delegate:
   *
   * The save delegate attached to this widget.
   */
  properties [PROP_SAVE_DELEGATE] =
    g_param_spec_object ("save-delegate",
                         "Save Delegate",
                         "A save delegate to perform a save operation on the page",
                         PANEL_TYPE_SAVE_DELEGATE,
                         (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);

  /**
   * PanelWidget::get-default-focus:
   * @self: a #PanelWidget
   *
   * Gets the default widget to focus within the #PanelWidget. The first
   * handler for this signal is expected to return a widget, or %NULL if
   * there is nothing to focus.
   *
   * Returns: (transfer none) (nullable): a #GtkWidget within #PanelWidget
   *   or %NULL.
   */
  signals [GET_DEFAULT_FOCUS] =
    g_signal_new ("get-default-focus",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (PanelWidgetClass, get_default_focus),
                  g_signal_accumulator_first_wins, NULL,
                  NULL,
                  GTK_TYPE_WIDGET, 0);

  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BIN_LAYOUT);
  gtk_widget_class_set_css_name (widget_class, "panelwidget");

  gtk_widget_class_install_action (widget_class, "page.maximize", NULL, panel_widget_maximize_action);

  /* Ensure we have quarks for known types */
  g_quark_from_static_string (PANEL_WIDGET_KIND_ANY);
  g_quark_from_static_string (PANEL_WIDGET_KIND_UNKNOWN);
  g_quark_from_static_string (PANEL_WIDGET_KIND_DOCUMENT);
  g_quark_from_static_string (PANEL_WIDGET_KIND_UTILITY);
}

static void
panel_widget_init (PanelWidget *self)
{
  PanelWidgetPrivate *priv = panel_widget_get_instance_private (self);

  panel_widget_update_actions (self);

  priv->kind = g_quark_from_static_string (PANEL_WIDGET_KIND_UNKNOWN);
  priv->reorderable = TRUE;
}

/**
 * panel_widget_get_icon_name:
 * @self: a #PanelWidget
 *
 * Gets the icon name for the widget.
 *
 * Returns: (transfer none) (nullable): the icon name or %NULL
 */
const char *
panel_widget_get_icon_name (PanelWidget *self)
{
  PanelWidgetPrivate *priv = panel_widget_get_instance_private (self);

  g_return_val_if_fail (PANEL_IS_WIDGET (self), NULL);

  if (priv->icon_name == NULL && G_IS_THEMED_ICON (priv->icon))
    {
      const char * const *names = g_themed_icon_get_names (G_THEMED_ICON (priv->icon));

      if (names != NULL && names[0] != NULL)
        return names[0];
    }

  return priv->icon_name;
}

/**
 * panel_widget_set_icon_name:
 * @self: a #PanelWidget
 * @icon_name: (transfer none) (nullable): the icon name or %NULL
 *
 * Sets the icon name for the widget.
 */
void
panel_widget_set_icon_name (PanelWidget *self,
                            const char  *icon_name)
{
  PanelWidgetPrivate *priv = panel_widget_get_instance_private (self);

  g_return_if_fail (PANEL_IS_WIDGET (self));

  if (g_strcmp0 (priv->icon_name, icon_name) != 0)
    {
      g_clear_object (&priv->icon);
      g_free (priv->icon_name);
      priv->icon_name = g_strdup (icon_name);
      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_ICON_NAME]);
      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_ICON]);
    }
}

/**
 * panel_widget_get_icon:
 * @self: a #PanelWidget
 *
 * Gets a #GIcon for the widget.
 *
 * Returns: (transfer none) (nullable): a #GIcon or %NULL
 */
GIcon *
panel_widget_get_icon (PanelWidget *self)
{
  PanelWidgetPrivate *priv = panel_widget_get_instance_private (self);

  g_return_val_if_fail (PANEL_IS_WIDGET (self), NULL);

  if (priv->icon == NULL && priv->icon_name != NULL)
    priv->icon = g_themed_icon_new (priv->icon_name);

  return priv->icon;
}

/**
 * panel_widget_set_icon:
 * @self: a #PanelWidget
 * @icon: (transfer none) (nullable): a #GIcon or %NULL
 *
 * Sets a #GIcon for the widget.
 */
void
panel_widget_set_icon (PanelWidget *self,
                       GIcon       *icon)
{
  PanelWidgetPrivate *priv = panel_widget_get_instance_private (self);

  g_return_if_fail (PANEL_IS_WIDGET (self));
  g_return_if_fail (!icon || G_IS_ICON (icon));

  if (g_set_object (&priv->icon, icon))
    {
      if (priv->icon_name != NULL)
        {
          g_clear_pointer (&priv->icon_name, g_free);
          g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_ICON_NAME]);
        }

      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_ICON]);
    }
}

gboolean
panel_widget_get_modified (PanelWidget *self)
{
  PanelWidgetPrivate *priv = panel_widget_get_instance_private (self);

  g_return_val_if_fail (PANEL_IS_WIDGET (self), FALSE);

  return priv->modified;
}

void
panel_widget_set_modified (PanelWidget *self,
                           gboolean     modified)
{
  PanelWidgetPrivate *priv = panel_widget_get_instance_private (self);

  g_return_if_fail (PANEL_IS_WIDGET (self));

  modified = !!modified;

  if (priv->modified != modified)
    {
      priv->modified = modified;
      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_MODIFIED]);
    }
}

/**
 * panel_widget_get_title:
 * @self: a #PanelWidget
 *
 * Gets the title for the widget.
 *
 * Returns: (transfer none) (nullable): the title or %NULL
 */
const char *
panel_widget_get_title (PanelWidget *self)
{
  PanelWidgetPrivate *priv = panel_widget_get_instance_private (self);

  g_return_val_if_fail (PANEL_IS_WIDGET (self), NULL);

  return priv->title;
}

/**
 * panel_widget_set_title:
 * @self: a #PanelWidget
 * @title: (transfer none) (nullable): the title or %NULL
 *
 * Sets the title for the widget.
 */
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

/**
 * panel_widget_set_child:
 * @self: a #PanelWidget
 * @child: (nullable): a #GtkWidget or %NULL
 *
 * Sets the child widget of the panel.
 */
void
panel_widget_set_child (PanelWidget *self,
                        GtkWidget   *child)
{
  PanelWidgetPrivate *priv = panel_widget_get_instance_private (self);

  g_return_if_fail (PANEL_IS_WIDGET (self));
  g_return_if_fail (!child || GTK_IS_WIDGET (child));

  if (priv->child == child)
    return;

  if (priv->child)
    gtk_widget_unparent (priv->child);
  priv->child = child;
  if (priv->child)
    gtk_widget_set_parent (priv->child, GTK_WIDGET (self));

  g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_CHILD]);
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
      panel_widget_update_actions (self);
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

  panel_widget_update_actions (self);

  g_object_ref (self);

  g_set_weak_pointer (&priv->maximize_frame, frame);
  g_set_weak_pointer (&priv->maximize_dock_child, dock_child);

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

  priv->maximized = FALSE;

  panel_widget_update_actions (self);

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

const char *
panel_widget_get_kind (PanelWidget *self)
{
  PanelWidgetPrivate *priv = panel_widget_get_instance_private (self);

  g_return_val_if_fail (PANEL_IS_WIDGET (self), NULL);

  return g_quark_to_string (priv->kind);
}

/**
 * panel_widget_set_kind:
 * @self: a #PanelWidget
 * @kind: (nullable): the kind of this widget
 *
 * Sets the kind of the widget.
 */
void
panel_widget_set_kind (PanelWidget *self,
                       const char  *kind)
{
  PanelWidgetPrivate *priv = panel_widget_get_instance_private (self);
  GQuark qkind;

  g_return_if_fail (PANEL_IS_WIDGET (self));

  if (kind == NULL)
    kind = PANEL_WIDGET_KIND_UNKNOWN;
  qkind = g_quark_from_static_string (kind);

  if (qkind != priv->kind)
    {
      priv->kind = qkind;
      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_KIND]);
    }
}

gboolean
panel_widget_get_needs_attention (PanelWidget *self)
{
  PanelWidgetPrivate *priv = panel_widget_get_instance_private (self);

  g_return_val_if_fail (PANEL_IS_WIDGET (self), FALSE);

  return priv->needs_attention;
}

void
panel_widget_set_needs_attention (PanelWidget *self,
                                  gboolean     needs_attention)
{
  PanelWidgetPrivate *priv = panel_widget_get_instance_private (self);

  g_return_if_fail (PANEL_IS_WIDGET (self));

  needs_attention = !!needs_attention;

  if (priv->needs_attention != needs_attention)
    {
      priv->needs_attention = needs_attention;
      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_NEEDS_ATTENTION]);
    }
}

gboolean
panel_widget_get_busy (PanelWidget *self)
{
  PanelWidgetPrivate *priv = panel_widget_get_instance_private (self);

  g_return_val_if_fail (PANEL_IS_WIDGET (self), FALSE);

  return priv->busy_count > 0;
}

void
panel_widget_mark_busy (PanelWidget *self)
{
  PanelWidgetPrivate *priv = panel_widget_get_instance_private (self);

  g_return_if_fail (PANEL_IS_WIDGET (self));

  priv->busy_count++;

  if (priv->busy_count == 1)
    g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_BUSY]);
}

void
panel_widget_unmark_busy (PanelWidget *self)
{
  PanelWidgetPrivate *priv = panel_widget_get_instance_private (self);

  g_return_if_fail (PANEL_IS_WIDGET (self));

  priv->busy_count--;

  if (priv->busy_count == 0)
    g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_BUSY]);
}

/**
 * panel_widget_get_background_rgba:
 * @self: a #PanelWidget
 *
 * Gets the background color of the widget.
 *
 * Returns: (nullable): the background color
 */
const GdkRGBA *
panel_widget_get_background_rgba (PanelWidget *self)
{
  PanelWidgetPrivate *priv = panel_widget_get_instance_private (self);

  g_return_val_if_fail (PANEL_IS_WIDGET (self), NULL);

  return priv->background_rgba_set ? &priv->background_rgba : NULL;
}

/**
 * panel_widget_set_background_rgba:
 * @self: a #PanelWidget
 * @background_rgba: (nullable): the background color
 *
 * Sets the background color of the widget.
 */
void
panel_widget_set_background_rgba (PanelWidget   *self,
                                  const GdkRGBA *background_rgba)
{
  PanelWidgetPrivate *priv = panel_widget_get_instance_private (self);

  g_return_if_fail (PANEL_IS_WIDGET (self));

  priv->background_rgba_set = background_rgba != NULL;

  if (background_rgba == NULL)
    background_rgba = &transparent;

  if (!gdk_rgba_equal (background_rgba, &priv->background_rgba))
    {
      priv->background_rgba = *background_rgba;
      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_BACKGROUND_RGBA]);
    }
}

/**
 * panel_widget_get_foreground_rgba:
 * @self: a #PanelWidget
 *
 * Gets the foreground color of the widget.
 *
 * Returns: (nullable): the foreground color
 */
const GdkRGBA *
panel_widget_get_foreground_rgba (PanelWidget *self)
{
  PanelWidgetPrivate *priv = panel_widget_get_instance_private (self);

  g_return_val_if_fail (PANEL_IS_WIDGET (self), NULL);

  return priv->foreground_rgba_set ? &priv->foreground_rgba : NULL;
}

/**
 * panel_widget_set_foreground_rgba:
 * @self: a #PanelWidget
 * @foreground_rgba: (nullable): the foreground color
 *
 * Sets the foreground color of the widget.
 */
void
panel_widget_set_foreground_rgba (PanelWidget   *self,
                                  const GdkRGBA *foreground_rgba)
{
  PanelWidgetPrivate *priv = panel_widget_get_instance_private (self);

  g_return_if_fail (PANEL_IS_WIDGET (self));

  priv->foreground_rgba_set = foreground_rgba != NULL;

  if (foreground_rgba == NULL)
    foreground_rgba = &transparent;

  if (!gdk_rgba_equal (foreground_rgba, &priv->foreground_rgba))
    {
      priv->foreground_rgba = *foreground_rgba;
      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_FOREGROUND_RGBA]);
    }
}

/**
 * panel_widget_get_menu_model:
 * @self: a #PanelWidget
 *
 * Gets the #GMenuModel for the widget.
 *
 * #PanelFrameHeader may use this model to display additional options
 * for the page to the user via menus.
 *
 * Returns: (transfer none) (nullable): a #GMenuModel
 */
GMenuModel *
panel_widget_get_menu_model (PanelWidget *self)
{
  PanelWidgetPrivate *priv = panel_widget_get_instance_private (self);

  g_return_val_if_fail (PANEL_IS_WIDGET (self), NULL);

  return priv->menu_model;
}

/**
 * panel_widget_set_menu_model:
 * @self: a #PanelWidget
 * @menu_model: (transfer none) (nullable): a #GMenuModel
 *
 * Sets the #GMenuModel for the widget.
 *
 * #PanelFrameHeader may use this model to display additional options
 * for the page to the user via menus.
 */
void
panel_widget_set_menu_model (PanelWidget *self,
                             GMenuModel  *menu_model)
{
  PanelWidgetPrivate *priv = panel_widget_get_instance_private (self);

  g_return_if_fail (PANEL_IS_WIDGET (self));
  g_return_if_fail (!menu_model || G_IS_MENU_MODEL (menu_model));

  if (g_set_object (&priv->menu_model, menu_model))
    g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_MENU_MODEL]);
}

void
panel_widget_raise (PanelWidget *self)
{
  GtkWidget *frame;

  g_return_if_fail (PANEL_IS_WIDGET (self));

  if ((frame = gtk_widget_get_ancestor (GTK_WIDGET (self), PANEL_TYPE_FRAME)))
    {
      GtkWidget *dock_child;
      GtkWidget *dock;

      panel_frame_set_visible_child (PANEL_FRAME (frame), self);

      if ((dock_child = gtk_widget_get_ancestor (frame, PANEL_TYPE_DOCK_CHILD)) &&
          (dock = gtk_widget_get_ancestor (dock_child, PANEL_TYPE_DOCK)))
        {
          switch (panel_dock_child_get_position (PANEL_DOCK_CHILD (dock_child)))
            {
            case PANEL_DOCK_POSITION_END:
              panel_dock_set_reveal_end (PANEL_DOCK (dock), TRUE);
              break;

            case PANEL_DOCK_POSITION_START:
              panel_dock_set_reveal_start (PANEL_DOCK (dock), TRUE);
              break;

            case PANEL_DOCK_POSITION_TOP:
              panel_dock_set_reveal_top (PANEL_DOCK (dock), TRUE);
              break;

            case PANEL_DOCK_POSITION_BOTTOM:
              panel_dock_set_reveal_bottom (PANEL_DOCK (dock), TRUE);
              break;

            case PANEL_DOCK_POSITION_CENTER:
            default:
              break;
            }
        }
    }
}

/**
 * panel_widget_get_default_focus:
 * @self: a #PanelWidget
 *
 * Discovers the widget that should be focused as the default widget
 * for the #PanelWidget.
 *
 * For example, if you want to focus a text editor by default, you might
 * return the #GtkTextView inside your widgetry.
 *
 * Returns: (transfer none) (nullable): the default widget to focus within
 *   the #PanelWidget.
 */
GtkWidget *
panel_widget_get_default_focus (PanelWidget *self)
{
  GtkWidget *default_focus = NULL;

  g_return_val_if_fail (PANEL_IS_WIDGET (self), NULL);

  g_signal_emit (self, signals [GET_DEFAULT_FOCUS], 0, &default_focus);

  g_return_val_if_fail (default_focus == NULL ||
                        GTK_WIDGET (self) == default_focus ||
                        gtk_widget_is_ancestor (default_focus, GTK_WIDGET (self)),
                        NULL);

  return default_focus;
}

gboolean
panel_widget_focus_default (PanelWidget *self)
{
  GtkWidget *default_focus;

  g_return_val_if_fail (PANEL_IS_WIDGET (self), FALSE);

  if ((default_focus = panel_widget_get_default_focus (self)))
    return gtk_widget_grab_focus (default_focus);

  return FALSE;
}

static void
panel_widget_add_child (GtkBuildable *buildable,
                        GtkBuilder   *builder,
                        GObject      *child,
                        const char   *type)
{
  g_assert (PANEL_IS_WIDGET (buildable));
  g_assert (GTK_IS_BUILDER (builder));
  g_assert (G_IS_OBJECT (child));

  if (GTK_IS_WIDGET (child))
    panel_widget_set_child (PANEL_WIDGET (buildable), GTK_WIDGET (child));
}

static void
buildable_iface_init (GtkBuildableIface *iface)
{
  iface->add_child = panel_widget_add_child;
}

/**
 * panel_widget_get_save_delegate:
 * @self: a #PanelWidget
 *
 * Gets the #PanelWidget:save-delegate property.
 *
 * The save delegate may be used to perform save operations on the
 * content within the widget.
 *
 * Document editors might use this to save the file to disk.
 *
 * Returns: (transfer none) (nullable): a #PanelSaveDelegate or %NULL
 */
PanelSaveDelegate *
panel_widget_get_save_delegate (PanelWidget *self)
{
  PanelWidgetPrivate *priv = panel_widget_get_instance_private (self);

  g_return_val_if_fail (PANEL_IS_WIDGET (self), NULL);

  return priv->save_delegate;
}

/**
 * panel_widget_set_save_delegate:
 * @self: a #PanelWidget
 * @save_delegate: (transfer none) (nullable): a #PanelSaveDelegate or %NULL
 *
 * Sets the #PanelWidget:save-delegate property.
 *
 * The save delegate may be used to perform save operations on the
 * content within the widget.
 *
 * Document editors might use this to save the file to disk.
 */
void
panel_widget_set_save_delegate (PanelWidget       *self,
                                PanelSaveDelegate *save_delegate)
{
  PanelWidgetPrivate *priv = panel_widget_get_instance_private (self);

  g_return_if_fail (PANEL_IS_WIDGET (self));
  g_return_if_fail (!save_delegate || PANEL_IS_SAVE_DELEGATE (save_delegate));

  if (g_set_object (&priv->save_delegate, save_delegate))
    g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_SAVE_DELEGATE]);
}

gboolean
_panel_widget_can_save (PanelWidget *self)
{
  PanelWidgetPrivate *priv = panel_widget_get_instance_private (self);

  g_return_val_if_fail (PANEL_IS_WIDGET (self), FALSE);

  return priv->modified && priv->save_delegate != NULL;
}
