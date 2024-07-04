#pragma once

#include <CommonLib.h>
#include <ArduinoJson.h>

namespace JSONLoader {

using lib::String;
using lib::byte;
using lib::Buffer;
using lib::Vector;
using lib::DeviceSlot;

enum LockingRule : byte
{
	Unlocked,
	LockedAny,
	LockedOn,
	LockedOff
};

struct LeverData
{
	String name;
	DeviceSlot slot;
};

struct InterlockingData
{
	String actingLever;
	String affectedLever;
	LockingRule ruleOn;
	LockingRule ruleOff;
};

class JSONLoader
{
	Vector<LeverData> _leverData;
	Vector<InterlockingData> _interlockingData;
public:
	JSONLoader(DynamicJsonDocument& doc);

	//! Get LeverData
	const Vector<LeverData>& GetLeverData() { return _leverData; }

	//! Get Interlocking Data
	const Vector<InterlockingData>& GetInterlockingData() { return _interlockingData; }

private:
	LockingRule LockingRuleFromString(const String& str);
};

} // namespace JSON Loader