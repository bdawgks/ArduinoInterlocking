#include "can_esp32.hpp"

#ifdef ESP32
namespace can
{

void ESP32Controller::ConvertToMessage(Message& msg, CanFrame& frame)
{
	msg.id = frame.identifier;
	msg.dataSize = frame.data_length_code;
	for (int i = 0; i < frame.data_length_code; i++)
	{
		msg.data[i] = frame.data[i];
	}
}

void ESP32Controller::ConvertToFrame(CanFrame& frame, Message& msg)
{
	frame.extd = 1;
	frame.identifier = msg.id;
	frame.data_length_code = msg.dataSize;
	for (int i = 0; i < msg.dataSize; i++)
	{
		frame.data[i] = msg.data[i];
	}
}

bool ESP32Controller::Start()
{
	twai_filter_config_t filter = {};
	filter.acceptance_code = _filter.code;
	filter.acceptance_mask = _filter.mask;
	filter.single_filter = true;
	return ESP32Can.begin(TWAI_SPEED_500KBPS, _txPin, _rxPin, 0xFFFF, 0xFFFF, &filter);
}

bool ESP32Controller::Read(Message& msg)
{
	CanFrame frame;
	if (ESP32Can.readFrame(frame))
	{
		ConvertToMessage(msg, frame);
		return true;
	}
	return false;
}

void ESP32Controller::Write(Message& msg)
{
	CanFrame frame;
	ConvertToFrame(frame, msg);
	ESP32Can.writeFrame(frame);
}

}

#endif // ESP32