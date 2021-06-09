/* panel-frame-header-bar.h
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

#include <gtk/gtk.h>

#include "panel-version-macros.h"

G_BEGIN_DECLS

#define PANEL_TYPE_FRAME_HEADER_BAR (panel_frame_header_bar_get_type())

PANEL_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (PanelFrameHeaderBar, panel_frame_header_bar, PANEL, FRAME_HEADER_BAR, GtkWidget)

PANEL_AVAILABLE_IN_ALL
GtkWidget      *panel_frame_header_bar_new              (void);
PANEL_AVAILABLE_IN_ALL
GMenuModel     *panel_frame_header_bar_get_menu_model   (PanelFrameHeaderBar *self);
PANEL_AVAILABLE_IN_ALL
void            panel_frame_header_bar_set_menu_model   (PanelFrameHeaderBar *self,
                                                         GMenuModel          *model);
PANEL_AVAILABLE_IN_ALL
GtkPopoverMenu *panel_frame_header_bar_get_menu_popover (PanelFrameHeaderBar *self);
PANEL_AVAILABLE_IN_ALL
GtkWidget      *panel_frame_header_bar_get_start_child  (PanelFrameHeaderBar *self);
PANEL_AVAILABLE_IN_ALL
void            panel_frame_header_bar_set_start_child  (PanelFrameHeaderBar *self,
                                                         GtkWidget           *start_child);
PANEL_AVAILABLE_IN_ALL
GtkWidget      *panel_frame_header_bar_get_end_child    (PanelFrameHeaderBar *self);
PANEL_AVAILABLE_IN_ALL
void            panel_frame_header_bar_set_end_child    (PanelFrameHeaderBar *self,
                                                         GtkWidget           *end_child);

G_END_DECLS