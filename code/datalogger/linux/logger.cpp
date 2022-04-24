/*
Adapted from theju/linux-keylogger
*/

#include <iostream>
#include <set>
#include <utility>
#include <search.h>
#include <stdlib.h>
#include <fcntl.h>
#include <linux/input.h>
#include <string.h>
#include <errno.h>
#include <sys/epoll.h>
#include <signal.h>
#include <unistd.h>
#include <chrono>
#include <thread>
#include <math.h>
#include <map>
#include <xdo.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>

#define MS_BETWEEN_KEYSTROKE 500

using namespace std;

FILE *pFile;
FILE *pFile2;
Display *display;
int keysInputCounter = 0;
int colorsInputCounter = 0;
string test_data = "this is a test string that is exactly 400 characters long that is being sent from client machine to the host computer in order to test the latency and response time. the computer is typing each character with a constant delay between each keystroke to ensure a consistent flow of data. this delay can be configured from test to test to evaluate the streaming application's abilities to transfer data.";

void CTRL_C_Handler(int);

void type_string()
{
  usleep(MS_BETWEEN_KEYSTROKE * 1000);
  xdo_t *xdo = xdo_new(NULL);
  xdo_enter_text_window(xdo, CURRENTWINDOW, test_data.c_str(), MS_BETWEEN_KEYSTROKE * 1000);
  xdo_free(xdo);
}

void log()
{
  Window root = DefaultRootWindow(display);

  Window current_window;
  int rev;
  XGetInputFocus(display, &current_window, &rev);

  XSelectInput(display, current_window, KeyPressMask | KeyReleaseMask | FocusChangeMask);

  while (true)
  {
    XEvent e;
    XNextEvent(display, &e);
    switch (e.type)
    {
    case FocusOut:
      XGetInputFocus(display, &current_window, &rev);
      if (current_window == PointerRoot || current_window == None)
      {
        current_window = root;
      }
      XSelectInput(display, current_window, KeyPressMask | KeyReleaseMask | FocusChangeMask);
      break;
    case KeyPress:
    {
      // cout << "key pressed" << XKeysymToString(XkbKeycodeToKeysym(d, e.xkey.keycode, 0, e.xkey.state & ShiftMask ? 1 : 0)) <<"\n";
      string search_result = XKeysymToString(XkbKeycodeToKeysym(display, e.xkey.keycode, 0, e.xkey.state & ShiftMask ? 1 : 0));
      if (!search_result.empty())
      {
        if (!strcmp((char *)search_result.data(), "equal"))
        {
          // If "=" is pressed, begin the test
          keysInputCounter = 0;
          colorsInputCounter = 0;

          thread type_thread(type_string);
          type_thread.detach();
        }
        // get time
        unsigned long long now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        if (fprintf(pFile, "%llu,%s,%d,linux\n", now, (char *)search_result.data(), keysInputCounter++) < 0)
        {
          cerr << "pFile write: " << strerror(errno) << endl;
          exit(1);
        }
        fflush(pFile);
      }
      break;
    }
      // case KeyRelease:
      //   cout << "key released\n";
      //   break;
      // }
    }
  }
          exit(0);
}

XColor getPixelColor(Display *display, int x, int y)
{
  XColor pixel_color;
  Window root = XRootWindow(display, XDefaultScreen(display));
  XMapRaised(display, root);
  XImage *screen_image = XGetImage(
      display,
      root,
      x, y,
      1, 1,
      AllPlanes,
      XYPixmap);
  pixel_color.pixel = XGetPixel(screen_image, 0, 0);
  XFree(screen_image);
  XQueryColor(display, XDefaultColormap(display, XDefaultScreen(display)), &pixel_color);

  return pixel_color;
}

void colorThread()
{

  double prev_r, prev_g, prev_b = 0;
  while (true)
  {
    // get center of screen
    int x = XDisplayWidth(display, XDefaultScreen(display)) / 2;
    int y = XDisplayHeight(display, XDefaultScreen(display)) / 2;
    // get color of pixel at center of screen
    XColor screen_pixel_color = getPixelColor(display, x, y);
    int _red = round(screen_pixel_color.red / 256.0);
    int _green = round(screen_pixel_color.green / 256.0);
    int _blue = round(screen_pixel_color.blue / 256.0);

    double colorDistance = sqrt(pow(_red - prev_r, 2) + pow(_green - prev_g, 2) + pow(_blue - prev_b, 2));

    if (colorDistance > 10)
    {
      // get time
      unsigned long long now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
      if (fprintf(pFile2, "%llu,[Color Change: %d %d %d],%d,linux\n", now, _red, _green, _blue, colorsInputCounter++) < 0)
      {
        cerr << "pFile2 write: " << strerror(errno) << endl;
        exit(1);
      }
      fflush(pFile2);
      // std::cout << "Color: " << _red << "," << _green << "," << _blue << std::endl;
      prev_r = _red;
      prev_g = _green;
      prev_b = _blue;
    }
  }
}

int main(int argc, char **argv)
{
  XInitThreads();
  display = XOpenDisplay(NULL);
  signal(SIGINT, CTRL_C_Handler);
  if (display == 0)
  {
    cout << "Cannot open display\n";
    exit(1);
  }

  if (!(pFile = fopen("./datalogger_keys_linux.csv", "a")))
  {
    cerr << "error opening file: " << strerror(errno) << endl;
    exit(1);
  }
  if (!(pFile2 = fopen("./datalogger_colors_linux.csv", "a")))
  {
    cerr << "error opening file: " << strerror(errno) << endl;
    exit(1);
  }

  //Start reading color
  std::thread t1(colorThread);

  log();

  t1.join();

  XCloseDisplay(display);
  exit(-1);
}

void CTRL_C_Handler(int sig)
{
  XCloseDisplay(display);
  exit(0);
}