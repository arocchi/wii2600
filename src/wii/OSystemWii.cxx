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
// Copyright (c) 1995-2009 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: OSystemWii.cxx,v 1.32 2009-01-16 16:38:06 stephena Exp $
//============================================================================

#include <cstdlib>
#include <unistd.h>

#include "bspf.hxx"
#include "OSystem.hxx"
#include "OSystemWii.hxx"

#ifdef HAVE_GETTIMEOFDAY
  #include <time.h>
  #include <sys/time.h>
#endif

#include "wii_app.hxx"

/**
  Each derived class is responsible for calling the following methods
  in its constructor:

  setBaseDir()
  setConfigFile()

  See OSystem.hxx for a further explanation
*/

static char basedir[WII_MAX_PATH];
static char configfile[WII_MAX_PATH];

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OSystemWii::OSystemWii()
  : OSystem()
{
    wii_get_app_relative( "", basedir );
    wii_get_app_relative( "stellarc", configfile );    
    setBaseDir(basedir);
    setConfigFile(configfile);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OSystemWii::~OSystemWii()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 OSystemWii::getTicks() const
{
#ifdef HAVE_GETTIMEOFDAY
  timeval now;
  gettimeofday(&now, 0);

  return (uInt32) (now.tv_sec * 1000000 + now.tv_usec);
#else
  return (uInt32) SDL_GetTicks() * 1000;
#endif
}
