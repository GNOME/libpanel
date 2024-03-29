/*
 * panel-gsettings-action-group.h
 *
 * Copyright 2022-2023 Christian Hergert <chergert@redhat.com>
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
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gio/gio.h>

#include "panel-version-macros.h"

G_BEGIN_DECLS

#define PANEL_TYPE_GSETTINGS_ACTION_GROUP (panel_gsettings_action_group_get_type())

PANEL_AVAILABLE_IN_1_4
G_DECLARE_FINAL_TYPE (PanelGSettingsActionGroup, panel_gsettings_action_group, PANEL, GSETTINGS_ACTION_GROUP, GObject)

PANEL_AVAILABLE_IN_1_4
GActionGroup *panel_gsettings_action_group_new (GSettings *settings);

G_END_DECLS
