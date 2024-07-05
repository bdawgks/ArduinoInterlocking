#include "JSONLoader.h"

namespace JSONLoader {

JSONLoader::JSONLoader(DynamicJsonDocument& doc)
{
	// Load lever data into vector
	_leverData = {};
	JsonArray levers = doc["Levers"];
	for (JsonObject lever : levers)
	{
		LeverData data = {};
		data.name = lever["Name"].as<String>();
		DeviceSlot slot = {};
		slot.address = (byte)lever["Device"].as<int>();
		slot.slot = (byte)lever["Slot"].as<int>();
		data.slot = slot;
		_leverData.push_back(data);
	}

	// Load interlocking data into vector
	_interlockingData = {};
	JsonArray interlocking = doc["Interlocking"];
	for (JsonObject locks : interlocking)
	{
		JsonArray affectingArray = locks["Affecting"];
		for (String affectingName : affectingArray)
		{
			InterlockingData data = {};
			data.actingLever = locks["Acting"].as<String>();
			data.affectedLever = affectingName;
			data.ruleOn = LockingRuleFromString(locks["Locking"]["StateOn"].as<String>());
			data.ruleOff = LockingRuleFromString(locks["Locking"]["StateOff"].as<String>());
			_interlockingData.push_back(data);
		}
	}
}

LockingRule JSONLoader::LockingRuleFromString(const String& str)
{
	if (str == "LockedAny") return LockedAny;
	if (str == "LockedOn") return LockedOn;
	if (str == "LockedOff") return LockedOff;
	return Unlocked;
}

} // namespace JSONLoader