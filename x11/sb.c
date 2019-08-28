/* PDCurses */

#include "pdcx11.h"

/*man-start**************************************************************

sb
--

### Synopsis

    int sb_init(void)
    int sb_set_horz(int total, int viewport, int cur)
    int sb_set_vert(int total, int viewport, int cur)
    int sb_get_horz(int *total, int *viewport, int *cur)
    int sb_get_vert(int *total, int *viewport, int *cur)
    int sb_refresh(void);

### Description

   These functions manipulate the scrollbar.

### Return Value

   All functions return OK on success and ERR on error.

### Portability
                             X/Open  ncurses  NetBSD
    sb_init                     -       -       -
    sb_set_horz                 -       -       -
    sb_set_vert                 -       -       -
    sb_get_horz                 -       -       -
    sb_get_vert                 -       -       -
    sb_refresh                  -       -       -

**man-end****************************************************************/

#include "scrlbox.h"

bool sb_started = FALSE;

#if NeedWidePrototypes
# define PDC_SCROLLBAR_TYPE double
#else
# define PDC_SCROLLBAR_TYPE float
#endif

static Widget scrollBox, scrollVert, scrollHoriz;

static void _scroll_up_down(Widget w, XtPointer client_data,
    XtPointer call_data)
{
    int pixels = (long) call_data;
    int total_y = SP->sb_total_y * font_height;
    int viewport_y = SP->sb_viewport_y * font_height;
    int cur_y = SP->sb_cur_y * font_height;

    /* When pixels is negative, right button pressed, move data down,
       thumb moves up.  Otherwise, left button pressed, pixels positive,
       move data up, thumb down. */

    cur_y += pixels;

    /* limit panning to size of overall */

    if (cur_y < 0)
        cur_y = 0;
    else
        if (cur_y > (total_y - viewport_y))
            cur_y = total_y - viewport_y;

    SP->sb_cur_y = cur_y / font_height;

    XawScrollbarSetThumb(w, (double)((double)cur_y / (double)total_y),
                         (double)((double)viewport_y / (double)total_y));
}

static void _scroll_left_right(Widget w, XtPointer client_data,
    XtPointer call_data)
{
    int pixels = (long) call_data;
    int total_x = SP->sb_total_x * font_width;
    int viewport_x = SP->sb_viewport_x * font_width;
    int cur_x = SP->sb_cur_x * font_width;

    cur_x += pixels;

    /* limit panning to size of overall */

    if (cur_x < 0)
        cur_x = 0;
    else
        if (cur_x > (total_x - viewport_x))
            cur_x = total_x - viewport_x;

    SP->sb_cur_x = cur_x / font_width;

    XawScrollbarSetThumb(w, (double)((double)cur_x / (double)total_x),
                         (double)((double)viewport_x / (double)total_x));
}

static void _thumb_up_down(Widget w, XtPointer client_data,
    XtPointer call_data)
{
    double percent = *(double *) call_data;
    double total_y = (double)SP->sb_total_y;
    double viewport_y = (double)SP->sb_viewport_y;
    int cur_y = SP->sb_cur_y;

    /* If the size of the viewport is > overall area simply return,
       as no scrolling is permitted. */

    if (SP->sb_viewport_y >= SP->sb_total_y)
        return;

    if ((SP->sb_cur_y = (int)((double)total_y * percent)) >=
        (total_y - viewport_y))
        SP->sb_cur_y = total_y - viewport_y;

    XawScrollbarSetThumb(w, (double)(cur_y / total_y),
                         (double)(viewport_y / total_y));
}

static void _thumb_left_right(Widget w, XtPointer client_data,
    XtPointer call_data)
{
    double percent = *(double *) call_data;
    double total_x = (double)SP->sb_total_x;
    double viewport_x = (double)SP->sb_viewport_x;
    int cur_x = SP->sb_cur_x;

    if (SP->sb_viewport_x >= SP->sb_total_x)
        return;

    if ((SP->sb_cur_x = (int)((float)total_x * percent)) >=
        (total_x - viewport_x))
        SP->sb_cur_x = total_x - viewport_x;

    XawScrollbarSetThumb(w, (double)(cur_x / total_x),
                         (double)(viewport_x / total_x));
}

bool XC_scrollbar_init(void)
{
    if (xc_app_data.scrollbarWidth && sb_started)
    {
        scrollBox = XtVaCreateManagedWidget(program_name,
            scrollBoxWidgetClass, topLevel, XtNwidth,
            window_width + xc_app_data.scrollbarWidth,
            XtNheight, window_height + xc_app_data.scrollbarWidth,
            XtNwidthInc, font_width, XtNheightInc, font_height, NULL);

        drawing = XtVaCreateManagedWidget(program_name,
            boxWidgetClass, scrollBox, XtNwidth,
            window_width, XtNheight, window_height, XtNwidthInc,
            font_width, XtNheightInc, font_height, NULL);

        scrollVert = XtVaCreateManagedWidget("scrollVert",
            scrollbarWidgetClass, scrollBox, XtNorientation,
            XtorientVertical, XtNheight, window_height, XtNwidth,
            xc_app_data.scrollbarWidth, NULL);

        XtAddCallback(scrollVert, XtNscrollProc, _scroll_up_down, drawing);
        XtAddCallback(scrollVert, XtNjumpProc, _thumb_up_down, drawing);

        scrollHoriz = XtVaCreateManagedWidget("scrollHoriz",
            scrollbarWidgetClass, scrollBox, XtNorientation,
            XtorientHorizontal, XtNwidth, window_width, XtNheight,
            xc_app_data.scrollbarWidth, NULL);

        XtAddCallback(scrollHoriz, XtNscrollProc, _scroll_left_right,
            drawing);
        XtAddCallback(scrollHoriz, XtNjumpProc, _thumb_left_right, drawing);

        return TRUE;
    }

    return FALSE;
}

/* sb_init() is the sb initialization routine.
   This must be called before initscr(). */

int sb_init(void)
{
    PDC_LOG(("sb_init() - called\n"));

    if (SP)
        return ERR;

    sb_started = TRUE;

    return OK;
}

/* sb_set_horz() - Used to set horizontal scrollbar.

   total = total number of columns
   viewport = size of viewport in columns
   cur = current column in total */

int sb_set_horz(int total, int viewport, int cur)
{
    PDC_LOG(("sb_set_horz() - called: total %d viewport %d cur %d\n",
             total, viewport, cur));

    if (!SP)
        return ERR;

    SP->sb_total_x = total;
    SP->sb_viewport_x = viewport;
    SP->sb_cur_x = cur;

    return OK;
}

/* sb_set_vert() - Used to set vertical scrollbar.

   total = total number of columns on line
   viewport = size of viewport in columns
   cur = current column in total */

int sb_set_vert(int total, int viewport, int cur)
{
    PDC_LOG(("sb_set_vert() - called: total %d viewport %d cur %d\n",
             total, viewport, cur));

    if (!SP)
        return ERR;

    SP->sb_total_y = total;
    SP->sb_viewport_y = viewport;
    SP->sb_cur_y = cur;

    return OK;
}

/* sb_get_horz() - Used to get horizontal scrollbar.

   total = total number of lines
   viewport = size of viewport in lines
   cur = current line in total */

int sb_get_horz(int *total, int *viewport, int *cur)
{
    PDC_LOG(("sb_get_horz() - called\n"));

    if (!SP)
        return ERR;

    if (total)
        *total = SP->sb_total_x;
    if (viewport)
        *viewport = SP->sb_viewport_x;
    if (cur)
        *cur = SP->sb_cur_x;

    return OK;
}

/* sb_get_vert() - Used to get vertical scrollbar.

   total = total number of lines
   viewport = size of viewport in lines
   cur = current line in total */

int sb_get_vert(int *total, int *viewport, int *cur)
{
    PDC_LOG(("sb_get_vert() - called\n"));

    if (!SP)
        return ERR;

    if (total)
        *total = SP->sb_total_y;
    if (viewport)
        *viewport = SP->sb_viewport_y;
    if (cur)
        *cur = SP->sb_cur_y;

    return OK;
}

/* sb_refresh() - Used to draw the scrollbars. */

int sb_refresh(void)
{
    PDC_LOG(("sb_refresh() - called\n"));

    if (!SP)
        return ERR;

    if (SP->sb_on)
    {
        PDC_SCROLLBAR_TYPE total_y = SP->sb_total_y;
        PDC_SCROLLBAR_TYPE total_x = SP->sb_total_x;

        if (total_y)
            XawScrollbarSetThumb(scrollVert,
                (PDC_SCROLLBAR_TYPE)(SP->sb_cur_y) / total_y,
                (PDC_SCROLLBAR_TYPE)(SP->sb_viewport_y) / total_y);

        if (total_x)
            XawScrollbarSetThumb(scrollHoriz,
                (PDC_SCROLLBAR_TYPE)(SP->sb_cur_x) / total_x,
                (PDC_SCROLLBAR_TYPE)(SP->sb_viewport_x) / total_x);
    }

    return OK;
}
