/*
    /dev/audio "emulation" program
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <io.h>
#include <fcntl.h>
#include <gnu/getopt.h>
#define INCL_DOS
#include "libDART.h"

static char *programname;
static char *programversion = "0.1.1";

static struct option long_options[] =
{
  {"bytes", no_argument, 0, 'b'},
  {"channels", required_argument, 0, 'c'},
  {"device", required_argument, 0, 'd'},
  {"format", required_argument, 0, 'f'},
  {"help", no_argument, 0, 'h'},
  {"progress", no_argument, 0, 'p'},
  {"rate", required_argument, 0, 'r'},
  {"switch-sign", no_argument, 0, 's'},
  {"verbose", no_argument, 0, 'v'},
  {"version", no_argument, 0, 1},
  {"words", no_argument, 0, 'w'},
  {0, no_argument, 0, 0}
};

static void display_help ()
{
  printf ("\n/dev/audio emulator version %s\n", programversion);
  printf ("Copyleft (L) 1998 FRIENDS software\n\n");
  printf ("Usage: devaudio [option/s] [filename/-]\n");
  printf ("By default devaudio will read raw data from stdin and dump it to the default\n");
  printf ("wave audio device. If you specify a filename, the data will be read from\n");
  printf ("the specified file. The data stream is interpreted depending on options:\n\n");
  printf ("  -d# --device=#    Choose audio device index (default = 0)\n");
  printf ("  -b  --bytes       One byte (8 bit) per sample\n");
  printf ("  -w  --words       One word (16 bit) per sample (default)\n");
  printf ("  -r# --rate=#      Define sampling rate (default = 44100Hz)\n");
  printf ("  -c# --channels=#  Define number of channels (default = 2 - stereo)\n");
  printf ("  -f# --format=#    Define sample format (default = pcm)\n");
  printf ("                    Supported formats: pcm, adpcm, ibm_cvsd,\n");
  printf ("                    alaw mulaw, oki_adpcm, dvi_adpcm, digistd,\n");
  printf ("                    digifix, avc_adpcm, ibm_adpcm, ibm_mulaw,\n");
  printf ("                    ibm_alaw, ct_adpcm, mpeg1\n");
  printf ("  -p  --progress    Display progress bar as sound is playing\n");
  printf ("  -s  --switch-sign Switch sign (unsigned->signed, signed->unsigned)\n");
  printf ("  -h  --help        Display usage help\n");
  printf ("  -v  --verbose     Show additional information\n");
  printf ("      --version     Show program version\n");
}

static void CheckError (DART &dart)
{
  if (dart.Error ())
  {
    printf ("%s: DART error: %s\n", programname, dart.GetErrorString ());
    exit (-1);
  }
}

char *format_name (long sample_format)
{
  switch (sample_format)
  {
    case MCI_WAVE_FORMAT_PCM:
      return "pcm";
    case MCI_WAVE_FORMAT_ADPCM:
      return "adpcm";
    case MCI_WAVE_FORMAT_IBM_CVSD:
      return "ibm_cvsd";
    case MCI_WAVE_FORMAT_ALAW:
      return "alaw";
    case MCI_WAVE_FORMAT_MULAW:
      return "mulaw";
    case MCI_WAVE_FORMAT_OKI_ADPCM:
      return "oki_adpcm";
    case MCI_WAVE_FORMAT_DVI_ADPCM:
      return "dvi_adpcm";
    case MCI_WAVE_FORMAT_DIGISTD:
      return "digistd";
    case MCI_WAVE_FORMAT_DIGIFIX:
      return "digifix";
    case MCI_WAVE_FORMAT_AVC_ADPCM:
      return "avc_adpcm";
  //case MCI_WAVE_FORMAT_IBM_ADPCM:
  //  return "ibm_adpcm";
    case MCI_WAVE_FORMAT_IBM_MULAW:
      return "ibm_mulaw";
    case MCI_WAVE_FORMAT_IBM_ALAW:
      return "ibm_alaw";
    case MCI_WAVE_FORMAT_CT_ADPCM:
      return "ct_adpcm";
    case MCI_WAVE_FORMAT_MPEG1:
      return "mpeg1";
    default:
      return "*unknown*";
  }
}

static int input_handle = 0;
static int switch_sign = false;
static int bits_per_sample = BPS_16;
static volatile size_t global_read = 0;

size_t dartCallback (void *Buffer, size_t BufferSize)
{
  size_t bytesread = 1, count = 0;
  while ((bytesread) && (count < BufferSize))
  {
    bytesread = read (input_handle, &((char *)Buffer) [count], BufferSize - count);
    if ((int)bytesread == -1)
      break;
    count += bytesread;
  }
  if (switch_sign)
  {
    char *sample = (char *)Buffer;
    char *lastsample = ((char *)Buffer) + count;
    if (bits_per_sample == BPS_16)
      while (sample < lastsample)
      {
        sample++;
        *sample ^= 0x80;
        sample++;
      } /* endwhile */
    else
      while (sample < lastsample)
      {
        *sample ^= 0x80;
        sample++;
      } /* endwhile */
  } /* endif */

  global_read += count;
  return count;
}

int main (int argc, char *argv[])
{
  int device_index = 0;
  int sampling_rate = 44100;
  int sample_channels = 2;
  long sample_format = MCI_WAVE_FORMAT_PCM;
  char *input_file = NULL;
  bool verbose = false;
  bool progress = false;

  programname = argv[0];

  int c;
  while ((c = getopt_long (argc, argv, "d:bwpsr:c:f:hv", long_options, (int *) 0)) != EOF)
    switch (c)
    {
      case '?':
        // unknown option
        return -1;
      case 'b':
        bits_per_sample = BPS_8;
        break;
      case 'w':
        bits_per_sample = BPS_16;
        break;
      case 'f':
        if (!stricmp (optarg, "pcm"))
          sample_format = MCI_WAVE_FORMAT_PCM;
        else if (!stricmp (optarg, "adpcm"))
          sample_format = MCI_WAVE_FORMAT_ADPCM;
        else if (!stricmp (optarg, "ibm_cvsd"))
          sample_format = MCI_WAVE_FORMAT_IBM_CVSD;
        else if (!stricmp (optarg, "alaw"))
          sample_format = MCI_WAVE_FORMAT_ALAW;
        else if (!stricmp (optarg, "mulaw"))
          sample_format = MCI_WAVE_FORMAT_MULAW;
        else if (!stricmp (optarg, "oki_adpcm"))
          sample_format = MCI_WAVE_FORMAT_OKI_ADPCM;
        else if (!stricmp (optarg, "dvi_adpcm"))
          sample_format = MCI_WAVE_FORMAT_DVI_ADPCM;
        else if (!stricmp (optarg, "digistd"))
          sample_format = MCI_WAVE_FORMAT_DIGISTD;
        else if (!stricmp (optarg, "digifix"))
          sample_format = MCI_WAVE_FORMAT_DIGIFIX;
        else if (!stricmp (optarg, "avc_adpcm"))
          sample_format = MCI_WAVE_FORMAT_AVC_ADPCM;
        else if (!stricmp (optarg, "ibm_adpcm"))
          sample_format = MCI_WAVE_FORMAT_IBM_ADPCM;
        else if (!stricmp (optarg, "ibm_mulaw"))
          sample_format = MCI_WAVE_FORMAT_IBM_MULAW;
        else if (!stricmp (optarg, "ibm_alaw"))
          sample_format = MCI_WAVE_FORMAT_IBM_ALAW;
        else if (!stricmp (optarg, "ct_adpcm"))
          sample_format = MCI_WAVE_FORMAT_CT_ADPCM;
        else if (!stricmp (optarg, "mpeg1"))
          sample_format = MCI_WAVE_FORMAT_MPEG1;
        else
        {
          printf ("%s: unknown sample format -- %s\n", programname, optarg);
          return -1;
        }
        break;
      case 'r':
        sampling_rate = atoi (optarg);
        if ((sampling_rate > 100000) || (sampling_rate < 1000))
        {
          printf ("%s: invalid sample rate -- %d\n", programname, sampling_rate);
          return -1;
        }
        break;
      case 'c':
        sample_channels = atoi (optarg);
        if ((sample_channels > 8) || (sample_channels < 1))
        {
          printf ("%s: invalid number of channels -- %d\n", programname, sample_channels);
          return -1;
        }
        break;
      case 'd':
        device_index = atoi (optarg);
        if ((device_index > 10) || (device_index < 0))
        {
          printf ("%s: invalid device index -- %d\n", programname, device_index);
          return -1;
        }
        break;
      case 's':
        switch_sign = true;
        break;
      case 'h':
        display_help ();
        return 0;
      case 'v':
        verbose = true;
        break;
      case 'p':
        progress = true;
        break;
      case 1:
        printf ("%s version %s\n\n", programname, programversion);
        printf ("This program is distributed in the hope that it will be useful,\n");
        printf ("but WITHOUT ANY WARRANTY; without even the implied warranty of\n");
        printf ("MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the\n");
        printf ("GNU Library General Public License for more details.\n");
        return 0;
      default:
        // oops!
        abort ();
    }

  // Interpret the non-option arguments as file names
  for (; optind < argc; ++optind)
  {
    if (input_file == 0)
      input_file = argv [optind];
    else
      printf ("%s: excess command line argument -- %s\n", programname, argv [optind]);
  }

  if (verbose)
  {
    printf ("Input file: %s\n", input_file ? input_file : "stdin");
    printf ("Format: %s (%d bits/sample) at %dHz, %d channels\n",
      format_name (sample_format), bits_per_sample, sampling_rate,
      sample_channels);
    printf ("The sign of samples is %s\n", switch_sign ? "switched" : "left as-is");
  }

  // Open input file
  if ((input_file != NULL)
   && (strcmp (input_file, "-") != 0))
    input_handle = open (input_file, O_RDONLY | O_BINARY);
  if (input_handle < 0)
  {
    printf ("%s: cannot open file '%s'\n", programname, input_file);
    return -1;
  }
  setmode (input_handle, O_BINARY);	// Set handle to binary mode

  // if progress bar is enabled, find out file size
  size_t file_size = 0;
  if (progress)
  {
    file_size = lseek (input_handle, 0, SEEK_END);
    lseek (input_handle, 0, SEEK_SET);
    if ((int)file_size == -1)
    {
      fprintf (stderr, "File is not seekable, disabling progress bar\n");
      progress = false;
    }
  }

  DART dart (device_index, bits_per_sample, sampling_rate, sample_format,
    sample_channels, 4);
  CheckError (dart);

  dart.SetInputCallback (dartCallback);
  dart.Play ();
  CheckError (dart);

  int retcode = 0;
  while (!dart.Stopped)
  {
    if (_read_kbd (0, 0, 0) >= 0)
    {
      retcode = 3;
      break;
    }
    if (progress)
    {
      if (global_read > file_size)
        global_read = file_size;
      size_t dart_read = dart.BytesPlayed;
      if (dart_read > file_size)
        dart_read = file_size;

      char bar [41];
      memset (bar, '°', 40); bar [40] = 0;
      memset (bar, '²', (global_read * 40) / file_size);
      memset (bar, 'Û', (dart_read * 40) / file_size);
      printf ("ÄÍ[%s]ÍÄ\r", bar);
    }
    DosSleep (100);
  }

  if (progress)
    printf ("\033[K");

  return retcode;
}
