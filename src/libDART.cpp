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

#include <stdlib.h>
#include <string.h>
#include "libDART.h"

DART::DART (int DeviceIndex, int BitsPerSample, int SamplingRate, int DataFormat,
  int Channels, int NumBuffers)
{
  MCI_AMP_OPEN_PARMS AmpOpenParms;
  MCI_GENERIC_PARMS GenericParms;
  APIRET rc;

  DeviceId = 0;				// Clear device ID
  MixBuffers = NULL;			// Clear mixer buffers
  InputCallback = NULL;			// Clear data input callback

  // Setup the open structure, pass the playlist and tell MCI_OPEN to use it
  memset (&AmpOpenParms, 0, sizeof (AmpOpenParms));

  AmpOpenParms.usDeviceID = (USHORT) 0;
  AmpOpenParms.pszDeviceType = (PSZ) (MCI_DEVTYPE_AUDIO_AMPMIX | (DeviceIndex << 16));

  rc = mciSendCommand (0, MCI_OPEN, MCI_WAIT | MCI_OPEN_TYPE_ID | MCI_OPEN_SHAREABLE,
         (PVOID) &AmpOpenParms, 0);
  if (SetError (rc))
    return;
  DeviceId = AmpOpenParms.usDeviceID;

  // Grab exclusive rights to device instance (NOT entire device)
  GenericParms.hwndCallback = 0;	// Not needed, so set to 0
  rc = mciSendCommand (DeviceId, MCI_ACQUIREDEVICE, MCI_EXCLUSIVE_INSTANCE,
       (PVOID) &GenericParms, 0);
  if (SetError (rc))
    return;

  // Allocate mixer buffers
  MixBuffers = new MCI_MIX_BUFFER [NumBuffers];

  // Setup the mixer for playback of wave data
  memset (&MixSetupParms, 0, sizeof (MCI_MIXSETUP_PARMS));
  MixSetupParms.ulBitsPerSample = BitsPerSample;
  MixSetupParms.ulSamplesPerSec = SamplingRate;
  MixSetupParms.ulFormatTag = DataFormat;
  MixSetupParms.ulChannels = Channels;
  MixSetupParms.ulFormatMode = MCI_PLAY;
  MixSetupParms.ulDeviceType = MCI_DEVTYPE_WAVEFORM_AUDIO;
  MixSetupParms.pmixEvent = MixHandler;
  rc = mciSendCommand (DeviceId, MCI_MIXSETUP, MCI_WAIT | MCI_MIXSETUP_INIT,
         (PVOID) &MixSetupParms, 0);
  if (SetError (rc))
    return;

  // Use the suggested buffer size provide by the mixer device
  // Set up the BufferParms data structure and allocate device buffers
  // from the Amp-Mixer
  BufferParms.ulStructLength = sizeof (BufferParms);
  BufferParms.ulNumBuffers = NumBuffers;
  BufferParms.ulBufferSize = MixSetupParms.ulBufferSize;
  BufferParms.pBufList = MixBuffers;
  for (int i = 0; i < NumBuffers; i++)
    MixBuffers [i].ulUserParm = (ULONG)this;

  rc = mciSendCommand (DeviceId, MCI_BUFFER, MCI_WAIT | MCI_ALLOCATE_MEMORY,
    (PVOID) &BufferParms, 0);
  if (SetError (rc))
    return;

  // The mixer possibly changed these values
  BufferCount = BufferParms.ulNumBuffers;

  return;
}

DART::~DART ()
{
  if (DeviceId)
  {
    Stop ();
    FreeBuffers ();
    Close ();
  } /* endif */
}

bool DART::SetError (APIRET rc)
{
  if (rc)
  {
    mciGetErrorString (rc, (PSZ) ErrorCode, sizeof (ErrorCode));
    return true;
  }
  ErrorCode [0] = 0;
  return false;
}

void DART::FreeBuffers ()
{
  if (!MixBuffers)
    return;

  // restore original buffer sizes -- just in case
  for (int i = 0; i < BufferCount; i++)
    MixBuffers [i].ulBufferLength = BufferParms.ulBufferSize;
  APIRET rc = mciSendCommand (DeviceId, MCI_BUFFER, MCI_WAIT | MCI_DEALLOCATE_MEMORY,
    (PVOID) &BufferParms, 0);
  if (MixBuffers)
    delete [] MixBuffers;
  MixBuffers = NULL;
  SetError (rc);
}

void DART::Close ()
{
  // Generic parameters
  MCI_GENERIC_PARMS GenericParms;
  GenericParms.hwndCallback = 0;
  APIRET rc = mciSendCommand (DeviceId, MCI_CLOSE, MCI_WAIT, (PVOID) &GenericParms, 0);
  SetError (rc);
}

bool DART::Play ()
{
  Stopped = WaitStreamEnd = false;
  BytesPlayed = 0;
  int buffcount;
  for (buffcount = 0; buffcount < BufferCount; buffcount++)
    if (!FillBuffer (&MixBuffers [buffcount]))
      break;

  if (buffcount == 0)
    Stopped = WaitStreamEnd = true;
  else
  {
    APIRET rc = MixSetupParms.pmixWrite (MixSetupParms.ulMixHandle,
      &MixBuffers [0], buffcount);
    if (rc)
    {
      mciGetErrorString (rc, (PSZ) ErrorCode, sizeof (ErrorCode));
      return FALSE;
    }
  }
  return TRUE;
}

void DART::Stop ()
{
  if (DeviceId)
  {
    MCI_GENERIC_PARMS GenericParms;
    GenericParms.hwndCallback = 0;
    mciSendCommand (DeviceId, MCI_STOP, MCI_WAIT, (PVOID) &GenericParms, 0);
  } /* endif */
  Stopped = true;
}

bool DART::FillBuffer (PMCI_MIX_BUFFER pBuffer)
{
  size_t top = 0;
  if (InputCallback && !WaitStreamEnd)
    top = InputCallback (pBuffer->pBuffer, BufferParms.ulBufferSize);
  if (top < BufferParms.ulBufferSize)
  {
    pBuffer->ulFlags = MIX_BUFFER_EOS;
    WaitStreamEnd = true;
  }
  else
    pBuffer->ulFlags = 0;
  pBuffer->ulBufferLength = top;
  return !!top;
}

LONG APIENTRY DART::MixHandler (ULONG ulStatus, PMCI_MIX_BUFFER pBuffer,
  ULONG ulFlags)
{
  DART *This = (DART *)pBuffer->ulUserParm;
  if (This->Stopped)
    return TRUE;

  switch (ulFlags)
  {
    case MIX_STREAM_ERROR | MIX_WRITE_COMPLETE:
      // on error, fill next buffer and continue
    case MIX_WRITE_COMPLETE:
      This->BytesPlayed += pBuffer->ulBufferLength;
      // If this is the last buffer, stop
      if (pBuffer->ulFlags & MIX_BUFFER_EOS)
        This->Stop ();
      else
      {
        // Transfer buffer to DART
        if (!This->FillBuffer (pBuffer))
        {
          // if user callback failed to fill the buffer, we should fill it
          // with silence to avoid annoying clicks that happens on some soundcards
          pBuffer->ulBufferLength = This->BufferParms.ulBufferSize;
          memset (pBuffer->pBuffer, 0x80, pBuffer->ulBufferLength);
        }
        This->MixSetupParms.pmixWrite (This->MixSetupParms.ulMixHandle, pBuffer, 1);
      }
      break;
  }
  return TRUE;
}
