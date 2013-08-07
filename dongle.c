#include "dongle.h"

#ifdef _WIN32
#include "win32_error.h"
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifndef _WIN32
#include <unistd.h>
#include <termios.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

static void dongleInit (MOBOTdongle *dongle);
static void dongleFini (MOBOTdongle *dongle);

/* NULL ms_delay means wait forever, otherwise it is a pointer to the number
 * of milliseconds to wait.
 *
 * Returns 0 on timeout (or read() returning zero), -1 on error, number of
 * bytes read otherwise. */
static ssize_t dongleTimedReadRaw (MOBOTdongle *dongle, uint8_t *buf,
    size_t len, const long *ms_delay);

static ssize_t dongleWriteRaw (MOBOTdongle *dongle, const uint8_t *buf, size_t len) {
  assert(dongle && buf);
  ssize_t ret;
#ifdef _WIN32
  DWORD bytesWritten;
  BOOL b = WriteFile(dongle->handle, buf, len, &bytesWritten, dongle->ovOutgoing);

  if (b) {
    return bytesWritten;
  }

  DWORD err = GetLastError();
  if (ERROR_IO_PENDING != err) {
    win32_error(_T("(barobo) ERROR: in dongleWriteRaw, "
        "WriteFile()"), err);
    return -1;
  }

  if (!GetOverlappedResult(dongle->handle, dongle->ovOutgoing, &bytesWritten, TRUE)) {
    win32_error(_T("(barobo) ERROR: in dongleWriteRaw, "
          "GetOverlappedResult()"), GetLastError());
    return -1;
  }

  if (!ResetEvent(dongle->ovOutgoing->hEvent)) {
    win32_error(_T("(barobo) ERROR: in dongleWriteRaw, "
          "ResetEvent()"), GetLastError());
    return -1;
  }

  ret = bytesWritten;
#else
  ret = write(dongle->fd, buf, len);

  if (-1 == ret) {
    char errbuf[256];
    strerror_r(errno, errbuf, sizeof(errbuf));
    fprintf(stderr, "(barobo) ERROR: in dongleWriteRaw, write(): %s\n", errbuf);
  }
#endif

  return ret;
}

#ifdef _WIN32

/* WIN32 implementation of dongleTimedReadRaw. */

static ssize_t dongleTimedReadRaw (MOBOTdongle *dongle, uint8_t *buf, size_t len, const long *ms_delay) {
  assert(dongle && buf);

  DWORD readbytes = 0;
  BOOL b = ReadFile(dongle->handle, buf, len, &readbytes, dongle->ovIncoming);

  if (b) {
    /* ReadFile completed synchronously. */
    return readbytes;
  }

  DWORD err = GetLastError();
  if (ERROR_IO_PENDING != err) {
    win32_error(_T("(barobo) ERROR: in dongleTimedReadRaw, "
          "ReadFile()"), err);
    return -1;
  }

  DWORD code = WaitForSingleObject(dongle->ovIncoming->hEvent, ms_delay ? *ms_delay : INFINITE);

  if (WAIT_TIMEOUT == code) {
    if (!CancelIo(dongle->handle)) {
      win32_error(_T("(barobo) ERROR: in dongleTimedReadRaw, "
            "CancelIo()"), GetLastError());
      return -1;
    }
    return 0;
  }
  else if (WAIT_FAILED == code) {
    win32_error(_T("(barobo) ERROR: in dongleTimedReadRaw, "
          "WaitForSingleObject()"), GetLastError());
    return -1;
  }
  else {
    assert(WAIT_OBJECT_0 == code);
  }

  if (!GetOverlappedResult(dongle->handle, dongle->ovIncoming, &readbytes, FALSE)) {
    win32_error(_T("(barobo) ERROR: in dongleTimedReadRaw, "
        "GetOverlappedResult()\n"), GetLastError());
    return -1;
  }

  if (!ResetEvent(dongle->ovIncoming->hEvent)) {
    win32_error(_T("(barobo) ERROR: in dongleTimedReadRaw, "
          "ResetEvent()\n"), GetLastError());
    return -1;
  }

  return readbytes;
}

#else /* _WIN32 */

/* POSIX implementation of dongleTimedReadRaw. */

static ssize_t dongleTimedReadRaw (MOBOTdongle *dongle, uint8_t *buf, size_t len, const long *ms_delay) {
  fd_set rfds;
  FD_ZERO(&rfds);
  FD_SET(dongle->fd, &rfds);

  struct timeval *ptimeout = NULL;
  struct timeval timeout;

  if (ms_delay) {
    timeout.tv_sec = 0;
    timeout.tv_usec = *ms_delay * 1000;
    ptimeout = &timeout;
  }

  int err = select(dongle->fd + 1, &rfds, NULL, NULL, ptimeout);

  if (-1 == err) {
    char errbuf[256];
    strerror_r(errno, errbuf, sizeof(errbuf));
    fprintf(stderr, "(barobo) ERROR: in dongleIsReadyToRead, select(): %s\n", errbuf);
    return -1;
  }

  if (!err || !FD_ISSET(dongle->fd, &rfds)) {
    /* We timed out. */
    return 0;
  }

  /* Perform the read. */
  err = read(dongle->fd, buf, len);

  if (-1 == err) {
    if (EAGAIN == errno || EWOULDBLOCK == errno) {
      /* The select() man page warns that this can happen: select() thinks a
       * file descriptor is ready for reading, but read() disagrees. In this
       * case, the proper way to handle it is probably to call select() again
       * with a delay of the amount of time remaining. This is easy to do
       * using the clock_gettime() POSIX call, which apparently doesn't exist
       * on OS X. So instead, let's just claim to have timed out--this case
       * should be exceedingly rare and probably harmless, anyway. */
      err = 0;
      fprintf(stderr, "(barobo) WARNING: in dongleTimedReadRaw, select() returned false positive\n");
    }
    else {
      char errbuf[256];
      strerror_r(errno, errbuf, sizeof(errbuf));
      fprintf(stderr, "(barobo) ERROR: in dongleReadRaw, read(): %s\n", errbuf);
    }
  }

  return err;
}

#endif

static ssize_t dongleReadRaw (MOBOTdongle *dongle, uint8_t *buf, size_t len) {
  return dongleTimedReadRaw(dongle, buf, len, NULL);
}

static int dongleDetectFraming (MOBOTdongle *dongle) {
  /* The following is a magic octet string which the old firmware, which used
   * no framing, should parse correctly, but which will look like one
   * complete, though corrupt, frame to libsfp. */
  static const uint8_t detection[] = {
    0x7e, // flag in command location--the old firmware never reads this octet
    0x09, // length, including the terminating flag
    0x00, 0x00, 0x01, // ZigBee stuff
    0x30, 0x03, 0x00, // CMD_GETSTATUS
    0x7e  // flag--the old firmware never asserts that this has to be 0x00
  };

  ssize_t err = dongleWriteRaw(dongle, detection, sizeof(detection));
  if (-1 == err) {
    return -1;
  }
  else if (sizeof(detection) != err) {
    fprintf(stderr, "(barobo) ERROR: in dongleDetectFraming, unable to write "
        "complete detection string.\n");
    return -1;
  }

  /* Give the dongle a little time to think about it. */
  long ms_delay = 1000;
  uint8_t response[256];
  ssize_t bytesread = dongleTimedReadRaw(dongle, response, sizeof(response), &ms_delay);
  if (!bytesread) {
    fprintf(stderr, "(barobo) ERROR: in dongleDetectFraming, timed out "
        "waiting for response.\n");
    return -1;
  }

  assert(bytesread <= sizeof(response));

  fprintf(stderr, "(barobo) DEBUG: dongleDetectFraming received:");
  for (size_t i = 0; i < bytesread; ++i) {
    fprintf(stderr, " %02x", response[i]);
  }
  fprintf(stderr, "\n");

  static const uint8_t old_response[]
    = { 0x10, 0x09, 0x00, 0x00, 0x01, 0x10, 0x03, 0x11, 0x11 };

  if (sizeof(old_response) <= bytesread
      && !memcmp(response, old_response, sizeof(old_response))) {
    fprintf(stderr, "(barobo) INFO: old (unframed serial protocol) firmware detected.\n");
    dongle->framing = MOBOT_DONGLE_FRAMING_NONE;
  }
  else {
    fprintf(stderr, "(barobo) INFO: new (framed serial protocol) firmware detected.\n");
    dongle->framing = MOBOT_DONGLE_FRAMING_SFP;
  }

  return 0;
}

static void sfp_lock (void *data) {
  MUTEX_LOCK((MUTEX_T *)data);
}

static void sfp_unlock (void *data) {
  MUTEX_UNLOCK((MUTEX_T *)data);
}

/* Callback to be provided to libsfp. */
static int sfp_write (uint8_t *octets, size_t len, size_t *outlen, void *data) {
  MOBOTdongle *dongle = (MOBOTdongle *)data;
  ssize_t ret = dongleWriteRaw(dongle, octets, len);
  if (outlen && ret >= 0) {
    *outlen = ret;
    ret = 0;
  }
  return ret;
}

ssize_t dongleWrite (MOBOTdongle *dongle, const uint8_t *buf, size_t len) {
  assert(dongle);
  assert(buf);

  if (MOBOT_DONGLE_FRAMING_SFP == dongle->framing) {
    sfpWritePacket(dongle->sfpContext, buf, len, NULL);
    return len;
  }
  else {
    return dongleWriteRaw(dongle, buf, len);
  }
}

/* Block until a complete message from the dongle is received. Return the
 * message in the output parameter buf, bounded by size len. Returns -1 on
 * error, otherwise the number of bytes read. */
ssize_t dongleRead (MOBOTdongle *dongle, uint8_t *buf, size_t len) {
  assert(dongle);
  assert(buf);

  size_t i = 0;

  while (1) {
    uint8_t byte;
    int err = dongleReadRaw(dongle, &byte, 1);
    if (-1 == err) {
      return -1;
    }
    if (1 != err) {
      continue;
    }

    if (MOBOT_DONGLE_FRAMING_SFP == dongle->framing) {
      size_t outlen = 0;
      int ret = sfpDeliverOctet(dongle->sfpContext, byte, buf, len, &outlen);
      if (ret > 0) {
        /* We got a packet. :) */
        return outlen;
      }
      else if (ret < 0) {
        /* We got an error. :( */
        return ret;
      }
    }
    else {
      /* The old way, where we rely on the length, in the second octet of a packet. */
      if (i >= len) {
        return -1;
      }
      buf[i++] = byte;
      if (i > 1 && buf[1] == i) {
        return i;
      }
    }
  }

  return -1;
}

static int dongleSetupSFP (MOBOTdongle *dongle) {
  MUTEX_NEW(dongle->sfpTxLock);
  MUTEX_INIT(dongle->sfpTxLock);

  dongle->sfpContext = malloc(sizeof(SFPcontext));
  assert(dongle->sfpContext);

  sfpInit(dongle->sfpContext);
  
  sfpSetWriteCallback(dongle->sfpContext, SFP_WRITE_MULTIPLE, sfp_write, dongle);
  /* We do not currently use the libsfp deliver callback in libbarobo, but
   * rather use sfpDeliverOctet's output parameters to get the complete
   * packets. */
  sfpSetLockCallback(dongle->sfpContext, sfp_lock, dongle->sfpTxLock);
  sfpSetUnlockCallback(dongle->sfpContext, sfp_unlock, dongle->sfpTxLock);

  sfpConnect(dongle->sfpContext);

  while (!sfpIsConnected(dongle->sfpContext)) {
    // nasty hax, this could block or die :(
    uint8_t byte;
    long delay = 1000;
    int err = dongleTimedReadRaw(dongle, &byte, 1, &delay);
    if (1 != err) {
      /* Either we hit an error, or there's nothing to read. If there's
       * nothing to read, the remote end is probably not actually using SFP. */
      return -1;
    }
    sfpDeliverOctet(dongle->sfpContext, byte, NULL, 0, NULL);
  }

  return 0;
}

static void dongleInit (MOBOTdongle *dongle) {
  dongle->framing = MOBOT_DONGLE_FRAMING_UNKNOWN;
  /* We don't malloc() sfpContext yet, because we don't know if we'll actually
   * need it until we detect the framing used by the firmware. */
  dongle->sfpContext = NULL;

#ifdef _WIN32
  /* Halp! Is this right? */
  dongle->handle = NULL;

  /* Initialize overlapped communication shit */
  dongle->ovIncoming = malloc(sizeof(OVERLAPPED));
  memset(dongle->ovIncoming, 0, sizeof(OVERLAPPED));
  dongle->ovIncoming->hEvent = CreateEvent(0, 1, 0, 0);

  dongle->ovOutgoing = malloc(sizeof(OVERLAPPED));
  memset(dongle->ovOutgoing, 0, sizeof(OVERLAPPED));
  dongle->ovOutgoing->hEvent = CreateEvent(0, 1, 0, 0);

#else
  dongle->fd = -1;
#endif
}

static void dongleFini (MOBOTdongle *dongle) {
  dongle->framing = MOBOT_DONGLE_FRAMING_UNKNOWN;
  if (dongle->sfpContext) {
    free(dongle->sfpContext);
    dongle->sfpContext = NULL;
  }

#ifdef _WIN32
  if (dongle->ovIncoming) {
    CloseHandle(dongle->ovIncoming->hEvent);
    free(dongle->ovIncoming);
    dongle->ovIncoming = NULL;
  }
  if (dongle->ovOutgoing) {
    CloseHandle(dongle->ovOutgoing->hEvent);
    free(dongle->ovOutgoing);
    dongle->ovOutgoing = NULL;
  }
#endif
}

#ifdef _WIN32

/* WIN32 implementation of dongleOpen. */

int dongleOpen (MOBOTdongle *dongle, const char *ttyfilename, unsigned long baud) {
  assert(dongle);
  assert(ttyfilename);

  dongleInit(dongle);

  /* hlh: for some reason, when I print to stdout and then to stderr, there
   * unicode issues */
  _ftprintf(stderr, _T("(barobo) INFO: opening %s with baud<%d>\n"), ttyfilename, baud);

  /* Check the file name. If we receive something like "COM45", we want to
   * change it to "\\.\COM45" */
  char *tty;
  tty = malloc((sizeof(char)*strlen(ttyfilename))+20);
  tty[0] = '\0';
  if(ttyfilename[0] != '\\') {
    strcpy(tty, "\\\\.\\");
  }
  strcat(tty, ttyfilename);
  /* For windows, we should connect to a com port */
  dongle->handle = CreateFile(
      tty, 
      GENERIC_READ | GENERIC_WRITE,
      0,
      0,
      OPEN_EXISTING,
      FILE_FLAG_OVERLAPPED,
      0 );
  if(dongle->handle == INVALID_HANDLE_VALUE) {
    win32_error(_T("(barobo) ERROR: in dongleOpen, CreateFile()"), GetLastError());
    return -1;
  }
  free(tty);
  _ftprintf(stderr, _T("(barobo) INFO: opened %s\n"), ttyfilename);
  /* Adjust settings */
  DCB dcb;
  FillMemory(&dcb, sizeof(dcb), 0);
  dcb.DCBlength = sizeof(dcb);
  char dcbconf[64];
  snprintf(dcbconf, sizeof(dcbconf), "%lu,n,8,1", baud);
  if (!BuildCommDCB(dcbconf, &dcb)) {
    win32_error(_T("(barobo) ERROR: in dongleOpen, BuildCommDCB()"), GetLastError());
    dongleClose(dongle);
    return -1;
  }
  dcb.BaudRate = baud;
  if (!SetCommState(dongle->handle, &dcb)) {
    win32_error(_T("(barobo) ERROR: in dongleOpen, SetCommState()"), GetLastError());
    dongleClose(dongle);
    return -1;
  }

  /* After setting the baud rate, we need to sleep for a bit. I presume this
   * is to let the hardware re-synchronize. On the Barobo office Windows
   * machine, 500ms suffices, 100ms does NOT. One second seems like a safe,
   * simple number. */
  Sleep(1000);

  if (!GetCommTimeouts(dongle->handle, &dongle->oldCommTimeouts)) {
    win32_error(_T("(barobo) ERROR: in dongleOpen, GetCommTimeouts()"),
        GetLastError());
    dongleClose(dongle);
    return -1;
  }

  COMMTIMEOUTS timeouts;
  memcpy(&timeouts, &dongle->oldCommTimeouts, sizeof(timeouts));
  /* ReadIntervalTimeout tunes ReadFile's completion behavior in the face of
   * bursty transmissions. A byte must be received within ReadIntervalTimeout
   * milliseconds of the previous byte, or else the operation completes with
   * whatever it collected up to that point. We should really set this to
   * max(1ms, (8 bits * 1000 (ms/sec)) / baud)
   * but for baud rates over 8000 this obviously saturates to 1ms. I don't
   * anticipate the Linkbot ever communicating at baud rates that slow... */
  timeouts.ReadIntervalTimeout = 1;
  timeouts.ReadTotalTimeoutMultiplier = 0;
  timeouts.ReadTotalTimeoutConstant = 0;
  timeouts.WriteTotalTimeoutMultiplier = 0;
  timeouts.WriteTotalTimeoutConstant = 0;

  if (!SetCommTimeouts(dongle->handle, &timeouts)) {
    win32_error(_T("(barobo) ERROR: in dongleOpen, SetCommTimeouts()"),
        GetLastError());
    dongleClose(dongle);
    return -1;
  }

  if (!PurgeComm(dongle->handle, PURGE_RXCLEAR | PURGE_TXCLEAR)) {
    win32_error(_T("(barobo) ERROR: in dongleOpen, PurgeComm()"),
        GetLastError());
    dongleClose(dongle);
    return -1;
  }

  if (-1 == dongleDetectFraming(dongle)) {
    fprintf(stderr, "(barobo) INFO: unable to detect dongle framing\n");
    dongleClose(dongle);
    return -1;
  }

  if (MOBOT_DONGLE_FRAMING_SFP == dongle->framing) {
    int status = dongleSetupSFP(dongle);
    if (-1 == status) {
      fprintf(stderr, "(barobo) ERROR: unable to complete SFP handshake\n");
      dongleClose(dongle);
      return -1;
    }
  }

  return 0;
}

#else /* WIN32 */

/* POSIX implementation of dongleOpen. */

static speed_t baud_to_speed_t (unsigned long baud) {
  /* Oh, the humanity */
  switch (baud) {
    case 230400:
      return B230400;
    case 500000:
      return B500000;
    default:
      return B0;
  }
}

int dongleOpen (MOBOTdongle *dongle, const char *ttyfilename, unsigned long baud) {
  assert(dongle);
  assert(ttyfilename);

  dongleInit(dongle);

  printf("(barobo) INFO: opening %s with baud<%d>\n", ttyfilename, baud);

  /* We use non-blocking I/O here so we can support the POSIX implementation
   * of dongleTimedReadRaw. dongleTimedReadRaw uses select, which can, per the
   * man page, return false positives. A false positive would then cause the
   * subsequent read call to block. */
  dongle->fd = open(ttyfilename, O_NONBLOCK | O_RDWR | O_NOCTTY);
  if(-1 == dongle->fd) {
    char errbuf[256];
    strerror_r(errno, errbuf, sizeof(errbuf));
    fprintf(stderr, "(barobo) ERROR: in dongleConnect, open(): %s\n", errbuf);
    return -1;
  }

  struct termios term;
  int status = tcgetattr(dongle->fd, &term);
  if (status) {
    char errbuf[256];
    strerror_r(errno, errbuf, sizeof(errbuf));
    fprintf(stderr, "(barobo) ERROR: in dongleOpen, tcgetattr(): %s\n", errbuf);
    return -1;
  }

  /* The following code appears to be from:
   * http://en.wikibooks.org/wiki/Serial_Programming/termios
   * Refer there for appropriate comments--the gist is that we're setting up
   * a serial link suitable for passing binary data, rather than a traditional
   * ASCII terminal. The flag settings are very similar to cfmakeraw. */

  // Input flags - Turn off input processing
  term.c_iflag &= ~(IGNBRK | BRKINT | ICRNL |
      INLCR | PARMRK | INPCK | ISTRIP | IXON);

  // Output flags - Turn off output processing
  term.c_oflag = 0;

  // No line processing:
  term.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN | ISIG);

  // Turn off character processing
  term.c_cflag &= ~(CSIZE | PARENB);
  term.c_cflag |= CS8;

  // One input byte is enough to return from read()
  // Inter-character timer off
  term.c_cc[VMIN]  = 1;
  term.c_cc[VTIME] = 0;

  // Communication speed

  speed_t speed = baud_to_speed_t(baud);
  assert(B0 != speed);

  if (-1 == cfsetispeed(&term, speed)) {
    char errbuf[256];
    strerror_r(errno, errbuf, sizeof(errbuf));
    fprintf(stderr, "(barobo) ERROR: in dongleOpen, cfsetispeed(): %s\n", errbuf);
    dongleClose(dongle);
    return -1;
  }

  if (-1 == cfsetospeed(&term, speed)) {
    char errbuf[256];
    strerror_r(errno, errbuf, sizeof(errbuf));
    fprintf(stderr, "(barobo) ERROR: in dongleOpen, cfsetospeed(): %s\n", errbuf);
    dongleClose(dongle);
    return -1;
  }

  if (-1 == tcsetattr(dongle->fd, TCSANOW, &term)) {
    char errbuf[256];
    strerror_r(errno, errbuf, sizeof(errbuf));
    fprintf(stderr, "(barobo) ERROR: Configuring %s: %s.\n", ttyfilename, errbuf);
    dongleClose(dongle);
    return -1;
  }

  if (-1 == tcgetattr(dongle->fd, &term)) {
    char errbuf[256];
    strerror_r(errno, errbuf, sizeof(errbuf));
    fprintf(stderr, "(barobo) ERROR: in dongleOpen, tcgetattr(): %s\n", errbuf);
    dongleClose(dongle);
    return -1;
  }

  if (cfgetispeed(&term) != speed) {
    fprintf(stderr, "(barobo) ERROR: Unable to set %s input speed.\n", ttyfilename);
    dongleClose(dongle);
    return -1;
  }

  if (cfgetospeed(&term) != speed) {
    fprintf(stderr, "(barobo) ERROR: Unable to set %s output speed.\n", ttyfilename);
    dongleClose(dongle);
    return -1;
  }

  if (-1 == tcflush(dongle->fd, TCIOFLUSH)) {
    char errbuf[256];
    strerror_r(errno, errbuf, sizeof(errbuf));
    fprintf(stderr, "(barobo) ERROR: in dongleOpen, tcflush(): %s\n", errbuf);
    dongleClose(dongle);
    return -1;
  }

  dongleDetectFraming(dongle);

  if (MOBOT_DONGLE_FRAMING_SFP == dongle->framing) {
    status = dongleSetupSFP(dongle);
    if (-1 == status) {
      dongleClose(dongle);
      return -1;
    }
  }

  //dongle->status = MOBOT_LINK_STATUS_UP;

  return 0;
}

#endif

void dongleClose (MOBOTdongle *dongle) {
  if (!dongle) {
    return;
  }

  if (MOBOT_DONGLE_FRAMING_SFP == dongle->framing) {
    /* sfpInit effectively disconnects */
    sfpInit(dongle->sfpContext);
    MUTEX_DESTROY(dongle->sfpTxLock);
    free(dongle->sfpTxLock);
    free(dongle->sfpContext);
  }

#ifdef _WIN32
  /* I dunno, MSDN said you're supposed to reset these to the way you found
   * them. Whatever. */
  if (!SetCommTimeouts(dongle->handle, &dongle->oldCommTimeouts)) {
    win32_error(_T("(barobo) ERROR: in dongleClose, SetCommTimeouts()"),
        GetLastError());
  }

  BOOL b = CloseHandle(dongle->handle);
  if (!b) {
    win32_error(_T("(barobo) ERROR: in dongleClose, CloseHandle()"), GetLastError());
  }
#else
  int err = close(dongle->fd);
  if (-1 == err) {
    char errbuf[256];
    strerror_r(errno, errbuf, sizeof(errbuf));
    fprintf(stderr, "(barobo) ERROR: in dongleClose, close(): %s\n", errbuf);
  }
#endif

  dongleFini(dongle);
}
