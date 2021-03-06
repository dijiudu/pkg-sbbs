#include <stdlib.h>

#include <gtk/gtk.h>

#include "dirwrap.h"

#include "chatfuncs.h"
#include "interface.h"
#include "support.h"

int lprintf(int level, char *fmt, ...)
{
	/* ToDo: Log output */
	return(0);
}


int
main (int argc, char *argv[])
{
  int		node;
  char		*ctrl_dir;
  GtkWidget *WaitWindow;

  gtk_set_locale ();
  gtk_init (&argc, &argv);

/*  add_pixmap_directory (PACKAGE_DATA_DIR "/" PACKAGE "/pixmaps"); */

  ctrl_dir=getenv("SBBSCTRL");
  if(ctrl_dir==NULL)
  	return(1);

  if(argc<1)
    return(1);

  node=atoi(argv[1]);

  if(node<1)
    return(1);

  /*
   * Request a chat
   */
  if(chat_open(node, ctrl_dir))
  	return(1);

  /*
   * Show "waiting" window
   */
  WaitWindow = create_WaitWindow ();
  gtk_widget_show (WaitWindow);

  gtk_main ();
  return 0;
}
