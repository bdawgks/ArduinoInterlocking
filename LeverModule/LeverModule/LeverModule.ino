#include "LeverModule.h"

constexpr int SlotCount = 6;

const int pinsIn[SlotCount] =
{
	2,3,4,5,6,7
};
const int pinsOut[SlotCount] =
{
	8,9,10,11,12,13
};

constexpr unsigned long FlashFreq = 100;

constexpr int thisAddress = 1;
bool showLocked = true;
auto flashPhase = LOW;
auto timePrev = millis();
Buffer inputBuffer(1 + SlotCount * 2);

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
	// Grow the buffer if it's too small
	if (size > inputBuffer.length)
	{
		inputBuffer.length = size;
	}
	inputBuffer.Reset();
	while (Wire.available() > 0)
	{
		inputBuffer.Write(Wire.read());
	}
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
	if (inputBuffer.position < 1)
		return;

	// Reset buffer for reading
	inputBuffer.Reset();

	// First byte sets flag to show lock status
	showLocked = inputBuffer.Read();

	// Subsequent data are 2 bytes per lever
	for (int i = 0; i < SlotCount; i++)
	{
		// First byte - state of lever from locking
		levers[i].SetLockState((LeverState)inputBuffer.Read());

		// Second byte - whether the lever is locked
		levers[i].SetLocked(inputBuffer.Read());
	}
	// Return the buffer to 0
	inputBuffer.Reset();
}

void setup() 
{
	// Initialize levers
	levers = new Lever[SlotCount];
	for (int i = 0; i < SlotCount; i++)
	{
		levers[i] = Lever(pinsIn[i], pinsOut[i]);
	}

	// Init I2C wire
	Wire.begin(thisAddress);
	Wire.onReceive(ReceiveMessage);
	Wire.onRequest(RequestMessage);
}

void loop() 
{
	// Update the phase of LED flashing
	auto timeNow = millis();
	auto timeElapsed = timeNow - timePrev;
	if (timeElapsed > FlashFreq)
	{
		timePrev = timeNow;
		flashPhase = flashPhase == HIGH ? LOW : HIGH;
	}

	// Process input in the buffer
	ProcessBuffer();

	// Update lever status
	for (int i = 0; i < SlotCount; i++)
	{
		// Update LED state
		auto ledStatus = LOW;
		if (levers[i].IsFaulted())
			ledStatus = flashPhase;
		else if (showLocked && levers[i].IsLocked())
			ledStatus = HIGH;

		digitalWrite(levers[i].GetPinOutput(), ledStatus);

		// Update lever state
		if (digitalRead(levers[i].GetPinInput()) == HIGH)
			levers[i].SetSlotState(Reversed);
		else
			levers[i].SetSlotState(Normal);
	}
}
