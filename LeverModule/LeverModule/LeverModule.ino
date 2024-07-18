#include "LeverModule.h"

constexpr int SlotCount = 6;

const int pinsIn[SlotCount] =
{
	12,11,10,87
};
const int pinsOut[SlotCount] =
{
	6,6,6,6,6,6
};

int pinsAddr[7] =
{
	26,27,28,29,24,25,18
};

constexpr unsigned long FlashFreq = 100;

//! Global variables for this sketch
namespace Glob 
{
	int thisAddress = 1;
	bool indicateLocks = true;
	auto flashPhase = LOW;
	auto timePrev = millis();
	Buffer inputBuffer(1 + SlotCount * 2);
} //namespace Glob

Lever::Lever(int pinSwitch, int pinLED) :
	_pinSwitch(pinSwitch),
	_pinLED(pinLED)
{
	pinMode(pinLED, OUTPUT);
	pinMode(pinSwitch, INPUT_PULLUP);
	_ready = true;
}

// Array of all levers
Lever* levers;

//! Store data from the core
void ReceiveMessage(int size)
{
	ilmod::ReadWireToBuffer(Glob::inputBuffer, size);
}

//! Send lever states when queried
void RequestMessage()
{
	for (int i = 0; i < SlotCount; i++)
	{
		LeverState state = levers[i].GetSlotState();
		Wire.write((byte)state);
	}
}

//! Read data from the input buffer and update lever states
void ProcessBuffer()
{
	// Check that there's data in the buffer
	if (Glob::inputBuffer.position < 1)
		return;

	// Reset buffer for reading
	Glob::inputBuffer.Reset();

	// First byte sets flag to show lock status
	Glob::indicateLocks = Glob::inputBuffer.Read();

	// Subsequent data are 2 bytes per lever
	for (int i = 0; i < SlotCount; i++)
	{
		// First byte - state of lever from locking
		levers[i].SetLockState((LeverState)Glob::inputBuffer.Read());

		// Second byte - whether the lever is locked
		levers[i].SetLocked(Glob::inputBuffer.Read());
	}
	// Return the buffer to 0
	Glob::inputBuffer.Reset();
}

void setup() 
{
	// Read address
	Glob::thisAddress = ilmod::ReadBitAddress(pinsAddr);

	// Initialize levers
	levers = new Lever[SlotCount];
	for (int i = 0; i < SlotCount; i++)
	{
		levers[i] = Lever(pinsIn[i], pinsOut[i]);
	}

	// Init I2C wire
	if (Glob::thisAddress > 0)
	{
		Wire.begin(Glob::thisAddress);
		Wire.onReceive(ReceiveMessage);
		Wire.onRequest(RequestMessage);
	}

  Serial.begin(9600);
  Serial.print("lever mod addr: ");
  Serial.println(Glob::thisAddress);
}

void loop() 
{
	// If all address switches are off, disable this module
	if (Glob::thisAddress == 0)
		return;

	// Update the phase of LED flashing
	auto timeNow = millis();
	auto timeElapsed = timeNow - Glob::timePrev;
	if (timeElapsed > FlashFreq)
	{
		Glob::timePrev = timeNow;
		Glob::flashPhase = Glob::flashPhase == HIGH ? LOW : HIGH;
	}

	// Process input in the buffer
	ProcessBuffer();

	// Update lever status
  //Serial.print("states");
	for (int i = 0; i < SlotCount; i++)
	{
		// Update LED state
		auto ledStatus = LOW;
		if (levers[i].IsFaulted())
			ledStatus = Glob::flashPhase;
		else if (Glob::indicateLocks && levers[i].IsLocked())
			ledStatus = HIGH;

		digitalWrite(levers[i].GetPinOutput(), ledStatus);

		// Update lever state
		if (digitalRead(levers[i].GetPinInput()) == LOW)
			levers[i].SetSlotState(Reversed);
		else
			levers[i].SetSlotState(Normal);

    //Serial.print(" | lever ");
    //Serial.print(i);
    //Serial.print(" = ");
    //Serial.print(levers[i].GetSlotState());
	}
  //Serial.println(";");
}
