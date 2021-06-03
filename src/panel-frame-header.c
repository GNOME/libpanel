/* panel-frame-header.c
 *
 * Copyright 2021 Christian Hergert <chergert@redhat.com>
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

#include "config.h"

#include "panel-frame-header-private.h"

G_DEFINE_INTERFACE (PanelFrameHeader, panel_frame_header, GTK_TYPE_WIDGET)

static void
panel_frame_header_default_init (PanelFrameHeaderInterface *iface)
{
}

void
_panel_frame_header_connect (PanelFrameHeader *self,
                             PanelFrame       *frame)
{
  g_return_if_fail (PANEL_IS_FRAME_HEADER (self));
  g_return_if_fail (PANEL_IS_FRAME (frame));

  PANEL_FRAME_HEADER_GET_IFACE (self)->connect (self, frame);
}

void
_panel_frame_header_disconnect (PanelFrameHeader *self)
{
  g_return_if_fail (PANEL_IS_FRAME_HEADER (self));

  PANEL_FRAME_HEADER_GET_IFACE (self)->disconnect (self);
}
