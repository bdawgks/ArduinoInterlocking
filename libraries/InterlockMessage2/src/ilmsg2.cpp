#include "ilmsg2.h"

#ifdef ESP32
#include "can_esp32.hpp"
#endif
#ifdef MCP2515
#include "can_mcp2515.hpp"
#endif

namespace ilmsg 
{

int BitOffset(int numBytes) { return numBytes * 8; }

MessageType GetTypeFromId(CAN_IdType id)
{
	CAN_IdType mask = 0xFF;
	byte num = id & mask;
	return (MessageType)num;
}

ModuleType GetModTypeFromId(CAN_IdType id)
{
	CAN_IdType mask = 0x7F << BitOffset(1);
	byte num = id & mask;
	return (ModuleType)num;
}

DeviceId GetAddressFromId(CAN_IdType id)
{
	CAN_IdType mask = 0xFF << BitOffset(2);
	DeviceId addr = id & mask;
	return addr;
}

CAN_Filter GetMsgFilter(ModuleType modType, DeviceId address)
{
	CAN_Filter filter = {};
	filter.code = 0;
	filter.mask = ~((CAN_IdType)modType << BitOffset(2)) & ~((CAN_IdType)address << BitOffset(1));
	return filter;
}

void MessageBase::PackMessageId(CAN_Message& msg) const
{
	msg.id = ((CAN_IdType)_mt << BitOffset(2)) | ((CAN_IdType)_destid << BitOffset(1)) | (CAN_IdType)_type;
}

void MessageRegister::PackMessage(CAN_Message& msg) const
{
	MessageBase::PackMessage(msg);
	msg.data[0] = (can::DataType)did;
	msg.data[1] = (can::DataType)mtype;

	msg.dataSize = 1;
}

bool MessageRegister::UnpackMessage(const CAN_Message& msg)
{
	if (msg.dataSize != 2)
		return false;

	MessageBase::UnpackMessage(msg);
	did = (DeviceId)msg.data[0];
	mtype = (ModuleType)msg.data[1];

	return true;
}

void MessageSetLeverState::PackMessage(CAN_Message& msg) const
{
	MessageBase::PackMessage(msg);
	msg.data[0] = (can::DataType)did;
	msg.data[1] = (can::DataType)slot;
	msg.data[2] = (can::DataType)state;
	msg.data[3] = (can::DataType)faulted;

	msg.dataSize = 4;
}

bool MessageSetLeverState::UnpackMessage(const CAN_Message& msg)
{
	if (msg.dataSize != 4)
		return false;

	MessageBase::UnpackMessage(msg);
	did = (DeviceId)msg.data[0];
	slot = (SlotId)msg.data[1];
	state = (LeverState)msg.data[2];
	faulted = (bool)msg.data[3];

	return true;
}

void MessageSetLockState::PackMessage(CAN_Message& msg) const
{
	MessageBase::PackMessage(msg);
	msg.data[0] = (can::DataType)slot;
	msg.data[1] = (can::DataType)state;
	msg.data[2] = (can::DataType)locked;

	msg.dataSize = 3;
}

bool MessageSetLockState::UnpackMessage(const CAN_Message& msg)
{
	if (msg.dataSize != 3)
		return false;

	MessageBase::UnpackMessage(msg);
	slot = (SlotId)msg.data[0];
	state = (LockState)msg.data[1];
	locked = (bool)msg.data[2];

	return true;
}

void MessageSetLockIndication::PackMessage(CAN_Message& msg) const
{
	MessageBase::PackMessage(msg);
	msg.data[0] = (can::DataType)showIndication;

	msg.dataSize = 1;
}

bool MessageSetLockIndication::UnpackMessage(const CAN_Message& msg)
{
	if (msg.dataSize != 1)
		return false;

	MessageBase::UnpackMessage(msg);
	showIndication = (bool)msg.data[0];

	return true;
}

void MessageProcessor::RegisterDevice(ModuleType mtype, DeviceId did)
{
	_mtype = mtype;
	_did = did;
	SetFilter(mtype, did);
}

bool MessageProcessor::Start(int txPin, int rxPin)
{
	if (_controller)
		return false;

#ifdef ESP32
	_controller = new can::ESP32Controller();
#else
	_controller = new can::MCP2515Controller();
#endif

	_controller->SetFilter(_filter);
	_controller->SetPins(txPin, rxPin);
	return _controller->Start();
}

void MessageProcessor::SetFilter(ModuleType mtype, DeviceId addr)
{
	_filter = GetMsgFilter(mtype, addr);
}

void MessageProcessor::ProcessMessage(const CAN_Message& msg)
{
	MessageType type = GetTypeFromId(msg.id);
	switch (type)
	{
	case MessageType::Init:
		InvokeProcessFunc<MessageInit>(type, msg);
		break;
	case MessageType::SetLeverState:
		InvokeProcessFunc<MessageSetLeverState>(type, msg);
		break;
	}

	// On receipt of init, return with register
	if (type == MessageType::Init && _did >= 0)
	{
		MessageRegister msg = {};
		msg.mtype = _mtype;
		msg.did = _did;
		SendMessage(msg);
	}
}

void MessageProcessor::OnMessage(MessageType type, MessageProcessFuncBase* func)
{
	_processEvents.insert(std::make_pair(type, func));
}

void MessageProcessor::ProcessReceived()
{
	if (!_controller)
		return;

	CAN_Message msg = {};
	if (_controller->Read(msg))
	{
		ProcessMessage(msg);
	}
}

void MessageProcessor::SendMessage(const MessageBase& msg)
{
	if (!_controller)
		return;

	CAN_Message cmsg = {};
	msg.PackMessage(cmsg);
	_controller->Write(cmsg);
}

// Processor instance
MessageProcessor Processor;

}// namespace ilmsg