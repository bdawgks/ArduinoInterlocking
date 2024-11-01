#include "LeverModule.h"
#include "HardwareProfile.h"

constexpr int SlotCount = 6;

int pinsIn[SlotCount] =
{
	13,12,11,10,9,8
};
int pinsOut[SlotCount] =
{
	7,6,5,4,3,2
};
int pinsAddr[7] =
{
	17,18,19,20,21,22,23
};

constexpr unsigned long FlashFreq = 100;
constexpr unsigned long SwitchMinChangeTime = 10;

#define REVERSED_STATE  HIGH

// Hardware setup
hwprofile::ProfileData hwdata = hwprofile::GetProfile(hwprofile::BoardType::ArduinoESP32);

//! Global variables for this sketch
namespace Glob 
{
	int thisAddress = 1;
	bool indicateLocks = true;
	auto flashPhase = LOW;
	auto timePrev = millis();
	bool processorStarted = false;
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
	if (millis() - _lastChangeTime < SwitchMinChangeTime)
		return;

	_lastChangeTime = millis();

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

		if (true) // TODO: find out how to check if MessageCom logging is enabled
		{
		  String logStr = "Lever " + String(_slot) + " state updated to " + String(state); 
		  Log.Message(MessageCom, logStr);
		}
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

	if (true) // TODO: find out how to check if MessageCom logging is enabled
	{
		String lockedStr = msg.locked ? "locked" : "unlocked";
		String stateStr = (LeverState)msg.state == LeverState::Reversed ? "reversed" : "normal";
		String logStr = "Lever " + String(slot) + " lock state updated to " + stateStr + ", " + lockedStr;
		Log.Message(MessageCom, logStr);
	}
}

//! Process a SetLockIndication message
void OnSetLockIndication(ilmsg::MessageSetLockIndication msg)
{
	Glob::indicateLocks = msg.showIndication;
}

void setup() 
{
	hwprofile::AssignPinData(hwdata, pinsAddr, pinsIn, pinsOut);

	Log[All] = true;

	// Set up serial logging
	if (Log.Enabled())
	{
		Serial.begin(9600);
		while(!Serial);
		Log.Message(General, "Lever Module started");
	}

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
	Glob::processorStarted = ilmsg::Processor.Start(hwdata.canTxPin, hwdata.canRxPin, hwdata.canClockSpeed);
	if(!Glob::processorStarted)
	{
		Log.Message(MessageCom, "CAN did not initialize");
	}

	if (Glob::thisAddress < 1)
		Log.Message(General, "Module address is zero, this module will remain inactive.");
	else
		Log.Message(General, "Module Address: " + String(Glob::thisAddress));
}

void loop() 
{
	// If all address switches are off, disable this module
	if (Glob::thisAddress == 0)
	{
		TestLoop();
		return;
	}

	// Update the phase of LED flashing
	auto timeNow = millis();
	auto timeElapsed = timeNow - Glob::timePrev;
	if (timeElapsed > FlashFreq)
	{
		Glob::timePrev = timeNow;
		Glob::flashPhase = Glob::flashPhase == HIGH ? LOW : HIGH;
	}

	// Process incomming messages
	if(Glob::processorStarted)
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
		if (digitalRead(levers[i].GetPinInput()) == REVERSED_STATE)
			levers[i].SetSlotState(Reversed);
		else
			levers[i].SetSlotState(Normal);
	}
}

void TestLoop()
{
	for (int i = 0; i < SlotCount; i++)
	{
		digitalWrite(pinsOut[i], HIGH);
		if (digitalRead(pinsIn[i]) != REVERSED_STATE)
		{
			delay(100);
			digitalWrite(pinsOut[i], LOW);
		}
	}
	delay(500);
}
