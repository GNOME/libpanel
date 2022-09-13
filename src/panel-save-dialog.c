/* panel-save-dialog.c
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

#include <glib/gi18n.h>

#include <adwaita.h>

#include "panel-save-delegate.h"
#include "panel-save-dialog.h"

struct _PanelSaveDialog
{
  GtkDialog            parent_instance;
  GtkHeaderBar        *headerbar;
  AdwPreferencesGroup *list;
  GTask               *task;
  guint                count;
};

G_DEFINE_TYPE (PanelSaveDialog, panel_save_dialog, GTK_TYPE_DIALOG)

/**
 * panel_save_dialog_new:
 *
 * Create a new #PanelSaveDialog.
 *
 * Returns: (transfer full): a newly created #PanelSaveDialog
 */
GtkWidget *
panel_save_dialog_new (void)
{
  return g_object_new (PANEL_TYPE_SAVE_DIALOG, NULL);
}

static void
panel_save_dialog_response (GtkDialog *dialog,
                            int        response)
{
  PanelSaveDialog *self = (PanelSaveDialog *)dialog;

  g_assert (PANEL_IS_SAVE_DIALOG (self));

  if (response == GTK_RESPONSE_NO)
    {
    }
  else if (response == GTK_RESPONSE_YES)
    {
    }
  else
    {
      g_task_return_new_error (self->task,
                               G_IO_ERROR,
                               G_IO_ERROR_CANCELLED,
                               "Operation was cancelled");
      g_clear_object (&self->task);
      gtk_window_destroy (GTK_WINDOW (dialog));
      return;
    }
}

static void
panel_save_dialog_constructed (GObject *object)
{
  PanelSaveDialog *self = (PanelSaveDialog *)object;
  GtkWidget *box;

  G_OBJECT_CLASS (panel_save_dialog_parent_class)->constructed (object);

  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_window_set_titlebar (GTK_WINDOW (self), box);
  gtk_widget_hide (box);
}

static void
panel_save_dialog_dispose (GObject *object)
{
  PanelSaveDialog *self = (PanelSaveDialog *)object;

  g_clear_object (&self->task);

  G_OBJECT_CLASS (panel_save_dialog_parent_class)->dispose (object);
}

static void
panel_save_dialog_class_init (PanelSaveDialogClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkDialogClass *dialog_class = GTK_DIALOG_CLASS (klass);

  object_class->constructed = panel_save_dialog_constructed;
  object_class->dispose = panel_save_dialog_dispose;

  dialog_class->response = panel_save_dialog_response;

  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/libpanel/panel-save-dialog.ui");
  gtk_widget_class_bind_template_child (widget_class, PanelSaveDialog, list);
  gtk_widget_class_bind_template_child (widget_class, PanelSaveDialog, headerbar);
}

static void
panel_save_dialog_init (PanelSaveDialog *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

void
panel_save_dialog_add_delegate (PanelSaveDialog   *self,
                                PanelSaveDelegate *delegate)
{
  AdwActionRow *row;

  g_return_if_fail (PANEL_IS_SAVE_DIALOG (self));
  g_return_if_fail (PANEL_IS_SAVE_DELEGATE (delegate));

  self->count++;

  row = ADW_ACTION_ROW (adw_action_row_new ());
  adw_preferences_group_add (self->list, GTK_WIDGET (row));
}

void
panel_save_dialog_run_async (PanelSaveDialog     *self,
                             GCancellable        *cancellable,
                             GAsyncReadyCallback  callback,
                             gpointer             user_data)
{
  GTask *task = NULL;

  g_return_if_fail (PANEL_IS_SAVE_DIALOG (self));
  g_return_if_fail (!cancellable || G_IS_CANCELLABLE (cancellable));

  g_object_ref_sink (self);

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_source_tag (task, panel_save_dialog_run_async);

  if (self->count == 0)
    {
      gtk_window_destroy (GTK_WINDOW (self));
      g_task_return_boolean (task, TRUE);
      g_clear_object (&task);
      return;
    }

  if (self->task)
    {
      g_task_return_new_error (task,
                               G_IO_ERROR,
                               G_IO_ERROR_INVAL,
                               "Run has already been called");
      g_clear_object (&task);
      return;
    }

  g_clear_object (&self->task);
  self->task = g_steal_pointer (&task);

  gtk_window_present (GTK_WINDOW (self));
}

gboolean
panel_save_dialog_run_finish (PanelSaveDialog  *self,
                              GAsyncResult     *result,
                              GError          **error)
{
  g_return_val_if_fail (PANEL_IS_SAVE_DIALOG (self), FALSE);
  g_return_val_if_fail (G_IS_TASK (result), FALSE);

  return g_task_propagate_boolean (G_TASK (result), error);
}
