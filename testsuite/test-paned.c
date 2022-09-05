#include <libpanel.h>

int
main (int argc,
      char *argv[])
{
  GtkWidget *window;
  GtkWidget *paned;
  GMainLoop *main_loop;

  gtk_init ();
  panel_init ();

  main_loop = g_main_loop_new (NULL, FALSE);
  window = gtk_window_new ();
  paned = panel_paned_new ();
  gtk_window_set_child (GTK_WINDOW (window), paned);

  for (guint i = 0; i < 5; i++)
    {
      char *label = g_strdup_printf ("Button %u", i);
      GtkWidget *button = gtk_button_new ();
      gtk_button_set_label (GTK_BUTTON (button), label);
      panel_paned_append (PANEL_PANED (paned), button);

      if (i == 2)
        gtk_widget_set_hexpand (button, TRUE);

      g_signal_connect_swapped (button, "clicked", G_CALLBACK (panel_paned_remove), paned);

      g_clear_pointer (&label, g_free);
    }

  g_signal_connect_swapped (window, "close-request", G_CALLBACK (g_main_loop_quit), main_loop);
  gtk_window_present (GTK_WINDOW (window));
  g_main_loop_run (main_loop);

  return 0;
}
