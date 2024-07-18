#pragma once

#include "can_wrapper.h"
#ifdef MCP2515
#include <CAN.h>

namespace can
{

class MCP2515Controller : public CanController
{
public:
	virtual ~MCP2515Controller() {}
	bool Start() override;
	bool Read(Message& msg) override;
	void Write(Message& mesg) override;
};

}
#endif // MCP2515