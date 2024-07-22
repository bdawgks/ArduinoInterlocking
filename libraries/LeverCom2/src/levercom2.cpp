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
	msg.state = _info[dSlot]->lockState;
	msg.locked = _info[dSlot]->leverLocked;
	msg.SetDestination(dSlot.address);
	ilmsg::Processor.SendMessage(msg);
}

void LeverComManager::OnRegister(ilmsg::MessageRegister msg)
{
	_registeredDevices.push_back(msg.did);
}

void LeverComManager::OnSetLeverState(ilmsg::MessageSetLeverState msg)
{
	DeviceSlot dSlot = { msg.did, msg.slot };

	if (_info.find(dSlot) == _info.end())
		return;

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

void LeverComManager::SetLeverLockState(LockingId lid, bool locked)
{
	if (_slotMap.find(lid) == _slotMap.end())
		return;

	DeviceSlot dSlot = _slotMap[lid];
	bool curLocked = _info[dSlot]->leverLocked;
	_info[dSlot]->leverLocked = locked;
	if (curLocked != locked)
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
	