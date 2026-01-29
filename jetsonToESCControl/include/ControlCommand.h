#pragma once

#include "Arduino.h"

struct ControlCommand{
  int controlType;
  int commandType;
  int value;
};
