#include <chrono>
#include <iostream>

constexpr size_t _10M = 10'000'000L;
#define LECS_MAX_ENTITIES _10M
#define LECS_IMPLEMENTATION
#include "lecs/lecs.hpp"

struct TransformComponent {
	float position[3];
	float rotation[3];

	TransformComponent() = default;

	~TransformComponent() {
		std::cout << "called ~TransformComponent()" << std::endl;
	}

	TransformComponent(const TransformComponent& other) {
		for (int i = 0; i < 3; i++) {
			position[i] = other.position[i];
			rotation[i] = other.rotation[i];
		}

		std::cout << "called TransformComponent() Copy Ctr" << std::endl;
	}

	TransformComponent(TransformComponent&& other) noexcept {
		for (int i = 0; i < 3; i++) {
			position[i] = other.position[i];
			rotation[i] = other.rotation[i];
		}

		std::cout << "called TransformComponent() Move Ctr" << std::endl;
	}
};

struct VelocityComponent {
	float velocity[3];
};

#define PRINT_ENTITY(e) std::cout << #e << ": { " << e.get_index() << " | " << e.get_generation() << " }" << std::endl;
void test_system_update(lecs::ECS& ecs) {
	for (auto e : lecs::EntityIterator<TransformComponent, VelocityComponent>(ecs)) {
		auto tc = ecs.get_component<TransformComponent>(e);
		auto vc = ecs.get_component<VelocityComponent>(e);

		PRINT_ENTITY(e);
		std::cout << "Has tc and vc" << std::endl;
	}
}

void test_entity_creation(lecs::ECS& ecs) {
	auto e0 = ecs.create_entity();
	PRINT_ENTITY(e0);

	auto e1 = ecs.create_entity();
	PRINT_ENTITY(e1);

	auto e2 = ecs.create_entity();
	PRINT_ENTITY(e2);

	auto e3 = ecs.create_entity();
	PRINT_ENTITY(e3);

	ecs.remove_entity(e2);

	auto e4 = ecs.create_entity();
	PRINT_ENTITY(e4);

	ecs.remove_entity(e0);
	ecs.remove_entity(e1);
}

std::vector<lecs::Entity> g_entities;
std::unique_ptr<lecs::ECS> test_entity_destruction_times(std::unique_ptr<lecs::ECS> ecs) {
	constexpr size_t num_entities = _10M;
	using namespace std::chrono;
	high_resolution_clock::time_point t1 = high_resolution_clock::now();
	for (auto i = 0; i < num_entities; i++) {
		ecs->remove_entity(g_entities[i]);
	}
	high_resolution_clock::time_point t2 = high_resolution_clock::now();
	duration<double> time_span = duration_cast<duration<double>>(t2 - t1);
	std::cout << "test_entity_destruction_times took " << time_span.count() << " seconds with " << num_entities << " entities\n";

	return std::move(ecs);
}

std::unique_ptr<lecs::ECS> test_entity_creation_times(std::unique_ptr<lecs::ECS> ecs) {
	constexpr size_t num_entities = _10M;
	g_entities.resize(num_entities);
	using namespace std::chrono;
	high_resolution_clock::time_point t1 = high_resolution_clock::now();
	for (auto i = 0; i < num_entities; i++) {
		g_entities[i] = ecs->create_entity();
	}
	high_resolution_clock::time_point t2 = high_resolution_clock::now();
	duration<double> time_span = duration_cast<duration<double>>(t2 - t1);
	std::cout << "test_entity_creation_times took " << time_span.count() << " seconds with " << num_entities << " entities\n";

	return std::move(ecs);
}


int main() {
	std::cout << "Welcome to LECS" << std::endl;
	std::cout << "TransformComponent ID: " << lecs::ComponentID::get<TransformComponent>() << std::endl;
	std::cout << "VelocityComponent ID: " << lecs::ComponentID::get<VelocityComponent>() << std::endl;

	std::unique_ptr<lecs::ECS> ecs = std::make_unique<lecs::ECS>();

	ecs = test_entity_creation_times(std::move(ecs));
	ecs = test_entity_destruction_times(std::move(ecs));

	test_entity_creation(*ecs);
	lecs::Entity ent = ecs->create_entity();
	ecs->add_component_to_entity<TransformComponent>(ent);
	ecs->add_component_to_entity<VelocityComponent>(ent);

	lecs::Entity ent2 = ecs->create_entity();
	ecs->add_component_to_entity<TransformComponent>(ent2);

	lecs::Entity ent3 = ecs->create_entity();
	ecs->add_component_to_entity<TransformComponent>(ent3);
	
	lecs::Entity ent4 = ecs->create_entity();
	ecs->add_component_to_entity<VelocityComponent>(ent4);
	ecs->add_component_to_entity<TransformComponent>(ent4);

	ecs->remove_component_from_entity<TransformComponent>(ent);
	
	auto tc = ecs->get_component<TransformComponent>(ent4);
	tc->position[0] = tc->position[1] = tc->position[2] = 1.0f;

	ecs->add_component_to_entity<TransformComponent>(ent);

	test_system_update(*ecs);
	return 0;
}