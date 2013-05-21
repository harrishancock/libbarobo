
#include "mobot.h"
#include "mobot_internal.h"

#define DEPRECATED(from, to) \
  fprintf(stderr, "Warning: The function \"%s()\" is deprecated. Please use \"%s()\"\n" , from, to)

int CMobot::driveJointToDirect(robotJointId_t id, double angle)
{
  return Mobot_driveJointToDirect(_comms, id, DEG2RAD(angle));
}

int CMobot::driveJointTo(robotJointId_t id, double angle)
{
  return Mobot_driveJointToDirect(_comms, id, DEG2RAD(angle));
}

int CMobot::driveJointToDirectNB(robotJointId_t id, double angle)
{
  return Mobot_driveJointToDirectNB(_comms, id, DEG2RAD(angle));
}

int CMobot::driveJointToNB(robotJointId_t id, double angle)
{
  return Mobot_driveJointToDirectNB(_comms, id, DEG2RAD(angle));
}

int CMobot::driveToDirect( double angle1,
                          double angle2,
                          double angle3,
                          double angle4)
{
  return Mobot_driveToDirect(
      _comms, 
      DEG2RAD(angle1), 
      DEG2RAD(angle2), 
      DEG2RAD(angle3), 
      DEG2RAD(angle4));
}

int CMobot::driveTo( double angle1,
                          double angle2,
                          double angle3,
                          double angle4)
{
  return Mobot_driveToDirect(
      _comms, 
      DEG2RAD(angle1), 
      DEG2RAD(angle2), 
      DEG2RAD(angle3), 
      DEG2RAD(angle4));
}

int CMobot::driveToDirectNB( double angle1,
                          double angle2,
                          double angle3,
                          double angle4)
{
  return Mobot_driveToDirectNB(
      _comms, 
      DEG2RAD(angle1), 
      DEG2RAD(angle2), 
      DEG2RAD(angle3), 
      DEG2RAD(angle4));
}

int CMobot::driveToNB( double angle1,
                          double angle2,
                          double angle3,
                          double angle4)
{
  return Mobot_driveToDirectNB(
      _comms, 
      DEG2RAD(angle1), 
      DEG2RAD(angle2), 
      DEG2RAD(angle3), 
      DEG2RAD(angle4));
}

int CMobot::isMoving()
{
  return Mobot_isMoving(_comms);
}

int CMobot::move( double angle1,
                        double angle2,
                        double angle3,
                        double angle4)
{
  return Mobot_move(
      _comms, 
      DEG2RAD(angle1), 
      DEG2RAD(angle2), 
      DEG2RAD(angle3), 
      DEG2RAD(angle4));
}

int CMobot::moveNB( double angle1,
                        double angle2,
                        double angle3,
                        double angle4)
{
  return Mobot_moveNB(
      _comms, 
      DEG2RAD(angle1), 
      DEG2RAD(angle2), 
      DEG2RAD(angle3), 
      DEG2RAD(angle4));
}

int CMobot::moveBackward(double angle)
{
  return Mobot_moveBackward(_comms, DEG2RAD(angle));
}

int CMobot::moveBackwardNB(double angle)
{
  return Mobot_moveBackwardNB(_comms, DEG2RAD(angle));
}

int CMobot::moveContinuousNB( mobotJointState_t dir1, mobotJointState_t dir2, mobotJointState_t dir3, mobotJointState_t dir4)
{
  DEPRECATED("moveContinuousNB", "setMovementStateNB");
  return Mobot_moveContinuousNB(_comms, dir1, dir2, dir3, dir4);
}

int CMobot::moveContinuousTime( mobotJointState_t dir1, mobotJointState_t dir2, mobotJointState_t dir3, mobotJointState_t dir4, double seconds)
{
  DEPRECATED("moveContinuousTime", "setMovementStateTime");
  return Mobot_moveContinuousTime(_comms, dir1, dir2, dir3, dir4, seconds);
}

int CMobot::moveDistance(double distance, double radius)
{
  return Mobot_moveDistance(_comms, distance, radius);
}

int CMobot::moveDistanceNB(double distance, double radius)
{
  return Mobot_moveDistanceNB(_comms, distance, radius);
}

int CMobot::moveForward(double angle)
{
  return Mobot_moveForward(_comms, DEG2RAD(angle));
}

int CMobot::moveForwardNB(double angle)
{
  return Mobot_moveForwardNB(_comms, DEG2RAD(angle));
}

int CMobot::moveJointContinuousNB(robotJointId_t id, mobotJointState_t dir)
{
  DEPRECATED("moveJointContinuousNB", "setJointMovementStateNB");
  return Mobot_moveJointContinuousNB(_comms, id, dir);
}

int CMobot::moveJointContinuousTime(robotJointId_t id, mobotJointState_t dir, double seconds)
{
  DEPRECATED("moveJointContinuousTime", "setJointMovementStateTime");
  return Mobot_moveJointContinuousTime(_comms, id, dir, seconds);
}

int CMobot::moveJoint(robotJointId_t id, double angle)
{
  return Mobot_moveJoint(_comms, id, DEG2RAD(angle));
}

int CMobot::moveJointNB(robotJointId_t id, double angle)
{
  return Mobot_moveJointNB(_comms, id, DEG2RAD(angle));
}

int CMobot::moveJointTo(robotJointId_t id, double angle)
{
  return Mobot_moveJointTo(_comms, id, DEG2RAD(angle));
}

int CMobot::moveJointToDirect(robotJointId_t id, double angle)
{
  return Mobot_moveJointToDirect(_comms, id, DEG2RAD(angle));
}

int CMobot::moveJointToNB(robotJointId_t id, double angle)
{
  return Mobot_moveJointToNB(_comms, id, DEG2RAD(angle));
}

int CMobot::moveJointToDirectNB(robotJointId_t id, double angle)
{
  return Mobot_moveJointToDirectNB(_comms, id, DEG2RAD(angle));
}

int CMobot::moveJointWait(robotJointId_t id)
{
  return Mobot_moveJointWait(_comms, id);
}

int CMobot::moveTo( double angle1,
                          double angle2,
                          double angle3,
                          double angle4)
{
  return Mobot_moveTo(
      _comms, 
      DEG2RAD(angle1), 
      DEG2RAD(angle2), 
      DEG2RAD(angle3), 
      DEG2RAD(angle4));
}

int CMobot::moveToDirect( double angle1,
                          double angle2,
                          double angle3,
                          double angle4)
{
  return Mobot_moveToDirect(
      _comms, 
      DEG2RAD(angle1), 
      DEG2RAD(angle2), 
      DEG2RAD(angle3), 
      DEG2RAD(angle4));
}

int CMobot::moveToNB( double angle1,
                          double angle2,
                          double angle3,
                          double angle4)
{
  return Mobot_moveToNB(
      _comms, 
      DEG2RAD(angle1), 
      DEG2RAD(angle2), 
      DEG2RAD(angle3), 
      DEG2RAD(angle4));
}

int CMobot::moveToDirectNB( double angle1,
                          double angle2,
                          double angle3,
                          double angle4)
{
  return Mobot_moveToDirectNB(
      _comms, 
      DEG2RAD(angle1), 
      DEG2RAD(angle2), 
      DEG2RAD(angle3), 
      DEG2RAD(angle4));
}

int CMobot::moveWait()
{
  return Mobot_moveWait(_comms);
}

int CMobot::moveToZero()
{
  return Mobot_moveToZero(_comms);
}

int CMobot::moveToZeroNB()
{
  return Mobot_moveToZeroNB(_comms);
}

int CMobot::stop()
{
  return Mobot_stop(_comms);
}

int CMobot::stopOneJoint(robotJointId_t id)
{
  return Mobot_stopOneJoint(_comms, id);
}

int CMobot::stopTwoJoints(robotJointId_t id1, robotJointId_t id2)
{
  return Mobot_stopTwoJoints(_comms, id1, id2);
}

int CMobot::stopThreeJoints(robotJointId_t id1, robotJointId_t id2, robotJointId_t id3)
{
  return Mobot_stopThreeJoints(_comms, id1, id2, id3);
}

int CMobot::stopAllJoints()
{
  return Mobot_stopAllJoints(_comms);
}

int CMobot::turnLeft(double angle)
{
  return Mobot_turnLeft(_comms, DEG2RAD(angle));
}

int CMobot::turnLeftNB(double angle)
{
  return Mobot_turnLeftNB(_comms, DEG2RAD(angle));
}

int CMobot::turnRight(double angle)
{
  return Mobot_turnRight(_comms, DEG2RAD(angle));
}

int CMobot::turnRightNB(double angle)
{
  return Mobot_turnRightNB(_comms, DEG2RAD(angle));
}
