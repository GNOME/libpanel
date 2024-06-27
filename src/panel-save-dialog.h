/* panel-save-dialog.h
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

#pragma once

#include <adwaita.h>

#include "panel-types.h"

G_BEGIN_DECLS

#define PANEL_TYPE_SAVE_DIALOG (panel_save_dialog_get_type())

PANEL_DEPRECATED_IN_1_8_FOR(PanelChangesDialog)
G_DECLARE_FINAL_TYPE (PanelSaveDialog, panel_save_dialog, PANEL, SAVE_DIALOG, AdwMessageDialog)

PANEL_DEPRECATED_IN_1_8_FOR(panel_changes_dialog_new)
GtkWidget *panel_save_dialog_new                  (void);
PANEL_DEPRECATED_IN_1_8_FOR(panel_changes_dialog_add_delegate)
void       panel_save_dialog_add_delegate         (PanelSaveDialog      *self,
                                                   PanelSaveDelegate    *delegate);
PANEL_DEPRECATED_IN_1_8_FOR(panel_changes_dialog_run_async)
void       panel_save_dialog_run_async            (PanelSaveDialog      *self,
                                                   GCancellable         *cancellable,
                                                   GAsyncReadyCallback   callback,
                                                   gpointer              user_data);
PANEL_DEPRECATED_IN_1_8_FOR(panel_changes_dialog_run_finish)
gboolean   panel_save_dialog_run_finish           (PanelSaveDialog      *self,
                                                   GAsyncResult         *result,
                                                   GError              **error);
PANEL_DEPRECATED_IN_1_8_FOR(panel_changes_dialog_get_close_after_save)
gboolean   panel_save_dialog_get_close_after_save (PanelSaveDialog      *self);
PANEL_DEPRECATED_IN_1_8_FOR(panel_changes_dialog_set_close_after_save)
void       panel_save_dialog_set_close_after_save (PanelSaveDialog      *self,
                                                   gboolean              close_after_save);

G_END_DECLS
