#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/XKBlib.h>

char *key_name[] = {
    "first",
    "second (or middle)",
    "third"
};

struct SendStruct {
    int type;
    int x;
    int y;
};

int main(int argc, char **argv)
{
    Display *display;
    XEvent xevent;
    Window window;

    if( (display = XOpenDisplay(NULL)) == NULL )
        return -1;


    window = DefaultRootWindow(display);
    XAllowEvents(display, AsyncBoth, CurrentTime);

    XGrabPointer(display, 
                 window,
                 1, 
                 PointerMotionMask | ButtonPressMask | ButtonReleaseMask , 
                 GrabModeAsync,
                 GrabModeAsync, 
                 None,
                 None,
                 CurrentTime);

    XGrabKeyboard(display, window, false, GrabModeAsync, GrabModeAsync, CurrentTime);

    while(1) {
        XNextEvent(display, &xevent);
        int mykey;
        switch (xevent.type) {
            case MotionNotify:
                printf("Mouse move      : [%d, %d]\n", xevent.xmotion.x_root, xevent.xmotion.y_root);
                break;
            case ButtonPress:
                printf("Button pressed  : %s, %d\n", key_name[xevent.xbutton.button - 1], xevent.xbutton.button);
                break;
            case ButtonRelease:
                printf("Button released : %s, %d\n", key_name[xevent.xbutton.button - 1], xevent.xbutton.button);
                break;
            case KeyPress:
                mykey = XKeycodeToKeysym(display, xevent.xkey.keycode, 0);
                printf("KeyPress : %s, %d\n", XKeysymToString(mykey), mykey);
                
                if (xevent.xkey.keycode == 27 || xevent.xkey.keycode == 9) {
                    return 0;
                }
                break;
            case KeyRelease:
                mykey = XKeycodeToKeysym(display, xevent.xkey.keycode, 0);
                printf("KeyRelease : %s, %d\n", XKeysymToString(mykey), mykey);
                break;
        }
    }

    return 0;
}