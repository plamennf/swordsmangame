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

Entity *Entity_Manager::get_entity_by_id(int id) {
    Entity **_e = entity_lookup.find(id);
    if (_e) return *_e;
    return NULL;
}

Guy *Entity_Manager::make_guy() {
    Guy *guy = new Guy();
    by_type._Guy.add(guy);
    register_entity(guy);
    guy->type = ENTITY_TYPE_GUY;

    guy->size = Vector2(1, 1);
    
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
    assert(!tilemap);
    
    Tilemap *tm = new Tilemap();
    tilemap = tm;
    register_entity(tm);
    tm->type = ENTITY_TYPE_TILEMAP;
    return tm;
}

Enemy *Entity_Manager::make_enemy() {
    Enemy *enemy = new Enemy();
    by_type._Enemy.add(enemy);
    register_entity(enemy);
    enemy->type = ENTITY_TYPE_ENEMY;

    enemy->size = Vector2(2, 2);
    
    return enemy;
}

Guy *Entity_Manager::get_active_hero() {
    for (Guy *guy : by_type._Guy) {
        if (guy->is_active) return guy;
    }
    return NULL;
}

void Entity_Manager::set_active_hero(Guy *guy) {
    for (Guy *g : by_type._Guy) {
        g->is_active = false;
    }
    guy->is_active = true;
}
