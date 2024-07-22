#pragma once

#include "stdint.h"

#ifdef ARDUINO_ARCH_ESP32
#define ESP32
#else
#define MCP2515
#endif

namespace can
{

typedef uint32_t IdType;
typedef uint8_t DataType;

struct Filter
{
	IdType mask;
	IdType code;
};

struct Message
{
	IdType id;
	DataType data[8];
	int dataSize;
};

//! Base class wrapper interface for different CAN libraries
class CanController
{
protected:
	int _txPin;
	int _rxPin;
	Filter _filter;
	long _clockSpeed;

public:
	virtual ~CanController() {}
	void SetPins(int txPin, int rxPin)
	{
		_txPin = txPin;
		_rxPin = rxPin;
	}
	void SetClockSpeed(long speed) { _clockSpeed = speed; }
	void SetFilter(Filter& filter) { _filter = filter; }
	virtual bool Start();
	virtual bool Read(Message& msg);
	virtual void Write(Message& mesg);
};

}