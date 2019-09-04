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
int left = 0;
pthread_mutex_t lock;
uint64_t timer_offset;
GC gc;
Pixmap tmpPix;

static void SetBackgroundToBitmap(Pixmap bitmap, unsigned int width, unsigned int height);
void SetPixel32(uint32_t* buf, int w, int h, int x, int y, uint32_t pixel);
Pixmap GetRootPixmap(Display* display, Window *root);
GC create_gc(Display* display, Window win, int reverse_video);
void CircleFill(XImage* img, int32_t centreX, int32_t centreY, int32_t radius, uint32_t color);
void Circle(XImage* img, int32_t centreX, int32_t centreY, int32_t radius, uint32_t color);
int bhm_line(XImage* img, uint32_t color, int x1,int y1,int x2,int y2);
void CircleFrac();
void SnowFlake();
void Galaxy();
void CirclePurge();
void Lightning();
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

    pool[0] = threadpool_create(64, 16192, 0);

    ASSERT(threadpool_add(pool[0], &CircleFrac, img, 0) == 0, "Failed threadpool_add");  //1  order is crucial
    ASSERT(threadpool_add(pool[0], &SnowFlake, img, 0) == 0, "Failed threadpool_add");   //2
    ASSERT(threadpool_add(pool[0], &Galaxy, img, 0) == 0, "Failed threadpool_add");      //3
    ASSERT(threadpool_add(pool[0], &CirclePurge, img, 0) == 0, "Failed threadpool_add"); //4

    timer_offset = GetTimerValue();
    
    //XImage* img = XGetImage(dpy, root, 0, 0, w, h, ~0, ZPixmap);

    tmpPix = XCreatePixmap(dpy, root, w, h, DefaultDepth(dpy, screen));
    //gc = create_gc(dpy, tmpPix, 0);
     
    double t1 = 0;
    double t2 = 0;
    double diff = 0;
    uint64_t tick1 = 0;
    uint64_t tick2 = 0;
    int copy;
    
    while(1)
      {
	t1 = GetTime();
	tick1++;
	diff = t1 - t2;
        if(diff > 0.01f)
	  {
	    XPutImage(dpy, tmpPix, DefaultGC(dpy,screen), img, 0, 0, 0, 0, w, h);
	    XSetWindowBackgroundPixmap(dpy, root, tmpPix);
	    XClearWindow(dpy, root);
	  }
	else
	  {
	    pthread_mutex_lock(&lock);
	    copy = left;
	    pthread_mutex_unlock(&lock);
	    
	    if((char)copy == -1) //feedback signal
	      {
		    pthread_mutex_lock(&lock);
		    left = 0;
		    pthread_mutex_unlock(&lock);
		    //printf("0x%08x\n", copy);
		    //printf("0x%01x\n", *((uint8_t*)&copy+3));
		    if(*((uint8_t*)&copy+3) == 0x1 && *((uint8_t*)&copy+2) == 0x1)
		      {
			printf("Thread 1 aborted, starting next algorithm\n");
			ASSERT(threadpool_add(pool[0], &Lightning, img, 0) == 0, "Failed threadpool_add"); //5
		      }
		    if(*((uint8_t*)&copy+3) == 0x5 && *((uint8_t*)&copy+2) == 0x1)
		      {
			printf("Thread 5 aborted, starting next algorithm\n");
			ASSERT(threadpool_add(pool[0], &CircleFrac, img, 0) == 0, "Failed threadpool_add"); //1
		      }		    
   
	      }
	    else if(copy == 1) //wait for circle purge
	      {
		tick2++;
		if(tick2 > 1000)
		  {
		    pthread_mutex_lock(&lock);
		    left = 0;
		    pthread_mutex_unlock(&lock);
		  }
	      }
	    else if(copy == 3)
	      {
		    pthread_mutex_lock(&lock);
		    left = 256; //set exit signal for thread 1
		    pthread_mutex_unlock(&lock);		
	      }
	    else if(copy == 4)
	      {
		    pthread_mutex_lock(&lock);
		    left = 260; //set exit signal for thread 5
		    pthread_mutex_unlock(&lock);		
	      }	    
	    else
	      {
		if(tick2 < tick1)
		  {
		    tick2 = ((rand() % 1000) + 100);
		    tick2 += tick1;
		  }
		else
		  {
		    if(tick1 == tick2)
		      {
			pthread_mutex_lock(&lock);
			left = (rand() % 4) + 1;
			pthread_mutex_unlock(&lock);
			tick2 = 0;
		      }
		    else
		      {
			//printf("dst %ld\n", tick2);
			//printf("tick: %ld\n", tick1);
		      }
		  }				
	      }	    
	    usleep((uint64_t)(diff * 1000000)); //70 fps
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

struct bolt_t
{
  uint16_t x;
  uint16_t y;
  uint16_t len;
  int x_comp;
  int y_comp;
  double angle;
  int sx;
  int sy;
};

void Lightning(XImage* img)
{
  int copy = 0;
  double t1 = 0;
  double t2 = 0;
  double diff = 0;
  int i = 0;
  clock_t ticks;
  int transition = 0;
  uint32_t color = 0xFFFFFFFF;
  
  struct bolt_t bolt[100];

  for(i=0; i<10; i++)
    {
      bolt[i].x = (rand() % w);
      bolt[i].y = (rand() % h);
      bolt[i].len = (rand() % 20)+1;
      bolt[i].angle = rand() % 360;
      bolt[i].x_comp = bolt[i].len * cos(-bolt[i].angle*M_PI/180) + bolt[i].x;
      bolt[i].y_comp = bolt[i].len * sin(-bolt[i].angle*M_PI/180) + bolt[i].y;
      bolt[i].sx = (bolt[i].x_comp - bolt[i].x);
      bolt[i].sy = (bolt[i].y_comp - bolt[i].y);

      /* printf("x: %d\n", bolt[i].x); */
      /* printf("y: %d\n", bolt[i].y); */
      /* printf("len: %d\n", bolt[i].len); */
      /* printf("angle: %lf\n", bolt[i].angle); */
      /* printf("xcomp: %d\n", bolt[i].x_comp); */
      /* printf("ycomp: %d\n", bolt[i].y_comp); */
      /* printf("sx: %d\n", bolt[i].sx); */
      /* printf("sy: %d\n", bolt[i].sy); */
    }

  
  int x = 0;
  int y = 0;
  while(1)
    {
      ticks += clock();
      t1 = GetTime();
      diff = t1-t2;
                 
      if(diff > 0.01f)
	{
	  for(i=0; i<10; i++)
	    {
	      bolt[i].x += bolt[i].sx ;
	      bolt[i].y += bolt[i].sy ;
	      x = bolt[i].x;
	      y = bolt[i].y;
	      int len = bhm_line(img, color, x, y, x-bolt[i].sx, y-bolt[i].sy);
	      if(len > bolt[i].len)
		{
		  if (x < 0 || x >= w || y < 0 || y >= h)
		    {
		      bolt[i].x = rand() % w;
		      bolt[i].y = rand() % h;
		    }
		  else
		    {
		      bolt[i].x = x;
		      bolt[i].y = y;
		    }
		  bolt[i].len = (rand() % 20)+1;
		  bolt[i].angle = rand() % 360;
		  bolt[i].x_comp = bolt[i].len * cos(-bolt[i].angle*M_PI/180) + bolt[i].x;
		  bolt[i].y_comp = bolt[i].len * sin(-bolt[i].angle*M_PI/180) + bolt[i].y;
		  bolt[i].sx = (bolt[i].x_comp - bolt[i].x);
		  bolt[i].sy = (bolt[i].y_comp - bolt[i].y);		  
		}
	    }
	}
      else
	{
	  pthread_mutex_lock(&lock);
	  copy = left;
	  pthread_mutex_unlock(&lock);
	  if(copy == 260) 
	    {
	      pthread_mutex_lock(&lock);
	      left = -1;
	      *((char*)&left+3)=5;
	      *((char*)&left+2)=1; 
	      pthread_mutex_unlock(&lock);
	      return;
	    }	  
	    usleep((uint64_t)(diff * 1000000));
	    continue;
	}
      t2 = GetTime();
    }
  return;  
}

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
                 
      if(diff > 0.01f)
	{
	  
	  for(i=0; i<4096; i++)
	    {
	      buf[i].direction += (diff) * 0.000635;
	      buf[i].x += (buf[i].speed * cos(buf[i].direction)) * diff;
	      buf[i].y += (buf[i].speed * sin(buf[i].direction)) * diff;
		      
	      int x = (buf[i].x + 1) * (w/2);
	      int y = (buf[i].y * (w/2)) + (h/2);
	      if (x < 0 || x >= w || y < 0 || y >= h)
	      	{
		  transition++;
	      	  continue;
	      	}
	      XPutPixel(img, x, y, color);
	    }
	  if(transition > 750)
	    {
	      pthread_mutex_lock(&lock);
	      left = 1;
	      pthread_mutex_unlock(&lock); 	      
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
	  pthread_mutex_lock(&lock);
	  copy = left;
	  pthread_mutex_unlock(&lock);
	  if(copy == 257) //random events index ranges to 255 max, so 257-255 = 2, which is our thread.
	    {
	      pthread_mutex_lock(&lock);
	      left = -1;
	      *((char*)&left+3)=2;
	      *((char*)&left+2)=1; // 1 = terminated thread
	      pthread_mutex_unlock(&lock);
	      return;
	    }	  
	    usleep((uint64_t)(diff * 1000000));
	    continue;
	}
      t2 = GetTime();
    }
  return;
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
  int copy;
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
	  if(copy == 1)
	    {
	      CircleFill(img, x, y, i, color);
	      goto lim;
	    }
	  int width = rand() % 10;
	  for(int z = 0; z<width; z++)
	    {
	      Circle(img, x, y, i+z, color);
	    }
	  i += width;
	lim:
	  if(i>limit+150) //accomodate the curve
	    {
	      i = 10;
	    }
 	}
      else
	{
	  pthread_mutex_lock(&lock);
	  copy = left;
	  pthread_mutex_unlock(&lock);
	  if(copy == 259) 
	    {
	      pthread_mutex_lock(&lock);
	      left = -1;
	      *((char*)&left+3)=4;
	      *((char*)&left+2)=1; // 1 = terminated thread
	      pthread_mutex_unlock(&lock);
	      return;
	    }	  
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
  clock_t ticks = 0;
  clock_t t3 = 0;
  clock_t t4 = 0;
  clock_t mod = 0;
  
  unsigned long val = 0;
  struct particle buf[4096];
  memset(buf, 0, sizeof(buf));

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

      if(diff < 1.0f)
	{
	  for(i = 0; i<4096; i++)
	    {
	      buf[i].direction += (mod) * 0.000635;
	      buf[i].x += (buf[i].speed * cos(buf[i].direction)) * mod;
	      buf[i].y += (buf[i].speed * sin(buf[i].direction)) * mod;
	    }
	}
            
      if(diff > 0.01f)
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
	  pthread_mutex_lock(&lock);
	  copy = left;
	  pthread_mutex_unlock(&lock);
	  if(copy == 256) 
	    {
	      pthread_mutex_lock(&lock);
	      left = -1;
	      *((char*)&left+3)=1;
	      *((char*)&left+2)=1; // 1 = terminated thread
	      pthread_mutex_unlock(&lock);
	      return;
	    }	  
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
  struct particle buf[4096];

  s:;
  
  int copy = 0;
  double t1 = 0;
  double t2 = 0;
  double diff = 0;
  int i = 0;
  clock_t ticks;
  int entropy = 0;
  
  unsigned long val = 0;

  for(i = 0; i<4096; i++)
    {
      buf[i].x = 0;
      buf[i].y = 0;
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
           
      if(diff > 0.01f)
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
	  pthread_mutex_lock(&lock);
	  copy = left;
	  pthread_mutex_unlock(&lock);
	  if(copy == 2)
	    {
	      for(i = 0; i<4096; i++)
		{
		  buf[i].direction = (2 * M_PI * rand()) / RAND_MAX; //chaos, the end of galaxy
		  buf[i].speed = (0.08 * rand()) / RAND_MAX;
		  buf[i].speed *= buf[i].speed;
		}
	      pthread_mutex_lock(&lock);
	      left = -1;
	      *((char*)&left+3) = 3; //virtual thread id
	      *((char*)&left+2) = 0; //our task 0 = dummy callback	  
	      pthread_mutex_unlock(&lock);
	      entropy++;
	      if(entropy > 5)
		{
		  goto s;
		}
	    }
	  if(copy == 258) 
	    {
	      pthread_mutex_lock(&lock);
	      left = -1;
	      *((char*)&left+3)=3;
	      *((char*)&left+2)=1; // 1 = terminated thread
	      pthread_mutex_unlock(&lock);
	      return;
	    }		  
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

void CircleFill(XImage* img, int32_t centreX, int32_t centreY, int32_t radius, uint32_t color)
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
	  XPutPixel(img, centreX + x, centreY - y, 0x00FF00FF);
	  for(int i = centreX + x; i>centreX-1; i--)
	    {
	      XPutPixel(img, i, centreY - y, color);
	    }
	}

      if (!(centreX - x < 0 || centreX - x >= w || centreY + y < 0 || centreY + y >= h))
	{
	  XPutPixel(img, centreX - x, centreY + y, color);
	  for(int i = centreX - x; i<centreX+1; i++)
	    {
	      XPutPixel(img, i, centreY + y, color);
	    }	  
	}

      if (!(centreX + x < 0 || centreX + x >= w || centreY + y < 0 || centreY + y >= h))
	{
	  XPutPixel(img, centreX + x, centreY + y, color);
	  for(int i = centreX + x; i>centreX-1; i--)
	    {
	      XPutPixel(img, i, centreY + y, color);
	    }	  
	}

      if (!(centreX - x < 0 || centreX - x >= w || centreY - y < 0 || centreY - y >= h))
	{
 	  XPutPixel(img, centreX - x, centreY - y, color);
	  for(int i = centreX - x; i<centreX+1; i++)
	    {
	      XPutPixel(img, i, centreY - y, color);
	    }	  
 	}      

      if (!(centreX + y < 0 || centreX + y >= w || centreY - x < 0 || centreY - x >= h))
	{	      		  	  
	  XPutPixel(img, centreX + y, centreY - x, color);
	  for(int i = centreX + y; i>centreX-1; i--)
	    {
	      XPutPixel(img, i, centreY - x, color);
	    }	  
	}      

      if (!(centreX - y < 0 || centreX - y >= w || centreY + x < 0 || centreY + x >= h))
	{	      
	  XPutPixel(img, centreX - y, centreY + x, color);
	  for(int i = centreX - y; i<centreX+1; i++)
	    {
	      XPutPixel(img, i, centreY + x, color);
	    }	  	  
	}

      if (!(centreX + y < 0 || centreX + y >= w || centreY + x < 0 || centreY + x >= h))
	{	      
	  XPutPixel(img, centreX + y, centreY + x, color);
	  for(int i = centreX + y; i>centreX-1; i--)
	    {
	      XPutPixel(img, i, centreY + x, color);
	    }
	}

      if (!(centreX - y < 0 || centreX - y >= w || centreY - x < 0 || centreY - x >= h))
	{	      
	  XPutPixel(img, centreX - y, centreY - x, color);
	  for(int i = centreX - y; i<centreX+1; i++)
	    {
	      XPutPixel(img, i, centreY - x, color);
	    }	  	  	  
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

int bhm_line(XImage* img, uint32_t color, int x1,int y1,int x2,int y2)
{
  int x,y,dx,dy,dx1,dy1,px,py,xe,ye,i,pc;
  dx=x2-x1;
  dy=y2-y1;
  dx1=fabs(dx);
  dy1=fabs(dy);
  px=2*dy1-dx1;
  py=2*dx1-dy1;
  if(dy1<=dx1)
    {
      if(dx>=0)
	{
	  x=x1;
	  y=y1;
	  xe=x2;
	}
      else
	{
	  x=x2;
	  y=y2;
	  xe=x1;
	}
      if (!(x < 0 || x >= w || y < 0 || y >= h))
	{
	  XPutPixel(img, x, y, color);
	  pc++;
	}
      for(i=0;x<xe;i++)
	{
	  x=x+1;
	  if(px<0)
	    {
	      px=px+2*dy1;
	    }
	  else
	    {
	      if((dx<0 && dy<0) || (dx>0 && dy>0))
		{
		  y=y+1;
		}
	      else
		{
		  y=y-1;
		}
	      px=px+2*(dy1-dx1);
	    }
	  if (!(x < 0 || x >= w || y < 0 || y >= h))
	    {
	      XPutPixel(img, x, y, color);
	      pc++;
	    }
	}
    }
  else
    {
      if(dy>=0)
	{
	  x=x1;
	  y=y1;
	  ye=y2;
	}
      else
	{
	  x=x2;
	  y=y2;
	  ye=y1;
	}
      if (!(x < 0 || x >= w || y < 0 || y >= h))
	{
	  XPutPixel(img, x, y, color);
	  pc++;
	}
      for(i=0;y<ye;i++)
	{
	  y=y+1;
	  if(py<=0)
	    {
	      py=py+2*dx1;
	    }
	  else
	    {
	      if((dx<0 && dy<0) || (dx>0 && dy>0))
		{
		  x=x+1;
		}
	      else
		{
		  x=x-1;
		}
	      py=py+2*(dx1-dy1);
	    }
	  if (!(x < 0 || x >= w || y < 0 || y >= h))
	    {
	      XPutPixel(img, x, y, color);
	      pc++;
	    }
	}
    }
  return pc;
}
