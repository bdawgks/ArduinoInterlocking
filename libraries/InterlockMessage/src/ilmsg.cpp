#include "ilmsg.h"

namespace ilmsg {

bool MessageBody::ValidateBufferCapacity(Buffer& buf, bool inclHeader)
{
	int size = inclHeader ? _size + 1 : _size;
	return buf.Remaining() >= size;
}

bool MessageBody::TryRead(Buffer& buf)
{
	if (!ValidateBufferCapacity(buf))
		return false;

	_parsed = Read(buf);
	return _parsed;
}

bool MessageBody::TryWrite(Buffer& buf)
{
	if (!ValidateBufferCapacity(buf, true))
		return false;

	buf.Write(_type);
	return Write(buf);
}

bool MsgLeverStateUpdate::Read(Buffer& buf)
{
	_slot = buf.Read();
	_lockState = (LockState)buf.Read();
	_locked = (int)buf.Read();
	return true;
}

bool MsgLeverStateUpdate::Write(Buffer& buf)
{
	buf.Write(_slot);
	buf.Write((byte)_lockState);
	buf.Write((byte)_locked);
	return true;
}

String MsgLeverStateUpdate::ToString()
{
	String str = "LeverStateUpdate | slot:" + String(_slot) + ", lockState:" + String((int)_lockState) + ", locked:" + String(_locked);
	return str;
}

bool MsgLeverStateRequest::Read(Buffer& buf)
{
	_slot = buf.Read();
	return true;
}

bool MsgLeverStateRequest::Write(Buffer& buf)
{
	buf.Write(_slot);
	return true;
}

String MsgLeverStateRequest::ToString()
{
	String str = "LeverStateRequest | slot:" + String(_slot);
	return str;
}

bool MsgLeverStateResponse::Read(Buffer& buf)
{
	_slot = buf.Read();
	_leverState = (LeverState)buf.Read();
	return true;
}

bool MsgLeverStateResponse::Write(Buffer& buf)
{
	buf.Write(_slot);
	buf.Write((byte)_leverState);
	return true;
}

String MsgLeverStateResponse::ToString()
{
	String str = "LeverStateResponse | slot:" + String(_slot) + ", lockState:" + String((int)_leverState);
	return str;
}

void MessageProcessor::OnMessage(MessageType type, MessageProcessFuncBase* func)
{
	_processEvents.insert(std::make_pair(type, func));
}

void MessageProcessor::OnResponse(MessageType type, ResponseGenerateFuncBase* func)
{
	_responseEvents.insert(std::make_pair(type, func));
}

void MessageProcessor::SendOutboundMessage(MessageBody& msg, DeviceId address)
{
	Buffer buf = Buffer(10);
	if (msg.TryWrite(buf))
	{
		/*for (int i = 0; i < buf.position; i++)
		{
			Serial.print("out byte: ");
			Serial.println((int)buf.bytes[i]);
		}*/
		Wire.beginTransmission(address);
		for (int i = 0; i < buf.position; i++)
		{
			Wire.write(buf.bytes[i]);
		}
		Wire.endTransmission();
	}
}

void MessageProcessor::ReceiveMessage(int expectedSize)
{
	//Serial.print("received message of size ");
	//Serial.println(expectedSize);

	if (expectedSize < 1)
		return;

	Buffer buf = Buffer(expectedSize);
	while (Wire.available() > 0)
	{
		byte b = Wire.read();
		if (!buf.Write(b))
		{
			//Serial.println("read failed");
			return; // Buffer overflow
		}
	}

	buf.Reset();
	ReadMessage(buf);
}

void MessageProcessor::ReadMessage(Buffer& buf)
{
	MessageType type = (MessageType)buf.Read();
	switch (type)
	{
	case MTLeverStateUpdate:
	{
		auto msg = MsgLeverStateUpdate();
		if (msg.TryRead(buf))
			InvokeProcessFunc<MsgLeverStateUpdate>(msg);
		break;
	}
	case MTLeverStateResponse:
	{
		//Serial.println("got a lever state response");
		auto msg = MsgLeverStateResponse();
		if (msg.TryRead(buf))
			InvokeProcessFunc<MsgLeverStateResponse>(msg);
		break;
	}
	case MTLeverStateRequest:
	{
		auto msg = MsgLeverStateRequest();
		if (msg.TryRead(buf))
		{
			_nextResponseType = msg.GetResponseType();
			InvokeProcessFunc< MsgLeverStateRequest>(msg);
		}
		break;
	}
	}
}

void MessageProcessor::RequestResponse(DeviceId address, MessageBodyRequest& requestMsg)
{
	MessageType responseType = requestMsg.GetResponseType();

	//Serial.println("send request message ");
	//Serial.println(requestMsg.ToString());

	SendOutboundMessage(requestMsg, address);

	int size = MessageSizes[responseType] + 1; // Need one additional byte for the header
	if (size < 1)
		return;

	//Serial.print("expecting size ");
	//Serial.println(size);

	Buffer buf = Buffer(size);
	Wire.requestFrom((int)address, size);
	//Serial.print("available: ");
	//Serial.println(Wire.available());
	while (Wire.available())
	{
		buf.Write(Wire.read());
	}

	//Serial.print("buffer data received");
	//for (int i = 0; i < buf.position; i++)
	//{
	//	Serial.print(": " + String((int)buf.bytes[i]));
	//}
	//Serial.println(";");
	//Serial.print("buffer contains ");
	//Serial.println(buf.position);

	buf.Reset();
	ReadMessage(buf);
}

void MessageProcessor::RequestEvent()
{
	if (_nextResponseType == MTInvalid)
	{
		Wire.write(0);
		return;
	}

	Buffer buf = Buffer(10);
	switch (_nextResponseType)
	{
	case MTLeverStateResponse:
	{
		MsgLeverStateResponse msg;
		if (GetResponseMessage(_nextResponseType, msg))
			msg.TryWrite(buf);
		break;
	}
	}
	
	//Serial.print("Response message in buffer: ");
	//Serial.println(buf.position);
	//Serial.print("buffer data sent");
	//for (int i = 0; i < buf.position; i++)
	//{
	//	Serial.print(": " + String((int)buf.bytes[i]));
	//}
	//Serial.println(";");
	if (buf.position < 1)
	{
		Wire.write(0);
		return;
	}
	buf.Write(0);
	Wire.write(buf.bytes, buf.position);
}

}// namespace ilmsg