// LECS (Lightweight Entity Component System)
//
// Written by Marco Vallario
//
// LICENSE: See end of file for license information
//
// USAGE:
// Components are simple structs eg.:
//	struct Transform {
//		float position[3];
//		float rotation[3];
//		float scale[3];
//	};
//
// You can create new entities from an ECS object:
// lecs::ECS my_ecs;
// lecs::Entity entity = my_ecs.create_entity();
//
// You can assign (or remove) components to entities like this:
// my_ecs.add_component_to_entity<Transform>(entity);
// or
// my_ecs.remove_component_from_entity<Transform>(entity);
//
// You can retrieve component data like this:
// auto component_data = ecs.get_component<Transform>(entity);
// component_data->position[0] = 1.0f;
// If there is no component data associated to that entity, the function will return nullptr.
// You can also check if an entity has a component:
// bool has_transform = ecs.has_component<Transform>(entity));
//
// Systems are just free functions or callable objects, you can choose:
// void velocity_system_update(lecs::ECS& ecs_instance, float delta_time) {
//		for (lecs::Entity entity : lecs::EntityIterator<Transform, Velocity>(ecs_instance)) {
//			Transform& transform_component = ecs_instance.get_component<Transform>(entity);
//			Velocity& velocity_component = ecs_instance.get_component<Velocity>(entity);
//			// ... do your things ...
//		}
// }
//
// Then you can call system updates from wherever you like, usually your main loop, but this gives you flexibility to how you organize your update:
// velocity_system_update(my_ecs, delta_time);
//
// Of course do not forget to remove any entity you don't need:
// my_ecs.remove_entity(entity);
//
// References:
// https://austinmorlan.com/posts/entity_component_system/
// https://www.david-colson.com/2020/02/09/making-a-simple-ecs.html

// TODOs:
// - Assert when components or entities do not exist instead of using pointers and returning nullptr
// - Fix TODOs across the code.
// - In the ComponentArray there is no invocation of the Component destructor when remove_data is invoked.
// - - A fix would be to make a byte array of the size MAX_ENTITIES * sizeof(T) and use placement new when creating the component. Calling a destructor explicitely will fix the destructor issue.

#pragma once

#include <array>
#include <bitset>
#include <cstdint>
#include <memory>
#include <vector>

// Config
// You can define these before including lecs.h
#ifndef LECS_MAX_COMPONENTS
#define LECS_MAX_COMPONENTS 32
#endif // LECS_MAX_COMPONENTS

#ifndef LECS_MAX_ENTITIES
#define LECS_MAX_ENTITIES 5000
#endif // LECS_MAX_ENTITIES

namespace lecs {
	// Provides an unique ID for components eg.:
	// int32_t transform_id = ComponentID::get<Transform>();
	// TODO: add a TAG to this, so we can have multiple ones for different ECSs (eg. template <int TAG> struct ComponentID ...)
	//		 or make this instantiable and use different instances?
	struct ComponentID {
		using IDType = int32_t;

		template <typename T>
		static IDType get() {
			static IDType id = counter++;
			return id;
		}

	private:
		static IDType counter;
	};
	ComponentID::IDType ComponentID::counter = 0;

	// CONFIGURATION
	const int32_t MAX_COMPONENTS = LECS_MAX_COMPONENTS;
	const int32_t MAX_ENTITIES = LECS_MAX_ENTITIES;

	// Implementation
	using EntityIndex = uint32_t;
	using EntityGeneration = uint32_t;
	// Entity is a combination of index and generation
	// EntityGeneration (32bits) | EntityIndex (32bits) = Entity (64Bits)
	//using Entity = uint64_t; // TODO: currently this is not ok, as in create_entity we use the vector's size (size t) which can result into a narrowing conversion. 

	struct Entity
	{
		static const EntityIndex INVALID_INDEX = -1;

		using IDType = uint64_t;
		IDType id;

		Entity() : id{ Invalid.id } {}

		Entity(IDType i) : id(i) {}

		Entity(EntityIndex index, EntityGeneration generation);

		EntityIndex get_index() const;

		EntityGeneration get_generation() const;

		bool is_valid();

		bool operator==(const Entity& other) const {
			return id == other.id;
		}

		static const Entity Invalid;
	};
	const Entity Entity::Invalid = { Entity::INVALID_INDEX, 0 };

	using ComponentMask = std::bitset<MAX_COMPONENTS>;

	class IComponentArray {
	public:
		virtual ~IComponentArray() = default;
		virtual void on_entity_removed(EntityIndex entity_index) = 0;
	};

	template <typename T>
	class ComponentArray;

	class EntityArray {
	public:
		EntityArray() = default;

		Entity create_entity();

		void remove_entity(Entity entity);

		ComponentMask& get_component_mask(EntityIndex entity_index);

		Entity get_id(EntityIndex entity_index) const;

		int32_t get_count() const;

	private:
		struct Entry {
			Entity id;
			ComponentMask mask;
		};

		using EntityArrayType = std::array<Entry, MAX_ENTITIES>;
		using EntityArraySizeType = typename EntityArrayType::size_type;

		using EntityIndexArrayType = std::array<EntityIndex, MAX_ENTITIES>;
		using EntityIndexArraySizeType = typename EntityIndexArrayType::size_type;

		EntityArrayType m_entities;
		EntityArraySizeType m_entities_count = 0;

		EntityIndexArrayType m_free_indices;
		EntityIndexArraySizeType m_free_indices_count = 0;
	};

	class ECS {
	public:
		Entity create_entity();

		void remove_entity(Entity entity);

		// Returns true if succeeded. False, if the entity already had this component, or if the entity passed was invalid.
		template <typename T>
		bool add_component_to_entity(Entity entity);

		// Returns true if succeeded. False, if the entity didn't have this component or the entity is invalid.
		template <typename T>
		bool remove_component_from_entity(Entity entity);

		template <typename T>
		bool has_component(Entity entity);

		// If there is no component of this type, returns a nullptr
		template <typename T>
		T* get_component(Entity entity);

		// Unsafe as it doesn't check if the entity is valid.
		ComponentMask get_component_mask_from_index(EntityIndex entity_index);

		// Safer than from index as it performs checks.
		ComponentMask get_component_mask_from_entity(Entity entity);

		// TODO: use better type
		int32_t get_entity_count() const;

		// TODO: return an std::optional if the Entity index is out of range?
		Entity get_entity_from_index(EntityIndex entity_index) const;

		bool is_entity_handle_active(Entity entity) const;

	private:
		struct EntityEntry {
			Entity id;
			ComponentMask mask;
		};

		using IComponentArrayPtr = std::unique_ptr<IComponentArray>;

		// Lazily initialize component arrays, so we don't waste memory if we don't need to
		template <typename T>
		ComponentArray<T>& get_component_array_by_component_id(ComponentID::IDType component_id);

		template <typename T>
		ComponentArray<T>& get_component_array();

		EntityArray m_entities;
		std::array<IComponentArrayPtr, MAX_COMPONENTS> m_components;
	};

	// This is a compact array for components.
	// Internally it maps entities to array indices, to keep components close to each other and improve cache efficiency.
	template <typename T>
	class ComponentArray : public IComponentArray {
	public:
		ComponentArray() : m_component_array(), m_size(0) {}
		~ComponentArray();

		void insert_data(EntityIndex entity_index, T component) {
			auto new_index = assign_new_index(entity_index);
			T* new_component = construct_at_index(new_index,  std::move(component));
		}

		// prefer this, as it doesn't copy data around. 
		// then use get_data_from_entity_index to modify the data.
		void insert_data_default_initialized(EntityIndex entity_index) {
			auto new_index = assign_new_index(entity_index);
			T* new_component = construct_at_index(new_index);
		}

		void remove_data(EntityIndex entity_index);

		bool has_data(EntityIndex entity_index) {
			return m_entity_to_index_map[entity_index].index != ComponentIndex::INVALID_INDEX;
		}

		T& get_data_from_entity_index(EntityIndex entity_index) {
			return get_data_from_component_index(m_entity_to_index_map[entity_index].index);
		}

		virtual void on_entity_removed(EntityIndex entity_index) override {
			if (has_data(entity_index)) {
				remove_data(entity_index);
			}
		}

	private:
		struct alignas(T) ComponentAsBytesBuffer {
			char bytes[sizeof(T)];
		};

		using ComponentArrayType = std::array<ComponentAsBytesBuffer, MAX_ENTITIES>;
		using ComponentArraySizeType = typename ComponentArrayType::size_type;

		struct ComponentIndex {	
			static const ComponentArraySizeType INVALID_INDEX = -1;
			ComponentArraySizeType index;

			ComponentIndex() : index(INVALID_INDEX) {}
		};

		ComponentArraySizeType assign_new_index(EntityIndex entity_index);

		T& get_data_from_component_index(ComponentArraySizeType component_index);

		T* construct_at_index(ComponentArraySizeType component_index) {
			return new (&m_component_array[component_index].bytes[0]) T{};
		}

		T* construct_at_index(ComponentArraySizeType component_index, T&& other) {
			return new (&m_component_array[component_index].bytes[0]) T(std::move(other));
		}

		void destroy_at_index(ComponentArraySizeType component_index) {
			get_data_from_component_index(component_index).~T();
		}

		ComponentArrayType m_component_array;

		std::array<ComponentIndex, MAX_ENTITIES> m_entity_to_index_map;
		std::array<EntityIndex, MAX_ENTITIES> m_index_to_entity_map;

		ComponentArraySizeType m_size;
	};

	// This is an iterator that lets you iterate through entities matching a particular ComponentMask.
	// If you don't specify any template Component Types then it will iterate over all of the entities.
	template <typename... ComponentTypes>
	class EntityIterator {
	public:
		EntityIterator(ECS& ecs) : m_ecs(ecs), m_entity_count(ecs.get_entity_count()) {
			if (sizeof...(ComponentTypes) == 0) {
				m_all = true;
			}
			else {
				ComponentID::IDType component_IDs[] = { 0, ComponentID::get<ComponentTypes>()... };
				for (int i = 1; i < (sizeof...(ComponentTypes) + 1); i++) {
					m_component_mask.set(component_IDs[i], true);
				}
			}
		}

		struct Iterator {
			Iterator(ECS& ecs, EntityIndex entity_index, ComponentMask mask, bool all) : m_ecs(ecs), m_entity_count(m_ecs.get_entity_count()), m_entity_index(entity_index), m_mask(mask), m_all(all) {}

			Entity operator*() const {
				return m_ecs.get_entity_from_index(m_entity_index);;
			}

			bool operator==(const Iterator& other) const {
				return m_entity_index == other.m_entity_index || m_entity_index == m_entity_count;
			}

			bool operator!=(const Iterator& other) const {
				return m_entity_index != other.m_entity_index || m_entity_index != m_entity_count;
			}

			Iterator& operator++() {
				do {
					m_entity_index++;
				} while (m_entity_index < m_entity_count && !valid_index(m_entity_index));

				return *this;
			}

		private:
			bool valid_index(EntityIndex entity_index) const {
				return m_ecs.get_entity_from_index(entity_index).is_valid() &&
					(m_all || m_mask == (m_mask & m_ecs.get_component_mask_from_index(entity_index)));
			}

			ECS& m_ecs;
			int32_t m_entity_count;
			EntityIndex m_entity_index;
			ComponentMask m_mask;
			bool m_all{ false };
		};

		const Iterator begin() const {
			auto first_index = 0;
			while (first_index < m_entity_count &&
					m_component_mask != (m_component_mask & m_ecs.get_component_mask_from_index(first_index))) {
				first_index++;
			}

			return Iterator(m_ecs, EntityIndex(first_index), m_component_mask, m_all);
		}

		const Iterator end() const {
			return Iterator(m_ecs, EntityIndex(m_entity_count), m_component_mask, m_all);
		}

	private:
		ECS& m_ecs;
		int32_t m_entity_count;
		ComponentMask m_component_mask;
		bool m_all{ false };
	};
}

// Inline definitions file
#include "lecs.inl"

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