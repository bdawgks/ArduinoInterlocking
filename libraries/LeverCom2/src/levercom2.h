/**
* Interlocking library
* Author: Kyle Sarnik
**/

#pragma once

#define STD_LIB
#include <CommonLib.h>
#include <iLock.h> 
#include <ilmsg2.h>

namespace levercom 
{

using lib::Map;
using lib::MapComp;
using lib::byte;
using lib::DeviceId;
using lib::SlotId;
using lib::DeviceSlot;
using lib::Vector;

using ilock::LockingId;
using ilock::LockState;
using LeverState = ilock::Lever::State;

struct LeverInfo
{
	LockingId lid;
	LockState lockState;
	LeverState currentState;
	bool leverLocked;
};

typedef bool (*StateChangedFunc)(LockingId, LeverState);

class LeverComManager
{
	Map<LockingId, DeviceSlot> _slotMap;
	MapComp<DeviceSlot, LeverInfo*, lib::DeviceSlotCompare> _info;
	StateChangedFunc _onStateChanged = nullptr;
	bool _indicateLeverLocks = true;
	Vector<DeviceId> _registeredDevices;

	//! Process SetLeverState message
	void OnSetLeverState(ilmsg::MessageSetLeverState msg);
	//! Send lock state message to specified lever
	void SendLockState(DeviceSlot dSlot);

public:
	//! Start manager
	void Start();
	//! Register a lever device slot
	void RegisterLever(DeviceSlot dSlot, LockingId lid, bool locked = false);
	//! Get lever state
	LeverState GetState(DeviceSlot slot);
	//! Get all addresses
	Vector<DeviceId> GetAddresses() { return _registeredDevices; }
	//! Callback for when lever state attempts to change
	//! Returned bool can return false to deny change (ex: lever locked)
	void OnStateChanged(StateChangedFunc func) { _onStateChanged = func; }
	//! Set lock state
	void SetLeverLockState(LockingId lid, bool locked);
	//! Set whether lock indication is on
	void SetLeverLockIndication(bool on);

	//! Process Register message
	void OnRegister(ilmsg::MessageRegister msg);
	//! Helper callback to process SetLeverState messages
	static void ManagerOnSetLeverState(ilmsg::MessageSetLeverState msg);
};

extern LeverComManager LeverManager;

} // namespace levercom
