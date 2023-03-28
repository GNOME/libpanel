/* panel-document-workspace.c
 *
 * Copyright 2023 Christian Hergert <chergert@redhat.com>
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

#include "panel-document-workspace.h"

typedef struct
{
  PanelGrid *grid;
  PanelDock *dock;
} PanelDocumentWorkspacePrivate;

enum {
  PROP_0,
  PROP_DOCK,
  PROP_GRID,
  N_PROPS
};

G_DEFINE_TYPE_WITH_PRIVATE (PanelDocumentWorkspace, panel_document_workspace, G_TYPE_OBJECT)

static GParamSpec *properties [N_PROPS];

static void
panel_document_workspace_dispose (GObject *object)
{
  G_OBJECT_CLASS (panel_document_workspace_parent_class)->dispose (object);
}

static void
panel_document_workspace_get_property (GObject    *object,
                                       guint       prop_id,
                                       GValue     *value,
                                       GParamSpec *pspec)
{
  PanelDocumentWorkspace *self = PANEL_DOCUMENT_WORKSPACE (object);

  switch (prop_id)
    {
    case PROP_DOCK:
      g_value_set_object (value, panel_document_workspace_get_dock (self));
      break;

    case PROP_GRID:
      g_value_set_object (value, panel_document_workspace_get_grid (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
panel_document_workspace_set_property (GObject      *object,
                                       guint         prop_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  switch (prop_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
panel_document_workspace_class_init (PanelDocumentWorkspaceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = panel_document_workspace_dispose;
  object_class->get_property = panel_document_workspace_get_property;
  object_class->set_property = panel_document_workspace_set_property;

  properties[PROP_DOCK] =
    g_param_spec_object ("dock", NULL, NULL,
                         PANEL_TYPE_DOCK,
                         (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  properties[PROP_GRID] =
    g_param_spec_object ("grid", NULL, NULL,
                         PANEL_TYPE_GRID,
                         (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);

  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/libpanel/panel-document-workspace.ui");
  gtk_widget_class_bind_template_child_private (widget_class, PanelDocumentWorkspace, dock);
  gtk_widget_class_bind_template_child_private (widget_class, PanelDocumentWorkspace, grid);
}

static void
panel_document_workspace_init (PanelDocumentWorkspace *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

/**
 * panel_document_workspace_new:
 * @self: a #PanelDocumentWorkspace
 *
 * Creates a new #PanelDocumentWorkspace.
 *
 * Returns: (transfer full) (type PanelDocumentWorkspace): a #PanelDocumentWorkspace
 *
 * Since: 1.4
 */
GtkWidget *
panel_document_workspace_new (void)
{
  return g_object_new (PANEL_TYPE_DOCUMENT_WORKSPACE, NULL);
}

/**
 * panel_document_workspace_get_dock:
 * @self: a #PanelDocumentWorkspace
 *
 * Get the #PanelDock for the workspace.
 *
 * Returns: (transfer none): a #PanelDock
 *
 * Since: 1.4
 */
PanelDock *
panel_document_workspace_get_dock (PanelDocumentWorkspace *self)
{
  PanelDocumentWorkspacePrivate *priv = panel_document_workspace_get_instance_private (self);

  g_return_val_if_fail (PANEL_IS_DOCUMENT_WORKSPACE (self), NULL);

  return priv->dock;
}

/**
 * panel_document_workspace_get_grid:
 * @self: a #PanelDocumentWorkspace
 *
 * Get the document grid for the workspace.
 *
 * Returns: (transfer none): a #PanelGrid
 *
 * Since: 1.4
 */
PanelGrid *
panel_document_workspace_get_grid (PanelDocumentWorkspace *self)
{
  PanelDocumentWorkspacePrivate *priv = panel_document_workspace_get_instance_private (self);

  g_return_val_if_fail (PANEL_IS_DOCUMENT_WORKSPACE (self), NULL);

  return priv->grid;
}
