/**
* Interlocking library
* Author: Kyle Sarnik
**/

#include "LeverCom2.h"

namespace levercom
{

void LeverComManager::ManagerOnSetLeverState(ilmsg::MessageSetLeverState msg)
{
	LeverManager.OnSetLeverState(msg);
}

void LeverComManager::RegisterLever(DeviceSlot dSlot, LockingId lid, bool locked)
{
	LeverInfo* info = new LeverInfo();
	info->lid = lid;
	info->leverLocked = locked;
	_info.insert(std::make_pair(dSlot, info));
	_slotMap.insert(std::make_pair(lid, dSlot));
}

void LeverComManager::SendLockState(DeviceSlot dSlot)
{
	if (_info.find(dSlot) == _info.end())
		return;

	ilmsg::MessageSetLockState msg = {};
	msg.slot = dSlot.slot;
	msg.state = _info[dSlot]->currentState;
	msg.locked = _info[dSlot]->leverLocked;
	msg.SetDestination(dSlot.address);

	//Serial.println("message composed for " + String(dSlot.address) + " | slot " + String(dSlot.slot) + " | locked " + String(msg.locked));

	ilmsg::Processor.SendMessage(msg);
}

bool LeverComManager::OnRegister(ilmsg::MessageRegister msg)
{
	if (std::find(_registeredDevices.begin(), _registeredDevices.end(), msg.did) != _registeredDevices.end())
		return false;

	_registeredDevices.push_back(msg.did);

	// Send lever state to newly registered module
	if (msg.mtype == ilmsg::ModuleType::Lever)
	{
		for (auto it = _info.begin(); it != _info.end(); it++)
		{
			if (it->first.address == msg.did)
			{
				Serial.println("Sending states to did " + String(msg.did) + " slot " + String(it->first.slot));
				SendLockState(it->first);
			}
		}
		return true;
	}

	return false;
}

void LeverComManager::OnSetLeverState(ilmsg::MessageSetLeverState msg)
{
	DeviceSlot dSlot = { msg.did, msg.slot };

	Serial.println("lever state received and slot found");

	if (_info.find(dSlot) == _info.end())
	{
		Serial.println("slot not found, addr = " + String(dSlot.address) + " slot = " + String(dSlot.slot));
		return;
	}

	auto curState = _info[dSlot]->currentState;

	Serial.println("slot found");

	if (msg.state != _info[dSlot]->currentState)
	{
		bool allowChange = true;
		if (_onStateChanged)
			allowChange = _onStateChanged(_info[dSlot]->lid, msg.state);

		if (allowChange)
		{
			_info[dSlot]->currentState = msg.state;
			SendLockState(dSlot);
		}
	}
}

void LeverComManager::Start()
{
	ilmsg::Processor.OnMessage(ilmsg::MessageType::SetLeverState, new ilmsg::MessageProcessFunc<ilmsg::MessageSetLeverState>(ManagerOnSetLeverState));
}

LeverState LeverComManager::GetState(DeviceSlot slot)
{
	if (_info.find(slot) == _info.end())
		return LeverState::Normal;

	return _info[slot]->currentState;
}

void LeverComManager::SetLeverLockState(LockingId lid, bool locked, bool forceUpdate)
{
	if (_slotMap.find(lid) == _slotMap.end())
		return;

	DeviceSlot dSlot = _slotMap[lid];
	bool curLocked = _info[dSlot]->leverLocked;
	_info[dSlot]->leverLocked = locked;
	if (curLocked != locked || forceUpdate)
	{
		SendLockState(dSlot);
	}
}

void LeverComManager::SetLeverLockIndication(bool on)
{
	if (on != _indicateLeverLocks)
	{
		// Send message to all levers
		ilmsg::MessageSetLockIndication msg = {};
		msg.showIndication = on;
		ilmsg::Processor.SendMessage(msg);
	}
	_indicateLeverLocks = on;
}

LeverComManager LeverManager;

} // namespace levercom
	