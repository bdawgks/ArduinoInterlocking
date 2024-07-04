// Third part libraries
#include <ArxContainer.h>
#include <ArduinoJson.h>

// Custom libraries
#define STD_LIB
#include <CommonLib.h>
#include <iLock.h>
#include <JSONLoader.h>
#include <ilmsg.h>

// Arduino libraries
#include <SPI.h>
#include <SD.h>
#include <Wire.h>

using DataLoader = JSONLoader::JSONLoader;
using lib::Vector;
using lib::MapComp;
using lib::DeviceSlot;
using ilock::Interlocking;
using ilock::Locking;
using ilock::LockingId;
using ilmsg::MessageProcessor;
using ilmsg::MessageType;
using ilmsg::MsgLeverStateRequest;
using ilmsg::MsgLeverStateResponse;
using ilmsg::MessageProcessFunc;

//! File name to look for config on SD card
const String configFileName = "config.txt";

//! Indicates a successful initialization (config loaded)
bool initSuccessful = false;

//! Pointer to the interlocking class
Interlocking* il = nullptr;

//! Map DeviceSlot to locking ID
MapComp<DeviceSlot, LockingId, lib::DeviceSlotCompare> leverSlots;

//! I2C Message processor
MessageProcessor* msgProcessor;

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
    leverSlots[data.slot] = lever->GetId();
    //slots.insert(std::make_pair(data.slot, lever->GetId()));
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
  }

  // Now finalize all locking rules
  for (auto& data : leverData)
  {
    il->GetLocking(data.name)->FinalizeLockRules();
  }
}

void SendLeverUpdates()
{
	for (auto it = leverSlots.begin(); it != leverSlots.end(); it++)
	{
    Locking* lever = il->GetLocking(it->second);
    if (!lever)
      continue;

    //Serial.print("Send update for lever: ");
    //Serial.print(it->first.address);
    //Serial.print(",");
    //Serial.println(it->first.slot);
    ilmsg::MsgLeverStateUpdate updMsg = ilmsg::MsgLeverStateUpdate(it->first.slot, lever->GetState(), lever->IsLocked());
    MessageProcessor::SendOutboundMessage(updMsg, it->first.address);

    auto requestMsg = MsgLeverStateRequest(it->first.slot);
    msgProcessor->RequestResponse(it->first.address, requestMsg);
	}
}

void ProcessLeverStateResponse(MsgLeverStateResponse msg)
{
    Serial.print("Got update from lever: ");
    Serial.print((int)msg.GetSlot());
    Serial.print(",");
    Serial.println((int)msg.GetLeverState());
}

void setup()
{
  // Set up seiral and wait for it to connect
  Serial.begin(9600);
  while (!Serial)
  {
    delay(10);
  }

  // Load data
  DataLoader* loader = LoadData();
  if (!loader)
  {
    LogError(Unknown, F("loader pointer is null"));
    return;
  }

  // Initialize the interlocking
  InitInterlocking(*loader);

  // Init I2C bus wire
  msgProcessor = new MessageProcessor();
  msgProcessor->OnMessage(MessageType::MTLeverStateResponse, new MessageProcessFunc<MsgLeverStateResponse>(ProcessLeverStateResponse));
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

  if (Serial.available() > 0)
  {
    SendLeverUpdates();
    Serial.read();
    delay(5000);
  }
}
