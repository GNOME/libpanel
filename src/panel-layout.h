/* panel-layout.h
 *
 * Copyright 2022 Christian Hergert <chergert@redhat.com>
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

#pragma once

#include <glib-object.h>

#include "panel-layout-item.h"
#include "panel-version-macros.h"

G_BEGIN_DECLS

#define PANEL_TYPE_LAYOUT (panel_layout_get_type())

PANEL_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (PanelLayout, panel_layout, PANEL, LAYOUT, GObject)

PANEL_AVAILABLE_IN_ALL
PanelLayout     *panel_layout_new              (void);
PANEL_AVAILABLE_IN_ALL
void             panel_layout_append           (PanelLayout      *self,
                                                PanelLayoutItem  *item);
PANEL_AVAILABLE_IN_ALL
void             panel_layout_prepend          (PanelLayout      *self,
                                                PanelLayoutItem  *item);
PANEL_AVAILABLE_IN_ALL
void             panel_layout_insert           (PanelLayout      *self,
                                                guint             position,
                                                PanelLayoutItem  *item);
PANEL_AVAILABLE_IN_ALL
void             panel_layout_remove           (PanelLayout      *self,
                                                PanelLayoutItem  *item);
PANEL_AVAILABLE_IN_ALL
void             panel_layout_remove_at        (PanelLayout      *self,
                                                guint             position);
PANEL_AVAILABLE_IN_ALL
guint            panel_layout_get_n_items      (PanelLayout      *self);
PANEL_AVAILABLE_IN_ALL
PanelLayoutItem *panel_layout_get_item         (PanelLayout      *self,
                                                guint             position);
PANEL_AVAILABLE_IN_ALL
PanelLayout     *panel_layout_new_from_variant (GVariant         *variant,
                                                GError          **error);
PANEL_AVAILABLE_IN_ALL
GVariant        *panel_layout_to_variant       (PanelLayout      *self);

G_END_DECLS
