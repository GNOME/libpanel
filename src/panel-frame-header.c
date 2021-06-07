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
  g_object_interface_install_property (iface,
                                       g_param_spec_object ("frame",
                                                            "Frame",
                                                            "Frame",
                                                            PANEL_TYPE_FRAME,
                                                            (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
}

void
panel_frame_header_set_frame (PanelFrameHeader *self,
                              PanelFrame       *frame)
{
  g_return_if_fail (PANEL_IS_FRAME_HEADER (self));
  g_return_if_fail (!frame || PANEL_IS_FRAME (frame));

  g_object_set (self, "frame", frame, NULL);
}

/**
 * panel_frame_header_get_frame:
 * @self: a #PanelFrameHeader
 *
 * Gets the frame the header is attached to.
 *
 * Returns: (transfer none) (nullable): a #PanelFrame or %NULL
 */
PanelFrame *
panel_frame_header_get_frame (PanelFrameHeader *self)
{
  PanelFrame *frame = NULL;

  g_return_val_if_fail (PANEL_IS_FRAME_HEADER (self), NULL);

  g_object_get (self, "frame", &frame, NULL);

  /* We return a borrowed reference */
  g_return_val_if_fail (!frame || PANEL_IS_FRAME (frame), NULL);
  g_return_val_if_fail (!frame || G_OBJECT (frame)->ref_count > 1, NULL);
  g_object_unref (frame);

  return frame;
}
