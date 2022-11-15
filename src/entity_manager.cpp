#include "pch.h"
#include "entity_manager.h"
#include "entities.h"
#include "animation.h"
#include "game.h"

#include "animation_registry.h"

void Entity_Manager::register_entity(Entity *e) {
    entity_lookup.add(next_entity_id, e);
    e->id = next_entity_id;
    e->manager = this;
    next_entity_id += 1;
}

Guy *Entity_Manager::make_guy() {
    Guy *guy = new Guy();
    by_type._Guy.add(guy);
    register_entity(guy);
    guy->type = ENTITY_TYPE_GUY;
    
    guy->looking_down_idle_animation = globals.animation_registry->get("player_looking_down_idle");
    guy->looking_right_idle_animation = globals.animation_registry->get("player_looking_right_idle");
    guy->looking_up_idle_animation = globals.animation_registry->get("player_looking_up_idle");
    guy->looking_left_idle_animation = globals.animation_registry->get("player_looking_left_idle");
    
    guy->looking_down_moving_animation = globals.animation_registry->get("player_looking_down_moving");
    guy->looking_right_moving_animation = globals.animation_registry->get("player_looking_right_moving");
    guy->looking_up_moving_animation = globals.animation_registry->get("player_looking_up_moving");
    guy->looking_left_moving_animation = globals.animation_registry->get("player_looking_left_moving");

    guy->current_animation = guy->looking_down_idle_animation;
    
    return guy;
}

Tilemap *Entity_Manager::make_tilemap() {
    Tilemap *tm = new Tilemap();
    by_type._Tilemap.add(tm);
    register_entity(tm);
    tm->type = ENTITY_TYPE_TILEMAP;
    return tm;
}
