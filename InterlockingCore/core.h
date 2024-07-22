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
#include <Logger.h>

// Standard libraries
#include <limits>

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
using levercom::LeverManager;

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
    CANFailed
};

enum LogType
{
    Error,
    General,
    Interlocking,
    MessageCom,
    All
};

//! Logging functions
class CoreLogger : public logger::Logger<unsigned int, LogType>
{  
protected:
    LogType GetAllType() override { return LogType::All; }

public:
    void Version()
    {
        if (!LogEnabled(General))
            return;

        Serial.print(F("[LOG] Interlocking Core v"));
        Serial.print(Version::Major);
        Serial.print('.');
        Serial.println(Version::Minor);
    }

    //! Log error code and message
    void Error(ErrorCode error, String msg)
    {
        if (!LogEnabled(LogType::Error))
            return;

        Serial.print(F("[ERROR] "));
        Serial.print(error);
        if (msg.isEmpty())
            return;
        Serial.print(F(": "));
        Serial.println(msg);
    }

    void Init()
    {
        if (!LogEnabled(General))
            return;

        Serial.println(F("[LOG] System initialized..."));
    }

    void Ping()
    {
        if (!LogEnabled(General))
            return;

        Serial.println(F("[LOG] System running..."));
    }

    void LeverInitState(Locking* lever)
    {
        if (!LogEnabled(Interlocking))
            return;

        Serial.print(F("[LOG] init state of lever "));
        Serial.print(lever->GetName());
        Serial.print(F(": "));
        Serial.println(lever->IsLocked() ? F("LOCKED") : F("UNLOCKED"));
    }

    void LockingRules(JSONLoader::InterlockingData& data)
    {
        if (!LogEnabled(Interlocking))
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

    void ModuleRegistered(ilmsg::MessageRegister msg)
    {
        if (!LogEnabled(MessageCom))
            return;

        Serial.print(F("[LOG] module registered: "));
        Serial.print(ilmsg::Processor.ModuleTypeToString(msg.mtype));
        Serial.print(F(" at address "));
        Serial.println(msg.did);
    }
};

CoreLogger Log;