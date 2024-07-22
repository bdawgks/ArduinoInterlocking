/**
* Hardware profiles
* Author: Kyle Sarnik
**/

#include <array>

#pragma once

namespace hwprofile
{

enum class BoardType
{
	ArduinoMKR,
	ArduinoESP32,
	FeatherRP2040
};

struct ProfileData
{
	std::array<int, 7> addressPins;
	std::array<int, 6> leverPins;
	std::array<int, 6> lockIndicatorPins;
	int canTxPin;
	int canRxPin;
	long canClockSpeed = 16E6;
};

ProfileData GetProfile(BoardType type);

void AssignPinData(ProfileData data, int* pinsAddr, int* pinsLever, int* pinsIndicators);

} // namespace hwprofile