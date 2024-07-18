#pragma once

#include "can_wrapper.h"
#ifdef ESP32
#include <ESP32-TWAI-CAN.hpp>

namespace can
{

class ESP32Controller : public CanController
{
	void ConvertToMessage(Message& msg, CanFrame& frame);
	void ConvertToFrame(CanFrame& frame, Message& msg);

public:
	virtual ~ESP32Controller() {}
	bool Start() override;
	bool Read(Message& msg) override;
	void Write(Message& mesg) override;
};

}
#endif // ESP32