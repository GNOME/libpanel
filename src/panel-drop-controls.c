/* panel-drop-controls.c
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

#include "panel-dock-private.h"
#include "panel-drop-controls-private.h"
#include "panel-enums.h"

struct _PanelDropControls
{
  GtkWidget          parent_instance;

  GtkWidget         *child;
  GtkImage          *top;
  GtkImage          *bottom;
  GtkImage          *left;
  GtkImage          *right;
  GtkImage          *center;

  PanelDock         *dock;

  PanelDockPosition  position;
};

G_DEFINE_TYPE (PanelDropControls, panel_drop_controls, GTK_TYPE_WIDGET)

enum {
  PROP_0,
  PROP_POSITION,
  N_PROPS
};

static GParamSpec *properties [N_PROPS];

GtkWidget *
panel_drop_controls_new (void)
{
  return g_object_new (PANEL_TYPE_DROP_CONTROLS, NULL);
}

void
panel_drop_controls_set_position (PanelDropControls *self,
                                  PanelDockPosition  position)
{
  g_return_if_fail (PANEL_IS_DROP_CONTROLS (self));
  g_return_if_fail (position <= PANEL_DOCK_POSITION_CENTER);

  self->position = position;

  switch (self->position)
    {
    case PANEL_DOCK_POSITION_START:
    case PANEL_DOCK_POSITION_END:
      gtk_widget_show (GTK_WIDGET (self->top));
      gtk_widget_show (GTK_WIDGET (self->bottom));
      gtk_widget_show (GTK_WIDGET (self->center));
      gtk_widget_hide (GTK_WIDGET (self->left));
      gtk_widget_hide (GTK_WIDGET (self->right));
      break;

    case PANEL_DOCK_POSITION_TOP:
    case PANEL_DOCK_POSITION_BOTTOM:
      gtk_widget_hide (GTK_WIDGET (self->top));
      gtk_widget_hide (GTK_WIDGET (self->bottom));
      gtk_widget_show (GTK_WIDGET (self->center));
      gtk_widget_show (GTK_WIDGET (self->left));
      gtk_widget_show (GTK_WIDGET (self->right));
      break;

    case PANEL_DOCK_POSITION_CENTER:
      gtk_widget_show (GTK_WIDGET (self->center));
      gtk_widget_show (GTK_WIDGET (self->top));
      gtk_widget_show (GTK_WIDGET (self->bottom));
      gtk_widget_show (GTK_WIDGET (self->left));
      gtk_widget_show (GTK_WIDGET (self->right));
      break;

    default:
      g_assert_not_reached ();
    }
}

PanelDockPosition
panel_drop_controls_get_position (PanelDropControls *self)
{
  g_return_val_if_fail (PANEL_IS_DROP_CONTROLS (self), 0);

  return self->position;
}

static void
panel_drop_controls_root (GtkWidget *widget)
{
  PanelDropControls *self = (PanelDropControls *)widget;
  GtkWidget *dock;

  g_assert (PANEL_IS_DROP_CONTROLS (self));

  if (!(dock = gtk_widget_get_ancestor (widget, PANEL_TYPE_DOCK)))
    {
      g_warning ("%s added without a dock, this cannot work.",
                 G_OBJECT_TYPE_NAME (self));
      return;
    }

  self->dock = PANEL_DOCK (dock);
}

static void
panel_drop_controls_unroot (GtkWidget *widget)
{
  PanelDropControls *self = (PanelDropControls *)widget;

  g_assert (PANEL_IS_DROP_CONTROLS (self));

  self->dock = NULL;
}

static void
panel_drop_controls_dispose (GObject *object)
{
  PanelDropControls *self = (PanelDropControls *)object;

  g_clear_pointer (&self->child, gtk_widget_unparent);

  G_OBJECT_CLASS (panel_drop_controls_parent_class)->dispose (object);
}

static void
panel_drop_controls_get_property (GObject    *object,
                                  guint       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  PanelDropControls *self = PANEL_DROP_CONTROLS (object);

  switch (prop_id)
    {
    case PROP_POSITION:
      g_value_set_enum (value, panel_drop_controls_get_position (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
panel_drop_controls_set_property (GObject      *object,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  PanelDropControls *self = PANEL_DROP_CONTROLS (object);

  switch (prop_id)
    {
    case PROP_POSITION:
      panel_drop_controls_set_position (self, g_value_get_enum (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
panel_drop_controls_class_init (PanelDropControlsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = panel_drop_controls_dispose;
  object_class->get_property = panel_drop_controls_get_property;
  object_class->set_property = panel_drop_controls_set_property;

  widget_class->root = panel_drop_controls_root;
  widget_class->unroot = panel_drop_controls_unroot;

  properties [PROP_POSITION] =
    g_param_spec_enum ("position",
                       "Position",
                       "The position of the drop controls",
                       PANEL_TYPE_DOCK_POSITION,
                       PANEL_DOCK_POSITION_CENTER,
                       (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);

  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BIN_LAYOUT);
  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/libpanel/panel-drop-controls.ui");
  gtk_widget_class_set_css_name (widget_class, "paneldropcontrols");

  gtk_widget_class_bind_template_child (widget_class, PanelDropControls, child);
  gtk_widget_class_bind_template_child (widget_class, PanelDropControls, left);
  gtk_widget_class_bind_template_child (widget_class, PanelDropControls, right);
  gtk_widget_class_bind_template_child (widget_class, PanelDropControls, top);
  gtk_widget_class_bind_template_child (widget_class, PanelDropControls, bottom);
  gtk_widget_class_bind_template_child (widget_class, PanelDropControls, center);
}

static void
panel_drop_controls_init (PanelDropControls *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}
