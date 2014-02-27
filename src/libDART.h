/*
    Direct Audio Interface library for OS/2
    Copyright (C) 1998 by Andrew Zabolotny <bit@eltech.ru>
  
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.
  
    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.
  
    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef __DART_h__
#define __DART_h__

#define INCL_OS2MM
#include <os2.h>
#include <os2me.h>
#include <stdio.h>
#include <stddef.h>

typedef size_t (*dartInputCallback) (void *Buffer, size_t BufferSize);

/**
 * This class implements raw soundcard access through the DART
 * ([D]irect [A]udio in [R]eal [T]ime) interface of the OS/2 Multimedia
 * subsystem
 */
class DART
{
public:
  volatile bool Stopped;
  volatile int BytesPlayed;

  /// Open given waveaudio device
  DART (int DeviceIndex, int BitsPerSample, int SamplingRate, int DataFormat,
    int Channels, int NumBuffers);
  /// Close the device
  virtual ~DART ();

  /// Get error status
  bool Error () { return !!ErrorCode [0]; }
  /// Get the last error string
  char *GetErrorString () { return ErrorCode; }

  /// Set data input callback
  void SetInputCallback (dartInputCallback Callback) { InputCallback = Callback; }

  /// Start playback
  bool Play ();
  /// Stop the playback
  void Stop ();

private:
  bool SetError (APIRET rc);
  void FreeBuffers ();
  void Close ();
  bool FillBuffer (PMCI_MIX_BUFFER pBuffer);
  static LONG APIENTRY MixHandler (ULONG ulStatus, PMCI_MIX_BUFFER pBuffer,
    ULONG ulFlags);
  dartInputCallback InputCallback;

  int BufferCount;
  MCI_MIX_BUFFER *MixBuffers;			/* Device buffers          */
  MCI_MIXSETUP_PARMS MixSetupParms;		/* Mixer parameters        */
  MCI_BUFFER_PARMS BufferParms;			/* Device buffer parms     */
  MCI_PLAY_PARMS PlayParams;
  USHORT DeviceId;
  CHAR ErrorCode [CCHMAXPATH];
  bool WaitStreamEnd;
};

#endif // __DART_h__
