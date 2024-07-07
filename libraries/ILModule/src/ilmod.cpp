/**
* Interlocking library
* Author: Kyle Sarnik
**/

#include "ilmod.h"
#include "wire.h"

namespace ilmod
{

byte ReadBitAddress(int pins[7])
{
	int address = 0;
	for (int i; i < 7; i++)
	{
		int pinNum = 6 - i; // start at right-most pin
		pinMode(pins[pinNum], INPUT_PULLUP);
		int set = digitalRead(pins[pinNum]) == HIGH ? 1 : 0;
		address |= (set << i);
	}
	return (byte)address;
}

void ReadWireToBuffer(Buffer& buf, int size)
{
	// Grow the buffer if it's too small
	if (size > buf.length)
	{
		buf.length = size;
	}
	buf.Reset();
	while (Wire.available() > 0)
	{
		buf.Write(Wire.read());
	}
}

} // namespace ilmod
	