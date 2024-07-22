#include "HardwareProfile.h"

namespace hwprofile
{

ProfileData GetProfile(BoardType type)
{
	ProfileData data = {};
	if (type == BoardType::ArduinoMKR)
	{
		data.addressPins = { 0,0,0,0,0,0,0 };
		data.leverPins = { 0,0,0,0,0,0 };
		data.lockIndicatorPins = { 0,0,0,0,0,0 };
		data.canTxPin = 7;
		data.canRxPin = 6;
		data.canClockSpeed = 8e6;
	}
	else if (type == BoardType::ArduinoESP32)
	{
		int NP = 255;
		data.addressPins = { 24,23,22,21,20,19,18 };
		data.leverPins = { 3,4,5,6,7,8 };
		data.lockIndicatorPins = { NP,NP,NP,NP,NP,NP };
		data.canTxPin = 43;
		data.canRxPin = 44;
		data.canClockSpeed = -1;
	}
	else if (type == BoardType::FeatherRP2040)
	{
		data.addressPins = { 0,0,0,0,0,0,0 };
		data.leverPins = { 0,0,0,0,0,0 };
		data.lockIndicatorPins = { 0,0,0,0,0,0 };
		data.canTxPin = 19;
		data.canRxPin = 22;
		data.canClockSpeed = -1;
	}

	return data;
}

void CopyArray(int* from, int* to, int size)
{
	for (int i = 0; i < size; i++)
	{
		to[i] = from[i];
	}
}

void AssignPinData(ProfileData data, int* pinsAddr, int* pinsLever, int* pinsIndicators)
{
	if (pinsAddr)
		CopyArray(data.addressPins.data(), pinsAddr, 7);

	if (pinsLever)
		CopyArray(data.leverPins.data(), pinsLever, 6);

	if (pinsIndicators)
		CopyArray(data.lockIndicatorPins.data(), pinsIndicators, 6);
}

} // namespace hwprofile