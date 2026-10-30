#pragma once

struct CRay {
    void Init(Vector& start_offset) {
        this->line.start_fffset = start_offset;
        this->line.radius = 0.0f;
        this->type = 0;
    }

    void Init(Vector& center, float radius) {
        if (radius > 0.0f) {
            this->sphere.center = center;
            this->sphere.radius = radius;
            this->type = 1;
        } else
            this->Init(center);
    }

    void Init(Vector& mins, Vector& maxs) {
        if (mins != maxs) {
            this->hull.mins = mins;
            this->hull.maxs = maxs;
            this->type = 2;
        } else
            this->Init(mins);
    }

    void Init(Vector& center_a, Vector& center_b, float radius) {
        if (center_a != center_b) {
            if (radius > 0.0f) {
                this->capsule.center[0] = center_a;
                this->capsule.center[1] = center_b;
                this->capsule.radius = radius;
                this->type = 3;
            } else {
                this->Init(center_a, center_b);
            }
        } else
            this->Init(center_a, radius);
    }

    void Init(Vector& mins, Vector& maxs, Vector* vertices, int num_vertices) {
        this->mesh.mins = mins;
        this->mesh.maxs = maxs;
        this->mesh.vertices = vertices;
        this->mesh.num_vertices = num_vertices;
        this->type = 4;
    }

    struct CLine {
        Vector start_fffset;
        float radius;
    };

    struct CSphere {
        Vector center;
        float radius;
    };

    struct CHull {
        Vector mins;
        Vector maxs;
    };

    struct CCapsule {
        Vector center[2];
        float radius;
    };

    struct CMesh {
        Vector mins;
        Vector maxs;
        const Vector* vertices;
        int num_vertices;
    };

    union {
        CLine line;
        CSphere sphere;
        CHull hull;
        CCapsule capsule;
        CMesh mesh;
    };

    uint8_t type;
};

struct CTransform {
    Vector4D position;
    Vector4D rotation;
};

struct CRnCollisionAttr {
    uint64_t interacts_as;
    uint64_t interacts_with;
    uint64_t interacts_exclude;
    uint32_t entity_id;
    uint32_t owner_id;
    uint16_t hierarchy_id;
    uint8_t collision_group;
    uint8_t collision_function_mask;
};

struct CPhysSurfacePropertiesPhysics {
    float fraction;
    float elasticity;
    float density;
    float thickness;
    float soft_contact_frequency;
    float soft_contact_damping_ratio;
};

struct CPhysSurfacePropertiesVehicle {
    float wheel_drag;
    float wheel_friction_scale;
};

struct CPhysSurfacePropertiesSoundNames {
    const char* impact_soft;
    const char* impact_hard;
    const char* scrape_smooth;
    const char* scrape_rough;
    const char* bullet_impact;
    const char* rolling;
    const char* break_sound;
    const char* strain;
    const char* melee_impact;
    const char* push_off;
    const char* skid_stop;
    const char* resonant;
};

struct CPhysSurfacePropertiesAudio {
    float reflectivity;
    float hardness_factor;
    float roughness_factor;
    float rough_threshold;
    float hard_threshold;
    float hard_velocity_threshold;
    float static_impact_volume;
    float occlusion_factor;
};

struct CSurfaceProperties {
    const char* name;
    uint32_t name_hash;
    uint32_t base_name_hash;
    int32_t list_index;
    int32_t base_list_index;
    bool hidden;
    const char* description;
    CPhysSurfacePropertiesPhysics physics;
    CPhysSurfacePropertiesVehicle vehicle_params;
    CPhysSurfacePropertiesSoundNames audio_sounds;
    CPhysSurfacePropertiesAudio audio_params;
};

struct CHitbox;
struct C_CSPlayerPawn;

struct CShape {
    int GetShapeType() { return Memory->Vtable.Execute<int>(this, 4); }
    CRnCollisionAttr* GetCollisionAttribute() { return Memory->Vtable.Execute<CRnCollisionAttr*>(this, 18); }
};

struct CBody {
    int GetEntityID() { return Memory->Vtable.Execute<int>(this, 149); }
    CTransform* GetBodyTransform(bool* a1) { return Memory->Vtable.Execute<CTransform*>(this, 61, a1); }
};

struct CGameTrace {
    CGameTrace() = default;

    CSurfaceProperties* surface_prop;
    C_CSPlayerPawn* entity;
    CHitbox* hitbox;
    CBody* body;
    CShape* shape;
    uint64_t contents;
    CTransform body_transform;
    CRnCollisionAttr shape_attributes;
    Vector start_pos;
    Vector end_pos;
    Vector hit_normal;
    Vector hit_point;
    float hit_offset;
    float fraction;
    int triangle;
    int shape_index;
    int16_t hitbox_bone_index;
    uint8_t ray_type;
    bool start_in_solid;
    bool exact_hit_point;
};

struct CQueryShapeAttr {
	uint64_t interacts_with;
	uint64_t interacts_exclude;
	uint64_t interacts_as;
	uint32_t entity_ids_to_ignore[2];
	uint32_t owner_ids_to_ignore[2];
	uint16_t hierarchy_ids[2];
	uint16_t included_detail_layers;
	uint8_t target_detail_layer;
	uint8_t object_set_mask;
	uint8_t collision_group;
	uint8_t flags;
};

struct CTraceFilter {
	void* vtable;
	CQueryShapeAttr shape_attr;
	bool iterate_entities;
};

struct CTraceInfo {
	float previous_length_mod;
	float current_length_mod;
	float damage;
	uint32_t penetration_count;
	uint32_t handle;
	uint32_t handle2;

	bool IsPenetrationSegment() const { return (handle2 & 1) != 0; }
};

struct CTraceSegment {
	Vector hit_normal;
	float fraction;
	CBody* body;
	CShape* shape;
	uint64_t triangle;
	uint16_t bone_index;
	uint32_t entity_handle;
	int16_t surface_index;
	bool start_in_solid;
	uint8_t type;
	char pad[4];
};

struct CTraceData {
	CTraceData();
	~CTraceData();
	CTraceData(const CTraceData&) = delete;
	CTraceData& operator=(const CTraceData&) = delete;

	int32_t filled_segment_count;
	char pad0[4];
	CTraceSegment* trace_segments;
	int32_t segment_capacity;
	uint32_t segment_flags;
	CTraceSegment trace_segments_array[128];
	bool start_in_solid;
	char pad[7];
	CUtlVector<CTraceInfo> trace_info;
	CTraceInfo trace_info_array[8];
	Vector start;
	Vector end;
	float max_fraction;
	char pad2[76];
};

static_assert(sizeof(CQueryShapeAttr) == 0x38);
static_assert(offsetof(CTraceFilter, iterate_entities) == 0x40);
static_assert(offsetof(CGameTrace, hitbox_bone_index) == 0xB8);
static_assert(sizeof(CGameTrace) == 0xC0);
static_assert(sizeof(CTraceInfo) == 0x18);
static_assert(sizeof(CTraceSegment) == 0x38);
static_assert(offsetof(CTraceSegment, bone_index) == 0x28);
static_assert(offsetof(CTraceSegment, entity_handle) == 0x2C);
static_assert(offsetof(CTraceData, trace_info) == 0x1C20);
static_assert(offsetof(CTraceData, start) == 0x1CF8);
static_assert(sizeof(CTraceData) == 0x1D60);

struct CPhysics {
	CTraceFilter MakeFilter(C_CSPlayerPawn* pawn, uint64_t mask, uint8_t collision_group, uint8_t object_set_mask);
    CTraceFilter MakeMovementFilter(C_CSPlayerPawn* pawn, uint64_t interacts_with, uint8_t collision_group);
	CGameTrace MakeTrace();
	void SetupTrace(CTraceData* data, const Vector& start, const Vector& delta, CTraceFilter& filter, int penetration_count);
	bool ClipTraceToPlayer(CTraceData* data, C_CSPlayerPawn* player);
	void TraceBullet(CTraceData* data, float damage, float penetration, float range_modifier, int team, int penetration_count);
	void GetTraceInfo(CTraceData* data, CGameTrace* trace, const CTraceInfo& info);
	bool TraceRay(CRay* ray, Vector* start, Vector* end, CTraceFilter* filter, CGameTrace* trace);
};
