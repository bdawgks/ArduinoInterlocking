// Third part libraries
#include <ArxContainer.h>
#include <ArduinoJson.h>

// Arduino libraries
#include <SPI.h>
#include <SD.h>
#include <Wire.h>

// Custom libraries
#define STD_LIB
#include <CommonLib.h>
#include <iLock.h>
#include <JSONLoader.h>
#include <levercom.h>

using DataLoader = JSONLoader::JSONLoader;
using lib::Vector;
using lib::DeviceId;
using lib::SlotId;
using lib::DeviceSlot;
using ilock::Interlocking;
using ilock::Locking;
using ilock::LockingId;
using levercom::LeverComManager;
using LeverState = ilock::Lever::State;

//! File name to look for config on SD card
const String configFileName = "config.txt";

//! Indicates a successful initialization (config loaded)
bool initSuccessful = false;

//! Pointer to the interlocking class
Interlocking* il = nullptr;

//! I2C Message processor
LeverComManager* leverManager;

// Debug variables
bool logLockingRules = true;

enum ErrorCode
{
    Unknown,
    MissingDataCard,
    MissingConfig,
    ConfigReadError,
    JSONDeserializeError,
    LeverNotFound,
};

//! Log error code and message
void LogError(ErrorCode error, String msg)
{
    Serial.print(F("[ERROR] "));
    Serial.print(error);
    if (msg.isEmpty())
    return;
    Serial.print(F(": "));
    Serial.println(msg);
}

// Loads data from the SD card config file
DataLoader* LoadData()
{
    if (!SD.begin(SDCARD_SS_PIN))
    {
        LogError(MissingDataCard, F("no SD card detected"));
        return nullptr;
    }

    // Check for config file
    if (!SD.exists(configFileName))
    {
        LogError(MissingConfig, F("config file does not exist"));
        return nullptr;
    }

    // Try to open config file
    File config = SD.open(configFileName);
    if (!config)
    {
        LogError(ConfigReadError, F("error reading config file"));
        return nullptr;
    }

    // Deserialize config file
    DynamicJsonDocument doc(5000);
    DeserializationError err = deserializeJson(doc, config);
    if (err)
    {
        LogError(JSONDeserializeError, "JSON deserialize error:" + String(err.c_str()));
        return nullptr;
    }

    // Load data from deserialized JSON
    return new DataLoader(doc);
}

//! Set up the interlocking from the loaded data
void InitInterlocking(DataLoader& loader)
{
    il = new Interlocking();

    // First create all levers
    Vector<JSONLoader::LeverData> leverData = loader.GetLeverData();
    for (auto& data : leverData)
    {
        ilock::Lever* lever = il->AddLever(data.name);
        leverManager->RegisterLever(data.slot, lever->GetId());
    }
    // Apply locking rules
    Vector<JSONLoader::InterlockingData> lockingData = loader.GetInterlockingData();
    for (auto& data : lockingData)
    {
        Locking* leverActing = il->GetLocking(data.actingLever);
        Locking* leverAffected = il->GetLocking(data.affectedLever);
        if (!leverActing)
        {
            LogError(LeverNotFound, "locking \"" + data.actingLever + "\" not found");
            continue;
        }
        if (!leverAffected)
        {
            LogError(LeverNotFound, "locking \"" + data.affectedLever + "\" not found");
            continue;
        }
        leverActing->AddLockRule(ilock::LockState::On, leverAffected->GetId(), (ilock::LockingRule)(byte)data.ruleOn);
        leverActing->AddLockRule(ilock::LockState::Off, leverAffected->GetId(), (ilock::LockingRule)(byte)data.ruleOff);

        if (logLockingRules)
        {
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
    }

    // Initialize levr com buffers
    leverManager->InitBuffers();

    // Now finalize all locking rules
    for (auto& data : leverData)
    {
        Locking* lever = il->GetLocking(data.name);
        lever->FinalizeLockRules();
    }

    // Finally we need to iterate every locking a final time to get their initial lock state
    for (auto lid : il->GetAllLockings())
    {
        Locking* lever = il->GetLocking(lid);
        leverManager->SetLeverLockState(lever->GetId(), lever->IsLocked());

        if (logLockingRules)
        {
            Serial.print(F("[LOG] init state of lever "));
            Serial.print(lever->GetName());
            Serial.print(F(": "));
            Serial.println(lever->IsLocked() ? F("LOCKED") : F("UNLOCKED"));
        }
    }
}

void RequestLeverStates()
{
    // Send requests
    Vector<DeviceId> addresses = leverManager->GetAddresses();
    for (auto address : addresses)
    {
        leverManager->RequestLeverState(address);
    }

    // Process the buffers and read the states
    leverManager->ProcessBuffers();
}

bool LeverStateChanged(LockingId lid, LeverState newState)
{
    Serial.print(F("state changed for lever "));
    Serial.print(il->GetLocking(lid)->GetName());
    Serial.print(F(" , new state: "));
    Serial.println((int)newState);
    return true;
}

void LeverLockChanged(LockingId lid, bool locked)
{
    leverManager->SetLeverLockState(lid, locked);
}

void setup()
{
    // Set up seiral and wait for it to connect
    Serial.begin(9600);
    while (!Serial)
    {
        delay(10);
    }

    leverManager = new LeverComManager();
    leverManager->OnStateChanged(LeverStateChanged);

    // Load data
    DataLoader* loader = LoadData();
    if (!loader)
    {
        LogError(Unknown, F("loader pointer is null"));
        return;
    }

    // Initialize the interlocking
    InitInterlocking(*loader);
    il->OnLockChange(LeverLockChanged);

    // Init I2C bus wire
    Wire.begin();

    // Indicate successful init
    initSuccessful = true;
    Serial.println(F("[LOG] System initialized..."));
}

void loop() 
{
    // Do nothing if init failed
    if (!initSuccessful)
    return;

    // Do nothing if interlocking is null
    if (!il)
    return;

    leverManager->PushLeverStates();
    delay(10);
    RequestLeverStates();

    if (Serial.available() > 0)
    {
        String str = Serial.readString();
        str.trim();
        if (str == "ping")
        {
            Serial.println(F("[LOG] System running..."));
        }
    }
}
