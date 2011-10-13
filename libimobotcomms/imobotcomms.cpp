
#include <stdio.h>
#include <stdlib.h>
#include "imobotcomms.h"


int iMobotComms_init(br_comms_t* comms)
{
  memset(&comms->addr, 0, sizeof(sockaddr_t));
  comms->connected = 0;
#ifdef _WIN32
  WSADATA wsd;
  WSAStartup (MAKEWORD(1,1), &wsd);
#endif
  return 0;
}

int iMobotComms_connect(br_comms_t* comms)
{
#ifndef _WIN32
  fprintf(stderr, 
      "ERROR; Function iMobotComms_connect() is not yet implemented "
      "on non-Windows systems. Please use iMobotComms_connectAddress() "
      "instead.");
  return -1;
#else
  char buf[80];

/* NUM_PORTS indicates the number of Windows COM ports to check for connections
 * to the robot. */
#define NUM_PORTS 30
  int i;
  for(i = 1; i < NUM_PORTS; i++) {
    sprintf(buf, "\\\\.\\COM%d", i);
    printf("Trying %s...\n", buf);
    comms->hSerial = CreateFileA(buf, 
        GENERIC_READ | GENERIC_WRITE,
        0,
        0,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        0);
    if(comms->hSerial == INVALID_HANDLE_VALUE) {
      printf("Could not open serial handle.\n");
      continue;
    }
    /* Set up the properties */
    DCB dcbSerialParams = {0};
    dcbSerialParams.DCBlength=sizeof(dcbSerialParams);
    if(!GetCommState(comms->hSerial, &dcbSerialParams)) {
      // error getting state
      printf("Could not get serial properties.\n");
      continue;
    }
    dcbSerialParams.BaudRate = CBR_115200;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;
    if(!SetCommState(comms->hSerial, &dcbSerialParams)) {
      // error setting state
      printf("Could not set serial properties.\n");
      continue;
    }

    /* Set up timeouts */
    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    timeouts.WriteTotalTimeoutConstant = 50;
    timeouts.WriteTotalTimeoutMultiplier = 10;
    if(!SetCommTimeouts(comms->hSerial, &timeouts)) {
      // Error setting timeouts
      printf("Could not set timeouts.\n");
      continue;
    }

    /* Send status request message to the iMobot */
    DWORD bytes;
    if(!WriteFile(comms->hSerial, "GET_IMOBOT_STATUS", strlen("GET_IMOBOT_STATUS")+1, &bytes, NULL)) {
      printf("Could not send status request message.\n");
      CloseHandle(comms->hSerial);
      continue;
    }
    printf("Wrote %d bytes.\n", bytes);
    Sleep(250);
    /* Check response message */
    if(!ReadFile(comms->hSerial, buf, 79, &bytes, NULL)) {
      printf("Could not receive response message.\n");
      CloseHandle(comms->hSerial);
      continue;
    }
    if(strcmp(buf, "IMOBOT READY")) {
      printf("Incorrect response message: %s.\n", buf);
      CloseHandle(comms->hSerial);
      continue;
    } else {
      comms->connected = 2;
      break;
    }
  }
  /* At this point, we _should_ be connected */
  if(i != NUM_PORTS) {
    return 0;
  } else {
    return -1;
  }
#endif
}

int iMobotComms_connectAddress(br_comms_t* comms, const char* address, int channel)
{
  int status;
  comms->socket = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);

  // set the connection parameters (who to connect to)
#ifndef _WIN32
  comms->addr.rc_family = AF_BLUETOOTH;
  comms->addr.rc_channel = (uint8_t) channel;
  str2ba( address, &comms->addr.rc_bdaddr );
#else
  comms->addr.addressFamily = AF_BTH;
  str2ba( address, (bdaddr_t*)&comms->addr.btAddr);
  comms->addr.port = channel;
#endif

  // connect to server
  status = connect(comms->socket, (const struct sockaddr *)&comms->addr, sizeof(comms->addr));
  if(status == 0) {
    comms->connected = 1;
  }
  return status;
}

int iMobotComms_disconnect(br_comms_t* comms)
{
#ifndef _WIN32
  close(comms->socket);
#else
  closesocket(comms->socket);
  CloseHandle((LPVOID)comms->socket);
#endif
  comms->connected = 0;
  return 0;
}

int iMobotComms_isConnected(br_comms_t* comms)
{
  return comms->connected;
}

int iMobotComms_setJointDirection(br_comms_t* comms, int id, int dir)
{
  char buf[160];
  int status;
  int bytes_read;
  sprintf(buf, "SET_MOTOR_DIRECTION %d %d", id, dir);
  status = SendToIMobot(comms, buf, strlen(buf)+1);
  if(status < 0) return status;
  bytes_read = RecvFromIMobot(comms, buf, sizeof(buf));
  if(strcmp(buf, "OK")) return -1;
  return 0;
}

int iMobotComms_getJointDirection(br_comms_t* comms, int id, int *dir)
{
  char buf[160];
  int status;
  int bytes_read;
  sprintf(buf, "GET_MOTOR_DIRECTION %d", id);
  status = SendToIMobot(comms, buf, strlen(buf)+1);
  if(status < 0) return status;
  bytes_read = RecvFromIMobot(comms, buf, sizeof(buf));
  if(!strcmp(buf, "ERROR")) return -1;
  sscanf(buf, "%d", dir);
  return 0;
}

int iMobotComms_setJointSpeed(br_comms_t* comms, int id, double speed)
{
  char buf[160];
  int status;
  int bytes_read;
  sprintf(buf, "SET_MOTOR_SPEED %d %d", id, (int)(speed*100));
  status = SendToIMobot(comms, buf, strlen(buf)+1);
  if(status < 0) return status;
  bytes_read = RecvFromIMobot(comms, buf, sizeof(buf));
  if(strcmp(buf, "OK")) return -1;
  return 0;
}

int iMobotComms_getJointSpeed(br_comms_t* comms, int id, double *speed)
{
  char buf[160];
  int status;
  int bytes_read;
  int ispeed;
  sprintf(buf, "GET_MOTOR_SPEED %d", id);
  status = SendToIMobot(comms, buf, strlen(buf)+1);
  if(status < 0) return status;
  bytes_read = RecvFromIMobot(comms, buf, sizeof(buf));
  if(!strcmp(buf, "ERROR")) return -1;
  sscanf(buf, "%d", &ispeed);
  *speed = (double)ispeed/100.0;
  return 0;
}

int iMobotComms_moveJointTo(br_comms_t* comms, int id, double angle)
{
  char buf[160];
  int status;
  int bytes_read;
  sprintf(buf, "SET_MOTOR_POSITION %d %lf", id, angle);
  status = SendToIMobot(comms, buf, strlen(buf)+1);
  if(status < 0) return status;
  bytes_read = RecvFromIMobot(comms, buf, sizeof(buf));
  if(strcmp(buf, "OK")) return -1;
  return 0;
}

int iMobotComms_getJointAngle(br_comms_t* comms, int id, double *angle)
{
  char buf[160];
  int status;
  int bytes_read;
  sprintf(buf, "GET_MOTOR_POSITION %d", id);
  status = SendToIMobot(comms, buf, strlen(buf)+1);
  if(status < 0) return status;
  bytes_read = RecvFromIMobot(comms, buf, sizeof(buf));
  if(!strcmp(buf, "ERROR")) return -1;
  sscanf(buf, "%lf", angle);
  return 0;
}

int iMobotComms_getJointState(br_comms_t* comms, int id, int *state)
{
  char buf[160];
  int status;
  int bytes_read;
  sprintf(buf, "GET_MOTOR_STATE %d", id);
  status = SendToIMobot(comms, buf, strlen(buf)+1);
  if(status < 0) return status;
  bytes_read = RecvFromIMobot(comms, buf, sizeof(buf));
  if(!strcmp(buf, "ERROR")) return -1;
  sscanf(buf, "%d", state);
  return 0;
}

int iMobotComms_moveZero(br_comms_t* comms)
{
  int i;
  for(i = 0; i < 4; i++) {
    if(iMobotComms_moveJointTo(comms, i, 0)) {
      return -1;
    }
  }
  return 0;
}

DLLIMPORT int iMobotComms_move(br_comms_t* comms,
                               double angle1,
                               double angle2,
                               double angle3,
                               double angle4)
{
  double angles[4];
  double curAngles[4];
  int i;
  angles[0] = angle1;
  angles[1] = angle2;
  angles[2] = angle3;
  angles[3] = angle4;
  for(i = 0; i < 4; i++) {
    if(iMobotComms_getJointAngle(comms, i, &curAngles[i])) {
      return -1;
    }
  }
  for(i = 0; i < 4; i++) {
    if(angles[i] == 0) {
      continue;
    }
    if(iMobotComms_moveJointTo(comms, i, angles[i] + curAngles[i])) {
      return -1;
    }
  }
  return 0;
}

DLLIMPORT int iMobotComms_moveTo(br_comms_t* comms,
                               double angle1,
                               double angle2,
                               double angle3,
                               double angle4)
{
  if(iMobotComms_moveJointTo(comms, IMOBOT_JOINT1, angle1)) {
    return -1;
  }
  if(iMobotComms_moveJointTo(comms, IMOBOT_JOINT2, angle2)) {
    return -1;
  }
  if(iMobotComms_moveJointTo(comms, IMOBOT_JOINT3, angle3)) {
    return -1;
  }
  if(iMobotComms_moveJointTo(comms, IMOBOT_JOINT4, angle4)) {
    return -1;
  }
  return 0;
}

int iMobotComms_moveJointWait(br_comms_t* comms, int id)
{
  int state;
  while(1)
  {
    if(iMobotComms_getJointState(comms, id, &state)) {
      return -1;
    }
    if(state == 0) {
      return 0;
    } else {
#ifndef _WIN32
      usleep(200000);
#else
      Sleep(200);
#endif
    }
  }
  return 0;
}

int iMobotComms_moveWait(br_comms_t* comms)
{
  int i;
  for(i = 0; i < 4; i++) {
    if(iMobotComms_moveJointWait(comms, i)) {
      return -1;
    }
  }
  return 0;
}

int iMobotComms_stop(br_comms_t* comms)
{
  char buf[160];
  int status;
  int bytes_read;
  status = SendToIMobot(comms, "STOP", 5);
  if(status < 0) return status;
  bytes_read = RecvFromIMobot(comms, buf, sizeof(buf));
  if(!strcmp(buf, "ERROR")) return -1;
  return 0;
}

int iMobotComms_motionRollForward(br_comms_t* comms)
{
  double motorPosition[2];
  iMobotComms_getJointAngle(comms, IMOBOT_JOINT3, &motorPosition[0]);
  iMobotComms_getJointAngle(comms, IMOBOT_JOINT4, &motorPosition[1]);
  iMobotComms_moveJointTo(comms, IMOBOT_JOINT3, motorPosition[0] + 90);
  iMobotComms_moveJointTo(comms, IMOBOT_JOINT4, motorPosition[1] - 90);
  return 0;
}

int iMobotComms_motionRollBackward(br_comms_t* comms)
{
  double motorPosition[2];
  iMobotComms_getJointAngle(comms, IMOBOT_JOINT3, &motorPosition[0]);
  iMobotComms_getJointAngle(comms, IMOBOT_JOINT4, &motorPosition[1]);
  iMobotComms_moveJointTo(comms, IMOBOT_JOINT3, motorPosition[0] - 90);
  iMobotComms_moveJointTo(comms, IMOBOT_JOINT4, motorPosition[1] + 90);
  return 0;
}

int iMobotComms_motionTurnLeft(br_comms_t* comms)
{
  double motorPosition[2];
  iMobotComms_getJointAngle(comms, IMOBOT_JOINT3, &motorPosition[0]);
  iMobotComms_getJointAngle(comms, IMOBOT_JOINT4, &motorPosition[1]);
  iMobotComms_moveJointTo(comms, IMOBOT_JOINT3, motorPosition[0] + 90);
  iMobotComms_moveJointTo(comms, IMOBOT_JOINT4, motorPosition[1] + 90);
  return 0;
}

int iMobotComms_motionTurnRight(br_comms_t* comms)
{
  double motorPosition[2];
  iMobotComms_getJointAngle(comms, IMOBOT_JOINT3, &motorPosition[0]);
  iMobotComms_getJointAngle(comms, IMOBOT_JOINT4, &motorPosition[1]);
  iMobotComms_moveJointTo(comms, IMOBOT_JOINT3, motorPosition[0] - 90);
  iMobotComms_moveJointTo(comms, IMOBOT_JOINT4, motorPosition[1] - 90);
  return 0;
}

int iMobotComms_motionInchwormLeft(br_comms_t* comms)
{
  iMobotComms_moveJointTo(comms, IMOBOT_JOINT1, 0);
  iMobotComms_moveJointTo(comms, IMOBOT_JOINT2, 0);
  iMobotComms_moveWait(comms);

  iMobotComms_moveJointTo(comms, IMOBOT_JOINT2, 50);
  iMobotComms_moveWait(comms);
  iMobotComms_moveJointTo(comms, IMOBOT_JOINT1, -50);
  iMobotComms_moveWait(comms);
  iMobotComms_moveJointTo(comms, IMOBOT_JOINT2, 0);
  iMobotComms_moveWait(comms);
  iMobotComms_moveJointTo(comms, IMOBOT_JOINT1, 0);
  iMobotComms_moveWait(comms);

  return 0;
}

int iMobotComms_motionInchwormRight(br_comms_t* comms)
{
  iMobotComms_moveJointTo(comms, IMOBOT_JOINT1, 0);
  iMobotComms_moveJointTo(comms, IMOBOT_JOINT2, 0);
  iMobotComms_moveWait(comms);

  iMobotComms_moveJointTo(comms, IMOBOT_JOINT1, -50);
  iMobotComms_moveWait(comms);
  iMobotComms_moveJointTo(comms, IMOBOT_JOINT2, 50);
  iMobotComms_moveWait(comms);
  iMobotComms_moveJointTo(comms, IMOBOT_JOINT1, 0);
  iMobotComms_moveWait(comms);
  iMobotComms_moveJointTo(comms, IMOBOT_JOINT2, 0);
  iMobotComms_moveWait(comms);

  return 0;
}

int iMobotComms_motionStand(br_comms_t* comms)
{
  iMobotComms_moveZero(comms);
  iMobotComms_moveWait(comms);
  iMobotComms_moveJointTo(comms, IMOBOT_JOINT1, -85);
  iMobotComms_moveJointTo(comms, IMOBOT_JOINT2, 80);
  iMobotComms_moveWait(comms);
  iMobotComms_moveJointTo(comms, IMOBOT_JOINT3, 45);
  iMobotComms_moveWait(comms);
  iMobotComms_moveJointTo(comms, IMOBOT_JOINT1, 20);
  iMobotComms_moveWait(comms);
  return 0;
}

#ifdef _WIN32
void baswap(bdaddr_t *dst, const bdaddr_t *src)
{
	register unsigned char *d = (unsigned char *) dst;
	register const unsigned char *s = (const unsigned char *) src;
	register int i;

	for (i = 0; i < 6; i++)
		d[i] = s[5-i];
}

int str2ba(const char *str, bdaddr_t *ba)
{
	UINT8 b[6];
	const char *ptr = str;
	int i;

	for (i = 0; i < 6; i++) {
		b[i] = (UINT8) strtol(ptr, NULL, 16);
		if (i != 5 && !(ptr = strchr(ptr, ':')))
			ptr = ":00:00:00:00:00";
		ptr++;
	}

	baswap(ba, (bdaddr_t *) b);

	return 0;
}
#endif

int SendToIMobot(br_comms_t* comms, const char* str, int len)
{
  if(comms->connected == 1) {
#ifdef _WIN32
    return write(comms->socket, str, len);
#else
    return send(comms->socket, str, len, 0);
#endif
  } else if (comms->connected == 2) {
#ifdef _WIN32
    DWORD bytes;
    if(!WriteFile(comms->hSerial, str, len, &bytes, NULL)) {
      return -1;
    }
#else
    return -1;
#endif
  } else {
    return -1;
  }
  return 0;
}

int RecvFromIMobot(br_comms_t* comms, char* buf, int size)
{
#ifdef _WIN32
  DWORD bytes;
#endif
  if(comms->connected == 1) {
#ifdef _WIN32
    return read(comms->socket, buf, size);
#else
    return recvfrom(comms->socket, buf, size, 0, (struct sockaddr*)0, 0);
#endif
  } else if (comms->connected == 2) {
#ifdef _WIN32
    if(!ReadFile(comms->hSerial, buf, size, &bytes, NULL)) {
      return -1;
    }
    return bytes;
#else
    return -1;
#endif
  } else {
    return -1;
  }
}

#ifndef C_ONLY

CiMobotComms::CiMobotComms()
{
  iMobotComms_init(&_comms);
  /* Try to connect */
  connect();
}

CiMobotComms::CiMobotComms(const char address[], int channel)
{
  iMobotComms_init(&_comms);
  /* Try to connect */
  connectAddress(address, channel);
}

CiMobotComms::~CiMobotComms()
{
}


int CiMobotComms::connect()
{
  return iMobotComms_connect(&_comms);
}

int CiMobotComms::connectAddress(const char* address, int channel)
{
  return iMobotComms_connectAddress(&_comms, address, channel);
}

int CiMobotComms::disconnect()
{
  return iMobotComms_disconnect(&_comms);
}

int CiMobotComms::isConnected()
{
  return iMobotComms_isConnected(&_comms);
}

int CiMobotComms::setJointDirection(int id, int dir)
{
  return iMobotComms_setJointDirection(&_comms, id, dir);
}

int CiMobotComms::getJointDirection(int id, int &dir)
{
  return iMobotComms_getJointDirection(&_comms, id, &dir);
}

int CiMobotComms::setJointSpeed(int id, double speed)
{
  return iMobotComms_setJointSpeed(&_comms, id, speed);
}

int CiMobotComms::getJointSpeed(int id, double &speed)
{
  return iMobotComms_getJointSpeed(&_comms, id, &speed);
}

int CiMobotComms::moveJointTo(int id, double angle)
{
  return iMobotComms_moveJointTo(&_comms, id, angle);
}

int CiMobotComms::getJointAngle(int id, double &angle)
{
  return iMobotComms_getJointAngle(&_comms, id, &angle);
}

int CiMobotComms::getJointState(int id, int &state)
{
  return iMobotComms_getJointState(&_comms, id, &state);
}

int CiMobotComms::move( double angle1,
                        double angle2,
                        double angle3,
                        double angle4)
{
  return iMobotComms_move(&_comms, angle1, angle2, angle3, angle4);
}

int CiMobotComms::moveTo( double angle1,
                          double angle2,
                          double angle3,
                          double angle4)
{
  return iMobotComms_moveTo(&_comms, angle1, angle2, angle3, angle4);
}

int CiMobotComms::moveZero()
{
  return iMobotComms_moveZero(&_comms);
}

int CiMobotComms::moveJointWait(int id)
{
  return iMobotComms_moveJointWait(&_comms, id);
}

int CiMobotComms::moveWait()
{
  return iMobotComms_moveWait(&_comms);
}

int CiMobotComms::stop()
{
  return iMobotComms_stop(&_comms);
}

int CiMobotComms::motionRollForward()
{
  return iMobotComms_motionRollForward(&_comms);
}

int CiMobotComms::motionRollBackward()
{
  return iMobotComms_motionRollBackward(&_comms);
}

int CiMobotComms::motionTurnLeft()
{
  return iMobotComms_motionTurnLeft(&_comms);
}

int CiMobotComms::motionTurnRight()
{
  return iMobotComms_motionTurnRight(&_comms);
}

int CiMobotComms::motionInchwormLeft()
{
  return iMobotComms_motionInchwormLeft(&_comms);
}

int CiMobotComms::motionInchwormRight()
{
  return iMobotComms_motionInchwormRight(&_comms);
}

int CiMobotComms::motionStand()
{
  return iMobotComms_motionStand(&_comms);
}

#endif
