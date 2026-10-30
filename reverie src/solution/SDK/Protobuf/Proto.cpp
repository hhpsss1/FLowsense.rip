#include <shared.h>

CSubtickMoveStep* CProtobuf::CreateNewSubtickMoveStep(CRepeatedPtrField<CSubtickMoveStep>* rept_ptr, void* arena)
{
    using fnSubtickMove = CSubtickMoveStep * (__fastcall*)(void*);
    using fnAddElement = CSubtickMoveStep * (__fastcall*)(CRepeatedPtrField<CSubtickMoveStep>*, void*);

    static auto create_subtick_move = Memory->Modules.Client.Find("E8 ? ? ? ? 48 8B D0 48 8B CE E8 ? ? ? ? 48 8B C8").Rel(1, 5).As<fnSubtickMove>();
    static auto add_element_to_rep_field_container = Memory->Modules.Client.Find("E8 ? ? ? ? 48 8B C8 8B 51").Rel(1,5).As<fnAddElement>();

    if (!create_subtick_move || !add_element_to_rep_field_container)
        return nullptr;

    auto subtick_move = create_subtick_move(arena);
    if (!subtick_move)
        return nullptr;

    return add_element_to_rep_field_container(rept_ptr, subtick_move);
}

CSubtickMoveStep* CProtobuf::AddSubtickMoveStep(CUserCommand* user_cmd, float when)
{
    auto rept_field_move_steps = reinterpret_cast<CRepeatedPtrField<CSubtickMoveStep>*>((PBYTE)user_cmd->pb.mutable_base() + 0x18);
    if (rept_field_move_steps->current_size >= 32)
        return nullptr;

    if (rept_field_move_steps->ss && rept_field_move_steps->current_size < rept_field_move_steps->ss->max)
        subtick_move_step = rept_field_move_steps->ss->elements[rept_field_move_steps->current_size++];
    else
        subtick_move_step = CreateNewSubtickMoveStep(rept_field_move_steps, rept_field_move_steps->pointer);

    if (!subtick_move_step)
        return nullptr;

    subtick_move_step->Clear();
    subtick_move_step->set_when(when);

    auto elements = rept_field_move_steps->ss->elements;
    int index = rept_field_move_steps->current_size - 1;
    while (index > 0 && elements[index - 1]->when() > when) {
        elements[index] = elements[index - 1];
        --index;
    }
    elements[index] = subtick_move_step;

    return subtick_move_step;
}

bool CProtobuf::ReserveSubtickMoveSteps(CUserCommand* user_cmd, int count, float when)
{
    auto steps = reinterpret_cast<CRepeatedPtrField<CSubtickMoveStep>*>((PBYTE)user_cmd->pb.mutable_base() + 0x18);
    if (steps->current_size + count <= 32)
        return true;

    auto elements = steps->ss->elements;
    float before = 0.f;
    float after = 1.f;

    for (int i = 0; i < steps->current_size; ++i) {
        const float fraction = std::nearbyint(elements[i]->when() * 64.f) / 64.f;
        if (fraction < when)
            before = std::max(before, fraction);
        else
            after = std::min(after, fraction);
    }

    auto can_merge = [&](int i) {
        if (elements[i]->button() || elements[i + 1]->button())
            return false;

        const float fraction = std::nearbyint(elements[i]->when() * 64.f) / 64.f;
        return fraction > 0.f && (fraction < before || fraction > after);
    };

    int available = 32 - steps->current_size;
    for (int i = 1; i + 1 < steps->current_size; ++i) {
        if (can_merge(i))
            ++available;
    }

    if (available < count)
        return false;

    while (steps->current_size + count > 32) {
        int index = -1;
        for (int i = 1; i + 1 < steps->current_size; ++i) {
            if (can_merge(i) && (index == -1 || elements[i]->when() > after))
                index = i;
        }

        if (index == -1)
            return false;

        auto step = elements[index];
        auto next = elements[index + 1];
        next->set_analog_forward_delta(next->analog_forward_delta() + step->analog_forward_delta());
        next->set_analog_left_delta(next->analog_left_delta() + step->analog_left_delta());
        next->set_pitch_delta(next->pitch_delta() + step->pitch_delta());
        next->set_yaw_delta(next->yaw_delta() + step->yaw_delta());

        for (int i = index; i + 1 < steps->current_size; ++i)
            elements[i] = elements[i + 1];

        elements[--steps->current_size] = step;
    }

    return true;
}