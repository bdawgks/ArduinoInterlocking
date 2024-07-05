#include <ArxContainer.h>
#include <CommonLib.h>
#include <Wire.h>
#include <iLock.h>

const int addr = 1;

using LeverState = ilock::Lever::State;
int nextSlotToSend = 0;

constexpr unsigned int SlotCount = 6;

LeverState leverStates[SlotCount] =
{
  LeverState::Normal,
  LeverState::Normal,
  LeverState::Normal,
  LeverState::Normal,
  LeverState::Normal,
  LeverState::Normal
};

bool lockStates[SlotCount] =
{
  false,
  false,
  false,
  false,
  false,
  false
};

void ReceiveMessage(int size)
{
}

void RequestMessage()
{
  for (int i = 0; i < SlotCount; i++)
  {
    Wire.write((byte)leverStates[i]);
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

  Wire.begin(addr);

  Wire.onReceive(ReceiveMessage);
  Wire.onRequest(RequestMessage);
}

void loop() {
  if (Serial.available() > 0)
  {
    //Serial.println(F("reading data..."));
    String input = Serial.readString();

    input.trim();
    String cmd = input.substring(0, input.indexOf(':'));
    String args = input.substring(input.indexOf(':') + 1);
    
    //Serial.print(F("read command: "));
    //Serial.print(cmd);
    //Serial.print(F(", args: "));
    //Serial.println(args);

    if (cmd == "ThrowLever")
    {
      int slot = args.toInt();
      if (slot < 0 || slot > 6)
        return;

      if (lockStates[slot])
      {
        Serial.print(F("lever locked: "));
        Serial.println(slot);
        return;
      }

      if (leverStates[slot] == LeverState::Normal)
        leverStates[slot] = LeverState::Reversed;
      else
        leverStates[slot] = LeverState::Normal;

      Serial.print(F("lever thrown: "));
      Serial.println(slot);
    }
    if (cmd == "GetState")
    {
      int slot = args.toInt();
      if (slot < 0 || slot > 6)
        return;

      Serial.print("slot ");
      Serial.print(slot);
      Serial.print(" state: ");
      Serial.print((int)leverStates[slot]);
      Serial.print(" locked: ");
      Serial.println(lockStates[slot]);
    }
  }
}
