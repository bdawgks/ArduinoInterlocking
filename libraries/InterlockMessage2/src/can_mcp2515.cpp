#include "can_mcp2515.hpp"

#ifdef MCP2515
namespace can
{

bool MCP2515Controller::Start()
{
	CAN.setPins(_txPin, _rxPin);
	//CAN.setClockFrequency(8E6);
	CAN.filterExtended(_filter.code, _filter.mask);
	return CAN.begin(500E3);
}

bool MCP2515Controller::Read(Message& msg)
{
	if (CAN.parsePacket() <= 0)
		return false;

	if (!CAN.packetExtended())
		return false;

	msg.dataSize = CAN.packetDlc();
	msg.id = CAN.packetId();
	for (int i = 0; i < CAN.packetDlc(); i++)
	{
		msg.data[i] = CAN.read();
	}

	return true;
}

void MCP2515Controller::Write(Message& msg)
{
	CAN.beginExtendedPacket(msg.id, msg.dataSize);
	for (int i = 0; i < msg.dataSize; i++)
	{
		CAN.write(msg.data[i]);
	}
	CAN.endPacket();
}

}

#endif // MCP2515