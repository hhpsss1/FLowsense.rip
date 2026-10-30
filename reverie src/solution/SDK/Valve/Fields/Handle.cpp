#include <shared.h>

void* CBaseHandle::GetBaseEntity() {
	if (!Valid() || !EntitySystem)
		return nullptr;

	auto entity = EntitySystem->GetBaseEntity(GetEntryIndex()).As<C_BaseEntity*>();
	if (!entity || entity->GetHandle() != *this)
		return nullptr;

	return entity;
}
