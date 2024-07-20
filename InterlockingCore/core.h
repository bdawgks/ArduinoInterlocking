//! Software version
namespace Version
{
    constexpr int Major = 0, Minor = 1;
}

// Third party libraries
#include <ArduinoJson.h>

// Arduino libraries
#include <SPI.h>
#include <SD.h>

// Custom libraries
#define STD_LIB
#include <CommonLib.h>
#include <iLock.h>
#include <JSONLoader.h>
#include <ilmsg2.h>
#include <levercom2.h>

// Used library types
using DataLoader = JSONLoader::JSONLoader;
using lib::Vector;
using lib::DeviceId;
using lib::SlotId;
using lib::DeviceSlot;
using ilock::Interlocking;
using ilock::Locking;
using ilock::LockingId;
using LeverState = ilock::Lever::State;
using levercom::LeverComManager;

// Global variables
namespace Glob
{
    //! File name to look for config on SD card
    const String configFileName = "config.txt";

    //! Indicates a successful initialization (config loaded)
    bool initSuccessful = false;
}

//! Error codes
enum ErrorCode
{
    Unknown,
    MissingDataCard,
    MissingConfig,
    ConfigReadError,
    JSONDeserializeError,
    LeverNotFound,
};

//! Static logging functions
class Log
{
public:
    const static bool logLockingRules = false;

    static void Version()
    {
        Serial.print(F("[LOG] Interlocking Core v"));
        Serial.print(Version::Major);
        Serial.print('.');
        Serial.println(Version::Minor);
    }

    //! Log error code and message
    static void Error(ErrorCode error, String msg)
    {
        Serial.print(F("[ERROR] "));
        Serial.print(error);
        if (msg.isEmpty())
            return;
        Serial.print(F(": "));
        Serial.println(msg);
    }

    static void Init()
    {
        Serial.println(F("[LOG] System initialized..."));
    }

    static void Ping()
    {
        Serial.println(F("[LOG] System running..."));
    }

    static void LeverInitState(Locking* lever)
    {
        if (!logLockingRules || !lever)
            return;

        Serial.print(F("[LOG] init state of lever "));
        Serial.print(lever->GetName());
        Serial.print(F(": "));
        Serial.println(lever->IsLocked() ? F("LOCKED") : F("UNLOCKED"));
    }

    static void LockingRules(JSONLoader::InterlockingData& data)
    {
        if (!logLockingRules)
            return;

        Serial.print(F("[LOG] lock rule; lever "));
        Serial.print(data.actingLever);
        Serial.print(F(" sets lever "));
        Serial.print(data.affectedLever);
        Serial.print(F(" to state "));
        Serial.print((byte)data.ruleOn);
        Serial.print(F(" when ON, state "));
        Serial.print(data.ruleOff);
        Serial.println(F(" when OFF"));
    }
};