/* panel-layout-item.h
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

#include "panel-types.h"

G_BEGIN_DECLS

#define PANEL_TYPE_LAYOUT_ITEM (panel_layout_item_get_type())

PANEL_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (PanelLayoutItem, panel_layout_item, PANEL, LAYOUT_ITEM, GObject)

PANEL_AVAILABLE_IN_ALL
PanelLayoutItem *panel_layout_item_new                    (void);
PANEL_AVAILABLE_IN_ALL
PanelPosition   *panel_layout_item_get_position           (PanelLayoutItem     *self);
PANEL_AVAILABLE_IN_ALL
void             panel_layout_item_set_position           (PanelLayoutItem     *self,
                                                           PanelPosition       *position);
PANEL_AVAILABLE_IN_ALL
const char      *panel_layout_item_get_id                 (PanelLayoutItem     *self);
PANEL_AVAILABLE_IN_ALL
void             panel_layout_item_set_id                 (PanelLayoutItem     *self,
                                                           const char          *id);
PANEL_AVAILABLE_IN_ALL
const char      *panel_layout_item_get_type_hint          (PanelLayoutItem     *self);
PANEL_AVAILABLE_IN_ALL
void             panel_layout_item_set_type_hint          (PanelLayoutItem     *self,
                                                           const char          *type_hint);
PANEL_AVAILABLE_IN_ALL
gboolean         panel_layout_item_has_metadata           (PanelLayoutItem     *self,
                                                           const char          *key,
                                                           const GVariantType **value_type);
PANEL_AVAILABLE_IN_ALL
gboolean         panel_layout_item_has_metadata_with_type (PanelLayoutItem     *self,
                                                           const char          *key,
                                                           const GVariantType  *expected_type);
PANEL_AVAILABLE_IN_ALL
void             panel_layout_item_get_metadata           (PanelLayoutItem     *self,
                                                           const char          *key,
                                                           const char          *format,
                                                           ...);
PANEL_AVAILABLE_IN_ALL
void             panel_layout_item_set_metadata           (PanelLayoutItem     *self,
                                                           const char          *key,
                                                           const char          *format,
                                                           ...);
PANEL_AVAILABLE_IN_ALL
GVariant        *panel_layout_item_get_metadata_value     (PanelLayoutItem     *self,
                                                           const char          *key,
                                                           const GVariantType  *expected_type);
PANEL_AVAILABLE_IN_ALL
void             panel_layout_item_set_metadata_value     (PanelLayoutItem     *self,
                                                           const char          *key,
                                                           GVariant            *value);

G_END_DECLS
