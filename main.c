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


typedef struct managedWnd_t
{
    Window client;
    Window frame;
    GC gc;

    int x,y,w,h;

    struct managedWnd_t* last, *next;
} managedWnd_t;

int s_wmRunning = 1;

Display* g_display;
XFontStruct* g_font;
Window g_rootWindow;
Cursor g_handCursor;
Screen* g_screen;

managedWnd_t* g_managedWindows = 0;
managedWnd_t* g_focusWnd = 0;


struct 
{
    Window wnd;
    GC gc;

    int w,h;
} g_desktopWnd;

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

managedWnd_t* getManaged(Window wnd)
{
    for(managedWnd_t* m = g_managedWindows; m; m = m->last)
    {
        if(m->frame == wnd)
            return 0;
        if(m->client == wnd)
            return m;
    }
    return 0;
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


void createDesktopWindow()
{
	Window wnd = XCreateSimpleWindow(g_display, g_rootWindow, 0, 0, g_screen->width, g_screen->height,
            0, g_screen->white_pixel,
            g_screen->black_pixel);

    XSelectInput(g_display, wnd, KeyPressMask | ExposureMask | ButtonPressMask | ButtonReleaseMask | StructureNotifyMask);

    GC gc = XCreateGC(g_display, wnd, 0, NULL);
    XSetFont(g_display, gc, g_font->fid);

	XMapWindow(g_display,wnd);

    // Send it to the bottom so it doesn't cover over anything
    XLowerWindow(g_display, wnd);

    XSetBackground(g_display, gc, g_screen->black_pixel);
    XSetForeground(g_display, gc, g_screen->white_pixel);

    g_desktopWnd.wnd = wnd;
    g_desktopWnd.gc = gc;
    g_desktopWnd.w = g_screen->width;
    g_desktopWnd.h = g_screen->height;
}


void paintDesktop()
{
    const int barsz = 5;
    
    const int xinc = barsz * 16;
    const int yinc = barsz * 16;

    int w = g_desktopWnd.w, h = g_desktopWnd.h;

    for(int y = 0; y < h; y += yinc)
    for(int x = y / yinc % 2 == 0 ? 0 : yinc * 0.5f; x < w; x += xinc)
        XFillRectangle(g_display, g_desktopWnd.wnd, g_desktopWnd.gc, x, y, barsz, barsz);
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

void createFrame(managedWnd_t* mng)
{
	Window bar = XCreateSimpleWindow(g_display, g_desktopWnd.wnd, mng->x, mng->y, mng->w + FRAME_PAD_WIDTH, mng->h + FRAME_PAD_HEIGHT,
            0, g_screen->white_pixel,
            g_screen->black_pixel);

    XSelectInput(g_display, bar, KeyPressMask | ExposureMask | ButtonPressMask | ButtonReleaseMask | StructureNotifyMask);

    GC gc = XCreateGC(g_display, bar, 0, NULL);
    XSetFont (g_display, gc, g_font->fid);


    mng->gc = gc;
    mng->frame = bar;
}

managedWnd_t* manageWindow(Window wnd, XWindowAttributes* att)
{
    managedWnd_t* mng = malloc(sizeof(managedWnd_t));

    mng->x = att->x;
    mng->y = att->y;
    mng->w = att->width;
    mng->h = att->height;

    mng->client = wnd;

    createFrame(mng);
    
    XAddToSaveSet(g_display, wnd);
    XReparentWindow(g_display, wnd, mng->frame, FRAME_PAD_LEFT, FRAME_PAD_TOP);

    XMapWindow(g_display, mng->frame);

    mng->last = g_managedWindows;
    mng->next = 0;
    if(g_managedWindows)
        g_managedWindows->next = mng;
    g_managedWindows = mng;

    printf("Managing new window\n");


    return mng;
}

/*
void checkManage(Window wnd)
{
    // Is it the desktop?
    if(wnd == g_desktopWnd.wnd)
        return;

    // Do we manage this already?
    managedWnd_t* mng = 0;
    for(managedWnd_t* m = g_managedWindows; m; m = mng->last)
    {
        if(m->client == wnd)
        {
            mng = m;
            break;
        }

        // Don't do work on frames!
        if(m->frame == wnd)
            return;
    }


    XWindowAttributes att;
    if(!XGetWindowAttributes(g_display, wnd, &att))
        return;
    if(att.override_redirect || att.class == InputOnly
    || att.map_state == IsUnmapped )
        return;

    // Is it a normal window?
    int state = WithdrawnState;
    XWMHints *wmhints = XGetWMHints(g_display, wnd);
    if(wmhints)
    {
        if(wmhints->flags & StateHint)
            state = wmhints->initial_state;
        XFree(wmhints);
    }
  /*
    if (wmhints) {
        int dock = ((wmhints->flags & StateHint) && wmhints->initial_state != NormalState);
        if(dock)
            return;
    }
  * /

    switch (state) {
    case WithdrawnState:
        if(mng)
        {
            // Already tracking it!
            return;
        }

        // Manage it

        // Fall through
        // NO break
    case NormalState:
        break;
    case IconicState:
        break;
    }

    if(!mng)
    {
        mng = manageWindow(wnd, &att);
        if(mng)
        {
            //XMapWindow(g_display, mng->client);
            XMapWindow(g_display, mng->frame);
        }
        
    }
/*
    if(!mng)
    {
        //XMapWindow(g_display, mng->frame);

    }
* /

}*/

void checkManage(Window wnd)
{
    if(wnd == g_desktopWnd.wnd)
        return;

    // Do we manage this already?
    managedWnd_t* mng = getManaged(wnd);

    XWindowAttributes att;
    if(!XGetWindowAttributes(g_display, wnd, &att))
        return;

    if(att.override_redirect || att.class == InputOnly)
        return;

    
    int state = WithdrawnState;
    XWMHints *wmhints = XGetWMHints(g_display, wnd);
    if(wmhints)
    {
        if(wmhints->flags & StateHint)
        {
            state = wmhints->initial_state;
            switch (state) {
            case WithdrawnState:
                printf("WithdrawnState\n");
            case NormalState:
                printf("NormalState\n");

                break;
            case IconicState:
                printf("IconicState\n");
                break;
            }
        }
        XFree(wmhints);
    }
    //else
    //    setWindowPos(wnd, 20, 20);

    if(!mng)
    {
        mng = manageWindow(wnd, &att);
        XMapWindow(g_display, mng->client);
    }
    //XRaiseWindow(g_display, mng->client);       
    //XRaiseWindow(g_display, mng->frame);    
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

void unmanageWindow(Window wnd)
{
    managedWnd_t* mng = getManaged(wnd);
    if(!mng)
        return;

    XReparentWindow(g_display, mng->client, g_rootWindow, mng->x, mng->y);
    XDestroyWindow(g_display, mng->frame);
    XRemoveFromSaveSet(g_display, mng->client);
    if(mng->last)
        mng->last->next = mng->next;
    if(mng->next)
        mng->next->last = mng->last;

    if(g_managedWindows == mng)
        g_managedWindows = mng->last;
    free(mng);
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

void exposureEvent(XEvent* event)
{
    if (event->xexpose.count != 0)
        return;

    Window wnd = event->xexpose.window;

    // Are we painting the desktop? Or maybe one of our frames?
    if(wnd == g_desktopWnd.wnd)
    {
        paintDesktop();

    }
    else
    {
        if (event->xexpose.count != 0)
            return;

        managedWnd_t* mng = findWindowFromTitle(wnd);

        if(mng)
            paintDecorations(mng);
    }
}

void processEvent(XEvent* event)
{
    if(event->type == UnmapNotify)
    {
        printf("UNMAP\n");
        unmanageWindow(event->xunmap.window);
    }
    else if(event->type == Expose)
    {  
        exposureEvent(event);
    }
    else if(event->type == KeyPress)
    {
#define EXIT_KEY (Mod1Mask|ShiftMask)
        if((event->xkey.state & EXIT_KEY ) == EXIT_KEY)// && event->xkey.keycode == XK_X)
        {
            s_wmRunning = 0;
            return;
        }
        
        
    }
    else if(event->type == MapRequest)
    {
        if(event->xmap.window)
            checkManage(event->xmap.window);
    }
    else if(event->type == ConfigureRequest)
    {
        XConfigureEvent* xce = &event->xconfigurerequest;
        managedWnd_t* mng = getManaged(event->xconfigurerequest.window);
        if(mng)
        {
            mng->x = xce->x;
            mng->y = xce->y;
            mng->w = xce->width;
            mng->h = xce->height;
            setWindowGeo(mng->frame, mng->x, mng->y, mng->w, mng->h);
            setWindowGeo(mng->client, FRAME_PAD_LEFT, FRAME_PAD_TOP, mng->w - FRAME_PAD_WIDTH, mng->h - FRAME_PAD_HEIGHT);
        }
    }
    else if(event->type == ButtonPress)
    {


        Window bar = event->xbutton.window;
        managedWnd_t* mng = findWindowFromTitle(bar);
        if(!mng)
            return;

        // Change focus
        XRaiseWindow(g_display, bar);
        XSetInputFocus(g_display, bar, RevertToParent, CurrentTime);

        // Take the mouse input
        XGrabPointer(g_display, bar, True,
                ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
                GrabModeAsync, GrabModeAsync,
                None, g_handCursor, CurrentTime);

        // Right mouse button resizes
        int resizing = event->xbutton.button == Button3;

        Time lasttime = CurrentTime;

        XWindowAttributes att;
        if(!XGetWindowAttributes(g_display, bar, &att))
            return;

        int startx  = event->xmotion.x_root;
        int starty  = event->xmotion.y_root;

        int deltax = att.x - startx;
        int deltay = att.y - starty;

        do
        {
            XNextEvent(g_display, event);
            switch(event->type)
            {
            case MotionNotify:
                // No need to update our position more than we can see
                if ((event->xmotion.time - lasttime) < (1000 / REFRESH_RATE)) continue;

                lasttime = event->xmotion.time;

                if(resizing)
                {
                    // Window Resizing
                    mng->x = att.x;
                    mng->y = att.y;
                    mng->w = att.width + event->xmotion.x_root - startx;
                    mng->h = att.height + event->xmotion.y_root - starty;
                    if(mng->w < 5)
                        mng->w = 5;
                    if(mng->h < 5)
                        mng->h = 5;
                        
                    setWindowGeo(bar, mng->x, mng->y, mng->w, mng->h);
                    //setWindowGeo(bar, mng->x, mng->y, mng->w, mng->h);
                    //setWindowGeo(mng->client, FRAME_PAD_LEFT, FRAME_PAD_TOP, mng->w - FRAME_PAD_WIDTH, mng->h - FRAME_PAD_HEIGHT);
                }
                else
                {
                    // Window Dragging
                    mng->x = event->xmotion.x_root + deltax;
                    mng->y = event->xmotion.y_root + deltay;
                    setWindowPos(bar, mng->x, mng->y);
                }
                break;
            case ButtonPress: // We're already buttonpress
                break;
            default:
                // Route all other events the normal way
                processEvent(event);
                break;
            }
        } while (event->type != ButtonRelease);
        
        if(resizing)
        {
            setWindowGeo(mng->client, FRAME_PAD_LEFT, FRAME_PAD_TOP, mng->w - FRAME_PAD_WIDTH, mng->h - FRAME_PAD_HEIGHT);
        }

        // Release control over the pointer
        XUngrabPointer(g_display, CurrentTime);

        // Redraw
        //paintDecorations(mng);
        //XFlush(display);

        // Swap the focus
        g_focusWnd = mng;
    }
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

    int screennum = DefaultScreen(display);
	Window rootWnd = RootWindow(g_display, screennum);
    g_rootWindow = rootWnd;
    g_handCursor = XCreateFontCursor(display, XC_hand2);
    g_screen = XScreenOfDisplay(g_display, screennum);
    
    g_font = XLoadQueryFont (g_display, "fixed");
    

    // Create the Desktop Window
    createDesktopWindow();

    // Manage all of our windows
    //XChangeWindowAttributes(g_display, rootWnd, CWEventMask|CWCursor, &wa);
	XSelectInput(g_display, rootWnd, SubstructureRedirectMask|SubstructureNotifyMask
		|ButtonPressMask|StructureNotifyMask|PropertyChangeMask);
    manageExistingWindows();


    s_wmRunning = 1;
	XEvent event;
    while (s_wmRunning) {
        XNextEvent(display, &event);
        processEvent(&event);
        //sleep(20);
    }


    printf("end!");

    unmanageAll();

	XCloseDisplay(display);

	return 0;
}
