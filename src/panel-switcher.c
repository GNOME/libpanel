/* panel-switcher.c
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

#include "panel-switcher.h"

#define TIMEOUT_EXPAND 500

struct _PanelSwitcher
{
  GtkWidget        parent_instance;

  PanelDock       *dock;

  GtkWidget       *box;
  GtkToggleButton *start_button;
  GtkToggleButton *end_button;
  GtkToggleButton *top_button;
  GtkToggleButton *bottom_button;
  GtkRevealer     *start_revealer;
  GtkRevealer     *end_revealer;
  GtkRevealer     *top_revealer;
  GtkRevealer     *bottom_revealer;

  GBinding        *top_binding;
  GBinding        *bottom_binding;
  GBinding        *start_binding;
  GBinding        *end_binding;
};

G_DEFINE_TYPE (PanelSwitcher, panel_switcher, GTK_TYPE_WIDGET)

enum {
  PROP_0,
  PROP_DOCK,
  N_PROPS
};

static GParamSpec *properties [N_PROPS];

GtkWidget *
panel_switcher_new (void)
{
  return g_object_new (PANEL_TYPE_SWITCHER, NULL);
}

static gboolean
panel_switcher_switch_timeout (gpointer data)
{
  GtkToggleButton *button = data;

  g_assert (GTK_IS_TOGGLE_BUTTON (button));

  g_object_steal_data (G_OBJECT (button), "-panel-switch-timer");
  gtk_toggle_button_set_active (button, TRUE);

  return G_SOURCE_REMOVE;
}

static void
drag_enter_cb (GtkDropControllerMotion *motion,
               double                   x,
               double                   y,
               G_GNUC_UNUSED gpointer   unused)
{
  GtkWidget *button;

  g_assert (GTK_IS_DROP_CONTROLLER_MOTION (motion));

  button = gtk_event_controller_get_widget (GTK_EVENT_CONTROLLER (motion));

  if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
    {
      guint switch_timer = g_timeout_add_full (G_PRIORITY_DEFAULT,
                                               TIMEOUT_EXPAND,
                                               panel_switcher_switch_timeout,
                                               g_object_ref (button),
                                               g_object_unref);
      g_source_set_name_by_id (switch_timer, "[panel] panel_switcher_switch_timeout");
      g_object_set_data (G_OBJECT (button), "-panel-switch-timer", GUINT_TO_POINTER (switch_timer));
    }
}

static void
drag_leave_cb (GtkDropControllerMotion *motion,
               G_GNUC_UNUSED gpointer   unused)
{
  GtkWidget *button;
  guint switch_timer;

  g_assert (GTK_IS_DROP_CONTROLLER_MOTION (motion));

  button = gtk_event_controller_get_widget (GTK_EVENT_CONTROLLER (motion));
  switch_timer = GPOINTER_TO_UINT (g_object_steal_data (G_OBJECT (button), "-panel-switch-timer"));
  if (switch_timer)
    g_source_remove (switch_timer);
}

static void
notify_child_revealed_cb (GtkRevealer            *revealer,
                          GParamSpec             *pspec,
                          G_GNUC_UNUSED gpointer  user_data)
{
  g_assert (GTK_IS_REVEALER (revealer));

  if (!gtk_revealer_get_child_revealed (revealer))
    gtk_widget_hide (GTK_WIDGET (revealer));
}

static void
panel_switcher_dispose (GObject *object)
{
  PanelSwitcher *self = (PanelSwitcher *)object;

  panel_switcher_set_dock (self, NULL);

  g_clear_pointer (&self->box, gtk_widget_unparent);

  G_OBJECT_CLASS (panel_switcher_parent_class)->dispose (object);
}

static void
panel_switcher_get_property (GObject    *object,
                                  guint       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  PanelSwitcher *self = PANEL_SWITCHER (object);

  switch (prop_id)
    {
    case PROP_DOCK:
      g_value_set_object (value, panel_switcher_get_dock (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
panel_switcher_set_property (GObject      *object,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  PanelSwitcher *self = PANEL_SWITCHER (object);

  switch (prop_id)
    {
    case PROP_DOCK:
      panel_switcher_set_dock (self, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
panel_switcher_class_init (PanelSwitcherClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = panel_switcher_dispose;
  object_class->get_property = panel_switcher_get_property;
  object_class->set_property = panel_switcher_set_property;

  properties [PROP_DOCK] =
    g_param_spec_object ("dock",
                         "Dock",
                         "The dock to switch panels for",
                         PANEL_TYPE_DOCK,
                         (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);

  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BIN_LAYOUT);
  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/libpanel/panel-switcher.ui");
  gtk_widget_class_set_css_name (widget_class, "dockswitcher");
  gtk_widget_class_bind_template_child (widget_class, PanelSwitcher, box);
  gtk_widget_class_bind_template_child (widget_class, PanelSwitcher, start_button);
  gtk_widget_class_bind_template_child (widget_class, PanelSwitcher, end_button);
  gtk_widget_class_bind_template_child (widget_class, PanelSwitcher, top_button);
  gtk_widget_class_bind_template_child (widget_class, PanelSwitcher, bottom_button);
  gtk_widget_class_bind_template_child (widget_class, PanelSwitcher, start_revealer);
  gtk_widget_class_bind_template_child (widget_class, PanelSwitcher, end_revealer);
  gtk_widget_class_bind_template_child (widget_class, PanelSwitcher, top_revealer);
  gtk_widget_class_bind_template_child (widget_class, PanelSwitcher, bottom_revealer);
  gtk_widget_class_bind_template_callback (widget_class, drag_enter_cb);
  gtk_widget_class_bind_template_callback (widget_class, drag_leave_cb);
  gtk_widget_class_bind_template_callback (widget_class, notify_child_revealed_cb);
}

static void
panel_switcher_init (PanelSwitcher *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

/**
 * panel_switcher_get_dock:
 * @self: a #PanelSwitcher
 *
 * Gets #PanelSwitcher:dock property.
 *
 * Returns: (transfer none) (nullable): a #PanelDock or %NULL
 */
PanelDock *
panel_switcher_get_dock (PanelSwitcher *self)
{
  g_return_val_if_fail (PANEL_IS_SWITCHER (self), NULL);

  return self->dock;
}

static void
panel_switcher_notify_can_reveal_cb (PanelDock   *dock,
                                          GParamSpec  *pspec,
                                          GtkRevealer *revealer)
{
  g_auto(GValue) value = G_VALUE_INIT;

  g_assert (PANEL_IS_DOCK (dock));
  g_assert (G_IS_PARAM_SPEC_BOOLEAN (pspec));
  g_assert (GTK_IS_REVEALER (revealer));

  g_value_init (&value, G_TYPE_BOOLEAN);
  g_object_get_property (G_OBJECT (dock), pspec->name, &value);

  if (g_value_get_boolean (&value))
    {
      gtk_widget_show (GTK_WIDGET (revealer));
      gtk_revealer_set_reveal_child (revealer, TRUE);
    }
  else
    {
      gtk_revealer_set_reveal_child (revealer, FALSE);
    }
}

void
panel_switcher_set_dock (PanelSwitcher *self,
                              PanelDock         *dock)
{
  g_return_if_fail (PANEL_IS_SWITCHER (self));
  g_return_if_fail (!dock || PANEL_IS_DOCK (dock));

  if (dock == self->dock)
    return;

  if (self->dock)
    {
      g_clear_pointer (&self->top_binding, g_binding_unbind);
      g_clear_pointer (&self->bottom_binding, g_binding_unbind);
      g_clear_pointer (&self->start_binding, g_binding_unbind);
      g_clear_pointer (&self->end_binding, g_binding_unbind);
      g_signal_handlers_disconnect_by_func (self->dock,
                                            G_CALLBACK (panel_switcher_notify_can_reveal_cb),
                                            self->top_revealer);
      g_signal_handlers_disconnect_by_func (self->dock,
                                            G_CALLBACK (panel_switcher_notify_can_reveal_cb),
                                            self->bottom_revealer);
      g_signal_handlers_disconnect_by_func (self->dock,
                                            G_CALLBACK (panel_switcher_notify_can_reveal_cb),
                                            self->start_revealer);
      g_signal_handlers_disconnect_by_func (self->dock,
                                            G_CALLBACK (panel_switcher_notify_can_reveal_cb),
                                            self->end_revealer);
    }

  g_set_object (&self->dock, dock);

  if (self->dock)
    {
      /* Set default state before binding */
      gtk_toggle_button_set_active (self->top_button,
                                    panel_dock_get_reveal_top (self->dock));
      gtk_toggle_button_set_active (self->bottom_button,
                                    panel_dock_get_reveal_bottom (self->dock));
      gtk_toggle_button_set_active (self->start_button,
                                    panel_dock_get_reveal_start (self->dock));
      gtk_toggle_button_set_active (self->end_button,
                                    panel_dock_get_reveal_end (self->dock));
      gtk_revealer_set_reveal_child (self->top_revealer,
                                     panel_dock_get_can_reveal_top (self->dock));
      gtk_revealer_set_reveal_child (self->bottom_revealer,
                                     panel_dock_get_can_reveal_bottom (self->dock));
      gtk_revealer_set_reveal_child (self->start_revealer,
                                     panel_dock_get_can_reveal_start (self->dock));
      gtk_revealer_set_reveal_child (self->end_revealer,
                                     panel_dock_get_can_reveal_end (self->dock));

      self->top_binding = g_object_bind_property (self->top_button, "active",
                                                  self->dock, "reveal-top",
                                                  G_BINDING_BIDIRECTIONAL);
      self->bottom_binding = g_object_bind_property (self->bottom_button, "active",
                                                     self->dock, "reveal-bottom",
                                                     G_BINDING_BIDIRECTIONAL);
      self->start_binding = g_object_bind_property (self->start_button, "active",
                                                    self->dock, "reveal-start",
                                                    G_BINDING_BIDIRECTIONAL);
      self->end_binding = g_object_bind_property (self->end_button, "active",
                                                  self->dock, "reveal-end",
                                                  G_BINDING_BIDIRECTIONAL);
      g_signal_connect_object (self->dock,
                               "notify::can-reveal-top",
                               G_CALLBACK (panel_switcher_notify_can_reveal_cb),
                               self->top_revealer,
                               0);
      g_signal_connect_object (self->dock,
                               "notify::can-reveal-bottom",
                               G_CALLBACK (panel_switcher_notify_can_reveal_cb),
                               self->bottom_revealer,
                               0);
      g_signal_connect_object (self->dock,
                               "notify::can-reveal-start",
                               G_CALLBACK (panel_switcher_notify_can_reveal_cb),
                               self->start_revealer,
                               0);
      g_signal_connect_object (self->dock,
                               "notify::can-reveal-end",
                               G_CALLBACK (panel_switcher_notify_can_reveal_cb),
                               self->end_revealer,
                               0);
    }

  g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_DOCK]);
}
