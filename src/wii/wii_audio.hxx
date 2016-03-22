/****************************************************************************
* From the FCE Ultra 0.98.12
* Nintendo Wii/Gamecube Port
*
* Tantric September 2008
* eke-eke October 2008
* Simon Kagstrom Jan 2009
*
* gcaudio.h
*
* Audio driver
****************************************************************************/

#include <stdint.h>
#include <gctypes.h>

void InitialiseAudio();
void StopAudio();
void ResetAudio();
void PlaySound( u32 *Buffer, int samples );

