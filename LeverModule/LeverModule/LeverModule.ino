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

constexpr int PinTX = 43;
constexpr int PinRX = 44;

constexpr unsigned long FlashFreq = 100;

//! Global variables for this sketch
namespace Glob 
{
	int thisAddress = 1;
	bool indicateLocks = true;
	auto flashPhase = LOW;
	auto timePrev = millis();
} //namespace Glob

// Lever member implementations
Lever::Lever(int slot, int pinSwitch, int pinLED) :
	_slot(slot),
	_pinSwitch(pinSwitch),
	_pinLED(pinLED)
{
	pinMode(pinLED, OUTPUT);
	pinMode(pinSwitch, INPUT_PULLUP);
	_ready = true;
}

void Lever::SetSlotState(LeverState state)
{
	if (state != _slotState)
	{
		_slotState = state;

		// Send message to core
		ilmsg::MessageSetLeverState msg = {};
		msg.did = Glob::thisAddress;
		msg.slot = _slot;
		msg.state = (ilock::Lever::State)_slotState;
		msg.faulted = IsFaulted();
		ilmsg::Processor.SendMessage(msg);
	}
}

// Array of all levers
Lever* levers;

//! Process a SetLockState message
void OnSetLockState(ilmsg::MessageSetLockState msg)
{
	int slot = msg.slot;
	if (slot >= SlotCount || slot < 0)
		return;

	// Update the state of the locking
	levers[slot].SetLockState((LeverState)msg.state);
	levers[slot].SetLocked(msg.locked);
}

//! Process a SetLockIndication message
void OnSetLockIndication(ilmsg::MessageSetLockIndication msg)
{
	Glob::indicateLocks = msg.showIndication;
}

void setup() 
{
	// Read address
	Glob::thisAddress = ilmod::ReadBitAddress(pinsAddr);

	// Initialize levers
	levers = new Lever[SlotCount];
	for (int i = 0; i < SlotCount; i++)
	{
		levers[i] = Lever(i, pinsIn[i], pinsOut[i]);
	}

	// Register with Message Processor
	ilmsg::Processor.RegisterDevice(ilmsg::ModuleType::Lever, Glob::thisAddress);

	// Set up event callbacks
	ilmsg::Processor.OnMessage(ilmsg::MessageType::SetLockState, new ilmsg::MessageProcessFunc<ilmsg::MessageSetLockState>(OnSetLockState));
	ilmsg::Processor.OnMessage(ilmsg::MessageType::SetLockIndication, new ilmsg::MessageProcessFunc<ilmsg::MessageSetLockIndication>(OnSetLockIndication));

	// Start Message Processor
	ilmsg::Processor.Start(PinTX, PinRX);
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

	// Process incomming messages
	ilmsg::Processor.ProcessReceived();

	// Update lever status
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
	}
}
