/**
* Interlocking library
* Author: Kyle Sarnik
**/

#pragma once

#include <limits>

namespace logger 
{

template <class T, class E>
class Logger
{
	T _logFlags = 0;
protected:
    virtual E GetAllType();

    bool LogEnabled(E type)
    {
        T mask = 1 << (T)type;
        return _logFlags & mask;
    }

public:
    void EnableLogType(E type, bool enabled)
    {
        T mask = 1 << (int)type;
        if (type == GetAllType())
            mask = std::numeric_limits<T>::max();

        if (enabled)
            _logFlags |= mask;
        else
            _logFlags &= ~mask;

        Serial.print("log flags: ");
        Serial.println(_logFlags);
    }

    class FlagsRef
    {
        E _type;
        Logger<T, E>& _ref;
    public:
        FlagsRef(Logger<T, E>& ref, E type) : _ref(ref), _type(type) {}
        bool& operator= (bool enabled) { _ref.EnableLogType(_type, enabled); }
        operator bool& () { _ref.LogEnabled(_type); }
    };

    FlagsRef operator [](E type)
    {
        return FlagsRef(*this, type);
    }

    void Message(E type, String msg)
    {
        if (LogEnabled(type))
        {
            Serial.print(F("[LOG] "));
            Serial.println(msg);
        }
    }    
    
    bool Enabled()
    {
        return _logFlags != 0;
    }
};

} // namespace logger
