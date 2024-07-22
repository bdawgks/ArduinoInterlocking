#include <ilmsg2.h>

//#define _FEATHER
#define MKR

int thisAddr = 42;

void ReceiveSetLeverState(ilmsg::MessageSetLeverState msg)
{
  Serial.println("Received lever state");
  Serial.print("Device id: ");
  Serial.println(msg.did);
}

void setup() 
{
  Serial.begin(9600);
  while(!Serial);

  ilmsg::Processor.RegisterDevice(ilmsg::ModuleType::Lever, thisAddr);
  //ilmsg::Processor.SetFilter(ilmsg::ModuleType::Lever, thisAddr);
  ilmsg::Processor.OnMessage(ilmsg::MessageType::SetLeverState, new ilmsg::MessageProcessFunc<ilmsg::MessageSetLeverState>(ReceiveSetLeverState));
#ifdef _FEATHER
  if (ilmsg::Processor.Start(19, 22))
#else
#ifdef MKR
  if (ilmsg::Processor.Start(7, 6, 8e6))
#else
  if (ilmsg::Processor.Start(43, 44))
#endif
#endif
  {
    Serial.println("CAN started");
  }
  else
  {
    Serial.println("CAN failed");
  }
}

void loop() 
{
  ilmsg::Processor.ProcessReceived();
  if (Serial.available() > 0)
  {
    while (Serial.available() > 0)
    {
      Serial.read();
    }

    Serial.println("send message");

    ilmsg::MessageSetLeverState msg;
    msg.did = 42;
    msg.slot = 5;
    msg.SetDestination(thisAddr);
    ilmsg::Processor.SendMessage(msg);

    ilmsg::MessageInit msg2;
    msg2.SetDestination(0);
    ilmsg::Processor.SendMessage(msg2);
  }
}
