/* panel-animation.h
 *
 * Copyright (C) 2010-2020 Christian Hergert <christian@hergert.me>
 *
 * This file is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2.1 of the License, or (at your option)
 * any later version.
 *
 * This file is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <gdk/gdk.h>

G_BEGIN_DECLS

#define PANEL_TYPE_ANIMATION      (panel_animation_get_type())
#define PANEL_TYPE_ANIMATION_MODE (panel_animation_mode_get_type())

G_DECLARE_FINAL_TYPE (PanelAnimation, panel_animation, PANEL, ANIMATION, GInitiallyUnowned)

typedef enum _PanelAnimationMode
{
  PANEL_ANIMATION_LINEAR,
  PANEL_ANIMATION_EASE_IN_QUAD,
  PANEL_ANIMATION_EASE_OUT_QUAD,
  PANEL_ANIMATION_EASE_IN_OUT_QUAD,
  PANEL_ANIMATION_EASE_IN_CUBIC,
  PANEL_ANIMATION_EASE_OUT_CUBIC,
  PANEL_ANIMATION_EASE_IN_OUT_CUBIC,
  PANEL_ANIMATION_LAST
} PanelAnimationMode;

GType            panel_animation_mode_get_type      (void) G_GNUC_CONST;
void             panel_animation_start              (PanelAnimation     *animation);
void             panel_animation_stop               (PanelAnimation     *animation);
void             panel_animation_add_property       (PanelAnimation     *animation,
                                                      GParamSpec          *pspec,
                                                      const GValue        *value);
PanelAnimation *panel_object_animatev              (gpointer             object,
                                                      PanelAnimationMode  mode,
                                                      guint                duration_msec,
                                                      GdkFrameClock       *frame_clock,
                                                      const gchar         *first_property,
                                                      va_list              args);
PanelAnimation *panel_object_animate               (gpointer             object,
                                                      PanelAnimationMode  mode,
                                                      guint                duration_msec,
                                                      GdkFrameClock       *frame_clock,
                                                      const gchar         *first_property,
                                                      ...) G_GNUC_NULL_TERMINATED;
PanelAnimation *panel_object_animate_full          (gpointer             object,
                                                      PanelAnimationMode  mode,
                                                      guint                duration_msec,
                                                      GdkFrameClock       *frame_clock,
                                                      GDestroyNotify       notify,
                                                      gpointer             notify_data,
                                                      const gchar         *first_property,
                                                      ...) G_GNUC_NULL_TERMINATED;
guint            panel_animation_calculate_duration (GdkMonitor          *monitor,
                                                      gdouble              from_value,
                                                      gdouble              to_value);

G_END_DECLS
