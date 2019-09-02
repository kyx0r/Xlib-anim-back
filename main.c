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

static void SetBackgroundToBitmap(Pixmap bitmap, unsigned int width, unsigned int height);
void SetPixel32(uint32_t* buf, int w, int h, int x, int y, uint32_t pixel);
Pixmap GetRootPixmap(Display* display, Window *root);
void CircleFrac();
void SnowFlake();
void Galaxy();
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

    pool[0] = threadpool_create(1, 8096, 0);

    ASSERT(threadpool_add(pool[0], &CircleFrac, img, 0) == 0, "Failed threadpool_add");

    timer_offset = GetTimerValue();
    
    //XImage* img = XGetImage(dpy, root, 0, 0, w, h, ~0, ZPixmap);

    Pixmap tmpPix = XCreatePixmap(dpy, root, w, h, DefaultDepth(dpy, screen));
    
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
	      	  continue;
	      	}
	      
	      val+=blue;
	      val <<=8;
	      val+=green;
	      val <<=8;
	      val+=red;
	      val <<=8;
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
