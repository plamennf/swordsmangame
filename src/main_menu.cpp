#include "pch.h"
#include "main_menu.h"
#include "render.h"
#include "draw_help.h"
#include "font.h"
#include "game.h"

static Dynamic_Font *big_font;
static Dynamic_Font *body_font;

enum Menu_Choices {
    MENU_CHOICE_RESUME,
    MENU_CHOICE_EXIT,
    MENU_CHOICE_COUNT,
};

static int current_menu_choice = 0;
static bool asking_for_quit_confirmation = false;

void init_menu_fonts() {
    if (!globals.render_height) return;
    
    int big_font_size = (int)(0.15f * globals.render_height);
    big_font = get_font_at_size("Teko-Regular", big_font_size);

    int body_font_size = (int)(0.09f * globals.render_height);
    body_font = get_font_at_size("DancingScript-Regular", body_font_size);
}

static void advance_choice(int delta) {
    current_menu_choice += delta;
    if (current_menu_choice >= MENU_CHOICE_COUNT) current_menu_choice = 0;
    if (current_menu_choice < 0) current_menu_choice = MENU_CHOICE_COUNT-1;

    asking_for_quit_confirmation = false;
}

void update_menu() {
    if (is_key_pressed(KEY_UP))   advance_choice(-1);
    if (is_key_pressed(KEY_DOWN)) advance_choice(+1);

    if (is_key_pressed(KEY_ENTER)) {
        if (current_menu_choice == MENU_CHOICE_RESUME) {
            toggle_menu();
        } else if (current_menu_choice == MENU_CHOICE_EXIT) {
            if (asking_for_quit_confirmation) globals.should_quit_game = true;
            else asking_for_quit_confirmation = true;
        }
    }
}

static void draw_item(int menu_choice, char *text, int y) {
    Dynamic_Font *font = body_font;
    int x = (globals.render_width - font->get_string_width_in_pixels(text)) / 2;
    
    float k = 0.4f;
    Vector4 color(k, k, k, 1);
    if (current_menu_choice == menu_choice) {
        color = Vector4(0.95f, 0.02f, 0.85f, 1.0f);
        int offset = font->character_height / 40;
        if (offset) {
            draw_text(font, text, x+offset, y-offset, Vector4(0, 0, 0, 1));
        }
    }
    draw_text(font, text, x, y, color);
}

void draw_menu() {
    if (!big_font) init_menu_fonts();
    
    set_render_targets(the_back_buffer, NULL);
    clear_color_target(the_back_buffer, 0.0f, 1.0f, 1.0f, 1.0f, globals.render_area);
    set_viewport(globals.render_area.x, globals.render_area.y, globals.render_width, globals.render_height);
    set_scissor(globals.render_area.x, globals.render_area.y, globals.render_width, globals.render_height);
    
    rendering_2d_right_handed(globals.render_width, globals.render_height);
    refresh_global_parameters();

    immediate_set_shader(globals.shader_text);
    
    {
        char *text = "Giovanni Yatsuro";
        int x = (globals.render_width - big_font->get_string_width_in_pixels(text)) / 2;
        int y = (int)(0.75f * globals.render_height);
        int offset = big_font->character_height / 40;
        draw_text(big_font, text, x+offset, y-offset, Vector4(0, 0, 0, 1));
        draw_text(big_font, text, x, y, Vector4(1, 1, 1, 1));
    }

    int y = (int)(0.55 * globals.render_height);
    char *text = "Resume";
    draw_item(MENU_CHOICE_RESUME, text, y);

    y -= body_font->character_height;
    text = "Exit";
    if (asking_for_quit_confirmation) text = "Exit? Are you sure?";
    draw_item(MENU_CHOICE_EXIT, text, y);
}

void toggle_menu() {
    if (globals.program_mode == PROGRAM_MODE_GAME) {
        globals.program_mode = PROGRAM_MODE_MENU;
    } else if (globals.program_mode == PROGRAM_MODE_MENU) {
        globals.program_mode = PROGRAM_MODE_GAME;
    }

    asking_for_quit_confirmation = false;
}
