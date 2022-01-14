#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/Xatom.h>

// * 2 to make it look a bit less choppy
#define REFRESH_RATE 60 * 2

#define FRAME_PAD_TOP 20
#define FRAME_PAD_BOTTOM 2
#define FRAME_PAD_LEFT 2
#define FRAME_PAD_RIGHT 2
#define FRAME_PAD_HEIGHT (FRAME_PAD_TOP + FRAME_PAD_BOTTOM)
#define FRAME_PAD_WIDTH (FRAME_PAD_LEFT + FRAME_PAD_RIGHT)


typedef struct managedWnd_s
{
    Window client;
    Window frame;
    GC gc;

    int x,y,w,h;

    struct managedWnd_s* last;
} managedWnd_t;

Display* g_display;
XFontStruct* g_font;
Window g_rootWindow;

managedWnd_t* g_managedWindows = 0;

managedWnd_t* g_focusWnd = 0;

void setWindowPos(Window wnd, int x, int y)
{
        XWindowChanges wc;
        wc.x = x;
        wc.y = y;
        XConfigureWindow(g_display, wnd, CWX|CWY, &wc);
        XSync(g_display, False);
}

void setWindowGeo(Window wnd, int x, int y, int w, int h)
{
        XWindowChanges wc;
        wc.x = x;
        wc.y = y;
        wc.width = w;
        wc.height = h;
        //wc.border_width = 20;
        XConfigureWindow(g_display, wnd, CWX|CWY|CWWidth|CWHeight/*|CWBorderWidth*/, &wc);
        XSync(g_display, False);
}

/*
void drawTitleBar(titleBar_t* bar)
{
    const char* str = "hello!";
    unsigned int len = strlen(str);
    XCharStruct extents;
    int di;
    XTextExtents(g_font, str, len, &di,&di,&di, &extents);
    XDrawString(g_display, bar->wnd, bar->gc, 0, extents.ascent, str, len);

    str = "__  []  X";
    len = strlen(str);
    XTextExtents(g_font, str, len, &di,&di,&di, &extents);
    XDrawString(g_display, bar->wnd, bar->gc, 500 - extents.width, extents.ascent, str, len);


        //XClearWindow(g_display, bar->wnd);

        int ypos =40;
        int flip = 0;
        XSetBackground(g_display, bar->gc, BlackPixel(g_display, DefaultScreen(g_display)));
        XSetForeground(g_display, bar->gc, WhitePixel(g_display, DefaultScreen(g_display)));
        unsigned int windowCount = 0;
        Window dw1, dw2, *windows;
        XQueryTree(g_display, g_rootWindow, &dw1, &dw2, &windows, &windowCount);
        for(int i = 0; i < windowCount; i++)
        {
            XTextProperty name;
            if(!XGetWMName(g_display, windows[i], &name))
                continue;
            unsigned int titlelen = strlen(name.value);
            if(titlelen > 0)
            {
                XCharStruct extents;
                int di;
                XTextExtents(g_font, name.value, titlelen,&di,&di,&di, &extents);

                XFillRectangle(g_display, bar->wnd, bar->gc, 0, ypos, extents.width, extents.ascent);
                if(flip)
                {
                    XSetBackground(g_display, bar->gc, BlackPixel(g_display, DefaultScreen(g_display)));
                    XSetForeground(g_display, bar->gc, WhitePixel(g_display, DefaultScreen(g_display)));
                }
                else
                {
                    XSetBackground(g_display, bar->gc, WhitePixel(g_display, DefaultScreen(g_display)));
                    XSetForeground(g_display, bar->gc, BlackPixel(g_display, DefaultScreen(g_display)));
                }
                flip = !flip;

                ypos += extents.ascent;
                XDrawString(g_display, bar->wnd, bar->gc, 0, ypos, name.value, titlelen);

                XWMHints* hints = XGetWMHints(g_display, windows[i]);
                if(hints && hints->icon_pixmap)
                    XCopyPlane(g_display, hints->icon_pixmap, bar->wnd, bar->gc,
                        0, 0,
                        16, 16,
                        extents.width,ypos,
                        1);
            }
            XFree(name.value);
            // XDrawImageString

        }
        XFree(windows);

//        XDrawString(g_display, bar->wnd, bar->gc, extents.width, 20+extents.ascent, str, len);


        //XFlushGC(g_display, bar->gc);
adope}
*/

void invertGCColor(int invert, GC gc)
{
    if(invert)
    {
        XSetBackground(g_display, gc, BlackPixel(g_display, DefaultScreen(g_display)));
        XSetForeground(g_display, gc, WhitePixel(g_display, DefaultScreen(g_display)));
    }
    else
    {
        XSetBackground(g_display, gc, WhitePixel(g_display, DefaultScreen(g_display)));
        XSetForeground(g_display, gc, BlackPixel(g_display, DefaultScreen(g_display)));
    }
}
void paintDecorations(managedWnd_t* mng)
{
    XTextProperty name;
    if(!XGetWMName(g_display, mng->client, &name))
        return;

    invertGCColor(1/*g_focusWnd != mng*/, mng->gc);
    XFillRectangle(g_display, mng->frame, mng->gc, 0, 0, mng->w, FRAME_PAD_TOP);
    invertGCColor(0/*g_focusWnd == mng*/, mng->gc);

    unsigned int titlelen = strlen(name.value);
    if(titlelen > 0)
    {
        XCharStruct extents;
        int di;
        XTextExtents(g_font, name.value, titlelen,&di,&di,&di, &extents);
        if(name.format == 8)
            XDrawString(g_display, mng->frame, mng->gc, 0, extents.ascent, name.value, titlelen);
        else if(name.format == 16)
            XDrawString16(g_display, mng->frame, mng->gc, 0, extents.ascent, name.value, titlelen);
    }
    XFree(name.value);

    // Buttons
    const char* str = "__  []  X";
    unsigned int len = strlen(str);
    XCharStruct extents;
    int di;
    XTextExtents(g_font, str, len, &di,&di,&di, &extents);
    XDrawString(g_display, mng->frame, mng->gc, mng->w - extents.width, extents.ascent, str, len);
}

void createTitleBar(managedWnd_t* mng)
{
    int screen_num = DefaultScreen(g_display);
	Window bar = XCreateSimpleWindow(g_display, g_rootWindow, mng->x, mng->y, mng->w + FRAME_PAD_WIDTH, mng->h + FRAME_PAD_HEIGHT,
            0, BlackPixel(g_display,screen_num),
            WhitePixel(g_display,screen_num));

    XSelectInput(g_display, bar, KeyPressMask | ExposureMask | ButtonPressMask | ButtonReleaseMask | StructureNotifyMask);

    GC gc = XCreateGC(g_display, bar, 0, NULL);
    XSetFont (g_display, gc, g_font->fid);

	XMapWindow(g_display,bar);

    mng->gc = gc;
    mng->frame = bar;
}

void manageWindow(Window wnd, XWindowAttributes* att)
{
    managedWnd_t* mng = malloc(sizeof(managedWnd_t));

    mng->x = att->x;
    mng->y = att->y;
    mng->w = att->width;
    mng->h = att->height;

    mng->client = wnd;

    createTitleBar(mng);

    XReparentWindow(g_display, wnd, mng->frame, FRAME_PAD_LEFT, FRAME_PAD_TOP);

    mng->last = g_managedWindows;
    g_managedWindows = mng;
}

void checkManage(Window wnd)
{
    // Do we manage this already?
    // FIXME: Don't run this on push of existing windows!
    for(managedWnd_t* mng = g_managedWindows; mng; mng = mng->last)
    {
        if(mng->frame == wnd || mng->client == wnd)
            return;
    }

    XWindowAttributes att;
    if(!XGetWindowAttributes(g_display, wnd, &att))
        return;

    if(att.override_redirect || att.class == InputOnly
    || att.map_state == IsUnmapped )
        return;

    // Is it a normal window?
    XWMHints *wmhints = XGetWMHints(g_display, wnd);
    if (wmhints) {
        int dock = ((wmhints->flags & StateHint) && wmhints->initial_state != NormalState);
        XFree(wmhints);
        if(dock)
            return;
    }

    // Manage it
    manageWindow(wnd, &att);
}

void manageExistingWindows()
{
    unsigned int windowCount = 0;
    Window dw1, dw2, *windows;
    XQueryTree(g_display, g_rootWindow, &dw1, &dw2, &windows, &windowCount);
    for(int i = 0; i < windowCount; i++)
    {
        checkManage(windows[i]);
    }
    XFree(windows);

}

void unmanageAll()
{
    for(managedWnd_t* mng = g_managedWindows; mng; mng = mng->last)
    {
        XReparentWindow(g_display, mng->client, g_rootWindow, mng->x, mng->y);

        XDestroyWindow(g_display, mng->frame);
    }
}

managedWnd_t* findWindowFromTitle(Window wnd)
{
    for(managedWnd_t* mng = g_managedWindows; mng; mng = mng->last)
    {
        if(mng->frame == wnd)
            return mng;
    }
    return 0;
}

int main()//int argc, char** args)
{
	Display* display = XOpenDisplay(":0");
	if(display)
		printf("Got Display!");
	else
	{
		printf("epic fail!");
		return 1;
	}
    g_display = display;

    int screen_num = DefaultScreen(display);
	Window rootWnd = RootWindow(g_display, screen_num);
    g_rootWindow = rootWnd;
    Cursor hand_cursor = XCreateFontCursor(display, XC_hand2);

//XSelectInput(display, RootWindow(display, screen_num),SubstructureNotifyMask);

    g_font = XLoadQueryFont (g_display, "fixed");
    
    //XChangeWindowAttributes(g_display, rootWnd, CWEventMask|CWCursor, &wa);
	XSelectInput(g_display, rootWnd, SubstructureRedirectMask|SubstructureNotifyMask
		|ButtonPressMask|StructureNotifyMask|PropertyChangeMask);

    manageExistingWindows();

    int run = 1;
	XEvent event;
    while (run) {
        XNextEvent(display, &event);

        //XFillRectangle(display, rootWnd, rootGc, 400, 400,100,100);
        if(event.type == Expose)
        {

            if (event.xexpose.count != 0)
                continue;
            managedWnd_t* mng = findWindowFromTitle(event.xexpose.window);

            if(mng)
                paintDecorations(mng);

        }
        else if(event.type == KeyPress)
        {
#define EXIT_KEY (Mod1Mask|ShiftMask)
            if((event.xkey.state & EXIT_KEY ) == EXIT_KEY)
                run = 0;
        }
        else if(event.type == MapRequest)
        {
            if(event.xmap.window)
                checkManage(event.xmap.window);
        }
        else if(event.type == ButtonPress)
        {
            if(event.xbutton.state & Mod4Mask)
            {
                run = 0;
                continue;
            }

            Window bar = event.xbutton.window;
            managedWnd_t* mng = findWindowFromTitle(bar);
            if(!mng)
                continue;

            // Change focus
            XRaiseWindow(g_display, bar);
            XSetInputFocus(g_display, bar, RevertToParent, CurrentTime);

            // Take the mouse input
            XGrabPointer(display, bar, True,
                    ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
                    GrabModeAsync, GrabModeAsync,
                    None, hand_cursor, CurrentTime);

            // Right mouse button resizes
            int resizing = event.xbutton.button == Button3;

            Time lasttime = CurrentTime;

            XWindowAttributes att;
            if(!XGetWindowAttributes(g_display, bar, &att))
                continue;

            int startx  = event.xmotion.x_root;
            int starty  = event.xmotion.y_root;

            int deltax = att.x - startx;
            int deltay = att.y - starty;

            do
            {
                XNextEvent(display, &event);
                switch(event.type)
                {
                case MotionNotify:
                    // No need to update our position more than we can see
                    if ((event.xmotion.time - lasttime) < (1000 / REFRESH_RATE)) continue;

                    lasttime = event.xmotion.time;

                    if(resizing)
                    {
                        mng->x = att.x;
                        mng->y = att.y;
                        mng->w = att.width + event.xmotion.x_root - startx;
                        mng->h = att.height + event.xmotion.y_root - starty;
                        setWindowGeo(bar, mng->x, mng->y, mng->w, mng->h);
                        setWindowGeo(bar, mng->x, mng->y, mng->w, mng->h);
                        //setWindowGeo(mng->client, FRAME_PAD_LEFT, FRAME_PAD_TOP, mng->w - FRAME_PAD_WIDTH, mng->h - FRAME_PAD_HEIGHT);

                    }
                    else
                    {
                        mng->x = event.xmotion.x_root + deltax;
                        mng->y = event.xmotion.y_root + deltay;
                        setWindowPos(bar, mng->x, mng->y);
                    }

                }
            } while (event.type != ButtonRelease);
            
            if(resizing)
            {
                setWindowGeo(mng->client, FRAME_PAD_LEFT, FRAME_PAD_TOP, mng->w - FRAME_PAD_WIDTH, mng->h - FRAME_PAD_HEIGHT);
            }


            // Release control over the pointer
            XUngrabPointer(display, CurrentTime);


            // Redraw
            paintDecorations(mng);
            XFlush(display);

            /*
            // Push a redraw to the old focus
            XEvent  exppp;
            memset(&exppp, 0, sizeof(XEvent));
            exppp.type = Expose;
            exppp.xexpose.window = g_focusWnd->window;
            XSendEvent(display,g_focusWnd->window,False,ExposureMask,&exppp);
            XFlush(display);
            */

            // Swap the focus
            g_focusWnd = mng;
        }

    }
//	getchar();

    printf("end!");

    unmanageAll();

	XCloseDisplay(display);

	return 0;
}
