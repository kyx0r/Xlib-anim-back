#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "sys/time.h"
#include <unistd.h>
#include <stdint.h>
#include <pthread.h>
#include <math.h>

#include "threadpool.h"

#define Dynamic 1

#define DEBUG
#ifdef DEBUG
#   define ASSERT(condition, message) \
    do { \
        if (! (condition)) \
		{ \
			printf("%s \n", message);\
			printf("Assertion %s failed in, %s line: %d \n", #condition, __FILE__, __LINE__);\
			char buf[10];					\
			fgets(buf, 10, stdin); \
			exit(1); \
        } \
    } while (0)
#else
#   define ASSERT(condition, message) do { } while (0)
#endif

static Display *dpy;
static int screen;
static Window root;
int w;
int h;
threadpool_t *pool[64];
int left;
pthread_mutex_t lock;
uint64_t timer_offset;
GC gc;
Pixmap tmpPix;

static void SetBackgroundToBitmap(Pixmap bitmap, unsigned int width, unsigned int height);
void SetPixel32(uint32_t* buf, int w, int h, int x, int y, uint32_t pixel);
Pixmap GetRootPixmap(Display* display, Window *root);
GC create_gc(Display* display, Window win, int reverse_video);
void CircleFrac();
void SnowFlake();
void Galaxy();
void CirclePurge();
uint64_t GetTimerValue();
double GetTime();

int main(int argc, char *argv[]) 
{
    dpy = XOpenDisplay(NULL);
    ASSERT(dpy, "Unable to open display!");
    screen = DefaultScreen(dpy);
    root = XRootWindow(dpy, screen);
    ASSERT(root, "Unable to open root window!");
    w = XDisplayWidth(dpy, screen);
    h = XDisplayHeight(dpy, screen);   

    Visual* visual = DefaultVisual(dpy, screen);

    char* data = (char*)malloc(w*h*4);

    XImage* img = XCreateImage(dpy,visual,DefaultDepth(dpy,screen),ZPixmap,0,data,w,h,32,0);

    XInitThreads();

    pthread_mutex_init(&lock, NULL);

    pool[0] = threadpool_create(4, 16192, 0);

    ASSERT(threadpool_add(pool[0], &CircleFrac, img, 0) == 0, "Failed threadpool_add");
    ASSERT(threadpool_add(pool[0], &SnowFlake, img, 0) == 0, "Failed threadpool_add");
    ASSERT(threadpool_add(pool[0], &Galaxy, img, 0) == 0, "Failed threadpool_add");
    ASSERT(threadpool_add(pool[0], &CirclePurge, img, 0) == 0, "Failed threadpool_add");
    
    timer_offset = GetTimerValue();
    
    //XImage* img = XGetImage(dpy, root, 0, 0, w, h, ~0, ZPixmap);

    tmpPix = XCreatePixmap(dpy, root, w, h, DefaultDepth(dpy, screen));
    //gc = create_gc(dpy, tmpPix, 0);
     
    double t1 = 0;
    double t2 = 0;
    double diff = 0;
    
    while(1)
      {
	t1 = GetTime();
	diff = t1 - t2;
        if(diff > 0.01f)
	  {
	    XPutImage(dpy, tmpPix, DefaultGC(dpy,screen), img, 0, 0, 0, 0, w, h);
	    XSetWindowBackgroundPixmap(dpy, root, tmpPix);
	    XClearWindow(dpy, root);
	    pthread_mutex_lock(&lock);
	    left = 1;
	    pthread_mutex_unlock(&lock);	    
	  }
	else
	  {
	    pthread_mutex_lock(&lock);
	    left = 0;
	    pthread_mutex_unlock(&lock);
	    usleep((uint64_t)(diff * 1000000));
	    continue;
	  }
	t2 = GetTime();
      }
    
    XCloseDisplay(dpy);
    exit (0);
}

struct particle
{
  double x;
  double y;
  double speed;
  double direction;
};

void SnowFlake(XImage* img)
{
  int copy = 0;
  double t1 = 0;
  double t2 = 0;
  double diff = 0;
  int i = 0;
  clock_t ticks;
  int transition = 0;
  uint32_t color = 0xFFFFFFFF;
  
  struct particle buf[4096];

  for(i = 0; i<4096; i++)
    {
      buf[i].direction = (2 * M_PI * rand()) / RAND_MAX;
      buf[i].speed = (0.08 * rand()) / RAND_MAX;
      buf[i].speed *= buf[i].speed;
    }
  
  while(1)
    {
      ticks += clock();
      t1 = GetTime();
      diff = t1-t2;            

      for(i = 0; i<4096; i++)
	{
	  buf[i].direction += (diff) * 0.000635;
	  buf[i].x += (buf[i].speed * cos(buf[i].direction)) * diff;
	  buf[i].y += (buf[i].speed * sin(buf[i].direction)) * diff;
	}
      
      pthread_mutex_lock(&lock);
      copy = left;
      pthread_mutex_unlock(&lock);      
      if(diff > 0.01f || copy > 0)
	{
	  for(i=0; i<4096; i++)
	    {
	      int x = (buf[i].x + 1) * (w/2);
	      int y = (buf[i].y * (w/2)) + (h/2);
	      if (x < 0 || x >= w || y < 0 || y >= h)
	      	{
		  transition++;
	      	  continue;
	      	}
	      XPutPixel(img, x, y, color);
	    }
	  //printf("%d\n", transition);
	  if(transition > 1000)
	    {
	      for(i = 0; i<4096; i++)
		{
		  buf[i].x = 0;
		  buf[i].y = 0;
		}
	      unsigned char red = (unsigned char)((1 + sin(ticks * 0.0001)) * 128);
	      unsigned char green = (unsigned char)((1 + sin(ticks * 0.0002)) * 128);
	      unsigned char blue = (unsigned char)((1 + sin(ticks * 0.0003)) * 128);
	      unsigned char alpha = (unsigned char)((1 + sin(ticks * 0.0004)) * 128);
	      color = 0;
	      color+=blue;
	      color <<=8;
	      color+=green;
	      color <<=8;
	      color+=red;
	      color <<=8;
	      color +=alpha;
	    }
	  transition = 0;
 	}
      else
	{
	    usleep((uint64_t)(diff * 1000000));
	    continue;
	}
      t2 = GetTime();
    }
  return;
}

void Circle(XImage* img, int32_t centreX, int32_t centreY, int32_t radius, uint32_t color)
{
  const int32_t diameter = (radius * 2);

  int32_t x = (radius - 1);
  int32_t y = 0;
  int32_t tx = 1;
  int32_t ty = 1;
  int32_t error = (tx - diameter);

  while (x >= y)
    {
      //  Each of the following renders an octant of the circle
      if (!(centreX + x < 0 || centreX + x >= w || centreY - y < 0 || centreY - y >= h))
	{
	  XPutPixel(img, centreX + x, centreY - y, color);
	}

      if (!(centreX - x < 0 || centreX - x >= w || centreY + y < 0 || centreY + y >= h))
	{
	  XPutPixel(img, centreX - x, centreY + y, color);
	}

      if (!(centreX + x < 0 || centreX + x >= w || centreY + y < 0 || centreY + y >= h))
	{
	  XPutPixel(img, centreX + x, centreY + y, color);
	}

      if (!(centreX - x < 0 || centreX - x >= w || centreY - y < 0 || centreY - y >= h))
	{
 	  XPutPixel(img, centreX - x, centreY - y, color);
	}      

      if (!(centreX + y < 0 || centreX + y >= w || centreY - x < 0 || centreY - x >= h))
	{	      		  	  
	  XPutPixel(img, centreX + y, centreY - x, color);
	}      

      if (!(centreX - y < 0 || centreX - y >= w || centreY + x < 0 || centreY + x >= h))
	{	      
	  XPutPixel(img, centreX - y, centreY + x, color);
	}

      if (!(centreX + y < 0 || centreX + y >= w || centreY + x < 0 || centreY + x >= h))
	{	      
	  XPutPixel(img, centreX + y, centreY + x, color);

	}

      if (!(centreX - y < 0 || centreX - y >= w || centreY - x < 0 || centreY - x >= h))
	{	      
	  XPutPixel(img, centreX - y, centreY - x, color);
	}
	  
      if (error <= 0)
	{
	  ++y;
	  error += ty;
	  ty += 2;
	}

      if (error > 0)
	{
	  --x;
	  tx += 2;
	  error += (tx - diameter);
	}
    }
}

void CirclePurge(XImage* img)
{
  double t1 = 0;
  double t2 = 0;
  double diff = 0;
  int i = 10;
  uint32_t color = 0;

  int x = w/2;
  int y = h/2;
  int limit;
  if(x>y)
    {
      limit = x;
    }
  else
    {
      limit = y;
    }
  
  while(1)
    {
      t1 = GetTime();
      diff = t1-t2;            

      i++;
      
      if(diff > 0.01f)
	{
	  Circle(img, x, y, i, color);
	  if(i>limit+150) //accomodate the curve
	    {
	      i = 10;
	    }
 	}
      else
	{
	  usleep((uint64_t)(diff * 1000000));
	  continue;
	}
      t2 = GetTime();
    }
  return;
}


void CircleFrac(XImage* img)
{
  int copy = 0;
  double t1 = 0;
  double t2 = 0;
  double diff = 0;
  int i = 0;
  clock_t ticks;
  clock_t t3;
  clock_t t4;
  clock_t mod;
  
  unsigned long val = 0;
  struct particle buf[4096];

  for(i = 0; i<4096; i++)
    {
      buf[i].direction = (2 * M_PI * rand()) / RAND_MAX;
      buf[i].speed = (0.08 * rand()) / RAND_MAX;
      buf[i].speed *= buf[i].speed;
    }
  
  while(1)
    {
      t3 = clock();
      ticks += t3;
      t1 = GetTime();
      diff = t1-t2;
      mod = t3-t4;
      unsigned char red = (unsigned char)((1 + sin(ticks * 0.0001)) * 128);
      unsigned char green = (unsigned char)((1 + sin(ticks * 0.0002)) * 128);
      unsigned char blue = (unsigned char)((1 + sin(ticks * 0.0003)) * 128);

      for(i = 0; i<4096; i++)
	{
	  buf[i].direction += (mod) * 0.000635;
	  buf[i].x += (buf[i].speed * cos(buf[i].direction)) * mod;
	  buf[i].y += (buf[i].speed * sin(buf[i].direction)) * mod;
	}
      
      pthread_mutex_lock(&lock);
      copy = left;
      pthread_mutex_unlock(&lock);      
      if(diff > 0.01f || copy > 0)
	{
	  for(i=0; i<4096; i++)
	    {
	      int x = (buf[i].x + 1) * (w/2);
	      int y = (buf[i].y * (w/2)) + (h/2);
	      if (x < 0 || x >= w || y < 0 || y >= h)
	      	{
	      	  continue;
	      	}
	      
	      val+=blue;
	      val <<=8;
	      val+=green;
	      val <<=8;
	      val+=red;
	      val <<=8;
	      //	      val +=0xFF;
	      XPutPixel(img, x, y, val);
	    }
 	}
      else
	{
	    usleep((uint64_t)(diff * 1000000));
	    t4 = clock();
	    continue;
	}
      t4 = clock();
      t2 = GetTime();
    }
  return;
}

void Galaxy(XImage* img)
{
  int copy = 0;
  double t1 = 0;
  double t2 = 0;
  double diff = 0;
  int i = 0;
  clock_t ticks;
  
  unsigned long val = 0;
  struct particle buf[4096];

  for(i = 0; i<4096; i++)
    {
      buf[i].direction = (2 * M_PI * rand()) / RAND_MAX;
      buf[i].speed = (0.08 * rand()) / RAND_MAX;
      buf[i].speed *= buf[i].speed;
    }
  
  while(1)
    {
      ticks += clock();
      t1 = GetTime();
      diff = t1-t2;            
      unsigned char red = (unsigned char)((1 + sin(ticks * 0.0001)) * 128);
      unsigned char green = (unsigned char)((1 + sin(ticks * 0.0002)) * 128);
      unsigned char blue = (unsigned char)((1 + sin(ticks * 0.0003)) * 128);

      for(i = 0; i<4096; i++)
	{
	  int mod = rand() % 5;
	  buf[i].direction += (mod) * 0.000635;
	  buf[i].x += (buf[i].speed * cos(buf[i].direction)) * mod;
	  buf[i].y += (buf[i].speed * sin(buf[i].direction)) * mod;
	}
      
      pthread_mutex_lock(&lock);
      copy = left;
      pthread_mutex_unlock(&lock);      
      if(diff > 0.01f || copy > 0)
	{
	  for(i=0; i<4096; i++)
	    {
	      int x = (buf[i].x + 1) * (w/2);
	      int y = (buf[i].y * (w/2)) + (h/2);
	      if (x < 0 || x >= w || y < 0 || y >= h)
	      	{
	      	  continue;
	      	}
	      
	      val+=blue;
	      val <<=8;
	      val+=green;
	      val <<=8;
	      val+=red;
	      val <<=8;
	      val +=0xFF;
	      XPutPixel(img, x, y, val);
	    }
 	}
      else
	{
	    usleep((uint64_t)(diff * 1000000));
	    continue;
	}
      t2 = GetTime();
    }
  return;
}

static unsigned long NameToPixel(char *name, unsigned long pixel)
{
    XColor ecolor;

    if (!name || !*name)
	return pixel;
    if (!XParseColor(dpy,DefaultColormap(dpy,screen),name,&ecolor)) {
	fprintf(stderr,"%s:  unknown color \"%s\"\n","",name);
	exit(1);
	/*NOTREACHED*/
    }
    if (!XAllocColor(dpy, DefaultColormap(dpy, screen),&ecolor)) {
	fprintf(stderr, "%s:  unable to allocate color for \"%s\"\n",
		"", name);
	exit(1);
	/*NOTREACHED*/
    }
    if ((ecolor.pixel != BlackPixel(dpy, screen)) &&
	(ecolor.pixel != WhitePixel(dpy, screen)) &&
	(DefaultVisual(dpy, screen)->class & Dynamic))

    return(ecolor.pixel);
}

static void SetBackgroundToBitmap(Pixmap bitmap, unsigned int width, unsigned int height)
{
    Pixmap pix;
    GC gc;
    XGCValues gc_init;

    gc_init.foreground = NameToPixel(NULL, BlackPixel(dpy, screen));
    gc_init.background = NameToPixel(NULL, WhitePixel(dpy, screen));
    gc = XCreateGC(dpy, root, GCForeground|GCBackground, &gc_init);
    pix = XCreatePixmap(dpy, root, width, height,
			(unsigned int)DefaultDepth(dpy, screen));
    XCopyPlane(dpy, bitmap, pix, gc, 0, 0, width, height, 0, 0, (unsigned long)1);
    XSetWindowBackgroundPixmap(dpy, root, pix);
    XFreeGC(dpy, gc);
    XFreePixmap(dpy, bitmap);
    XFreePixmap(dpy, pix);
    XClearWindow(dpy, root);
}

Pixmap GetRootPixmap(Display* display, Window *root)
{
    Pixmap currentRootPixmap;
    Atom act_type;
    int act_format;
    unsigned long nitems, bytes_after;
    unsigned char *data = NULL;
    Atom _XROOTPMAP_ID;

    _XROOTPMAP_ID = XInternAtom(display, "_XROOTPMAP_ID", False);

    if (XGetWindowProperty(display, *root, _XROOTPMAP_ID, 0, 1, False,
                XA_PIXMAP, &act_type, &act_format, &nitems, &bytes_after,
                &data) == Success) {

        if (data) {
            currentRootPixmap = *((Pixmap *) data);
            XFree(data);
        }
    }

    return currentRootPixmap;
}

uint64_t GetTimerValue()
{
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return (uint64_t) tv.tv_sec * (uint64_t) 1000000 + (uint64_t) tv.tv_usec;
}

double GetTime()
{
  return (double)(GetTimerValue()-timer_offset) / 1000000;
}

GC create_gc(Display* display, Window win, int reverse_video)
{
  GC gc;				/* handle of newly created GC.  */
  unsigned long valuemask = 0;		/* which values in 'values' to  */
					/* check when creating the GC.  */
  XGCValues values;			/* initial values for the GC.   */
  unsigned int line_width = 2;		/* line width for the GC.       */
  int line_style = LineSolid;		/* style for lines drawing and  */
  int cap_style = CapButt;		/* style of the line's edje and */
  int join_style = JoinBevel;		/*  joined lines.		*/
  int screen_num = DefaultScreen(display);

  gc = XCreateGC(display, win, valuemask, &values);
  if (gc < 0) {
	fprintf(stderr, "XCreateGC: \n");
  }

  /* allocate foreground and background colors for this GC. */
  if (reverse_video) {
    XSetForeground(display, gc, WhitePixel(display, screen_num));
    XSetBackground(display, gc, BlackPixel(display, screen_num));
  }
  else {
    XSetForeground(display, gc, BlackPixel(display, screen_num));
    XSetBackground(display, gc, WhitePixel(display, screen_num));
  }

  /* define the style of lines that will be drawn using this GC. */
  XSetLineAttributes(display, gc,
                     line_width, line_style, cap_style, join_style);

  /* define the fill style for the GC. to be 'solid filling'. */
  XSetFillStyle(display, gc, FillSolid);

  return gc;
}
