/* example-window.c
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

#include <libpanel.h>

#include "example-window.h"

struct _ExampleWindow
{
  GtkApplicationWindow parent_instance;
  PanelDock *dock;
  PanelGrid *grid;
  GMenuModel *page_menu;
  GtkToggleButton *frame_header_bar;
};

G_DEFINE_TYPE (ExampleWindow, example_window, ADW_TYPE_APPLICATION_WINDOW)

static GdkRGBA white;
static GdkRGBA grey;

GtkWidget *
example_window_new (GtkApplication *application)
{
  return g_object_new (EXAMPLE_TYPE_WINDOW,
                       "application", application,
                       NULL);
}

static GtkWidget *
get_default_focus_cb (GtkWidget *widget,
                      GtkWidget *text_view)
{
  return text_view;
}

static gboolean
on_save_cb (PanelSaveDelegate *delegate,
            GTask             *task,
            PanelWidget       *widget)
{
  g_assert (PANEL_IS_SAVE_DELEGATE (delegate));
  g_assert (G_IS_TASK (task));
  g_assert (PANEL_IS_WIDGET (widget));

  // actually do the save here, ideally asynchronously

  g_print ("Actually save the file\n");

  panel_widget_set_modified (widget, FALSE);
  panel_save_delegate_set_progress (delegate, 1.0);
  g_task_return_boolean (task, TRUE);

  return TRUE;
}

static void
apply_theme_color (GtkSettings *settings,
                   PanelWidget *widget)
{
  static GdkRGBA black = {0,0,0,1};
  gboolean dark;

  g_object_get (settings, "gtk-application-prefer-dark-theme", &dark, NULL);

  if (dark)
    {
      panel_widget_set_background_rgba (widget, &black);
      panel_widget_set_foreground_rgba (widget, &white);
    }
  else
    {
      panel_widget_set_background_rgba (widget, &white);
      panel_widget_set_foreground_rgba (widget, &grey);
    }
}

static void
notify_prefer_dark_theme_cb (GtkSettings *settings,
                             GParamSpec  *pspec,
                             PanelWidget *widget)
{
  apply_theme_color (settings, widget);
}

static void
example_window_add_document (ExampleWindow *self)
{
  static guint count;
  PanelWidget *widget;
  GtkWidget *text_view;
  PanelSaveDelegate *save_delegate;
  GtkTextBuffer *buffer;
  GtkSettings *settings;
  char *title;

  g_return_if_fail (EXAMPLE_IS_WINDOW (self));

  title = g_strdup_printf ("Untitled Document %u", ++count);
  buffer = g_object_new (GTK_TYPE_TEXT_BUFFER,
                         "text", title,
                         NULL);
  text_view = g_object_new (GTK_TYPE_TEXT_VIEW,
                            "buffer", buffer,
                            NULL);

  save_delegate = panel_save_delegate_new ();
  panel_save_delegate_set_title (save_delegate, title);
  panel_save_delegate_set_subtitle (save_delegate, "~/Documents");

  widget = g_object_new (PANEL_TYPE_WIDGET,
                         "title", title,
                         "kind", PANEL_WIDGET_KIND_DOCUMENT,
                         "icon-name", "text-x-generic-symbolic",
                         "menu-model", self->page_menu,
                         "can-maximize", TRUE,
                         "save-delegate", save_delegate,
                         "modified", TRUE,
                         "child", g_object_new (GTK_TYPE_SCROLLED_WINDOW,
                                                "child", text_view,
                                                NULL),
                         NULL);

  settings = gtk_settings_get_default ();
  g_signal_connect_object (settings,
                           "notify::gtk-application-prefer-dark-theme",
                           G_CALLBACK (notify_prefer_dark_theme_cb),
                           widget, 0);
  apply_theme_color (settings, widget);

  g_signal_connect (widget,
                    "get-default-focus",
                    G_CALLBACK (get_default_focus_cb),
                    text_view);
  g_signal_connect (save_delegate,
                    "save",
                    G_CALLBACK (on_save_cb),
                    widget);

  panel_grid_add (self->grid, widget);
  panel_widget_raise (widget);
  panel_widget_focus_default (widget);

  g_object_unref (save_delegate);
  g_object_unref (buffer);
}

static void
add_document_action (GtkWidget  *widget,
                     const char *action_name,
                     GVariant   *param)
{
  example_window_add_document (EXAMPLE_WINDOW (widget));
}

static PanelFrame *
create_frame_cb (PanelGrid     *grid,
                 ExampleWindow *self)
{
  PanelFrame *frame;
  PanelFrameHeader *header;
  AdwStatusPage *status;
  GtkGrid *shortcuts;

  g_assert (EXAMPLE_IS_WINDOW (self));

  frame = PANEL_FRAME (panel_frame_new ());

  status = ADW_STATUS_PAGE (adw_status_page_new ());
  adw_status_page_set_title (status, "Open a File or Terminal");
  adw_status_page_set_icon_name (status, "document-new-symbolic");
  adw_status_page_set_description (status, "Use the page switcher above or use one of the following:");
  shortcuts = GTK_GRID (gtk_grid_new ());
  gtk_grid_set_row_spacing (shortcuts, 6);
  gtk_grid_set_column_spacing (shortcuts, 32);
  gtk_widget_set_halign (GTK_WIDGET (shortcuts), GTK_ALIGN_CENTER);
  gtk_grid_attach (shortcuts, gtk_label_new ("New Document"), 0, 0, 1, 1);
  gtk_grid_attach (shortcuts, gtk_label_new ("Ctrl+N"), 1, 0, 1, 1);
  gtk_grid_attach (shortcuts, gtk_label_new ("Close Document"), 0, 1, 1, 1);
  gtk_grid_attach (shortcuts, gtk_label_new ("Ctrl+W"), 1, 1, 1, 1);
  for (GtkWidget *child = gtk_widget_get_first_child (GTK_WIDGET (shortcuts));
       child;
       child = gtk_widget_get_next_sibling (child))
    gtk_widget_set_halign (child, GTK_ALIGN_START);
  adw_status_page_set_child (status, GTK_WIDGET (shortcuts));
  panel_frame_set_placeholder (frame, GTK_WIDGET (status));

  if (gtk_toggle_button_get_active (self->frame_header_bar))
     header = PANEL_FRAME_HEADER (panel_frame_header_bar_new ());
  else
     header = PANEL_FRAME_HEADER (panel_frame_tab_bar_new ());

  panel_frame_set_header (frame, header);
  panel_frame_header_pack_start (header,
                                 -100,
                                 g_object_new (GTK_TYPE_BUTTON,
                                               "width-request", 32,
                                               "focus-on-click", FALSE,
                                               "icon-name", "go-previous-symbolic",
                                               NULL));
  panel_frame_header_pack_start (header,
                                 -50,
                                 g_object_new (GTK_TYPE_BUTTON,
                                               "width-request", 32,
                                               "focus-on-click", FALSE,
                                               "icon-name", "go-next-symbolic",
                                               NULL));
  return frame;
}

static void
example_window_constructed (GObject *object)
{
  ExampleWindow *self = (ExampleWindow *)object;

  G_OBJECT_CLASS (example_window_parent_class)->constructed (object);

  /* Create 0,0 frame */
  (void)panel_grid_column_get_row (panel_grid_get_column (self->grid, 0), 0);
}

static void
example_window_class_init (ExampleWindowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed = example_window_constructed;

  gtk_widget_class_set_template_from_resource (widget_class, "/example-window.ui");
  gtk_widget_class_bind_template_child (widget_class, ExampleWindow, dock);
  gtk_widget_class_bind_template_child (widget_class, ExampleWindow, grid);
  gtk_widget_class_bind_template_child (widget_class, ExampleWindow, page_menu);
  gtk_widget_class_bind_template_child (widget_class, ExampleWindow, frame_header_bar);
  gtk_widget_class_bind_template_callback (widget_class, create_frame_cb);

  gtk_widget_class_install_action (widget_class, "document.new", NULL, add_document_action);

  gtk_widget_class_add_binding_action (widget_class, GDK_KEY_n, GDK_CONTROL_MASK, "document.new", NULL);
  gtk_widget_class_add_binding_action (widget_class, GDK_KEY_F9, 0, "win.reveal-start", NULL);
  gtk_widget_class_add_binding_action (widget_class, GDK_KEY_F9, GDK_CONTROL_MASK, "win.reveal-bottom", NULL);
  gtk_widget_class_add_binding_action (widget_class, GDK_KEY_F9, GDK_SHIFT_MASK, "win.reveal-end", NULL);

  gdk_rgba_parse (&white, "#fff");
  gdk_rgba_parse (&grey, "#241f31");
}

static void
example_window_init (ExampleWindow *self)
{
  g_autoptr(GPropertyAction) reveal_start = NULL;
  g_autoptr(GPropertyAction) reveal_end = NULL;
  g_autoptr(GPropertyAction) reveal_bottom = NULL;

  gtk_widget_init_template (GTK_WIDGET (self));

  reveal_start = g_property_action_new ("reveal-start", self->dock, "reveal-start");
  reveal_bottom = g_property_action_new ("reveal-bottom", self->dock, "reveal-bottom");
  reveal_end = g_property_action_new ("reveal-end", self->dock, "reveal-end");

  g_action_map_add_action (G_ACTION_MAP (self), G_ACTION (reveal_start));
  g_action_map_add_action (G_ACTION_MAP (self), G_ACTION (reveal_end));
  g_action_map_add_action (G_ACTION_MAP (self), G_ACTION (reveal_bottom));
}
