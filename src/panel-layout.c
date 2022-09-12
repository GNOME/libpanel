/* panel-layout.c
 *
 * Copyright 2022 Christian Hergert <chergert@redhat.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "config.h"

#include "panel-layout.h"
#include "panel-layout-item-private.h"
#include "panel-position-private.h"

struct _PanelLayout
{
  GObject    parent_instance;
  GPtrArray *items;
};

G_DEFINE_FINAL_TYPE (PanelLayout, panel_layout, G_TYPE_OBJECT)

static void
panel_layout_dispose (GObject *object)
{
  PanelLayout *self = (PanelLayout *)object;

  g_clear_pointer (&self->items, g_ptr_array_unref);

  G_OBJECT_CLASS (panel_layout_parent_class)->dispose (object);
}

static void
panel_layout_class_init (PanelLayoutClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = panel_layout_dispose;
}

static void
panel_layout_init (PanelLayout *self)
{
  self->items = g_ptr_array_new_with_free_func (g_object_unref);
}

/**
 * panel_layout_to_variant:
 * @self: a #PanelLayout
 *
 * Serializes a #PanelLayout as a #GVariant
 *
 * The result of this function may be passed to
 * panel_layout_new_from_variant() to recreate a #PanelLayout.
 *
 * Returns: (transfer full): a #GVariant
 */
GVariant *
panel_layout_to_variant (PanelLayout *self)
{
  GVariantBuilder builder;

  g_return_val_if_fail (PANEL_IS_LAYOUT (self), NULL);

  g_variant_builder_init (&builder, G_VARIANT_TYPE ("a{sv}"));
    g_variant_builder_add_parsed (&builder, "{'version',<%u>}", 1);
    g_variant_builder_open (&builder, G_VARIANT_TYPE ("{sv}"));
      g_variant_builder_add (&builder, "s", "items");
      g_variant_builder_open (&builder, G_VARIANT_TYPE ("v"));
        g_variant_builder_open (&builder, G_VARIANT_TYPE ("av"));
        for (guint i = 0; i < self->items->len; i++)
          {
            PanelLayoutItem *item = g_ptr_array_index (self->items, i);

            _panel_layout_item_to_variant (item, &builder);
          }
        g_variant_builder_close (&builder);
      g_variant_builder_close (&builder);
    g_variant_builder_close (&builder);
  return g_variant_builder_end (&builder);
}

static gboolean
panel_layout_load_1 (PanelLayout  *self,
                     GVariant     *variant,
                     GError      **error)
{
  g_autoptr(GVariant) items = NULL;

  g_assert (PANEL_IS_LAYOUT (self));
  g_assert (variant != NULL);
  g_assert (g_variant_is_of_type (variant, G_VARIANT_TYPE_VARDICT));

  if ((items = g_variant_lookup_value (variant, "items", G_VARIANT_TYPE ("av"))))
    {
      gsize n_children = g_variant_n_children (items);

      for (gsize i = 0; i < n_children; i++)
        {
          g_autoptr(GVariant) itemv = g_variant_get_child_value (items, i);
          g_autoptr(GVariant) infov = g_variant_get_variant (itemv);
          PanelLayoutItem *item = _panel_layout_item_new_from_variant (infov, error);

          if (item == NULL)
            return FALSE;

          g_ptr_array_add (self->items, g_steal_pointer (&item));
        }

      return TRUE;
    }
  else
    {
      g_set_error_literal (error,
                           G_IO_ERROR,
                           G_IO_ERROR_INVALID_DATA,
                           "items missing from variant");
      return FALSE;
    }
}

static gboolean
panel_layout_load (PanelLayout  *self,
                   GVariant     *variant,
                   GError      **error)
{
  guint version = 0;

  g_assert (PANEL_IS_LAYOUT (self));
  g_assert (variant != NULL);
  g_assert (g_variant_is_of_type (variant, G_VARIANT_TYPE_VARDICT));

  if (g_variant_lookup (variant, "version", "u", &version))
    {
      if (version == 1)
        return panel_layout_load_1 (self, variant, error);
    }

  g_set_error_literal (error,
                       G_IO_ERROR,
                       G_IO_ERROR_INVALID_DATA,
                       "Invalid version number in serialized layout");

  return FALSE;
}

/**
 * panel_layout_new_from_variant:
 * @variant: a #GVariant from panel_layout_to_variant()
 * @error: a location for a #GError, or %NULL
 *
 * Creates a new #PanelLayout from a #GVariant.
 *
 * This creates a new #PanelLayout instance from a previous layout
 * which had been serialized to @variant.
 *
 * Returns: (transfer full): a #PanelLayout
 */
PanelLayout *
panel_layout_new_from_variant (GVariant  *variant,
                               GError   **error)
{
  PanelLayout *self;

  g_return_val_if_fail (variant != NULL, NULL);
  g_return_val_if_fail (g_variant_is_of_type (variant, G_VARIANT_TYPE_VARDICT), NULL);

  self = g_object_new (PANEL_TYPE_LAYOUT, NULL);

  if (!panel_layout_load (self, variant, error))
    g_clear_object (&self);

  return self;
}

PanelLayout *
panel_layout_new (void)
{
  return g_object_new (PANEL_TYPE_LAYOUT, NULL);
}

guint
panel_layout_get_n_items (PanelLayout *self)
{
  g_return_val_if_fail (PANEL_IS_LAYOUT (self), 0);

  return self->items->len;
}

/**
 * panel_layout_get_item:
 * @self: a #PanelLayout
 * @position: the index of the item
 *
 * Gets the item at @position.
 *
 * Returns: (transfer none) (nullable): The #PanelLayoutItem at @position
 *   or %NULL if there is no item at that position.
 */
PanelLayoutItem *
panel_layout_get_item (PanelLayout *self,
                       guint        position)
{
  g_return_val_if_fail (PANEL_IS_LAYOUT (self), NULL);

  if (position >= self->items->len)
    return NULL;

  return g_ptr_array_index (self->items, position);
}

void
panel_layout_remove (PanelLayout     *self,
                     PanelLayoutItem *item)
{
  guint position;

  g_return_if_fail (PANEL_IS_LAYOUT (self));
  g_return_if_fail (PANEL_IS_LAYOUT_ITEM (item));

  if (g_ptr_array_find (self->items, item, &position))
    panel_layout_remove_at (self, position);
}

void
panel_layout_remove_at (PanelLayout *self,
                        guint        position)
{
  g_return_if_fail (PANEL_IS_LAYOUT (self));
  g_return_if_fail (position < self->items->len);

  g_ptr_array_remove_index (self->items, position);
}

void
panel_layout_append (PanelLayout     *self,
                     PanelLayoutItem *item)
{
  g_return_if_fail (PANEL_IS_LAYOUT (self));
  g_return_if_fail (PANEL_IS_LAYOUT_ITEM (item));

  g_ptr_array_add (self->items, g_object_ref (item));
}

void
panel_layout_prepend (PanelLayout     *self,
                      PanelLayoutItem *item)
{
  g_return_if_fail (PANEL_IS_LAYOUT (self));
  g_return_if_fail (PANEL_IS_LAYOUT_ITEM (item));

  g_ptr_array_insert (self->items, 0, g_object_ref (item));
}

void
panel_layout_insert (PanelLayout     *self,
                     guint            position,
                     PanelLayoutItem *item)
{
  g_return_if_fail (PANEL_IS_LAYOUT (self));
  g_return_if_fail (PANEL_IS_LAYOUT_ITEM (item));

  g_ptr_array_insert (self->items, position, g_object_ref (item));
}
