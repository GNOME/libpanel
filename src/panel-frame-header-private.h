/* panel-frame-header-private.h
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

#pragma once

#include "panel-frame-header.h"
#include "panel-frame-private.h"

G_BEGIN_DECLS

struct _PanelFrameHeaderInterface
{
  GTypeInterface parent_iface;

  void (*connect)    (PanelFrameHeader *self,
                      PanelFrame       *frame);
  void (*disconnect) (PanelFrameHeader *self);
};

void _panel_frame_header_connect    (PanelFrameHeader *self,
                                     PanelFrame       *frame);
void _panel_frame_header_disconnect (PanelFrameHeader *self);

G_END_DECLS
