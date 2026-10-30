#pragma once

#include "usercmd.pb.h"
#include "../Valve/Datatypes/UserCommand.h"

class CProtobuf
{
    CSubtickMoveStep* subtick_move_step = nullptr;
    CSubtickMoveStep* CreateNewSubtickMoveStep(CRepeatedPtrField<CSubtickMoveStep>* rept_ptr, void* arena);
public:
    CSubtickMoveStep* AddSubtickMoveStep(CUserCommand* user_cmd, float when);
    bool ReserveSubtickMoveSteps(CUserCommand* user_cmd, int count, float when);
};

inline CProtobuf* Protobuf = new CProtobuf();
