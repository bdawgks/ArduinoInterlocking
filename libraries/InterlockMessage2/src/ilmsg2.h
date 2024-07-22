#pragma once

#include "can_wrapper.h"
#include <CommonLib.h>
#include <iLock.h>

namespace ilmsg 
{

using lib::Map;
using lib::Buffer;
using lib::byte;
using ilock::LockState;
using LeverState = ilock::Lever::State;
using ilock::LockState;

using CAN_IdType = can::IdType;
using CAN_DataType = can::DataType;
using CAN_Filter = can::Filter;
using CAN_Message = can::Message;
using CAN_Controller = can::CanController;

typedef byte DeviceId;
typedef byte SlotId;

enum class ModuleType : byte
{
	All = 0,
	Core,
	Lever,
	NModuleType
};

enum class MessageType : byte
{
	Init,
	Register,
	SetLeverState,
	SetLockState,
	SetLockIndication
};

MessageType GetTypeFromId(CAN_IdType id);

CAN_Filter GetMsgFilter(ModuleType modType, DeviceId address);


// Message classes
class MessageBase
{
	ModuleType _mt = ModuleType::All;
	DeviceId _destid = 0;
	MessageType _type;

protected:
	MessageBase(MessageType type, ModuleType mt) : _type(type), _mt(mt) {}
	void PackMessageId(CAN_Message& msg) const;

public:
	virtual ~MessageBase() {}
	virtual void PackMessage(CAN_Message& msg) const { PackMessageId(msg); }
	virtual bool UnpackMessage(const CAN_Message& msg) 
	{
		_type = GetTypeFromId(msg.id);
		return false;
	}

	//! Set destination ID
	void SetDestination(DeviceId did)
	{
		_destid = did;
	}
};

class MessageInit: public MessageBase
{
public:
	virtual ~MessageInit() {}
	MessageInit() : MessageBase(MessageType::Init, ModuleType::All) {}
};

class MessageRegister : public MessageBase
{
public:
	ModuleType mtype;
	DeviceId did = 0;

	virtual ~MessageRegister() {}
	MessageRegister() : MessageBase(MessageType::Register, ModuleType::Core) {}
	void PackMessage(CAN_Message& msg) const override;
	bool UnpackMessage(const CAN_Message& msg) override;
};


class MessageSetLeverState : public MessageBase
{
public:
	DeviceId did = 0;
	SlotId slot = 0;
	LeverState state = LeverState::Normal;
	bool faulted = false;

	virtual ~MessageSetLeverState() {}
	MessageSetLeverState() : MessageBase(MessageType::SetLeverState, ModuleType::Core) {}
	void PackMessage(CAN_Message& msg) const override;
	bool UnpackMessage(const CAN_Message& msg) override;
};

class MessageSetLockState : public MessageBase
{
public:
	SlotId slot = 0;
	LockState state = LockState::On;
	bool locked = false;

	virtual ~MessageSetLockState() {}
	MessageSetLockState() : MessageBase(MessageType::SetLockState, ModuleType::Lever) {}
	void PackMessage(CAN_Message& msg) const override;
	bool UnpackMessage(const CAN_Message& msg) override;
};

class MessageSetLockIndication : public MessageBase
{
public:
	bool showIndication = false;

	virtual ~MessageSetLockIndication() {}
	MessageSetLockIndication() : MessageBase(MessageType::SetLockIndication, ModuleType::Lever) {}
	void PackMessage(CAN_Message& msg) const override;
	bool UnpackMessage(const CAN_Message& msg) override;
};

//! Base class for a message process callback function
class MessageProcessFuncBase
{
public:
	virtual ~MessageProcessFuncBase() {}
};

//! Template class to create a message process callback for specified message type
template <class T>
class MessageProcessFunc : public MessageProcessFuncBase
{
	void (*_func)(T);
public:
	MessageProcessFunc<T>(void (*func)(T)) :
		_func(func)
	{}
	virtual ~MessageProcessFunc<T>() {}

	//! Invoke the callback function
	void InvokeFunc(T msg)
	{
		_func(msg);
	}
};

class MessageProcessor
{
	ModuleType _mtype;
	DeviceId _did = -1;
	Map<MessageType, MessageProcessFuncBase*> _processEvents;
	CAN_Filter _filter;
	CAN_Controller* _controller = nullptr;
	String _moduleNames[(int)ModuleType::NModuleType];

	//! Template function for invoking message processor function callback
	template <class T>
	void InvokeProcessFunc(MessageType type, CAN_Message msg)
	{
		if (_processEvents.find(type) != _processEvents.end())
		{
			T unpackedMsg = T();
			if (unpackedMsg.UnpackMessage(msg))
			{
				MessageProcessFunc<T>* func = static_cast<MessageProcessFunc<T>*>(_processEvents.at(type));
				func->InvokeFunc(unpackedMsg);
			}
		}
	}
	//! Process a CAN message
	void ProcessMessage(const CAN_Message& msg);

public:
	MessageProcessor();

	//! Set the ID of this device
	void RegisterDevice(ModuleType mtype, DeviceId did);

	//! Start processor and open CAN connection
	bool Start(int txPin, int rxPin, long clockSpeed = -1);

	//! Set filter
	void SetFilter(ModuleType mtype, DeviceId addr);

	//! Register a callback for when a specific message is processed
	void OnMessage(MessageType type, MessageProcessFuncBase* func);

	//! Process any received messages
	void ProcessReceived();

	//! Send a message over the bus
	void SendMessage(const MessageBase& msg);

	//! Get module type name;
	String ModuleTypeToString(ModuleType mtype) { return _moduleNames[(int)mtype]; }
};

extern MessageProcessor Processor;

}// namespace ilmsg