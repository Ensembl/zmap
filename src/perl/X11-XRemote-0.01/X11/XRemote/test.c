/*  Last edited: Nov 10 11:56 2006 (rds) */
/*  compile using compile_test.sh */

#include <ZMap/zmapXRemote.h>            /* Public header for Perl Stuff */
#include <stdlib.h>
#include <X11/Xlib.h>

Window createMeAWindow(Display *dpy);
void myUselessCallback(gpointer data);

int main(int argc, char *argv[]){
  zMapXRemoteObj obj;
  Window win;
  char *response;

  if(argc != 2)
    printf("Usage: ./test windowId\n"); /* e.g. windowId = 0x2200210 */

  if(obj = zMapXRemoteNew()){

    zMapXRemoteInitClient(obj, strtoul(argv[1], (char **)NULL, 16));

    zMapXRemoteSetRequestAtomName(obj, "_PERL_REQUEST_NAME");
    zMapXRemoteSetResponseAtomName(obj, "_PERL_RESPONSE_NAME");
    zMapXRemoteSendRemoteCommand(obj, "zoom_in", &response);
#ifdef AAAAA
    zMapXRemoteSetWindowID(obj, strtoul(argv[1], (char **)NULL, 16));
    zMapXRemoteSendRemoteCommand(obj, "zoom_in");
    printf("response is: %s\n", zMapXRemoteGetResponse(obj));
    zMapXRemoteSendRemoteCommand(obj, "zoomed_in");
    win = createMeAWindow( XOpenDisplay(getenv("DISPLAY")) );
    printf("Made a window with id 0x%0x\n",win);
    zMapXRemoteAddUserAtoms(obj, "alpha", "beta", win);
    zMapXRemoteAddPropertyNotifyHandler(obj, myUselessCallback, (gpointer)obj);
    zMapXRemoteSendUserRemoteCommand(obj, "boo.");
#endif
    printf("still running\n");
  }
}

void myUselessCallback(gpointer data)
{
  printf("%s\n", "I'm a useless callback short and stout.");
}

Window createMeAWindow(Display *dpy)
{
  /* this variable will store the ID of the newly created window. */
  Window win;
  /* these variables will store the window's width and height. */
  int win_width = 100;
  int win_height = 100;
  int screen_num = 0;
  /* these variables will store the window's location. */
  int win_x, win_border_width, win_border_height;
  int win_y;

  /* position of the window is top-left corner - 0,0. */
  win_x = win_y = win_border_height = win_border_width = 0;

  /* create the window, as specified earlier. */
  win = XCreateSimpleWindow(dpy,
                            RootWindow(dpy, screen_num),
                            win_x, win_y,
                            win_width, win_height,
                            win_border_width, BlackPixel(dpy, screen_num),
                            WhitePixel(dpy, screen_num));
  XMapWindow(dpy, win);
  XSync(dpy, False);
  return win;
}
