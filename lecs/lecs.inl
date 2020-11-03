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

template <typename T>
bool lecs::ECS::add_component_to_entity(Entity entity) {
	auto component_id = ComponentID::get<T>();

	const EntityIndex entity_index = entity.get_index();
	if (!is_entity_handle_active(entity) || m_entities.get_component_mask(entity_index).test(component_id)) {
		return false;
	}

	auto& component_array = get_component_array_by_component_id<T>(component_id);
	component_array.insert_data_default_initialized(entity_index);
	m_entities.get_component_mask(entity_index).set(component_id, true);

	return true;
}

template <typename T>
bool lecs::ECS::remove_component_from_entity(Entity entity) {
	auto component_id = ComponentID::get<T>();

	const EntityIndex entity_index = entity.get_index();
	if (!is_entity_handle_active(entity) || m_entities.get_component_mask(entity_index).test(component_id) == false) {
		// The entity doesn't have this component!
		return false;
	}

	auto& component_array = get_component_array_by_component_id<T>(component_id);
	component_array.remove_data(entity_index);
	m_entities.get_component_mask(entity_index).set(component_id, false);

	return true;
}

template <typename T>
bool lecs::ECS::has_component(Entity entity) {
	if (!is_entity_handle_active(entity)) {
		return false;
	}

	auto component_id = ComponentID::get<T>();
	return m_entities.get_component_mask(entity.get_index()).test(component_id);
}

template <typename T>
T* lecs::ECS::get_component(Entity entity) {
	if (!has_component<T>(entity))
	{
		return nullptr;
	}

	auto& component_array = get_component_array<T>();
	return &component_array.get_data_from_entity_index(entity.get_index());
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

template <typename T>
lecs::ComponentArray<T>
& lecs::ECS::get_component_array_by_component_id(ComponentID::IDType component_id) {
	if (m_components[component_id] == nullptr) {
		m_components[component_id] = std::make_unique<ComponentArray<T>>();
	}

	return *(static_cast<ComponentArray<T>*>(m_components[component_id].get()));
}

template <typename T>
lecs::ComponentArray<T>
& lecs::ECS::get_component_array() {
	auto component_id = ComponentID::get<T>();

	return get_component_array_by_component_id<T>(component_id);
}

// ComponentArray<T>
template <typename T>
lecs::ComponentArray<T>::~ComponentArray() {
	for (auto i = 0; i < m_size; ++i) {
		destroy_at_index(i); // explicitly call destructor
	}
}

template <typename T>
void lecs::ComponentArray<T>::remove_data(EntityIndex entity_index) {
	// Copy the last element of the array into the removed component's place. This keeps the array compact.
	ComponentArraySizeType index_of_removed_entity = m_entity_to_index_map[entity_index].index;
	ComponentArraySizeType index_of_last_element = m_size - 1;
	destroy_at_index(index_of_removed_entity); // explicitly call destructor
	construct_at_index(index_of_removed_entity, std::move(get_data_from_component_index(index_of_last_element)));
	destroy_at_index(index_of_last_element); // explicitly call destructor

	// Update the indices for the maps
	EntityIndex entity_index_of_last_element = m_index_to_entity_map[index_of_last_element];
	m_entity_to_index_map[entity_index_of_last_element].index = index_of_removed_entity;
	m_index_to_entity_map[index_of_removed_entity] = entity_index_of_last_element;

	// Remove deprecated entries
	m_entity_to_index_map[entity_index].index = ComponentIndex::INVALID_INDEX;
	m_index_to_entity_map[index_of_last_element] = Entity::INVALID_INDEX;

	--m_size;
}

template <typename T>
typename lecs::ComponentArray<T>::ComponentArraySizeType lecs::ComponentArray<T>::assign_new_index(EntityIndex entity_index) {
	ComponentArraySizeType new_index = m_size;
	m_entity_to_index_map[entity_index].index = new_index;
	m_index_to_entity_map[new_index] = entity_index;

	m_size++;

	return new_index;
}

template <typename T>
T& lecs::ComponentArray<T>::get_data_from_component_index(ComponentArraySizeType component_index) {
	auto& bytes = m_component_array[component_index].bytes;
	T* component = reinterpret_cast<T*>(&bytes[0]);
	return *component;
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