// Custom libraries
#include "CommonLib.h"
#include "ilmsg2.h"
#include "ilmod.h"

using lib::byte;
using lib::Buffer;

enum LeverState : byte
{
	Normal,
	Reversed
};

class Lever
{
	int _slot;
	LeverState _slotState;
	LeverState _lockState;
	bool _locked;
	int _pinSwitch;
	int _pinLED;
	bool _ready = false;

public:
	Lever() {};
	Lever(int slot, int pinSwitch, int pinLED);

	LeverState GetSlotState() { return _slotState; }
	LeverState GetLockState() { return _lockState; }

	void SetSlotState(LeverState state);
	void SetLockState(LeverState state) { _lockState = state; }

	bool IsFaulted() { return _slotState != _lockState; }
	bool IsLocked() { return _locked; }

	int GetPinInput() { return _pinSwitch; }
	int GetPinOutput() { return _pinLED; }

	void SetLocked(bool locked) { _locked = locked; }
};