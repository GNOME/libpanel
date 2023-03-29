/* main.c
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

#include <libpanel.h>

#include "demo-workspace.h"

static void
on_activate (GtkApplication *app,
             gpointer        user_data)
{
  PanelWorkbench *workbench;
  DemoWorkspace *workspace;

  workbench = panel_workbench_new ();
  workspace = demo_workspace_new ();

  panel_workbench_add_workspace (workbench, PANEL_WORKSPACE (workspace));
  panel_workbench_focus_workspace (workbench, PANEL_WORKSPACE (workspace));
}

int
main (int argc,
      char *argv[])
{
  GtkApplication *app;
  int ret;

  app = g_object_new (PANEL_TYPE_APPLICATION,
                      "application-id", "org.gnome.libpanel.demo",
                      NULL);
  g_signal_connect (app, "activate", G_CALLBACK (on_activate), NULL);
  ret = g_application_run (G_APPLICATION (app), argc, argv);
  g_clear_object (&app);

  return ret;
}
