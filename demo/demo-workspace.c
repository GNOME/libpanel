/* demo-workspace.c
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

#include "demo-page.h"
#include "demo-workspace.h"

struct _DemoWorkspace
{
  PanelDocumentWorkspace parent_instance;
};

G_DEFINE_FINAL_TYPE (DemoWorkspace, demo_workspace, PANEL_TYPE_DOCUMENT_WORKSPACE)

static void
add_page_action (GtkWidget  *widget,
                 const char *action_name,
                 GVariant   *param)
{
  DemoWorkspace *self = DEMO_WORKSPACE (widget);
  DemoPage *page = demo_page_new ();

  panel_document_workspace_add_widget (PANEL_DOCUMENT_WORKSPACE (self),
                                       PANEL_WIDGET (page),
                                       NULL);
}

static void
demo_workspace_dispose (GObject *object)
{
  G_OBJECT_CLASS (demo_workspace_parent_class)->dispose (object);
}

static void
demo_workspace_class_init (DemoWorkspaceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = demo_workspace_dispose;

  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/libpanel/demo/demo-workspace.ui");

  gtk_widget_class_install_action (widget_class, "workspace.add-page", NULL, add_page_action);
}

static void
demo_workspace_init (DemoWorkspace *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

#ifdef DEVELOPMENT_BUILD
  gtk_widget_add_css_class (GTK_WIDGET (self), "devel");
#endif
}

DemoWorkspace *
demo_workspace_new (void)
{
  return g_object_new (DEMO_TYPE_WORKSPACE, NULL);
}
