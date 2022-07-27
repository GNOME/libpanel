/* panel-action-muxer-private.h
 *
 * Copyright 2022 Christian Hergert <chergert@redhat.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Panelntifier: GPL-3.0-or-later
 */

#pragma once

#include <gio/gio.h>

G_BEGIN_DECLS

#define PANEL_TYPE_ACTION_MUXER (panel_action_muxer_get_type())

G_DECLARE_FINAL_TYPE (PanelActionMuxer, panel_action_muxer, PANEL, ACTION_MUXER, GObject)

PanelActionMuxer  *panel_action_muxer_new                 (void);
void               panel_action_muxer_insert_action_group (PanelActionMuxer *self,
                                                           const char       *prefix,
                                                           GActionGroup     *action_group);
void               panel_action_muxer_remove_action_group (PanelActionMuxer *self,
                                                           const char       *prefix);
char             **panel_action_muxer_list_groups         (PanelActionMuxer *self);
GActionGroup      *panel_action_muxer_get_action_group    (PanelActionMuxer *self,
                                                           const char       *prefix);
void               panel_action_muxer_clear               (PanelActionMuxer *self);

G_END_DECLS
