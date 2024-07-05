#pragma once

#include <CommonLib.h>
#include <iLock.h>
#include <Wire.h>

namespace ilmsg {

using lib::Map;
using lib::Buffer;
using ilock::LockState;
using LeverState = ilock::Lever::State;

typedef byte DeviceId;
typedef byte SlotId;

#define MSG_TYPES(XX) \
	XX(Invalid,0) \
	XX(SetResponseType,1) \
	XX(LeverStateUpdate,3) \
	XX(LeverStateRequest,1) \
	XX(LeverStateResponse,3)

#define MSG_ENUM(e,s) \
	MT##e,

#define MSG_SIZE(e,s) \
	s,

enum MessageType : byte
{
	MSG_TYPES(MSG_ENUM)
	MTCount
};

static int MessageSizes[MTCount + 1] =
{
	MSG_TYPES(MSG_SIZE)
	0
};

class MessageBody
{
private:
	MessageType _type = MTInvalid;
	bool _parsed = false;
	int _size = 0;

protected:
	MessageBody(MessageType type) :
		_type(type)
	{
		_size = MessageSizes[type];
	}

	//! Returns true if the buffer has capacity for this message
	bool ValidateBufferCapacity(Buffer& buf, bool inclHeader = false);

public:
	MessageBody() {}
	virtual ~MessageBody() {}
	//! Read message from buffer
	virtual bool Read(Buffer& buf) { return false; }
	//! Write message to buffer
	virtual bool Write(Buffer& buf) { return false; }
	//! Parse message from string
	virtual void Parse(const String& str) {}
	//! Write message to string
	virtual String ToString() {}
	//! Try reading message from buffer
	bool TryRead(Buffer& buf);
	//! Try writing message to buffer
	bool TryWrite(Buffer& buf);
	//! Get type enum
	MessageType GetType() { return _type; }
};

class MessageBodyRequest : public MessageBody
{
	MessageType _responseType;
protected:
	MessageBodyRequest(MessageType t, MessageType rt)
		: MessageBody(t),
		_responseType(rt)
	{}
public:
	virtual ~MessageBodyRequest() {}

	//! Get response message type
	MessageType GetResponseType() { return _responseType; }
};

class MsgLeverStateUpdate : public MessageBody
{
	SlotId _slot;
	LockState _lockState;
	bool _locked;

public:
	MsgLeverStateUpdate() : MessageBody(MTLeverStateUpdate) {}
	MsgLeverStateUpdate(SlotId slot, LockState state, bool locked)
		: MessageBody(MTLeverStateUpdate),
		_slot(slot),
		_lockState(state),
		_locked(locked)
	{}
	virtual ~MsgLeverStateUpdate() {}
	virtual bool Read(Buffer & buf) override;
	virtual bool Write(Buffer & buf) override;
	virtual String ToString() override;

	//! Get slot on device
	SlotId GetSlot() { return _slot; }
	//! Get lock state
	const LockState& GetLockState() { return _lockState; }
	//! Get lock status
	bool GetLocked() { return _locked; }
};

class MsgLeverStateRequest : public MessageBodyRequest
{
	SlotId _slot;

public:
	MsgLeverStateRequest() : MessageBodyRequest(MTLeverStateRequest, MTLeverStateResponse) {}
	MsgLeverStateRequest(SlotId slot)
		: MessageBodyRequest(MTLeverStateRequest, MTLeverStateResponse),
		_slot(slot)
	{}
	virtual ~MsgLeverStateRequest() {}
	virtual bool Read(Buffer& buf) override;
	virtual bool Write(Buffer& buf) override;
	virtual String ToString() override;

	//! Get slot on device
	SlotId GetSlot() { return _slot; }
};

class MsgLeverStateResponse : public MessageBody
{
	DeviceId _address;
	SlotId _slot;
	LeverState _leverState;

public:
	MsgLeverStateResponse() : MessageBody(MTLeverStateResponse) {}
	MsgLeverStateResponse(DeviceId address, SlotId slot, LeverState state)
		: MessageBody(MTLeverStateResponse),
		_address(address),
		_slot(slot),
		_leverState(state)
	{}
	virtual ~MsgLeverStateResponse() {}
	virtual bool Read(Buffer & buf) override;
	virtual bool Write(Buffer & buf) override;
	virtual String ToString() override;

	//! Get device address
	DeviceId GetAddress() { return _address; }
	//! Get slot on device
	SlotId GetSlot() { return _slot; }
	//! Get lever state
	const LeverState& GetLeverState() { return _leverState; }
};

//! Base class for a message process callback function
class MessageProcessFuncBase
{
public:
	virtual ~MessageProcessFuncBase() {};
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
	virtual ~MessageProcessFunc<T>() {};

	//! Invoke the callback function
	void InvokeFunc(T msg)
	{
		_func(msg);
	}
};

//! Base class for a responce generate callback function
class ResponseGenerateFuncBase
{
public:
	virtual ~ResponseGenerateFuncBase() {};
};

//! Template class to create a responce generate callback for specified message type
template <class T>
class ResponseGenerateFunc : public ResponseGenerateFuncBase
{
	T (*_func)();
public:
	ResponseGenerateFunc<T>(T (*func)()) :
		_func(func)
	{}
	virtual ~ResponseGenerateFunc<T>() {};

	//! Invoke the callback function
	T GetResponse()
	{
		return _func();
	}
};

class MessageProcessor
{
	MessageType _nextResponseType = MTInvalid;
	Map<MessageType, MessageProcessFuncBase*> _processEvents;
	Map<MessageType, ResponseGenerateFuncBase*> _responseEvents;

	//! Template function for invoking message processor function callback
	template <class T>
	void InvokeProcessFunc(T msg)
	{
		MessageType type = msg.GetType();
		if (_processEvents.find(type) != _processEvents.end())
		{
			//Serial.println("found a function callback");
			//Serial.println(msg.ToString());
			MessageProcessFunc<T>* func = static_cast<MessageProcessFunc<T>*>(_processEvents.at(type));
			func->InvokeFunc(msg);
		}
	}

	template <class T>
	bool GetResponseMessage(MessageType responseType, T& msg)
	{
		if (_responseEvents.find(responseType) != _responseEvents.end())
		{
			//Serial.println("response function found");
			ResponseGenerateFunc<T>* func = static_cast<ResponseGenerateFunc<T>*>(_responseEvents.at(responseType));
			msg = func->GetResponse();
			return true;
		}
		return false;
	}

	//! Create and read message from buffer
	void ReadMessage(Buffer& buf);
public:
	//! Register a callback for when a specific message is received
	void OnMessage(MessageType type, MessageProcessFuncBase* func);
	//! Register a callback for when a response is requested
	void OnResponse(MessageType type, ResponseGenerateFuncBase* func);
	//! Receive message from master
	void ReceiveMessage(int expectedBytes);
	//! Process a request event
	void RequestEvent();
	//! Request data
	void RequestResponse(DeviceId address, MessageBodyRequest& requestMsg);
	//! Send outgoing message from master to addressed device
	static void SendOutboundMessage(MessageBody& msg, DeviceId address);
};

}// namespace ilmsg