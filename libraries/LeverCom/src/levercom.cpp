/**
* Interlocking library
* Author: Kyle Sarnik
**/

#include "LeverCom.h"

namespace levercom
{

void LeverComManager::RegisterLever(DeviceSlot dSlot, LockingId lid, bool locked)
{
	LeverInfo* info = new LeverInfo();
	info->lid = lid;
	info->leverLocked = locked;
	_info.insert(std::make_pair(dSlot, info));
	_slotMap.insert(std::make_pair(lid, dSlot));
}

void LeverComManager::InitBuffers()
{
	for (auto it = _info.begin(); it != _info.end(); it++)
	{
		DeviceId address = it->first.address;
		SlotId slot = it->first.slot;

		if (_buffers.find(address) == _buffers.end())
		{
			LeverComBuffer* buffer = new LeverComBuffer();
			_buffers.insert(std::make_pair(address, buffer));
		}
		_buffers[address]->slotCount++;
	}
	for (auto it = _buffers.begin(); it != _buffers.end(); it++)
	{
		it->second->Resize();
	}
}

int LeverComManager::GetResponseSize(DeviceId address)
{
	if (_buffers.find(address) == _buffers.end())
		return 0;

	return _buffers[address]->GetSize();
}

void LeverComManager::ResetBuffer(DeviceId address)
{
	if (_buffers.find(address) == _buffers.end())
		return;

	_buffers[address]->buffer.Reset();
}

void LeverComManager::ReadByte(DeviceId address, byte b)
{
	if (_buffers.find(address) == _buffers.end())
		return;

	_buffers[address]->buffer.Write(b);
}

void LeverComManager::ReadStates(DeviceId address, LeverComBuffer* buffer)
{
	if (!buffer)
		return;

	Buffer& buf = buffer->buffer;
	if (buf.position < 1)
	{
		return;
	}

	buf.Reset();
	for (int slot = 0; slot < buffer->slotCount; slot++)
	{
		LeverState state = (LeverState)buf.Read();
		DeviceSlot dSlot = { address, slot };

		if (state != _info[dSlot]->currentState)
		{
			bool allowChange = true;
			if (_onStateChanged)
				allowChange = _onStateChanged(_info[dSlot]->lid, state);

			if (allowChange)
				_info[dSlot]->currentState = state;
		}
	}
}

void LeverComManager::ProcessBuffers()
{
	for (auto it = _buffers.begin(); it != _buffers.end(); it++)
	{
		ReadStates(it->first, it->second);
	}
}

LeverState LeverComManager::GetState(DeviceSlot slot)
{
	if (_info.find(slot) == _info.end())
		return LeverState::Normal;

	return _info[slot]->currentState;
}

void LeverComManager::RequestLeverState(DeviceId address)
{
	ResetBuffer(address);

	Wire.requestFrom(address, GetResponseSize(address));
	while (Wire.available() > 0)
	{
		ReadByte(address, Wire.read());
	}
}

Vector<DeviceId> LeverComManager::GetAddresses()
{
	Vector<DeviceId> vector = Vector<DeviceId>();
	for (auto it = _buffers.begin(); it != _buffers.end(); it++)
	{
		vector.push_back(it->first);
	}
	return vector;
}

void LeverComManager::SetLeverLockState(LockingId lid, bool locked)
{
	if (_slotMap.find(lid) == _slotMap.end())
		return;

	DeviceSlot dSlot = _slotMap[lid];
	bool curLocked = _info[dSlot]->leverLocked;
	if (curLocked && !locked || !curLocked && locked)
	{
		if (_buffers.find(dSlot.address) == _buffers.end())
		{
			return;
		}
		_buffers[dSlot.address]->pushNeeded = true;
		_info[dSlot]->leverLocked = locked;
	}
}

void LeverComManager::PushLeverStates()
{
	for (auto it = _buffers.begin(); it != _buffers.end(); it++)
	{
		DeviceId address = it->first;
		if (it->second->pushNeeded)
		{
			Wire.beginTransmission(address);
			for (int slot = 0; slot < it->second->slotCount; slot++)
			{
				DeviceSlot dSlot = { address, slot };
				bool locked = _info[dSlot]->leverLocked;
				Wire.write(locked);
			}
			Wire.endTransmission();
		}
		it->second->pushNeeded = false;
	}
}

} // namespace levercom
	