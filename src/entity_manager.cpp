#include "pch.h"
#include "entity_manager.h"
#include "entities.h"
#include "animation.h"
#include "game.h"

#include "animation_registry.h"

void Entity_Manager::register_entity(Entity *e, int id) {
    if (id == -1) {
        entity_lookup.add(next_entity_id, e);
        e->id = next_entity_id;
        next_entity_id += 1;
    } else {
        entity_lookup.add(id, e);
        e->id = id;
        next_entity_id = id + 1;
    }
    all_entities.add(e);
    e->manager = this;
}

Entity *Entity_Manager::get_entity_by_id(int id) {
    Entity **_e = entity_lookup.find(id);
    if (_e) return *_e;
    return NULL;
}

Entity *Entity_Manager::add_entity(Entity *source, int id) {
    s64 size = 0;
    Entity *e = NULL;
    switch (source->type) {
        case ENTITY_TYPE_GUY: {
            e = (Entity *)new Guy();
            size = sizeof(Guy);
            by_type._Guy.add((Guy *)e);
        }

        case ENTITY_TYPE_ENEMY: {
            e = (Entity *)new Enemy();
            size = sizeof(Enemy);
            by_type._Enemy.add((Enemy *)e);
        }

        case ENTITY_TYPE_THUMBLEWEED: {
            e = (Entity *)new Thumbleweed();
            size = sizeof(Thumbleweed);
            by_type._Thumbleweed.add((Thumbleweed *)e);
        }

        case ENTITY_TYPE_TREE: {
            e = (Entity *)new Tree();
            size = sizeof(Tree);
            by_type._Tree.add((Tree *)e);
        }
    }

    memcpy(e, source, size);
    register_entity(e, id);
    return e;
}

Guy *Entity_Manager::make_guy(int id) {
    Guy *guy = new Guy();
    by_type._Guy.add(guy);
    register_entity(guy, id);
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

Thumbleweed *Entity_Manager::make_thumbleweed(int id) {
    Thumbleweed *thumbleweed = new Thumbleweed();
    by_type._Thumbleweed.add(thumbleweed);
    register_entity(thumbleweed, id);
    thumbleweed->type = ENTITY_TYPE_THUMBLEWEED;

    thumbleweed->size = Vector2(1, 1);

    thumbleweed->idle_animation = globals.animation_registry->get("thumbleweed_idle");
    thumbleweed->moving_animation = globals.animation_registry->get("thumbleweed_moving");
    thumbleweed->attack_animation = globals.animation_registry->get("thumbleweed_attack");
    thumbleweed->transformation_animation = globals.animation_registry->get("thumbleweed_transformation");

    thumbleweed->current_animation = thumbleweed->idle_animation;

    return thumbleweed;
}

Light_Source *Entity_Manager::make_light_source(int id) {
    Light_Source *source = new Light_Source();
    by_type._Light_Source.add(source);
    register_entity(source, id);
    source->type = ENTITY_TYPE_LIGHT_SOURCE;

    return source;
}

Tilemap *Entity_Manager::make_tilemap(int id) {
    assert(!tilemap);
    
    Tilemap *tm = new Tilemap();
    tilemap = tm;
    register_entity(tm, id);
    tm->type = ENTITY_TYPE_TILEMAP;
    return tm;
}

Tree *Entity_Manager::make_tree(int id) {
    Tree *tree = new Tree();
    by_type._Tree.add(tree);
    register_entity(tree, id);
    tree->type = ENTITY_TYPE_TREE;

    tree->size.y = 3.0f;
    tree->size.x = tree->size.y * 0.833333f;
    
    tree->current_animation = globals.animation_registry->get("tree");
    
    return tree;
}

Enemy *Entity_Manager::make_enemy(int id) {
    Enemy *enemy = new Enemy();
    by_type._Enemy.add(enemy);
    register_entity(enemy, id);
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
