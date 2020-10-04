# LECS
LECS (Lightweight Entity Component System) is a simple, lightweight and header only C++ library to add ECS (Entity Component System) to your game or your project.

Also check [Leviathan (Lightweight Event System)](https://github.com/MarkSkyzoid/leviathan)!

## Installation
You can download the whole repo and copy the contents of the 'lecs' folder into your project's include directories. <br>
Then you can include lecs into your project:
```cpp
#include "lecs/lecs.h"
```

## Usage
Components are simple structs eg.:
```cpp
struct Transform {
	float position[3];
	float rotation[3];
	float scale[3];
};
```
 You can create new entities from an ECS object:
```cpp
 lecs::ECS my_ecs;
 lecs::Entity entity = my_ecs.create_entity();
```
 You can assign (or remove) components to entities like this:
```cpp
 my_ecs.add_component_to_entity<Transform>(entity);
```
 or
```cpp
 my_ecs.remove_component_from_entity<Transform>(entity);
```

 You can retrieve component data like this:
 ```cpp
 auto component_data = ecs.get_component<Transform>(entity);
 component_data->position[0] = 1.0f;
 ```
 If there is no component data associated to that entity, the function will return nullptr.
 You can also check if an entity has a component:
```cpp
 bool has_transform = ecs.has_component<Transform>(entity));
```
 Systems are just free functions or callable objects, you can choose:
```cpp
 void velocity_system_update(lecs::ECS& ecs_instance, float delta_time) {
		for (lecs::Entity entity : lecs::EntityIterator<Transform, Velocity>(ecs_instance)) {
			Transform& transform_component = ecs_instance.get_component<Transform>(entity);
			Velocity& velocity_component = ecs_instance.get_component<Velocity>(entity);
			 ... do your things ...
		}
 }
```
 Then you can call system updates from wherever you like, usually your main loop, but this gives you flexibility to how you organize your update:
```cpp 
 velocity_system_update(my_ecs, delta_time);
```
 Of course do not forget to remove any entity you don't need:
```cpp
 my_ecs.remove_entity(entity);
```
## Contributing
Pull requests are welcome. For major changes, please open an issue first to discuss what you would like to change.

## Authors
- Marco Vallario - *Main author* - [MarkSkyzoid](https://github.com/MarkSkyzoid/)

## License
This project is licensed under the MIT License - see the [LICENSE](/LICENSE) file for details

### References:
 - https://austinmorlan.com/posts/entity_component_system/
 - https://www.david-colson.com/2020/02/09/making-a-simple-ecs.html
