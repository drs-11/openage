// Copyright 2015-2021 the openage authors. See copying.md for legal info.

#include "../gamestate/old/player.h"
#include "../terrain/terrain_object.h"
#include "../util/math_constants.h"
#include "action.h"
#include "unit.h"
#include "unit_container.h"
#include "unit_type.h"

#include <utility>

namespace openage {

UnitTypeMeta::UnitTypeMeta(std::string name, int id, init_func f)
	:
	init{f},
	type_name{std::move(name)},
	type_id{id} {
}

std::string UnitTypeMeta::name() const {
	return this->type_name;
}

int UnitTypeMeta::id() const {
	return this->type_id;
}

UnitType::UnitType(const Player &owner)
	:
	owner{owner},
	have_limit{math::INT_INF},
	had_limit{math::INT_INF} {
}

void UnitType::reinitialise(Unit *unit, Player &player) {
	// In case reinitialise is not implemented separately

	Attributes tmp;
	// copy only unshared
	tmp.add_copies(unit->attributes, false, true);
	// initialise the new unit
	this->initialise(unit, player);
	// replace new unshared attributes with the old
	unit->attributes.add_copies(tmp);
}

bool UnitType::operator==(const UnitType &other) const {
	return this->type_abilities == other.type_abilities;
}

bool UnitType::operator!=(const UnitType &other) const {
	return !(*this == other);
}

UnitTexture *UnitType::default_texture() {
	return this->graphics[graphic_type::standing].get();
}

TerrainObject *UnitType::place_beside(Unit *u, TerrainObject const *other) const {
	if (!u || !other) {
		return nullptr;
	}

	// find the range of possible tiles
	tile_range outline{other->pos.start - coord::tile_delta{1, 1},
	                   other->pos.end   + coord::tile_delta{1, 1},
	                   other->pos.draw};

	// find a free position adjacent to the object
	auto terrain = other->get_terrain();
	for (coord::tile temp_pos : tile_list(outline)) {
		TerrainChunk *chunk = terrain->get_chunk(temp_pos);

		if (chunk == nullptr) {
			continue;
		}

		auto placed = this->place(u, terrain, temp_pos.to_phys3(*terrain));
		if (placed) {
			return placed;
		}
	}
	return nullptr;
}

void UnitType::copy_attributes(Unit *unit) const {
	unit->add_attributes(this->default_attributes);
}

void UnitType::upgrade(const std::shared_ptr<AttributeContainer> &attr) {
	this->default_attributes.add(attr);
}

UnitType *UnitType::parent_type() const {
	return this->owner.get_type(this->parent_id());
}

NyanType::NyanType(const Player &owner)
	:
	UnitType(owner) {
	// TODO: the type should be given attributes and abilities
}

NyanType::~NyanType() = default;

int NyanType::id() const {
	return 1;
}

int NyanType::parent_id() const {
	return -1;
}

std::string NyanType::name() const {
	return "Nyan";
}

void NyanType::initialise(Unit *unit, Player &) {
	// removes all actions and abilities
	unit->reset();

	// initialise unit
	unit->unit_type = this;

	// the parsed nyan data gives the list of attributes
	// and abilities which are given to the unit
	for (auto &ability : this->type_abilities) {
		unit->give_ability(ability);
	}

	// copy all attributes
	this->copy_attributes(unit);

	// give idle action
	unit->push_action(std::make_unique<IdleAction>(unit), true);
}

TerrainObject *NyanType::place(Unit *unit, std::shared_ptr<Terrain> terrain, coord::phys3 init_pos) const {
	// the parsed nyan data gives the rules for terrain placement
	// which includes valid terrains, base radius and shape

	unit->make_location<SquareObject>(coord::tile_delta{1, 1});

	// allow unit to go anywhere
	unit->location->passable = [](const coord::phys3 &) {
		return true;
	};

	// try to place the obj, it knows best whether it will fit.
	if (unit->location->place(terrain, init_pos, object_state::placed)) {
		return unit->location.get();
	}

	// placing at the given position failed
	unit->log(MSG(dbg) << "failed to place object");
	return nullptr;
}

} // namespace openage
