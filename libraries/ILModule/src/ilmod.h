/**
* Interlocking library
* Author: Kyle Sarnik
**/

#pragma once

#include <CommonLib.h>

namespace ilmod {

using lib::byte;
using lib::Buffer;

//! Get 0-127 address from 7bit pin input
byte ReadBitAddress(int pins[7], int onState = LOW);

//! Reads I2C data to buffer
void ReadWireToBuffer(Buffer& buf, int size);


} // namespace levercom
