/* panel-save-dialog-row.c
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

#include "config.h"

#include "panel-save-delegate.h"
#include "panel-save-dialog-row-private.h"

struct _PanelSaveDialogRow
{
  AdwActionRow       parent_instance;

  PanelSaveDelegate *delegate;

  GtkCheckButton    *check;
};

enum {
  PROP_0,
  PROP_DELEGATE,
  PROP_SELECTED,
  N_PROPS
};

G_DEFINE_FINAL_TYPE (PanelSaveDialogRow, panel_save_dialog_row, ADW_TYPE_ACTION_ROW)

static GParamSpec *properties [N_PROPS];

static void
on_notify_active_cb (PanelSaveDialogRow *self,
                     GParamSpec         *pspec,
                     GtkCheckButton     *check)
{
  g_assert (PANEL_IS_SAVE_DIALOG_ROW (self));
  g_assert (GTK_IS_CHECK_BUTTON (check));

  g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_SELECTED]);
}

static void
panel_save_dialog_row_dispose (GObject *object)
{
  PanelSaveDialogRow *self = (PanelSaveDialogRow *)object;

  g_clear_object (&self->delegate);

  G_OBJECT_CLASS (panel_save_dialog_row_parent_class)->dispose (object);
}

static void
panel_save_dialog_row_get_property (GObject    *object,
                                    guint       prop_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  PanelSaveDialogRow *self = PANEL_SAVE_DIALOG_ROW (object);

  switch (prop_id)
    {
    case PROP_DELEGATE:
      g_value_set_object (value, panel_save_dialog_row_get_delegate (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
panel_save_dialog_row_set_property (GObject      *object,
                                    guint         prop_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  PanelSaveDialogRow *self = PANEL_SAVE_DIALOG_ROW (object);

  switch (prop_id)
    {
    case PROP_DELEGATE:
      self->delegate = g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
panel_save_dialog_row_class_init (PanelSaveDialogRowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = panel_save_dialog_row_dispose;
  object_class->get_property = panel_save_dialog_row_get_property;
  object_class->set_property = panel_save_dialog_row_set_property;

  properties [PROP_DELEGATE] =
    g_param_spec_object ("delegate", NULL, NULL,
                         PANEL_TYPE_SAVE_DELEGATE,
                         (G_PARAM_READWRITE |
                          G_PARAM_CONSTRUCT_ONLY |
                          G_PARAM_STATIC_STRINGS));

  properties [PROP_SELECTED] =
    g_param_spec_boolean ("selected", NULL, NULL,
                          TRUE,
                         (G_PARAM_READWRITE |
                          G_PARAM_EXPLICIT_NOTIFY |
                          G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);

  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/libpanel/panel-save-dialog-row.ui");
  gtk_widget_class_bind_template_child (widget_class, PanelSaveDialogRow, check);
  gtk_widget_class_bind_template_callback (widget_class, on_notify_active_cb);
}

static void
panel_save_dialog_row_init (PanelSaveDialogRow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

PanelSaveDelegate *
panel_save_dialog_row_get_delegate (PanelSaveDialogRow *self)
{
  g_return_val_if_fail (PANEL_IS_SAVE_DIALOG_ROW (self), NULL);

  return self->delegate;
}

GtkWidget *
panel_save_dialog_row_new (PanelSaveDelegate *delegate)
{
  g_return_val_if_fail (PANEL_IS_SAVE_DELEGATE (delegate), NULL);

  return g_object_new (PANEL_TYPE_SAVE_DIALOG_ROW,
                       "delegate", delegate,
                       NULL);
}

gboolean
panel_save_dialog_row_get_selected (PanelSaveDialogRow *self)
{
  g_return_val_if_fail (PANEL_IS_SAVE_DIALOG_ROW (self), FALSE);

  return gtk_check_button_get_active (self->check);
}

void
panel_save_dialog_row_set_selected (PanelSaveDialogRow *self,
                                    gboolean            selected)
{
  g_return_if_fail (PANEL_IS_SAVE_DIALOG_ROW (self));

  gtk_check_button_set_active (self->check, selected);
}
