#include <libpanel.h>

int
main (int argc,
      char *argv[])
{
  GMainLoop *main_loop;
  GtkBuilder *builder;
  GtkWindow *window;
  g_autofree char *filename = NULL;
  GError *error = NULL;

  gtk_init ();
  panel_init ();

  main_loop = g_main_loop_new (NULL, FALSE);
  filename = g_build_filename (g_getenv ("G_TEST_SRCDIR"), "test-dock.ui", NULL);
  builder = gtk_builder_new ();
  gtk_builder_add_from_file (builder, filename, &error);
  g_assert_no_error (error);

  window = GTK_WINDOW (gtk_builder_get_object (builder, "window"));
  g_signal_connect_swapped (window, "close-request", G_CALLBACK (g_main_loop_quit), main_loop);
  gtk_window_present (window);
  g_main_loop_run (main_loop);

  return 0;
}
