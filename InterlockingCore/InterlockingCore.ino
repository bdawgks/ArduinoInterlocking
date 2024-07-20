// Header for this file
#include "core.h"

//! Pointer to the interlocking class
Interlocking* il = nullptr;

//! Lever manager
LeverComManager* leverManager;

// Loads data from the SD card config file
DataLoader* LoadData()
{
    if (!SD.begin(SDCARD_SS_PIN))
    {
        Log::Error(MissingDataCard, F("no SD card detected"));
        return nullptr;
    }

    // Check for config file
    if (!SD.exists(Glob::configFileName))
    {
        Log::Error(MissingConfig, F("config file does not exist"));
        return nullptr;
    }

    // Try to open config file
    File config = SD.open(Glob::configFileName);
    if (!config)
    {
        Log::Error(ConfigReadError, F("error reading config file"));
        return nullptr;
    }

    // Deserialize config file
    DynamicJsonDocument doc(5000);
    DeserializationError err = deserializeJson(doc, config);
    if (err)
    {
        Log::Error(JSONDeserializeError, "JSON deserialize error:" + String(err.c_str()));
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
            Log::Error(LeverNotFound, "locking \"" + data.actingLever + "\" not found");
            continue;
        }
        if (!leverAffected)
        {
            Log::Error(LeverNotFound, "locking \"" + data.affectedLever + "\" not found");
            continue;
        }
        leverActing->AddLockRule(ilock::LockState::On, leverAffected->GetId(), (ilock::LockingRule)(byte)data.ruleOn);
        leverActing->AddLockRule(ilock::LockState::Off, leverAffected->GetId(), (ilock::LockingRule)(byte)data.ruleOff);

        Log::LockingRules(data);
    }

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

        Log::LeverInitState(lever);
    }
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

    // Print software version
    Log::Version();

    // Set up ILMSG Communication
    ilmsg::Processor.RegisterDevice(ilmsg::ModuleType::Core, 0);
    // TODO: ilmsg::Processor.Start()
    ilmsg::Processor.SendMessage(ilmsg::MessageInit());

    // Set up lever coms
    leverManager = new LeverComManager();
    leverManager->OnStateChanged(LeverStateChanged);

    // Load data
    DataLoader* loader = LoadData();
    if (!loader)
    {
        Log::Error(Unknown, F("loader pointer is null"));
        return;
    }

    // Initialize the interlocking
    InitInterlocking(*loader);
    il->OnLockChange(LeverLockChanged);

    // Indicate successful init
    Glob::initSuccessful = true;
    Log::Init();
}

void loop() 
{
    // Do nothing if init failed
    if (!Glob::initSuccessful)
    return;

    // Do nothing if interlocking is null
    if (!il)
    return;

    if (Serial.available() > 0)
    {
        String str = Serial.readString();
        str.trim();
        if (str == "ping")
        {
            Log::Ping();
        }
    }
}
