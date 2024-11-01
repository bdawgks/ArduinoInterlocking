#include "HardwareProfile.h"

namespace hwprofile
{

ProfileData GetProfile(BoardType type)
{
	ProfileData data = {};
	if (type == BoardType::ArduinoESP32)
	{
		data.addressPins = { 17,18,19,20,21,22,23 };
		data.leverPins = { 13,12,11,10,9,8 };
		data.lockIndicatorPins = { 7,6,5,4,3,2 };
		data.canTxPin = 43;
		data.canRxPin = 44;
		data.canClockSpeed = -1;
	}
	else if (type == BoardType::ArduinoMKR)
	{
		data.addressPins = { 0,0,0,0,0,0,0 };
		data.leverPins = { 0,0,0,0,0,0 };
		data.lockIndicatorPins = { 0,0,0,0,0,0 };
		data.canTxPin = 7;
		data.canRxPin = 6;
		data.canClockSpeed = 8e6;
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