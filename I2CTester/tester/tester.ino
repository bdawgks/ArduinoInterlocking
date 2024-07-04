#include <ArxContainer.h>
#include <CommonLib.h>
#include <ilmsg.h>
#include <Wire.h>

const int addr = 1;

using ilmsg::MessageProcessor;
using ilmsg::MsgLeverStateUpdate;
using ilmsg::MsgLeverStateRequest;
using ilmsg::MsgLeverStateResponse;
using ilmsg::MessageType;
using ilmsg::MessageProcessFunc;
using ilmsg::ResponseGenerateFunc;

MessageProcessor* msgProcessor;

int nextSlotToSend = 0;

void ProcessMessage(MsgLeverStateUpdate msg)
{
  Serial.println("got update msg");
  Serial.println(msg.ToString());
}

void RequestLeverState(MsgLeverStateRequest msg)
{
  Serial.print("requested state of ");
  Serial.println((int)msg.GetSlot());
  nextSlotToSend = msg.GetSlot();
}

MsgLeverStateResponse GetLeverState()
{
  auto msg = MsgLeverStateResponse(nextSlotToSend, ilock::Lever::State::Reversed);
  Serial.println("sending lever state");
  Serial.println(msg.ToString());
  return msg;
}

void ReceiveMessage(int size)
{
  msgProcessor->ReceiveMessage(size);
}

void RequestMessage()
{
  //Serial.println("response requested");
  msgProcessor->RequestEvent();
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

  msgProcessor = new MessageProcessor();
  auto stateUpdFunc = MessageProcessFunc<MsgLeverStateUpdate>(ProcessMessage);
  auto stateRequestFunc = MessageProcessFunc<MsgLeverStateRequest>(RequestLeverState);
  auto stateResponseFunc = ResponseGenerateFunc<MsgLeverStateResponse>(GetLeverState);

  msgProcessor->OnMessage(MessageType::MTLeverStateUpdate, &stateUpdFunc);
  msgProcessor->OnMessage(MessageType::MTLeverStateRequest, &stateRequestFunc);
  msgProcessor->OnResponse(MessageType::MTLeverStateResponse, &stateResponseFunc);

  Wire.begin(addr);

  Wire.onReceive(ReceiveMessage);
  Wire.onRequest(RequestMessage);
}

void loop() {
}
