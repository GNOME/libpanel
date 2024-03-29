/* panel-application.h
 *
 * Copyright 2023 Christian Hergert <chergert@redhat.com>
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

#include <adwaita.h>

#include "panel-version-macros.h"

G_BEGIN_DECLS

#define PANEL_TYPE_APPLICATION (panel_application_get_type())

PANEL_AVAILABLE_IN_1_4
G_DECLARE_DERIVABLE_TYPE (PanelApplication, panel_application, PANEL, APPLICATION, AdwApplication)

struct _PanelApplicationClass
{
  AdwApplicationClass parent_class;

  /*< private >*/
  gpointer _reserved[8];
};

PANEL_AVAILABLE_IN_1_4
PanelApplication *panel_application_new (const char        *application_id,
                                         GApplicationFlags  flags);

G_END_DECLS
