/**
* Interlocking library
* Author: Kyle Sarnik
**/

#pragma once

#define STD_LIB
#include <CommonLib.h>
#include <iLock.h> 
#include <Wire.h>

namespace levercom {

using lib::Map;
using lib::MapComp;
using lib::String;
using lib::Buffer;
using lib::byte;
using lib::DeviceId;
using lib::SlotId;
using lib::DeviceSlot;
using lib::Vector;

using ilock::LockingId;
using LeverState = ilock::Lever::State;

constexpr auto LeverStateDataSize = sizeof(LeverState);

//! Stores data buffer of all lever states on a single device
struct LeverComBuffer
{
	Buffer buffer;
	unsigned int slotCount = 0;
	bool pushNeeded = true;

	LeverComBuffer() : buffer(0) {}

	//! Get size
	int GetSize() { return buffer.length; }

	//! Resize
	void Resize() { buffer = Buffer(slotCount * LeverStateDataSize); }
};

struct LeverInfo
{
	LockingId lid;
	LeverState currentState;
	bool leverLocked;
};

typedef bool (*StateChangedFunc)(LockingId, LeverState);

class LeverComManager
{
	Map<LockingId, DeviceSlot> _slotMap;
	MapComp<DeviceSlot, LeverInfo*, lib::DeviceSlotCompare> _info;
	Map<DeviceId, LeverComBuffer*> _buffers;
	StateChangedFunc _onStateChanged = nullptr;

	//! Get expected number of bytes to read for a specific device
	int GetResponseSize(DeviceId address);
	//! Resets the buffer to begin reading or parsing
	void ResetBuffer(DeviceId address);
	//! Write response byte to buffer
	void ReadByte(DeviceId address, byte b);
	//! Process buffer of address
	void ReadStates(DeviceId address, LeverComBuffer* buffer);

public:
	//! Register a lever device slot
	void RegisterLever(DeviceSlot dSlot, LockingId lid, bool locked = false);
	//! Create appropriate buffers for each device; should be called only after all levers registered
	void InitBuffers();
	//! Process buffers, will update current state of all levers
	void ProcessBuffers();
	//! Get lever state
	LeverState GetState(DeviceSlot slot);
	//! Request lever state over wire
	void RequestLeverState(DeviceId address);
	//! Get all addresses
	Vector<DeviceId> GetAddresses();
	//! Callback for when lever state attempts to change
	//! Returned bool can return false to deny change (ex: lever locked)
	void OnStateChanged(StateChangedFunc func) { _onStateChanged = func; }
	//! Set lock state
	void SetLeverLockState(LockingId lid, bool locked);
	//! Push lock state to all devices that require it
	void PushLeverStates();
};

} // namespace levercom
