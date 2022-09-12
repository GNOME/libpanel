#include <libpanel.h>

#include "panel-layout-private.h"

static gboolean
position_equal (PanelPosition *a,
                PanelPosition *b)
{
  return panel_position_get_area_set (a) == panel_position_get_area_set (b) &&
         panel_position_get_column_set (a) == panel_position_get_column_set (b) &&
         panel_position_get_row_set (a) == panel_position_get_row_set (b) &&
         panel_position_get_depth_set (a) == panel_position_get_depth_set (b) &&
         panel_position_get_area (a) == panel_position_get_area (b) &&
         panel_position_get_column (a) == panel_position_get_column (b) &&
         panel_position_get_row (a) == panel_position_get_row (b) &&
         panel_position_get_depth (a) == panel_position_get_depth (b);
}

static void
test_layout (void)
{
  PanelLayoutItem *item1, *alt_item1;
  PanelPosition *position1, *alt_position1;
  PanelLayout *layout;
  PanelLayout *recreated;
  GVariant *variant;
  GError *error = NULL;

  layout = g_object_new (PANEL_TYPE_LAYOUT, NULL);

  position1 = panel_position_new ();
  panel_position_set_area (position1, PANEL_AREA_CENTER);
  panel_position_set_column (position1, 1);
  panel_position_set_row (position1, 2);
  panel_position_set_depth (position1, 3);

  item1 = panel_layout_item_new ();
  panel_layout_item_set_id (item1, "item-1");
  panel_layout_item_set_type_hint (item1, "Item1");
  panel_layout_item_set_metadata (item1, "Item1:Key", "s", "Item1:Value");
  panel_layout_item_set_position (item1, position1);
  panel_layout_append (layout, item1);

  variant = panel_layout_to_variant (layout);
  g_assert_nonnull (variant);
  g_assert_true (g_variant_is_of_type (variant, G_VARIANT_TYPE_VARDICT));

  recreated = panel_layout_new_from_variant (variant, &error);
  g_assert_no_error (error);
  g_assert_true (PANEL_IS_LAYOUT (recreated));

  g_assert_cmpint (panel_layout_get_n_items (recreated), >, 0);
  g_assert_cmpint (panel_layout_get_n_items (recreated), ==, panel_layout_get_n_items (layout));

  alt_item1 = panel_layout_get_item (recreated, 0);

  g_assert_nonnull (alt_item1);
  g_assert_cmpstr (panel_layout_item_get_id (item1),
                   ==,
                   panel_layout_item_get_id (alt_item1));
  g_assert_cmpstr (panel_layout_item_get_type_hint (item1),
                   ==,
                   panel_layout_item_get_type_hint  (alt_item1));

  alt_position1 = panel_layout_item_get_position (alt_item1);
  g_assert_nonnull (alt_position1);
  g_assert_true (position_equal (position1, alt_position1));

  g_assert_finalize_object (g_steal_pointer (&layout));
  g_assert_finalize_object (g_steal_pointer (&recreated));
  g_assert_finalize_object (g_steal_pointer (&item1));
  g_assert_finalize_object (g_steal_pointer (&position1));
  g_clear_pointer (&variant, g_variant_unref);

  g_assert_null (error);
  g_assert_null (layout);
  g_assert_null (recreated);
  g_assert_null (variant);
}

int
main (int argc,
      char *argv[])
{
  gtk_init ();
  g_test_init (&argc, &argv, NULL);
  g_test_add_func ("/Layout/basic", test_layout);
  return g_test_run ();
}
