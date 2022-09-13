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
#include "panel-save-dialog-row-private.h"

struct _PanelSaveDialog
{
  AdwMessageDialog     parent_instance;
  AdwPreferencesGroup *group;
  GTask               *task;
  guint                count;
};

G_DEFINE_FINAL_TYPE (PanelSaveDialog, panel_save_dialog, ADW_TYPE_MESSAGE_DIALOG)

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
panel_save_dialog_response_cancel_cb (PanelSaveDialog *self,
                                      const char      *response)
{
  GTask *task;

  g_assert (PANEL_IS_SAVE_DIALOG (self));

  task = g_steal_pointer (&self->task);
  g_task_return_new_error (task,
                           G_IO_ERROR,
                           G_IO_ERROR_CANCELLED,
                           "Operation was cancelled");
  gtk_window_destroy (GTK_WINDOW (self));

  g_clear_object (&task);
}

static void
panel_save_dialog_response_discard_cb (PanelSaveDialog *self,
                                       const char      *response)
{
  GTask *task;

  g_assert (PANEL_IS_SAVE_DIALOG (self));

  task = g_steal_pointer (&self->task);

  /* TODO: Discard widgets */
  g_task_return_boolean (task, TRUE);

  gtk_window_destroy (GTK_WINDOW (self));

  g_clear_object (&task);
}

static void
panel_save_dialog_response_save_cb (PanelSaveDialog *self,
                                    const char      *response)
{
  GTask *task;

  g_assert (PANEL_IS_SAVE_DIALOG (self));

  task = g_steal_pointer (&self->task);

  /* TODO: Save using delegates */
  g_task_return_boolean (task, TRUE);

  gtk_window_destroy (GTK_WINDOW (self));

  g_clear_object (&task);
}

static void
panel_save_dialog_class_init (PanelSaveDialogClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/libpanel/panel-save-dialog.ui");

  gtk_widget_class_bind_template_child (widget_class, PanelSaveDialog, group);

  gtk_widget_class_bind_template_callback (widget_class, panel_save_dialog_response_cancel_cb);
  gtk_widget_class_bind_template_callback (widget_class, panel_save_dialog_response_discard_cb);
  gtk_widget_class_bind_template_callback (widget_class, panel_save_dialog_response_save_cb);
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
  g_return_if_fail (PANEL_IS_SAVE_DIALOG (self));
  g_return_if_fail (PANEL_IS_SAVE_DELEGATE (delegate));

  self->count++;

  adw_preferences_group_add (self->group,
                             panel_save_dialog_row_new (delegate));
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
