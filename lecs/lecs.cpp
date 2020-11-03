// LECS (Lightweight Entity Component System) inline definitions file
//
// Written by Marco Vallario
//
// LICENSE: See end of file for license information
//

lecs::ComponentID::IDType lecs::ComponentID::counter = 0;

//Entity
const lecs::Entity lecs::Entity::Invalid = { lecs::Entity::INVALID_INDEX, 0 };

lecs::Entity::Entity(EntityIndex index, EntityGeneration generation) {
	id = (static_cast<IDType>(generation) << 32) | static_cast<IDType>(index);
}

lecs::EntityIndex lecs::Entity::get_index() const {
	return static_cast<EntityIndex>(id);
}

lecs::EntityGeneration lecs::Entity::get_generation() const {
	return static_cast<EntityGeneration>(id >> 32);
}

bool lecs::Entity::is_valid() {
	return get_index() != INVALID_INDEX;
}

// EntityArray
lecs::Entity lecs::EntityArray::create_entity() {
	Entity new_id = Entity::Invalid;

	if (m_free_indices_count > 0) {
		EntityIndex new_index = m_free_indices[m_free_indices_count - 1];
		EntityGeneration new_generation = m_entities[new_index].id.get_generation();
		new_id = Entity{ new_index, new_generation };
		m_free_indices_count--;
	}
	else {
		EntityIndex new_index = static_cast<EntityIndex>(m_entities_count);
		EntityGeneration new_generation = 0;
		new_id = Entity{ new_index, new_generation };
		m_entities_count++;
	}

	m_entities[new_id.get_index()] = { new_id, ComponentMask{} };

	return new_id;
}

void lecs::EntityArray::remove_entity(Entity entity) {
	// Invalidate Entity handle at position and increase generation
	EntityGeneration old_gen = entity.get_generation();
	Entity new_id = Entity{ Entity::INVALID_INDEX, old_gen + 1 };
	const EntityIndex entity_index = entity.get_index();
	m_entities[entity_index].id = new_id;
	m_entities[entity_index].mask.reset();

	// Add index to free list
	m_free_indices[m_free_indices_count] = entity.get_index();
	m_free_indices_count++;
}

lecs::ComponentMask& lecs::EntityArray::get_component_mask(EntityIndex entity_index) {
	return m_entities[entity_index].mask;
}

lecs::Entity lecs::EntityArray::get_id(EntityIndex entity_index) const {
	return m_entities[entity_index].id;
}

int32_t lecs::EntityArray::get_count() const {
	return static_cast<int32_t>(m_entities_count);
}

// ECS
lecs::Entity lecs::ECS::create_entity() {
	return m_entities.create_entity();
}

void lecs::ECS::remove_entity(Entity entity) {
	if (is_entity_handle_active(entity)) {
		for (auto& component_array : m_components) {
			if (component_array) component_array->on_entity_removed(entity.get_index());
		}

		m_entities.remove_entity(entity);
	}
}

lecs::ComponentMask lecs::ECS::get_component_mask_from_index(EntityIndex entity_index) {
	return m_entities.get_component_mask(entity_index);
}

lecs::ComponentMask lecs::ECS::get_component_mask_from_entity(Entity entity) {
	return is_entity_handle_active(entity) ? get_component_mask_from_index(entity.get_index()) : ComponentMask{};
}

int32_t lecs::ECS::get_entity_count() const {
	return m_entities.get_count();
}

lecs::Entity lecs::ECS::get_entity_from_index(EntityIndex entity_index) const {
	return m_entities.get_id(entity_index);
}

bool lecs::ECS::is_entity_handle_active(Entity entity) const {
	return entity.is_valid() &&
		m_entities.get_id(entity.get_index()) == entity;
}

//MIT License
//
//Copyright(c) 2020 Marco Vallario
//
//Permission is hereby granted, free of charge, to any person obtaining a copy
//of this software and associated documentation files(the "Software"), to deal
//in the Software without restriction, including without limitation the rights
//to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
//copies of the Software, and to permit persons to whom the Software is
//furnished to do so, subject to the following conditions :
//
//The above copyright notice and this permission notice shall be included in all
//copies or substantial portions of the Software.
//
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
//AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//SOFTWARE.