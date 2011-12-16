#include <stdlib.h>
#include "GtkiMobotController.h"

void on_button_forward_clicked(GtkWidget* widget, gpointer data)
{
  g_buttonState[B_FORWARD].clicked = 1;
}

void buttonForward()
{
  setMotorDirection(1, 2);
  setMotorDirection(4, 1);
  double speeds[2];
  speeds[0] = gtk_range_get_value(GTK_RANGE(scale_motorSpeeds[0]));
  speeds[1] = gtk_range_get_value(GTK_RANGE(scale_motorSpeeds[3]));
  setMotorSpeed(1, speeds[0]);
  setMotorSpeed(4, speeds[1]);
}

void on_button_backward_clicked(GtkWidget* widget, gpointer data)
{
  g_buttonState[B_BACKWARD].clicked = 1;
}

void buttonBackward()
{
  setMotorDirection(1, 1);
  setMotorDirection(4, 2);
  double speeds[2];
  speeds[0] = gtk_range_get_value(GTK_RANGE(scale_motorSpeeds[0]));
  speeds[1] = gtk_range_get_value(GTK_RANGE(scale_motorSpeeds[3]));
  setMotorSpeed(1, speeds[0]);
  setMotorSpeed(4, speeds[1]);
}

void on_button_rotateLeft_clicked(GtkWidget* widget, gpointer data)
{
  g_buttonState[B_ROTATELEFT].clicked = 1;
}

void buttonRotateLeft()
{
  setMotorDirection(1, 2);
  setMotorDirection(4, 2);
  double speeds[2];
  speeds[0] = gtk_range_get_value(GTK_RANGE(scale_motorSpeeds[0]));
  speeds[1] = gtk_range_get_value(GTK_RANGE(scale_motorSpeeds[3]));
  setMotorSpeed(1, speeds[0]);
  setMotorSpeed(4, speeds[1]);
}

void on_button_rotateRight_clicked(GtkWidget* widget, gpointer data)
{
  g_buttonState[B_ROTATERIGHT].clicked = 1;
}

void buttonRotateRight()
{
  setMotorDirection(1, 1);
  setMotorDirection(4, 1);
  double speeds[2];
  speeds[0] = gtk_range_get_value(GTK_RANGE(scale_motorSpeeds[0]));
  speeds[1] = gtk_range_get_value(GTK_RANGE(scale_motorSpeeds[3]));
  setMotorSpeed(1, speeds[0]);
  setMotorSpeed(4, speeds[1]);
}

void on_button_lfaceForward_clicked(GtkWidget* widget, gpointer data)
{
  g_buttonState[B_LFACEFORWARD].clicked = 1;
}

void buttonLFaceForward()
{
  setMotorDirection(1, 1);
  double speed;
  speed = gtk_range_get_value(GTK_RANGE(scale_motorSpeeds[0]));
  setMotorSpeed(1, speed);
}

void on_button_lfaceBackward_clicked(GtkWidget* widget, gpointer data)
{
  g_buttonState[B_LFACEBACKWARD].clicked = 1;
}

void buttonLFaceBackward()
{
  setMotorDirection(1, 2);
  double speed;
  speed = gtk_range_get_value(GTK_RANGE(scale_motorSpeeds[0]));
  setMotorSpeed(1, speed);
}

void on_button_rfaceForward_clicked(GtkWidget* widget, gpointer data)
{
  g_buttonState[B_RFACEFORWARD].clicked = 1;
}

void buttonRFaceForward()
{
  setMotorDirection(4, 2);
  double speed;
  speed = gtk_range_get_value(GTK_RANGE(scale_motorSpeeds[3]));
  setMotorSpeed(4, speed);
}

void on_button_rfaceBackward_clicked(GtkWidget* widget, gpointer data)
{
  g_buttonState[B_RFACEBACKWARD].clicked = 1;
}

void buttonRFaceBackward()
{
  setMotorDirection(4, 1);
  double speed;
  speed = gtk_range_get_value(GTK_RANGE(scale_motorSpeeds[3]));
  setMotorSpeed(4, speed);
}

void on_button_inchLeft_clicked(GtkWidget* widget, gpointer data)
{
  executeGait(findGait("Inch Left"));
}

void on_button_inchRight_clicked(GtkWidget* widget, gpointer data)
{
  executeGait(findGait("Inch Right"));
}

void on_button_stop_clicked(GtkWidget* widget, gpointer data)
{
  g_buttonState[B_STOP].clicked = 1;
}

void buttonStop()
{
  stop();
  int i;

  for(i = 1; i < 5; i++) {
    /* Make all motor direction modes back to auto */
    setMotorDirection(i, 0);
  }
}

void on_button_home_clicked(GtkWidget* widget, gpointer data) 
{
  g_buttonState[B_HOME].clicked = 1;
}

void buttonHome()
{
  int i;
  GtkRange* range;
  double speed;
  for(i = 1; i < 5; i++) {
    range = GTK_RANGE(scale_motorSpeeds[i]);
    speed = gtk_range_get_value(range);
    if(speed < 30) {
      speed = 30;
    }
    setMotorSpeed(i, speed);
    setMotorDirection(i, 0);
    setMotorPosition(i, 0);
  }
}

void on_button_playGait_clicked(GtkWidget* widget, gpointer data) 
{
  /* Need to get the selected gait */
  GtkTreeView *treeView;
  GtkTreeSelection *treeSelection;
  GtkTreeModel *model;
  GtkTreeIter iter;
  char* name;

  treeView = GTK_TREE_VIEW(
      gtk_builder_get_object(builder, "treeview_gaits"));
  model = GTK_TREE_MODEL(
      gtk_builder_get_object(builder, "liststore_gaits"));

  treeSelection = gtk_tree_view_get_selection( treeView );
  gtk_tree_selection_set_mode(treeSelection, GTK_SELECTION_BROWSE);
  bool success = 
    gtk_tree_selection_get_selected( treeSelection, &model, &iter );
  if(!success) {
    return;
  }
  gtk_tree_model_get(model, &iter,
      0, &name,
      -1);
  executeGait(findGait(name));
  free(name);
}

void on_button_moveJoints_clicked(GtkWidget* widget, gpointer data)
{
  g_buttonState[B_MOVEJOINTS].clicked = 1;
}

void buttonMoveJoints()
{
  GtkEntry* entries[4];
  double val;
  const char* string;
  entries[0] = GTK_ENTRY( gtk_builder_get_object(builder, "entry_motorPos0"));
  entries[1] = GTK_ENTRY( gtk_builder_get_object(builder, "entry_motorPos1"));
  entries[2] = GTK_ENTRY( gtk_builder_get_object(builder, "entry_motorPos2"));
  entries[3] = GTK_ENTRY( gtk_builder_get_object(builder, "entry_motorPos3"));
  for(int i = 0; i < 4; i++) {
    string = gtk_entry_get_text(entries[i]);
    if(strlen(string) == 0) {
      continue;
    }
    sscanf(string, "%lf", &val);
    setMotorPosition(i+1, val);
  }
}

void buttonMotorforward(int id)
{
  g_buttonState[B_M1F + id - 1].clicked = 1;
}

void motor_forward(int id)
{
  double speed;
  speed = gtk_range_get_value(GTK_RANGE(scale_motorSpeeds[id-1]));
  setMotorSpeed(id, speed);
  setMotorDirection(id, 1);
}

void buttonMotorstop(int id)
{
  g_buttonState[B_M1S + id - 1].clicked = 1;
}

void motor_stop(int id)
{
  setMotorSpeed(id, 0);
  setMotorDirection(id, 0);
}

void buttonMotorback(int id)
{
  g_buttonState[B_M1B + id - 1].clicked = 1;
}

void motor_back(int id)
{
  double speed;
  speed = gtk_range_get_value(GTK_RANGE(scale_motorSpeeds[id-1]));
  setMotorSpeed(id, speed);
  setMotorDirection(id, 2);
}

#define ON_MOTOR_BUTTON_CLICKED(id, motion) \
void on_button_motor##id##motion##_clicked(GtkWidget* widget, gpointer data) \
{ \
  buttonMotor##motion (id+1); \
}
ON_MOTOR_BUTTON_CLICKED(0, forward)
ON_MOTOR_BUTTON_CLICKED(1, forward)
ON_MOTOR_BUTTON_CLICKED(2, forward)
ON_MOTOR_BUTTON_CLICKED(3, forward)
ON_MOTOR_BUTTON_CLICKED(0, stop)
ON_MOTOR_BUTTON_CLICKED(1, stop)
ON_MOTOR_BUTTON_CLICKED(2, stop)
ON_MOTOR_BUTTON_CLICKED(3, stop)
ON_MOTOR_BUTTON_CLICKED(0, back)
ON_MOTOR_BUTTON_CLICKED(1, back)
ON_MOTOR_BUTTON_CLICKED(2, back)
ON_MOTOR_BUTTON_CLICKED(3, back)
#undef ON_MOTOR_BUTTON_CLICKED


gboolean on_vscale_motorspeed0_button_release_event(GtkRange* range, GdkEvent* event, gpointer data)
{
  double speed = gtk_range_get_value(range);
  g_buttonState[B_SPEED1].clicked = 1;
  g_buttonState[B_SPEED1].dargs[0] = speed;
  return FALSE;
}

void buttonSpeed1(double speed)
{
  setMotorSpeed(1, speed);
}

gboolean on_vscale_motorspeed1_button_release_event(GtkRange* range, GdkEvent* event, gpointer data)
{
  double speed = gtk_range_get_value(range);
  g_buttonState[B_SPEED2].clicked = 1;
  g_buttonState[B_SPEED2].dargs[0] = speed;
  return FALSE;
}

void buttonSpeed2(double speed)
{
  setMotorSpeed(2, speed);
}

gboolean on_vscale_motorspeed2_button_release_event(GtkRange* range, GdkEvent* event, gpointer data)
{
  double speed = gtk_range_get_value(range);
  g_buttonState[B_SPEED3].clicked = 1;
  g_buttonState[B_SPEED3].dargs[0] = speed;
  return FALSE;
}

void buttonSpeed3(double speed)
{
  setMotorSpeed(3, speed);
}

gboolean on_vscale_motorspeed3_button_release_event(GtkRange* range, GdkEvent* event, gpointer data)
{
  double speed = gtk_range_get_value(range);
  g_buttonState[B_SPEED4].clicked = 1;
  g_buttonState[B_SPEED4].dargs[0] = speed;
  return FALSE;
}

void buttonSpeed4(double speed)
{
  setMotorSpeed(4, speed);
}

gboolean on_vscale_motorPos_button_press_event(GtkRange* range, GdkEvent* event, gpointer data)
{
  int i;
  /* Figure out which scale has been pressed */
  for(i = 0; i < 4; i++) {
    if(range == GTK_RANGE(scale_motorPositions[i])) {
      break;
    }
  }
  if(i == 4) {
    return FALSE;
  }
  
  motor_position_scale_pressed[i] = 1;
  return FALSE;
}

gboolean on_vscale_motorPos_button_release_event(GtkRange* range, GdkEvent* event, gpointer data)
{
  int i;
  /* Figure out which scale has been pressed */
  for(i = 0; i < 4; i++) {
    if(range == GTK_RANGE(scale_motorPositions[i])) {
      break;
    }
  }
  if(i == 4) {
    return FALSE;
  }
  
  motor_position_scale_pressed[i] = 0;
  return FALSE;
}


