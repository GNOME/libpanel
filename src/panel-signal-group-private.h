/* panel-signal-group.h
 *
 * Copyright (C) 2015 Christian Hergert <christian@hergert.me>
 * Copyright (C) 2015 Garrett Regier <garrettregier@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
 * License, or (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PANEL_SIGNAL_GROUP_H
#define PANEL_SIGNAL_GROUP_H

#include <glib-object.h>

G_BEGIN_DECLS

#define PANEL_TYPE_SIGNAL_GROUP (panel_signal_group_get_type())

G_DECLARE_FINAL_TYPE (PanelSignalGroup, panel_signal_group, PANEL, SIGNAL_GROUP, GObject)

PanelSignalGroup *panel_signal_group_new             (GType             target_type);
void              panel_signal_group_set_target      (PanelSignalGroup *self,
                                                      gpointer          target);
gpointer          panel_signal_group_get_target      (PanelSignalGroup *self);
void              panel_signal_group_block           (PanelSignalGroup *self);
void              panel_signal_group_unblock         (PanelSignalGroup *self);
void              panel_signal_group_connect_object  (PanelSignalGroup *self,
                                                      const gchar      *detailed_signal,
                                                      GCallback         c_handler,
                                                      gpointer          object,
                                                      GConnectFlags     flags);
void              panel_signal_group_connect_data    (PanelSignalGroup *self,
                                                      const gchar      *detailed_signal,
                                                      GCallback         c_handler,
                                                      gpointer          data,
                                                      GClosureNotify    notify,
                                                      GConnectFlags     flags);
void              panel_signal_group_connect         (PanelSignalGroup *self,
                                                      const gchar      *detailed_signal,
                                                      GCallback         c_handler,
                                                      gpointer          data);
void              panel_signal_group_connect_after   (PanelSignalGroup *self,
                                                      const gchar      *detailed_signal,
                                                      GCallback         c_handler,
                                                      gpointer          data);
void              panel_signal_group_connect_swapped (PanelSignalGroup *self,
                                                      const gchar      *detailed_signal,
                                                      GCallback         c_handler,
                                                      gpointer          data);

G_END_DECLS

#endif /* PANEL_SIGNAL_GROUP_H */
