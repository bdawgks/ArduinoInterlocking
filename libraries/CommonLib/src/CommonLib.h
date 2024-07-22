/**
* Common library
* Author: Kyle Sarnik
**/

#pragma once

#define ENV_ARDUINO 1
#ifndef NO_STD_LIB
#define STD_LIB
#endif

#if ENV_ARDUINO
#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif
#else
#include <string>
#endif

#if ENV_ARDUINO < 1
#define STD_LIB
#endif
#ifdef STD_LIB
#include <map>
#include <vector>
#else
#include <../ArxContainer/ArxContainer.h>
#endif

namespace lib 
{

#ifdef STD_LIB
template <typename K, typename V, typename C>
using MapComp = std::map<K, V, C>;
#endif
template <typename K, typename V>
using Map = std::map<K, V>;

template <typename T>
using Vector = std::vector<T>;

#if ENV_ARDUINO
using String = String;
#else
using String = std::string;
#endif

typedef unsigned char byte;

typedef byte* ByteArray;

template <typename T, size_t N>
struct Array
{
	T* value;
	size_t length;

	Array(T v[N])
	{
		length = N;
		value = v;
	}
};

struct Buffer
{
public:
	ByteArray bytes;
	int length;
	int position;

	Buffer(int len)
	{
		length = len;
		bytes = new byte[len];
		position = 0;
	}

	Buffer(String str)
	{
		length = str.length();
		bytes = (byte*)str.c_str();
		position = 0;
	}

	int Remaining() const
	{
		return length - position;
	}

	bool Write(byte bt)
	{
		if (position >= length)
			return false;

		bytes[position++] = bt;
		return true;
	}

	byte Read()
	{
		if (position >= length)
			return 0;

		return bytes[position++];
	}

	ByteArray ReadBytes(int& num)
	{
		ByteArray bytes = new byte[num];
		int i = 0;
		while (position < length && i < num)
		{
			bytes[i++] = Read();
		}
		num = i;
		return bytes;
	}

	void Reset() { position = 0; }

	char* GetCharArray() const { return (char*)bytes; }
};

typedef byte DeviceId;
typedef byte SlotId;

//! Holds an ID for I2C bus slot
constexpr int MaxModuleAddr = 127;
struct DeviceSlot
{
	// I2C address of device
	DeviceId address;
	// Slot on device
	SlotId slot;

	int FlatId() const { return address * MaxModuleAddr + slot; }

	operator int() { return FlatId(); }
}; 

struct DeviceSlotCompare
{
	bool operator() (const DeviceSlot& lhs, const DeviceSlot& rhs) const
	{
		return lhs.FlatId() < rhs.FlatId();
	}
};

} // namespace lib