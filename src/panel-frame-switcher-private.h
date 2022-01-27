/* panel-frame-switcher-private.h
 *
 * Copyright 2022 Christian Hergert <chergert@redhat.com>
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

#include "panel-frame-switcher.h"

G_BEGIN_DECLS

AdwTabPage *_panel_frame_switcher_get_page        (PanelFrameSwitcher *self,
                                                   GtkWidget          *button);
void        _panel_frame_switcher_set_drop_before (PanelFrameSwitcher *self,
                                                   PanelWidget        *widget);

G_END_DECLS
