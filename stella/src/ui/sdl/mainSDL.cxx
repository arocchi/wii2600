//============================================================================
//
//   SSSS    tt          lll  lll       
//  SS  SS   tt           ll   ll        
//  SS     tttttt  eeee   ll   ll   aaaa 
//   SSSS    tt   ee  ee  ll   ll      aa
//      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
//  SS  SS   tt   ee      ll   ll  aa  aa
//   SSSS     ttt  eeeee llll llll  aaaaa
//
// Copyright (c) 1995-1999 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: mainSDL.cxx,v 1.12 2002-03-10 01:29:55 stephena Exp $
//============================================================================

#include <fstream>
#include <iostream>
#include <string>
#include <algorithm>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#include <SDL.h>
#include <SDL_syswm.h>

#include "bspf.hxx"
#include "Console.hxx"
#include "Event.hxx"
#include "MediaSrc.hxx"
#include "PropsSet.hxx"
#include "System.hxx"
#include "SndUnix.hxx"
#include "RectList.hxx"

#ifdef HAVE_PNG
  #include "Snapshot.hxx"
#endif

// Hack for SDL < 1.2.0
#ifndef SDL_ENABLE
  #define SDL_ENABLE 1
#endif
#ifndef SDL_DISABLE
  #define SDL_DISABLE 0
#endif


SDL_Joystick* theLeftJoystick;
SDL_Joystick* theRightJoystick;

// function prototypes
bool setupDisplay();
bool setupJoystick();
bool createScreen(int width, int height);
void recalculate8BitPalette();
void setupPalette();
void cleanup();

void updateDisplay(MediaSource& mediaSource);
void handleEvents();

void doQuit();
void resizeWindow(int mode);
void centerWindow();
void showCursor(bool show);
void grabMouse(bool grab);
void toggleFullscreen();
void takeSnapshot();
void togglePause();
uInt32 maxWindowSizeForScreen();

bool setupProperties(PropertiesSet& set);
void handleCommandLineArguments(int ac, char* av[]);
void handleRCFile();
void parseRCOptions(istream& in);
void usage();

// Globals for the SDL stuff
static SDL_Surface* screen;
static Uint32 palette[256];
static int bpp;
static Display* theX11Display;
static Window theX11Window;
static int theX11Screen;
static int mouseX = 0;
static bool x11Available = false;
static SDL_SysWMinfo info;
static int sdlflags;
static RectList* rectList;

#ifdef HAVE_PNG
  static Snapshot* snapshot;
#endif

struct Switches
{
  SDLKey scanCode;
  Event::Type eventCode;
};

static Switches list[] = {
    { SDLK_1,           Event::KeyboardZero1 },
    { SDLK_2,           Event::KeyboardZero2 },
    { SDLK_3,           Event::KeyboardZero3 },
    { SDLK_q,           Event::KeyboardZero4 },
    { SDLK_w,           Event::KeyboardZero5 },
    { SDLK_e,           Event::KeyboardZero6 },
    { SDLK_a,           Event::KeyboardZero7 },
    { SDLK_s,           Event::KeyboardZero8 },
    { SDLK_d,           Event::KeyboardZero9 },
    { SDLK_z,           Event::KeyboardZeroStar },
    { SDLK_x,           Event::KeyboardZero0 },
    { SDLK_c,           Event::KeyboardZeroPound },

    { SDLK_8,           Event::KeyboardOne1 },
    { SDLK_9,           Event::KeyboardOne2 },
    { SDLK_0,           Event::KeyboardOne3 },
    { SDLK_i,           Event::KeyboardOne4 },
    { SDLK_o,           Event::KeyboardOne5 },
    { SDLK_p,           Event::KeyboardOne6 },
    { SDLK_k,           Event::KeyboardOne7 },
    { SDLK_l,           Event::KeyboardOne8 },
    { SDLK_SEMICOLON,   Event::KeyboardOne9 },
    { SDLK_COMMA,       Event::KeyboardOneStar },
    { SDLK_PERIOD,      Event::KeyboardOne0 },
    { SDLK_SLASH,       Event::KeyboardOnePound },

    { SDLK_UP,          Event::JoystickZeroUp },
    { SDLK_DOWN,        Event::JoystickZeroDown },
    { SDLK_LEFT,        Event::JoystickZeroLeft },
    { SDLK_RIGHT,       Event::JoystickZeroRight },
    { SDLK_SPACE,       Event::JoystickZeroFire }, 
    { SDLK_RETURN,      Event::JoystickZeroFire }, 
    { SDLK_LCTRL,       Event::JoystickZeroFire }, 
    { SDLK_z,           Event::BoosterGripZeroTrigger },
    { SDLK_x,           Event::BoosterGripZeroBooster },

    { SDLK_w,           Event::JoystickZeroUp },
    { SDLK_s,           Event::JoystickZeroDown },
    { SDLK_a,           Event::JoystickZeroLeft },
    { SDLK_d,           Event::JoystickZeroRight },
    { SDLK_TAB,         Event::JoystickZeroFire }, 
    { SDLK_1,           Event::BoosterGripZeroTrigger },
    { SDLK_2,           Event::BoosterGripZeroBooster },

    { SDLK_o,           Event::JoystickOneUp },
    { SDLK_l,           Event::JoystickOneDown },
    { SDLK_k,           Event::JoystickOneLeft },
    { SDLK_SEMICOLON,   Event::JoystickOneRight },
    { SDLK_j,           Event::JoystickOneFire }, 
    { SDLK_n,           Event::BoosterGripOneTrigger },
    { SDLK_m,           Event::BoosterGripOneBooster },

    { SDLK_F1,          Event::ConsoleSelect },
    { SDLK_F2,          Event::ConsoleReset },
    { SDLK_F3,          Event::ConsoleColor },
    { SDLK_F4,          Event::ConsoleBlackWhite },
    { SDLK_F5,          Event::ConsoleLeftDifficultyA },
    { SDLK_F6,          Event::ConsoleLeftDifficultyB },
    { SDLK_F7,          Event::ConsoleRightDifficultyA },
    { SDLK_F8,          Event::ConsoleRightDifficultyB }
  };

static Event keyboardEvent;

// Default window size of 0, meaning it must be set somewhere else
uInt32 theWindowSize = 0;

// Indicates the maximum window size for the current screen
uInt32 theMaxWindowSize;

// Indicates the width and height of the game display based on properties
uInt32 theHeight;
uInt32 theWidth;

// Pointer to the console object or the null pointer
Console* theConsole;

// Event object to use
Event theEvent;

// Indicates if the user wants to quit
bool theQuitIndicator = false;

// Indicates if the emulator should be paused
bool thePauseIndicator = false;

// Indicates if the entire frame should be redrawn
bool theRedrawEntireFrameFlag = true;

// Indicates whether to use fullscreen
bool theUseFullScreenFlag = false;

// Indicates whether mouse can leave the game window
bool theGrabMouseFlag = false;

// Indicates whether to center the game window
bool theCenterWindowFlag = false;

// Indicates whether to show some game info on program exit
bool theShowInfoFlag = false;

// Indicates whether to show cursor in the game window
bool theHideCursorFlag = false;

// Indicates whether to allocate colors from a private color map
bool theUsePrivateColormapFlag = false;

// Indicates whether the game is currently in fullscreen
bool isFullscreen = false;

// Indicates whether the window is currently centered
bool isCentered = false;

// Indicates what the desired volume is
uInt32 theDesiredVolume = 75;

// Indicates what the desired frame rate is
uInt32 theDesiredFrameRate = 60;

// Indicate which paddle mode we're using:
//   0 - Mouse emulates paddle 0
//   1 - Mouse emulates paddle 1
//   2 - Mouse emulates paddle 2
//   3 - Mouse emulates paddle 3
//   4 - Use real Atari 2600 paddles
uInt32 thePaddleMode = 0;

// An alternate properties file to use
string theAlternateProFile = "";

#ifdef HAVE_PNG
  // The path to save snapshot files
  string theSnapShotDir = "";

  // What the snapshot should be called (romname or md5sum)
  string theSnapShotName = "";

  // Indicates whether to generate multiple snapshots or keep
  // overwriting the same file.  Set to true by default.
  bool theMultipleSnapShotFlag = true;
#endif 


/**
  This routine should be called once the console is created to setup
  the SDL window for us to use.  Return false if any operation fails,
  otherwise return true.
*/
bool setupDisplay()
{
  Uint32 initflags = SDL_INIT_VIDEO | SDL_INIT_JOYSTICK;
  if(SDL_Init(initflags) < 0)
    return false;

  atexit(doQuit);

  // Check which system we are running under
  x11Available = false;
  SDL_VERSION(&info.version);
  if(SDL_GetWMInfo(&info) > 0)
    if(info.subsystem == SDL_SYSWM_X11)
      x11Available = true;

  sdlflags = SDL_HWSURFACE;
  sdlflags |= theUseFullScreenFlag ? SDL_FULLSCREEN : 0;
  sdlflags |= theUsePrivateColormapFlag ? SDL_HWPALETTE : 0;

  // Get the desired width and height of the display
  theWidth = theConsole->mediaSource().width();
  theHeight = theConsole->mediaSource().height();

  // Get the maximum size of a window for THIS screen
  // Must be called after display and screen are known, as well as
  // theWidth and theHeight
  // Defaults to 3 on systems without X11, maximum of 4 on any system.
  theMaxWindowSize = maxWindowSizeForScreen();

  // If theWindowSize is not 0, then it must have been set on the commandline
  // Now we check to see if it is within bounds
  if(theWindowSize != 0)
  {
    if(theWindowSize < 1)
      theWindowSize = 1;
    else if(theWindowSize > theMaxWindowSize)
      theWindowSize = theMaxWindowSize;
  }
  else  // theWindowSize hasn't been set so we do the default
  {
    if(theMaxWindowSize < 2)
      theWindowSize = 1;
    else
      theWindowSize = 2;
  }

#ifdef HAVE_PNG
  // Take care of the snapshot stuff.
  snapshot = new Snapshot();

  if(theSnapShotDir == "")
    theSnapShotDir = getenv("HOME");
  if(theSnapShotName == "")
    theSnapShotName = "romname";
#endif

  // Set up the rectangle list to be used in updateDisplay
  rectList = new RectList();
  if(!rectList)
  {
    cerr << "ERROR: Unable to get memory for SDL rects" << endl;
    return false;
  }

  // Set the window title and icon
  char name[512];
  sprintf(name, "Stella: \"%s\"", 
      theConsole->properties().get("Cartridge.Name").c_str());
  SDL_WM_SetCaption(name, "stella");

  // Figure out the desired size of the window
  int width = theWidth * 2 * theWindowSize;
  int height = theHeight * theWindowSize;

  // Create the screen
  if(!createScreen(width, height))
    return false;
  setupPalette();

  // Make sure that theUseFullScreenFlag sets up fullscreen mode correctly
  if(theUseFullScreenFlag)
  {
    grabMouse(true);
    showCursor(false);
    isFullscreen = true;
  }
  else
  {
    // Keep mouse in game window if grabmouse is selected
    grabMouse(theGrabMouseFlag);

    // Show or hide the cursor depending on the 'hidecursor' argument
    showCursor(!theHideCursorFlag);
  }

  // Center the window if centering is selected and not fullscreen
  if(theCenterWindowFlag && !theUseFullScreenFlag)
    centerWindow();

  return true;
}


/**
  This routine should be called once setupDisplay is called
  to create the joystick stuff.
*/
bool setupJoystick()
{
  if(SDL_NumJoysticks() <= 0)
  {
    cout << "No joysticks present, use the keyboard.\n";
    theLeftJoystick = theRightJoystick = 0;
    return true;
  }

  if((theLeftJoystick = SDL_JoystickOpen(0)) != NULL)
    cout << "Left joystick is a " << SDL_JoystickName(0) <<
      " with " << SDL_JoystickNumButtons(theLeftJoystick) << " buttons.\n";
  else
    cout << "Left joystick not present, use keyboard instead.\n";

  if((theRightJoystick = SDL_JoystickOpen(1)) != NULL)
    cout << "Right joystick is a " << SDL_JoystickName(1) <<
      " with " << SDL_JoystickNumButtons(theRightJoystick) << " buttons.\n";
  else
    cout << "Right joystick not present, use keyboard instead.\n";

  return true;
}


/**
  This routine is called whenever the screen needs to be recreated.
  It updates the global screen variable.  When this happens, the
  8-bit palette needs to be recalculated.
*/
bool createScreen(int w, int h)
{
  screen = SDL_SetVideoMode(w, h, 0, sdlflags);
  if(screen == NULL)
  {
    cerr << "ERROR: Unable to open SDL window: " << SDL_GetError() << endl;
    return false;
  }

  bpp = screen->format->BitsPerPixel;
  if(bpp == 8)
    recalculate8BitPalette();

  return true;
}


/**
  Recalculates palette of an 8-bit (256 color) screen.
*/
void recalculate8BitPalette()
{
  if(bpp != 8)
    return;

  // Map 2600 colors to the current screen
  const uInt32* gamePalette = theConsole->mediaSource().palette();
  SDL_Color colors[256];
  for(uInt32 i = 0; i < 256; i += 2)
  {
    Uint8 r, g, b;

    r = (Uint8) ((gamePalette[i] & 0x00ff0000) >> 16);
    g = (Uint8) ((gamePalette[i] & 0x0000ff00) >> 8);
    b = (Uint8) (gamePalette[i] & 0x000000ff);

    colors[i].r = colors[i+1].r = r;
    colors[i].g = colors[i+1].g = g;
    colors[i].b = colors[i+1].b = b;
  }
  SDL_SetColors(screen, colors, 0, 256);

  // Now see which colors we actually got
  SDL_PixelFormat* format = screen->format;
  for(uInt32 i = 0; i < 256; i += 2)
  {
    Uint8 r, g, b;

    r = format->palette->colors[i].r;
    g = format->palette->colors[i].g;
    b = format->palette->colors[i].b;

    palette[i] = palette[i+1] = SDL_MapRGB(format, r, g, b);
  }
}


/**
  Set up the palette for a screen with > 8 bits
*/
void setupPalette()
{
  if(bpp == 8)
    return;

  const uInt32* gamePalette = theConsole->mediaSource().palette();
  for(uInt32 i = 0; i < 256; i += 2)
  {
    Uint8 r, g, b;

    r = (Uint8) ((gamePalette[i] & 0x00ff0000) >> 16);
    g = (Uint8) ((gamePalette[i] & 0x0000ff00) >> 8);
    b = (Uint8) (gamePalette[i] & 0x000000ff);

    switch(bpp)
    {
      case 15:
        palette[i] = palette[i+1] = ((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3);
        break;

      case 16:
        palette[i] = palette[i+1] = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
        break;

      case 24:
      case 32:
        palette[i] = palette[i+1] = (r << 16) | (g << 8) | b;
        break;
    }
  }
}


/**
  This routine is called when the program is about to quit.
*/
void doQuit()
{
  theQuitIndicator = true;
}


/**
  This routine is called when the user wants to resize the window.
  A '1' argument indicates that the window should increase in size, while '0'
  indicates that the windows should decrease in size.
  Can't resize in fullscreen mode.  Will only resize up to the maximum size
  of the screen.
*/
void resizeWindow(int mode)
{
  if(isFullscreen)
    return;

  if(mode == 1)   // increase size
  {
    if(theWindowSize == theMaxWindowSize)
      theWindowSize = 1;
    else
      theWindowSize++;
  }
  else   // decrease size
  {
    if(theWindowSize == 1)
      theWindowSize = theMaxWindowSize;
    else
      theWindowSize--;
  }

  // Figure out the desired size of the window
  int width = theWidth * 2 * theWindowSize;
  int height = theHeight * theWindowSize;

  if(!createScreen(width, height))
    return;

  theRedrawEntireFrameFlag = true;

  // A resize may mean that the window is no longer centered
  isCentered = false;

  if(theCenterWindowFlag)
    centerWindow();
}


/**
  Centers the game window onscreen.  Only works in X11 for now.
*/
void centerWindow()
{
  if(!x11Available)
  {
    cerr << "Window centering only available under X11.\n";
    return;
  }

  if(isFullscreen || isCentered)
    return;

  int x, y, w, h;
  info.info.x11.lock_func();
  theX11Display = info.info.x11.display;
  theX11Window  = info.info.x11.wmwindow;
  theX11Screen  = DefaultScreen(theX11Display);

  w = DisplayWidth(theX11Display, theX11Screen);
  h = DisplayHeight(theX11Display, theX11Screen);
  x = (w - screen->w)/2;
  y = (h - screen->h)/2;

  XMoveWindow(theX11Display, theX11Window, x, y);
  info.info.x11.unlock_func();

  isCentered = true;
}


/**
  Toggles between fullscreen and window mode.  Grabmouse and hidecursor
  activated when in fullscreen mode.
*/
void toggleFullscreen()
{
  int width = theWidth * 2 * theWindowSize;
  int height = theHeight * theWindowSize;

  isFullscreen = !isFullscreen;
  if(isFullscreen)
    sdlflags |= SDL_FULLSCREEN;
  else
    sdlflags &= ~SDL_FULLSCREEN;

  if(!createScreen(width, height))
    return;

  if(isFullscreen)  // now in fullscreen mode
  {
    grabMouse(true);
    showCursor(false);
  }
  else    // now in windowed mode
  {
    grabMouse(theGrabMouseFlag);
    showCursor(!theHideCursorFlag);

    if(theCenterWindowFlag)
        centerWindow();
  }

  theRedrawEntireFrameFlag = true;
}


/**
  Toggles pausing of the emulator
*/
void togglePause()
{
// todo: implement pause functionality

  if(thePauseIndicator)	// emulator is already paused so continue
  {
    thePauseIndicator = false;
  }
  else	// we want to pause the game
  {
    thePauseIndicator = true;
  }
}


/**
  Shows or hides the cursor based on the given boolean value.
*/
void showCursor(bool show)
{
  if(show)
    SDL_ShowCursor(SDL_ENABLE);
  else
    SDL_ShowCursor(SDL_DISABLE);
}


/**
  Grabs or ungrabs the mouse based on the given boolean value.
*/
void grabMouse(bool grab)
{
  if(grab)
    SDL_WM_GrabInput(SDL_GRAB_ON);
  else
    SDL_WM_GrabInput(SDL_GRAB_OFF);
}


/**
  This routine should be called anytime the display needs to be updated
*/
void updateDisplay(MediaSource& mediaSource)
{
  uInt8* currentFrame = mediaSource.currentFrameBuffer();
  uInt8* previousFrame = mediaSource.previousFrameBuffer();
  uInt16 screenMultiple = (uInt16)theWindowSize;

  struct Rectangle
  {
    uInt8 color;
    uInt16 x, y, width, height;
  } rectangles[2][160];

  // Start a new reclist on each display update
  rectList->start();

  // This array represents the rectangles that need displaying
  // on the current scanline we're processing
  Rectangle* currentRectangles = rectangles[0];

  // This array represents the rectangles that are still active
  // from the previous scanlines we have processed
  Rectangle* activeRectangles = rectangles[1];

  // Indicates the number of active rectangles
  uInt16 activeCount = 0;

  // This update procedure requires theWidth to be a multiple of four.  
  // This is validated when the properties are loaded.
  for(uInt16 y = 0; y < theHeight; ++y)
  {
    // Indicates the number of current rectangles
    uInt16 currentCount = 0;

    // Look at four pixels at a time to see if anything has changed
    uInt32* current = (uInt32*)(currentFrame); 
    uInt32* previous = (uInt32*)(previousFrame);

    for(uInt16 x = 0; x < theWidth; x += 4, ++current, ++previous)
    {
      // Has something changed in this set of four pixels?
      if((*current != *previous) || theRedrawEntireFrameFlag)
      {
        uInt8* c = (uInt8*)current;
        uInt8* p = (uInt8*)previous;

        // Look at each of the bytes that make up the uInt32
        for(uInt16 i = 0; i < 4; ++i, ++c, ++p)
        {
          // See if this pixel has changed
          if((*c != *p) || theRedrawEntireFrameFlag)
          {
            // Can we extend a rectangle or do we have to create a new one?
            if((currentCount != 0) && 
               (currentRectangles[currentCount - 1].color == *c) &&
               ((currentRectangles[currentCount - 1].x + 
                 currentRectangles[currentCount - 1].width) == (x + i)))
            {
              currentRectangles[currentCount - 1].width += 1;
            }
            else
            {
              currentRectangles[currentCount].x = x + i;
              currentRectangles[currentCount].y = y;
              currentRectangles[currentCount].width = 1;
              currentRectangles[currentCount].height = 1;
              currentRectangles[currentCount].color = *c;
              currentCount++;
            }
          }
        }
      }
    }

    // Merge the active and current rectangles flushing any that are of no use
    uInt16 activeIndex = 0;

    for(uInt16 t = 0; (t < currentCount) && (activeIndex < activeCount); ++t)
    {
      Rectangle& current = currentRectangles[t];
      Rectangle& active = activeRectangles[activeIndex];

      // Can we merge the current rectangle with an active one?
      if((current.x == active.x) && (current.width == active.width) &&
         (current.color == active.color))
      {
        current.y = active.y;
        current.height = active.height + 1;

        ++activeIndex;
      }
      // Is it impossible for this active rectangle to be merged?
      else if(current.x >= active.x)
      {
        // Flush the active rectangle
        SDL_Rect temp;

        temp.x = active.x * 2 * screenMultiple;
        temp.y = active.y * screenMultiple;
        temp.w = active.width * 2 * screenMultiple;
        temp.h = active.height * screenMultiple;

        rectList->add(&temp);
        SDL_FillRect(screen, &temp, palette[active.color]);

        ++activeIndex;
      }
    }

    // Flush any remaining active rectangles
    for(uInt16 s = activeIndex; s < activeCount; ++s)
    {
      Rectangle& active = activeRectangles[s];

      SDL_Rect temp;
      temp.x = active.x * 2 * screenMultiple;
      temp.y = active.y * screenMultiple;
      temp.w = active.width * 2 * screenMultiple;
      temp.h = active.height * screenMultiple;

      rectList->add(&temp);
      SDL_FillRect(screen, &temp, palette[active.color]);
    }

    // We can now make the current rectangles into the active rectangles
    Rectangle* tmp = currentRectangles;
    currentRectangles = activeRectangles;
    activeRectangles = tmp;
    activeCount = currentCount;
 
    currentFrame += theWidth;
    previousFrame += theWidth;
  }

  // Flush any rectangles that are still active
  for(uInt16 t = 0; t < activeCount; ++t)
  {
    Rectangle& active = activeRectangles[t];

    SDL_Rect temp;
    temp.x = active.x * 2 * screenMultiple;
    temp.y = active.y * screenMultiple;
    temp.w = active.width * 2 * screenMultiple;
    temp.h = active.height * screenMultiple;

    rectList->add(&temp);
    SDL_FillRect(screen, &temp, palette[active.color]);
  }

  // Now update all the rectangles at once
  SDL_UpdateRects(screen, rectList->numRects(), rectList->rects());

  // The frame doesn't need to be completely redrawn anymore
  theRedrawEntireFrameFlag = false;
}


/**
  This routine should be called regularly to handle events
*/
void handleEvents()
{
  SDL_Event event;
  Uint8 axis;
  Uint8 button;
  Sint16 value;
  Uint8 type;
  Uint8 state;
  SDLKey key;
  SDLMod mod;

  // Check for an event
  while(SDL_PollEvent(&event))
  {
    // keyboard events
    if(event.type == SDL_KEYDOWN)
    {
      key = event.key.keysym.sym;
      mod = event.key.keysym.mod;
      type = event.type;

      if(key == SDLK_ESCAPE)
      {
        doQuit();
      }
      else if(key == SDLK_EQUALS)
      {
        resizeWindow(1);
      }
      else if(key == SDLK_MINUS)
      {
        resizeWindow(0);
      }
      else if(key == SDLK_RETURN && mod & KMOD_ALT)
      {
        toggleFullscreen();
      }
      else if(key == SDLK_F12)
      {
        takeSnapshot();
      }
      else if(key == SDLK_PAUSE)
      {
        togglePause();
      }
      else if(key == SDLK_g)
      {
        // don't change grabmouse in fullscreen mode
        if(!isFullscreen)
        {
          theGrabMouseFlag = !theGrabMouseFlag;
          grabMouse(theGrabMouseFlag);
        }
      }
      else if(key == SDLK_h)
      {
        // don't change hidecursor in fullscreen mode
        if(!isFullscreen)
        {
          theHideCursorFlag = !theHideCursorFlag;
          showCursor(!theHideCursorFlag);
        }
      }
      else // check all the other keys
      {
        for(unsigned int i = 0; i < sizeof(list) / sizeof(Switches); ++i)
        { 
          if(list[i].scanCode == key)
          {
            theEvent.set(list[i].eventCode, 1);
            keyboardEvent.set(list[i].eventCode, 1);
          }
        }
      }
    }
    else if(event.type == SDL_KEYUP)
    {
      key = event.key.keysym.sym;
      type = event.type;

      for(unsigned int i = 0; i < sizeof(list) / sizeof(Switches); ++i)
      { 
        if(list[i].scanCode == key)
        {
          theEvent.set(list[i].eventCode, 0);
          keyboardEvent.set(list[i].eventCode, 0);
        }
      }
    }
    else if(event.type == SDL_MOUSEMOTION)
    {
      int resistance = 0, x = 0;
      float fudgeFactor = 1000000.0;
      int width = theWidth * 2 * theWindowSize;

      // Grabmouse and hidecursor introduce some lag into the mouse movement,
      // so we need to fudge the numbers a bit
      if(theGrabMouseFlag && theHideCursorFlag)
      {
        mouseX = (int)((float)mouseX + (float)event.motion.xrel
                 * 1.5 * (float)theWindowSize);
      }
      else
      {
        mouseX = mouseX + event.motion.xrel * theWindowSize;
      }

      // Check to make sure mouseX is within the game window
      if(mouseX < 0)
        mouseX = 0;
      else if(mouseX > width)
        mouseX = width;
  
      x = width - mouseX;
      resistance = (Int32)((fudgeFactor * x) / width);

      // Now, set the event of the correct paddle to the calculated resistance
      if(thePaddleMode == 0)
        theEvent.set(Event::PaddleZeroResistance, resistance);
      else if(thePaddleMode == 1)
        theEvent.set(Event::PaddleOneResistance, resistance);
      else if(thePaddleMode == 2)
        theEvent.set(Event::PaddleTwoResistance, resistance);
      else if(thePaddleMode == 3)
        theEvent.set(Event::PaddleThreeResistance, resistance);
    }
    else if(event.type == SDL_MOUSEBUTTONDOWN)
    {
      if(thePaddleMode == 0)
        theEvent.set(Event::PaddleZeroFire, 1);
      else if(thePaddleMode == 1)
        theEvent.set(Event::PaddleOneFire, 1);
      else if(thePaddleMode == 2)
        theEvent.set(Event::PaddleTwoFire, 1);
      else if(thePaddleMode == 3)
        theEvent.set(Event::PaddleThreeFire, 1);
    }
    else if(event.type == SDL_MOUSEBUTTONUP)
    {
      if(thePaddleMode == 0)
        theEvent.set(Event::PaddleZeroFire, 0);
      else if(thePaddleMode == 1)
        theEvent.set(Event::PaddleOneFire, 0);
      else if(thePaddleMode == 2)
        theEvent.set(Event::PaddleTwoFire, 0);
      else if(thePaddleMode == 3)
        theEvent.set(Event::PaddleThreeFire, 0);
    }
    else if(event.type == SDL_ACTIVEEVENT)
    {
      if((event.active.state & SDL_APPACTIVE) && (event.active.gain == 0))
      {
        if(!thePauseIndicator)
        {
          // togglePause();
          cerr << "todo: Pause on minimize.\n";
        }
      }
    }
    else if(event.type == SDL_QUIT)
    {
      doQuit();
    }

    // Read joystick events and modify event states
    if(theLeftJoystick)
    {
      if(((event.type == SDL_JOYBUTTONDOWN) || (event.type == SDL_JOYBUTTONUP))
          && (event.jbutton.which == 0))
      {
        button = event.jbutton.button;
        state = event.jbutton.state;
        state = (state == SDL_PRESSED) ? 1 : 0;

        if(button == 0)  // fire button
        {
          theEvent.set(Event::JoystickZeroFire, state ? 
              1 : keyboardEvent.get(Event::JoystickZeroFire));

          // If we're using real paddles then set paddle event as well
          if(thePaddleMode == 4)
            theEvent.set(Event::PaddleZeroFire, state);
        }
        else if(button == 1)  // booster button
        {
          theEvent.set(Event::BoosterGripZeroTrigger, state ? 
              1 : keyboardEvent.get(Event::BoosterGripZeroTrigger));

          // If we're using real paddles then set paddle event as well
          if(thePaddleMode == 4)
            theEvent.set(Event::PaddleOneFire, state);
        }
      }
      else if((event.type == SDL_JOYAXISMOTION) && (event.jaxis.which == 0))
      {
        axis = event.jaxis.axis;
        value = event.jaxis.value;

        if(axis == 0)  // x-axis
        {
          theEvent.set(Event::JoystickZeroLeft, (value < -16384) ? 
              1 : keyboardEvent.get(Event::JoystickZeroLeft));
          theEvent.set(Event::JoystickZeroRight, (value > 16384) ? 
              1 : keyboardEvent.get(Event::JoystickZeroRight));

          // If we're using real paddles then set paddle events as well
          if(thePaddleMode == 4)
          {
            uInt32 r = (uInt32)((1.0E6L * (value + 32767L)) / 65536);
            theEvent.set(Event::PaddleZeroResistance, r);
          }
        }
        else if(axis == 1)  // y-axis
        {
          theEvent.set(Event::JoystickZeroUp, (value < -16384) ? 
              1 : keyboardEvent.get(Event::JoystickZeroUp));
          theEvent.set(Event::JoystickZeroDown, (value > 16384) ? 
              1 : keyboardEvent.get(Event::JoystickZeroDown));

          // If we're using real paddles then set paddle events as well
          if(thePaddleMode == 4)
          {
            uInt32 r = (uInt32)((1.0E6L * (value + 32767L)) / 65536);
            theEvent.set(Event::PaddleOneResistance, r);
          }
        }
      }
    }

    if(theRightJoystick)
    {
      if(((event.type == SDL_JOYBUTTONDOWN) || (event.type == SDL_JOYBUTTONUP))
          && (event.jbutton.which == 1))
      {
        button = event.jbutton.button;
        state = event.jbutton.state;
        state = (state == SDL_PRESSED) ? 1 : 0;

        if(button == 0)  // fire button
        {
          theEvent.set(Event::JoystickOneFire, state ? 
              1 : keyboardEvent.get(Event::JoystickOneFire));

          // If we're using real paddles then set paddle event as well
          if(thePaddleMode == 4)
            theEvent.set(Event::PaddleTwoFire, state);
        }
        else if(button == 1)  // booster button
        {
          theEvent.set(Event::BoosterGripOneTrigger, state ? 
              1 : keyboardEvent.get(Event::BoosterGripOneTrigger));

          // If we're using real paddles then set paddle event as well
          if(thePaddleMode == 4)
            theEvent.set(Event::PaddleThreeFire, state);
        }
      }
      else if((event.type == SDL_JOYAXISMOTION) && (event.jaxis.which == 1))
      {
        axis = event.jaxis.axis;
        value = event.jaxis.value;

        if(axis == 0)  // x-axis
        {
          theEvent.set(Event::JoystickOneLeft, (value < -16384) ? 
              1 : keyboardEvent.get(Event::JoystickOneLeft));
          theEvent.set(Event::JoystickOneRight, (value > 16384) ? 
              1 : keyboardEvent.get(Event::JoystickOneRight));

          // If we're using real paddles then set paddle events as well
          if(thePaddleMode == 4)
          {
            uInt32 r = (uInt32)((1.0E6L * (value + 32767L)) / 65536);
            theEvent.set(Event::PaddleTwoResistance, r);
          }
        }
        else if(axis == 1)  // y-axis
        {
          theEvent.set(Event::JoystickOneUp, (value < -16384) ? 
              1 : keyboardEvent.get(Event::JoystickOneUp));
          theEvent.set(Event::JoystickOneDown, (value > 16384) ? 
              1 : keyboardEvent.get(Event::JoystickOneDown));

          // If we're using real paddles then set paddle events as well
          if(thePaddleMode == 4)
          {
            uInt32 r = (uInt32)((1.0E6L * (value + 32767L)) / 65536);
            theEvent.set(Event::PaddleThreeResistance, r);
          }
        }
      }
    }
  }
}


/**
  Called when the user wants to take a snapshot of the current display.
  Images are stored in png format in the directory specified by the 'ssdir'
  argument, or in $HOME by default.
  Images are named consecutively as "NAME".png, where name is specified by
  the 'ssname' argument.  If that name exists, they are named as "Name"_x.png,
  where x starts with 1 and increases if the previous name already exists.
  All spaces in filenames are converted to underscore '_'.
  If theMultipleSnapShotFlag is false, then consecutive images are overwritten.
*/
void takeSnapshot()
{
#ifdef HAVE_PNG
  if(!snapshot)
  {
    cerr << "Snapshot support disabled.\n";
    return;
  }

  // Now find the correct name for the snapshot
  string filename = theSnapShotDir;
  if(theSnapShotName == "romname")
    filename = filename + "/" + theConsole->properties().get("Cartridge.Name");
  else if(theSnapShotName == "md5sum")
    filename = filename + "/" + theConsole->properties().get("Cartridge.MD5");
  else
  {
    cerr << "ERROR: unknown name " << theSnapShotName
         << " for snapshot type" << endl;
    return;
  }

  // Replace all spaces in name with underscores
  replace(filename.begin(), filename.end(), ' ', '_');

  // Check whether we want multiple snapshots created
  if(theMultipleSnapShotFlag)
  {
    // Determine if the file already exists, checking each successive filename
    // until one doesn't exist
    string extFilename = filename + ".png";
    if(access(extFilename.c_str(), F_OK) == 0 )
    {
      uInt32 i;
      char buffer[1024];

      for(i = 1; ;++i)
      {
        snprintf(buffer, 1023, "%s_%d.png", filename.c_str(), i);
        if(access(buffer, F_OK) == -1 )
          break;
      }
      filename = buffer;
    }
    else
      filename = extFilename;
  }
  else
    filename = filename + ".png";

  // Now save the snapshot file
  snapshot->savePNG(filename, theConsole->mediaSource(), theWindowSize);

  if(access(filename.c_str(), F_OK) == 0)
    cerr << "Snapshot saved as " << filename << endl;
  else
    cerr << "Couldn't create snapshot " << filename << endl;
#else
  cerr << "Snapshot mode not supported.\n";
#endif
}


/**
  Calculate the maximum window size that the current screen can hold.
  Only works in X11 for now.  If not running under X11, always return 3.
*/
uInt32 maxWindowSizeForScreen()
{
  if(!x11Available)
    return 3;

  // Otherwise, lock the screen and get the width and height
  info.info.x11.lock_func();
  theX11Display = info.info.x11.display;
  theX11Window  = info.info.x11.wmwindow;
  theX11Screen  = DefaultScreen(theX11Display);
  info.info.x11.unlock_func();

  int screenWidth  = DisplayWidth(info.info.x11.display,
      DefaultScreen(theX11Display));
  int screenHeight = DisplayHeight(info.info.x11.display,
      DefaultScreen(theX11Display));

  uInt32 multiplier = screenWidth / (theWidth * 2);
  bool found = false;

  while(!found && (multiplier > 0))
  {
    // Figure out the desired size of the window
    int width = theWidth * 2 * multiplier;
    int height = theHeight * multiplier;

    if((width < screenWidth) && (height < screenHeight))
      found = true;
    else
      multiplier--;
  }

  if(found)
    return (multiplier > 4 ? 4 : multiplier);
  else
    return 1;
}


/**
  Display a usage message and exit the program
*/
void usage()
{
  static const char* message[] = {
    "",
    "SDL Stella version 1.2",
    "",
    "Usage: stella.sdl [option ...] file",
    "",
    "Valid options are:",
    "",
    "  -fps <number>           Display the given number of frames per second",
    "  -owncmap                Install a private colormap",
    "  -zoom <size>            Makes window be 'size' times normal (1 - 4)",
    "  -fullscreen             Play the game in fullscreen mode",
    "  -grabmouse              Keeps the mouse in the game window",
    "  -hidecursor             Hides the mouse cursor in the game window",
    "  -center                 Centers the game window onscreen",
    "  -volume <number>        Set the volume (0 - 100)",
    "  -paddle <0|1|2|3|real>  Indicates which paddle the mouse should emulate",
    "                          or that real Atari 2600 paddles are being used",
    "  -showinfo               Shows some game info on exit",
#ifdef HAVE_PNG
    "  -ssdir <path>           The directory to save snapshot files to",
    "  -ssname <name>          How to name the snapshot (romname or md5sum)",
    "  -sssingle               Generate single snapshot instead of many",
#endif
    "  -pro <props file>       Use the given properties file instead of stella.pro",
    "",
    0
  };

  for(uInt32 i = 0; message[i] != 0; ++i)
  {
    cerr << message[i] << endl;
  }
  exit(1);
}


/**
  Setup the properties set by first checking for a user file ".stella.pro",
  then a system-wide file "/etc/stella.pro".  Return false if neither file
  is found, else return true.

  @param set The properties set to setup
*/
bool setupProperties(PropertiesSet& set)
{
  string homePropertiesFile = getenv("HOME");
  homePropertiesFile += "/.stella.pro";
  string systemPropertiesFile = "/etc/stella.pro";

  // Check to see if the user has specified an alternate .pro file.
  // If it exists, use it.
  if(theAlternateProFile != "")
  {
    if(access(theAlternateProFile.c_str(), R_OK) == 0)
    {
      set.load(theAlternateProFile, &Console::defaultProperties(), false);
      return true;
    }
    else
    {
      cerr << "ERROR: Couldn't find \"" << theAlternateProFile <<
              "\" properties file." << endl;
      return false;
    }
  }

  if(access(homePropertiesFile.c_str(), R_OK) == 0)
  {
    set.load(homePropertiesFile, &Console::defaultProperties(), false);
    return true;
  }
  else if(access(systemPropertiesFile.c_str(), R_OK) == 0)
  {
    set.load(systemPropertiesFile, &Console::defaultProperties(), false);
    return true;
  }
  else
  {
    cerr << "ERROR: Couldn't find stella.pro file." << endl;
    return false;
  }
}


/**
  Should be called to parse the command line arguments

  @param argc The count of command line arguments
  @param argv The command line arguments
*/
void handleCommandLineArguments(int argc, char* argv[])
{
  // Make sure we have the correct number of command line arguments
  if(argc == 1)
    usage();

  for(Int32 i = 1; i < (argc - 1); ++i)
  {
    // See which command line switch they're using
    if(string(argv[i]) == "-fps")
    {
      // They're setting the desired frame rate
      Int32 rate = atoi(argv[++i]);
      if((rate < 1) || (rate > 300))
      {
        rate = 60;
      }

      theDesiredFrameRate = rate;
    }
    else if(string(argv[i]) == "-paddle")
    {
      // They're trying to set the paddle emulation mode
      if(string(argv[i + 1]) == "real")
      {
        thePaddleMode = 4;
      }
      else
      {
        thePaddleMode = atoi(argv[i + 1]);
        if((thePaddleMode < 0) || (thePaddleMode > 3))
        {
          usage();
        }
      }
      ++i;
    }
    else if(string(argv[i]) == "-owncmap")
    {
      theUsePrivateColormapFlag = true;
    }
    else if(string(argv[i]) == "-fullscreen")
    {
      theUseFullScreenFlag = true;
    }
    else if(string(argv[i]) == "-grabmouse")
    {
      theGrabMouseFlag = true;
    }
    else if(string(argv[i]) == "-hidecursor")
    {
      theHideCursorFlag = true;
    }
    else if(string(argv[i]) == "-center")
    {
      theCenterWindowFlag = true;
    }
    else if(string(argv[i]) == "-showinfo")
    {
      theShowInfoFlag = true;
    }
    else if(string(argv[i]) == "-zoom")
    {
      uInt32 size = atoi(argv[++i]);
      theWindowSize = size;
    }
    else if(string(argv[i]) == "-volume")
    {
      // They're setting the desired volume
      Int32 volume = atoi(argv[++i]);
      if(volume < 0)
        volume = 0;
      if(volume > 100)
        volume = 100;

      theDesiredVolume = volume;
    }
#ifdef HAVE_PNG
    else if(string(argv[i]) == "-ssdir")
    {
      theSnapShotDir = argv[++i];
    }
    else if(string(argv[i]) == "-ssname")
    {
      theSnapShotName = argv[++i];
    }
    else if(string(argv[i]) == "-sssingle")
    {
      theMultipleSnapShotFlag = false;
    }
#endif
    else if(string(argv[i]) == "-pro")
    {
      theAlternateProFile = argv[++i];
    }
    else
    {
      cout << "Undefined option " << argv[i] << endl;
    }
  }
}


/**
  Should be called to determine if an rc file exists.  First checks if there
  is a user specified file ".stellarc" and then if there is a system-wide
  file "/etc/stellarc".
*/
void handleRCFile()
{
  string homeRCFile = getenv("HOME");
  homeRCFile += "/.stellarc";

  if(access(homeRCFile.c_str(), R_OK) == 0 )
  {
    ifstream homeStream(homeRCFile.c_str());
    parseRCOptions(homeStream);
  }
  else if(access("/etc/stellarc", R_OK) == 0 )
  {
    ifstream systemStream("/etc/stellarc");
    parseRCOptions(systemStream);
  }
}


/**
  Parses each line of the given rcfile and sets options accordingly.

  @param in The file to parse for options
*/
void parseRCOptions(istream& in)
{
  string line, key, value;
  uInt32 equalPos;

  while(getline(in, line))
  {
    // Strip all whitespace and tabs from the line
    uInt32 garbage;
    while((garbage = line.find(" ")) != string::npos)
      line.erase(garbage, 1);
    while((garbage = line.find("\t")) != string::npos)
      line.erase(garbage, 1);

    // Ignore commented and empty lines
    if((line.length() == 0) || (line[0] == ';'))
      continue;

    // Search for the equal sign and discard the line if its not found
    if((equalPos = line.find("=")) == string::npos)
      continue;

    key   = line.substr(0, equalPos);
    value = line.substr(equalPos + 1, line.length() - key.length() - 1);

    // Check for absent key or value
    if((key.length() == 0) || (value.length() == 0))
      continue;

    // Now set up the options by key
    if(key == "fps")
    {
      // They're setting the desired frame rate
      uInt32 rate = atoi(value.c_str());
      if((rate < 1) || (rate > 300))
      {
        rate = 60;
      }

      theDesiredFrameRate = rate;
    }
    else if(key == "paddle")
    {
      // They're trying to set the paddle emulation mode
      uInt32 pMode;
      if(value == "real")
      {
        thePaddleMode = 4;
      }
      else
      {
        pMode = atoi(value.c_str());
        if((pMode > 0) && (pMode < 4))
          thePaddleMode = pMode;
      }
    }
    else if(key == "owncmap")
    {
      uInt32 option = atoi(value.c_str());
      if(option == 1)
        theUsePrivateColormapFlag = true;
      else if(option == 0)
        theUsePrivateColormapFlag = false;
    }
    else if(key == "fullscreen")
    {
      uInt32 option = atoi(value.c_str());
      if(option == 1)
        theUseFullScreenFlag = true;
      else if(option == 0)
        theUseFullScreenFlag = false;
    }
    else if(key == "grabmouse")
    {
      uInt32 option = atoi(value.c_str());
      if(option == 1)
        theGrabMouseFlag = true;
      else if(option == 0)
        theGrabMouseFlag = false;
    }
    else if(key == "hidecursor")
    {
      uInt32 option = atoi(value.c_str());
      if(option == 1)
        theHideCursorFlag = true;
      else if(option == 0)
        theHideCursorFlag = false;
    }
    else if(key == "center")
    {
      uInt32 option = atoi(value.c_str());
      if(option == 1)
        theCenterWindowFlag = true;
      else if(option == 0)
        theCenterWindowFlag = false;
    }
    else if(key == "showinfo")
    {
      uInt32 option = atoi(value.c_str());
      if(option == 1)
        theShowInfoFlag = true;
      else if(option == 0)
        theShowInfoFlag = false;
    }
    else if(key == "zoom")
    {
      // They're setting the initial window size
      // Don't do bounds checking here, it will be taken care of later
      uInt32 size = atoi(value.c_str());
      theWindowSize = size;
    }
    else if(key == "volume")
    {
      // They're setting the desired volume
      uInt32 volume = atoi(value.c_str());
      if(volume < 0)
        volume = 0;
      if(volume > 100)
        volume = 100;

      theDesiredVolume = volume;
    }
#ifdef HAVE_PNG
    else if(key == "ssdir")
    {
      theSnapShotDir = value;
    }
    else if(key == "ssname")
    {
      theSnapShotName = value;
    }
    else if(key == "sssingle")
    {
      uInt32 option = atoi(value.c_str());
      if(option == 1)
        theMultipleSnapShotFlag = false;
      else if(option == 0)
        theMultipleSnapShotFlag = true;
    }
#endif
  }
}


/**
  Does general cleanup in case any operation failed (or at end of program).
*/
void cleanup()
{
  if(theConsole)
    delete theConsole;

#ifdef HAVE_PNG
  if(snapshot)
    delete snapshot;
#endif

  if(rectList)
    delete rectList;

  if(SDL_JoystickOpened(0))
    SDL_JoystickClose(theLeftJoystick);
  if(SDL_JoystickOpened(1))
    SDL_JoystickClose(theRightJoystick);

  SDL_Quit();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int main(int argc, char* argv[])
{
  // Load in any user defined settings from an RC file
  handleRCFile();

  // Handle the command line arguments
  handleCommandLineArguments(argc, argv);

  // Get a pointer to the file which contains the cartridge ROM
  const char* file = argv[argc - 1];

  // Open the cartridge image and read it in
  ifstream in(file); 
  if(!in)
  {
    cerr << "ERROR: Couldn't open " << file << "..." << endl;
    exit(1);
  }

  uInt8* image = new uInt8[512 * 1024];
  in.read(image, 512 * 1024);
  uInt32 size = in.gcount();
  in.close();

  // Create a properties set for us to use and set it up
  PropertiesSet propertiesSet;
  if(!setupProperties(propertiesSet))
  {
    delete[] image;
    exit(1);
  }

  // Create a sound object for use with the console
  SoundUnix sound(theDesiredVolume);

  // Get just the filename of the file containing the ROM image
  const char* filename = (!strrchr(file, '/')) ? file : strrchr(file, '/') + 1;

  // Create the 2600 game console
  theConsole = new Console(image, size, filename, 
      theEvent, propertiesSet, sound);

  // Free the image since we don't need it any longer
  delete[] image;

  // Setup the SDL window and joystick
  if(!setupDisplay())
  {
    cerr << "ERROR: Couldn't set up display.\n";
    cleanup();
  }
  if(!setupJoystick())
  {
    cerr << "ERROR: Couldn't set up joysticks.\n";
    cleanup();
  }

  // Get the starting time in case we need to print statistics
  timeval startingTime;
  gettimeofday(&startingTime, 0);

  uInt32 numberOfFrames = 0;
  uInt32 frameTime = 1000000 / theDesiredFrameRate;
  for( ; ; ++numberOfFrames)
  {
    // Exit if the user wants to quit
    if(theQuitIndicator)
    {
      break;
    }

    // Remember the current time before we start drawing the frame
    timeval before;
    gettimeofday(&before, 0);

    theConsole->mediaSource().update();
    updateDisplay(theConsole->mediaSource());
    handleEvents();

    // Now, waste time if we need to so that we are at the desired frame rate
    timeval after;
    for(;;)
    {
      gettimeofday(&after, 0);

      uInt32 delta = (uInt32)((after.tv_sec - before.tv_sec) * 1000000 +
          (after.tv_usec - before.tv_usec));

      if(delta > frameTime)
        break;
//else SDL_Delay(1);
    }
  }

  if(theShowInfoFlag)
  {
    timeval endingTime;
    gettimeofday(&endingTime, 0);
    double executionTime = (endingTime.tv_sec - startingTime.tv_sec) +
            ((endingTime.tv_usec - startingTime.tv_usec) / 1000000.0);
    double framesPerSecond = numberOfFrames / executionTime;

    cout << endl;
    cout << numberOfFrames << " total frames drawn\n";
    cout << framesPerSecond << " frames/second\n";
    cout << theConsole->mediaSource().scanlines() << " scanlines in last frame\n";
    cout << endl;
    cout << "Cartridge Name: " << theConsole->properties().get("Cartridge.Name");
    cout << endl;
    cout << "Cartridge MD5:  " << theConsole->properties().get("Cartridge.MD5");
    cout << endl << endl;
  }

  // Cleanup time ...
  cleanup();
}
