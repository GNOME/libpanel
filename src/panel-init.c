/* panel-init.c
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

#include "config.h"

#include "panel-dock.h"
#include "panel-init.h"
#include "panel-resources.h"
#include "panel-switcher.h"
#include "panel-widget.h"

void
panel_init (void)
{
  g_resources_register (panel_get_resource ());

  g_type_ensure (PANEL_TYPE_DOCK);
  g_type_ensure (PANEL_TYPE_WIDGET);
  g_type_ensure (PANEL_TYPE_SWITCHER);
}

void
panel_finalize (void)
{
  g_resources_unregister (panel_get_resource ());
}
