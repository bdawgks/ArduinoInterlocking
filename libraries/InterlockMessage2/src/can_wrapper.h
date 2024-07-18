#pragma once

#include "stdint.h"

#define ESP32
//#define MCP2515

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

public:
	virtual ~CanController() {}
	void SetPins(int txPin, int rxPin)
	{
		_txPin = txPin;
		_rxPin = rxPin;
	}
	void SetFilter(Filter& filter) { _filter = filter; }
	virtual bool Start();
	virtual bool Read(Message& msg);
	virtual void Write(Message& mesg);
};

}