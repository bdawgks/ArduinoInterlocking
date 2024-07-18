#include <ilmsg2.h>

//#define _FEATHER

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

  ilmsg::Processor.SetFilter(ilmsg::ModuleType::Lever, thisAddr);
  ilmsg::Processor.OnMessage(ilmsg::MessageType::SetLeverState, new ilmsg::MessageProcessFunc<ilmsg::MessageSetLeverState>(ReceiveSetLeverState));
#ifdef _FEATHER
  if (ilmsg::Processor.Start(19, 22))
#else
  if (ilmsg::Processor.Start(43, 44))
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
  }
}
