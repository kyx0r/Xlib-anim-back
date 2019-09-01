#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
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

static void SetBackgroundToBitmap(Pixmap bitmap, unsigned int width, unsigned int height);
void SetPixel32(uint32_t* buf, int w, int h, int x, int y, uint32_t pixel);
Pixmap GetRootPixmap(Display* display, Window *root);
void algorithm();

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

    ASSERT(threadpool_add(pool[0], &algorithm, img, 0) == 0, "Failed threadpool_add");    
    
    //XImage* img = XGetImage(dpy, root, 0, 0, w, h, ~0, ZPixmap);

    Pixmap tmpPix = XCreatePixmap(dpy, root, w, h, DefaultDepth(dpy, screen));
    
    clock_t t1;
    clock_t t2;
    clock_t diff;
    
    while(1)
      {
	t1 = clock();
	diff = t1-t2;
	if(diff > 1)
	  {
	    XPutImage(dpy, tmpPix, DefaultGC(dpy,screen), img, 0, 0, 0, 0, w, h);
	    XSetWindowBackgroundPixmap(dpy, root, tmpPix);
	    XClearWindow(dpy, root);
	    pthread_mutex_lock(&lock);
	    left = diff;
	    pthread_mutex_unlock(&lock);	    
	  }
	else
	  {
	    pthread_mutex_lock(&lock);
	    left = 0;
	    pthread_mutex_unlock(&lock);				   	    
	    sleep(1);    		    
	  }
	t2 = clock();
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

void algorithm(XImage* img)
{
  int copy = 0;
  clock_t t1;
  clock_t t2 = 0;
  clock_t diff;
  int i = 0;
  //double dmlsec;
  uint32_t ticks;
  uint32_t interval;
  uint32_t prev = 0;
  
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
      t1 = clock();
      ticks += t1;
      unsigned char red = (unsigned char)((1 + sin(ticks * 0.0001)) * 128);
      unsigned char green = (unsigned char)((1 + sin(ticks * 0.0002)) * 128);
      unsigned char blue = (unsigned char)((1 + sin(ticks * 0.0003)) * 128);       
      diff = t1-t2;

      //dmlsec += (double)(diff)/CLOCKS_PER_SEC * 1000;
      
      // printf("%lf \n", dmlsec);

      interval = ticks - prev;
      
      for(i = 0; i<4096; i++)
	{
	  buf[i].direction += (interval) * 0.000635;
	  buf[i].x += (buf[i].speed * cos(buf[i].direction)) * interval;
	  buf[i].y += (buf[i].speed * sin(buf[i].direction)) * interval;
	  //	  printf("x: %lf\n", (double)buf[i].x);
	}

      prev = ticks;
      
      pthread_mutex_lock(&lock);
      copy = left;
      pthread_mutex_unlock(&lock);      
      if(diff > 1 || copy > 0)
	{
	  for(i=0; i<4096; i++)
	    {
	      int x = (buf[i].x + 1) * (w/2);
	      int y = (buf[i].y * (w/2)) + (h/2);
	      //printf("x: %d\n", x);
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
	      //printf("x: %d\n", (int)(buf[i].x + 1) * 1920/2);
	      /* XPutPixel(img, 10, 10, val); */
	      /* XPutPixel(img, 11, 11, val); */
	      /* XPutPixel(img, 11, 10, val); */
	      /* XPutPixel(img, 10, 11, val); */
	      XPutPixel(img, x, y, val);
	    }
 	}
      else
	{				   	    
	  sleep(1);    		    
	}
      t2 = clock();
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
