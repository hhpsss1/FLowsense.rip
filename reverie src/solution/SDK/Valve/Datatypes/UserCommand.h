#pragma once

enum protobuf_bits : uint32_t {
    PROTOSLOT_1 = 1 << 0,
    PROTOSLOT_2 = 1 << 1,
    PROTOSLOT_3 = 1 << 2,
    PROTOSLOT_4 = 1 << 3,
    PROTOSLOT_5 = 1 << 4,
    PROTOSLOT_6 = 1 << 5,
    PROTOSLOT_7 = 1 << 6,
    PROTOSLOT_8 = 1 << 7,
    PROTOSLOT_9 = 1 << 8,
    PROTOSLOT_10 = 1 << 9,
    PROTOSLOT_11 = 1 << 10,
    PROTOSLOT_12 = 1 << 11,
    PROTOSLOT_13 = 1 << 12,
    PROTOSLOT_14 = 1 << 13,
    PROTOSLOT_15 = 1 << 15,
    PROTOSLOT_16 = 1 << 16,
    PROTOSLOT_17 = 1 << 17
};

template <typename t>
struct CRepeatedPtrField {
    struct Snapshot_t {
        int max;
        t* elements[(INT_MAX - 2 * sizeof(int)) / sizeof(void*)];
    };

    void* pointer;
    int current_size;
    int capacity;
    Snapshot_t* ss;
};

struct CInButtonState {
    void* pointer;
    uint64_t held;
    uint64_t toggle;
    uint64_t scroll;

	void SetButton(uint64_t button, bool pressed) {
		const auto changed = (this->held ^ (pressed ? button : 0)) & button;
        this->scroll |= this->toggle & changed;
        this->toggle ^= changed;
		this->held ^= changed;
	}

    enum CButtonState : int8_t {
        in_button_up = 0,
        in_button_down = 1,
        in_button_down_up = 2,
        in_button_up_down = 3,
        in_button_up_down_up = 4,
        in_button_down_up_down = 5,
        in_button_down_up_down_up = 6,
        in_button_up_down_up_down = 7
    };

    void SetButtonState(const uint64_t& nValue, CButtonState eButtonState) {
        switch (eButtonState) {
        case in_button_up: {
            this->held &= ~nValue;
            this->toggle &= ~nValue;
            this->scroll &= ~nValue;
            break;
        }
        case in_button_down: {
            this->held |= nValue;
            this->toggle &= ~nValue;
            this->scroll &= ~nValue;
            break;
        }
        case in_button_down_up: {
            this->held &= ~nValue;
            this->toggle |= nValue;
            this->scroll &= ~nValue;
            break;
        }
        case in_button_up_down: {
            this->held |= nValue;
            this->toggle |= nValue;
            this->scroll &= ~nValue;
            break;
        }
        case in_button_up_down_up: {
            this->held &= ~nValue;
            this->toggle &= ~nValue;
            this->scroll |= nValue;
            break;
        }
        case in_button_down_up_down: {
            this->held |= nValue;
            this->toggle &= ~nValue;
            this->scroll |= nValue;
            break;
        }
        case in_button_down_up_down_up: {
            this->held &= ~nValue;
            this->toggle |= nValue;
            this->scroll |= nValue;
            break;
        }
        case in_button_up_down_up_down: {
            this->held |= nValue;
            this->toggle |= nValue;
            this->scroll |= nValue;
            break;
        }
        }
    }
};

static_assert(sizeof(CInButtonState) == 0x20);

enum ECmdPredictionReason {
    reason_none = 0,
    reason_repredict = 1,
    reason_fresh_input = 2,
    reason_stale_rerun = 3
};

struct CUserCommand {
    void* __vtable;
    int command_number;
    float simulation_time;
    CSGOUserCmdPB pb;
    CInButtonState button_state;
    int target_history_index;
    double creation_time;
    int server_tick;
    bool has_been_predicted;
    bool has_been_predicted_shoot_weapon_timings;
    int last_prediction_reason;
    int next_prediction_reason;
};

static_assert(sizeof(CUserCommand) == 0x98);

struct CUserCommandManager {
    CUserCommand user_commands[150];
    uint32_t command_number;

    CUserCommand* GetUserCommand(int seq = -1) {
        if (seq == -1)
            seq = this->command_number;

        return &this->user_commands[seq % 150];
    }
};

static_assert(sizeof(CUserCommandManager) == 0x5918);
