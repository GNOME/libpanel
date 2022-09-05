#include <libpanel.h>

static GMenuModel *menu_model;
static GdkRGBA white = {1,1,1,1};
static GdkRGBA grey;

static PanelFrame *
create_frame_cb (PanelGrid *grid)
{
  PanelFrame *frame = PANEL_FRAME (panel_frame_new ());
  PanelFrameHeader *header = PANEL_FRAME_HEADER (panel_frame_header_bar_new ());

  panel_frame_header_add_prefix (header, 0, gtk_label_new ("Start Child"));
  panel_frame_header_add_suffix (header, 0, gtk_label_new ("End Child"));
  panel_frame_set_header (frame, header);
  panel_frame_set_placeholder (frame,
                               g_object_new (GTK_TYPE_IMAGE,
                                             "icon-name", "tab-new-symbolic",
                                             "pixel-size", 128,
                                             NULL));

  return frame;
}

static GtkWidget *
get_default_focus (GtkWidget *page,
                   GtkWidget *text_view)
{
  g_assert (GTK_IS_TEXT_VIEW (text_view));
  return text_view;
}

static PanelWidget *
create_document (void)
{
  static guint count;
  char *title = g_strdup_printf ("Untitled Document %u", ++count);
  PanelWidget *ret;
  GtkTextView *text_view;

  text_view = g_object_new (GTK_TYPE_TEXT_VIEW,
                            "buffer", g_object_new (GTK_TYPE_TEXT_BUFFER,
                                                    "text", "Hello libpanel!",
                                                    NULL),
                            NULL);

  ret = g_object_new (PANEL_TYPE_WIDGET,
                      "can-maximize", TRUE,
                      "title", title,
                      "icon-name", "text-x-generic-symbolic",
                      "menu-model", menu_model,
                      "foreground-rgba", &grey,
                      "background-rgba", &white,
                      "child", g_object_new (GTK_TYPE_SCROLLED_WINDOW,
                                             "child", text_view,
                                             NULL),
                      NULL);
  g_signal_connect (ret,
                    "get-default-focus",
                    G_CALLBACK (get_default_focus),
                    text_view);

  g_free (title);

  return ret;
}

static void
add_child (PanelGrid *grid)
{
  PanelWidget *widget = create_document ();
  panel_grid_add (grid, widget);
  panel_widget_raise (widget);
}

static void
add_clicked_cb (PanelGrid *grid,
                GtkButton *button)
{
  g_assert (GTK_IS_BUTTON (button));
  g_assert (PANEL_IS_GRID (grid));

  add_child (grid);
}

int
main (int argc,
      char *argv[])
{
  GMainLoop *main_loop;
  GtkBuilder *builder;
  GtkWindow *window;
  char *filename = NULL;
  GtkBuilderScope *scope;
  GError *error = NULL;

  gtk_init ();
  panel_init ();

  gdk_rgba_parse (&grey, "#241f31");

  main_loop = g_main_loop_new (NULL, FALSE);
  filename = g_build_filename (g_getenv ("G_TEST_SRCDIR"), "test-dock.ui", NULL);
  builder = gtk_builder_new ();
  scope = gtk_builder_get_scope (builder);
  gtk_builder_cscope_add_callback_symbol (GTK_BUILDER_CSCOPE (scope), "create_frame_cb", G_CALLBACK (create_frame_cb));
  gtk_builder_cscope_add_callback_symbol (GTK_BUILDER_CSCOPE (scope), "add_clicked_cb", G_CALLBACK (add_clicked_cb));
  gtk_builder_add_from_file (builder, filename, &error);
  g_clear_pointer (&filename, g_free);
  g_assert_no_error (error);

  menu_model = G_MENU_MODEL (gtk_builder_get_object (builder, "page_menu"));

  add_child (PANEL_GRID (gtk_builder_get_object (builder, "grid")));

  window = GTK_WINDOW (gtk_builder_get_object (builder, "window"));
  g_signal_connect_swapped (window, "close-request", G_CALLBACK (g_main_loop_quit), main_loop);
  gtk_window_present (window);
  g_main_loop_run (main_loop);

  return 0;
}
