/**
* Interlocking library
* Author: Kyle Sarnik
**/

#include "ilmod.h"

namespace ilmod
{

byte ReadBitAddress(int pins[7], int onState)
{
	int address = 0;
	for (int i; i < 7; i++)
	{
		int pinNum = 6 - i; // start at right-most pin
		pinMode(pins[pinNum], INPUT_PULLUP);
		delay(100);
		int set = digitalRead(pins[pinNum]) == onState ? 1 : 0;
		address |= (set << i);
	}
	return (byte)address;
}

} // namespace ilmod
	