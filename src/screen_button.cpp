#pragma once

#include "common.hpp"
#include "projectile.h"
#include "fireball.h"
#include "enemy_01.hpp"
#include "enemy_02.hpp"
#include "hero.hpp"

// actual used includes
#include "screen_button.hpp"

#include <cmath>
#include "string.h"




// static Texture button_texture;
Texture Button::button_texture;

typedef void(*ClickCallback)();
using namespace std;

// Gets the top-left corner coordinates, width, height, and message for the button, and returns a button
Button::Button(int x, int y, int w, int h, std::string path, std::string type, ClickCallback onClick) {
	if (path == "") {
		// empty string for path, therefore no path; regular button with only a type, and a white color probably
		// init((double)x, (double)y, (double)w, (double)h, type);
		fprintf(stderr, "CANNOT FIND BUTTON AT POSITION X= %d , Y= %d \n", x, y);
	}
	else {
		// we wouldn't care what the text says, unless we come back and decide we do.
		if (type == "") {
			fprintf(stderr, "BUTTON HAS NO TYPE AT POSITION X= %d , Y= %d \n", x, y);
		}
		else {
			init((double)x, (double)y, (double)w, (double)h, path, type, onClick);
		}
	}
}

// referenced help from https://codereview.stackexchange.com/questions/154623/custom-opengl-buttons
bool Button::init(double x, double y, double w, double h, std::string path1, std::string type1, ClickCallback onClick1) {
	try {
		left_corner = x;
		top_corner = y;
		width = w;
		height = h;
		path = path1;
		type = type1;
		onClick = onClick1;
	} catch (const char* e) { // possibly of type std::exception& ?
		fprintf(stderr, "ERROR OCCURRED IN SCREENBUTTON INIT!");
		fprintf(stderr, "error message : %s", e);
	}

	/* Generic loading code */
	// Load shared texture
	if (!button_texture.is_valid())
	{
		if (!button_texture.load_from_file(textures_path("button.png")))
		// TODO: Change static path to "path" variable! After first successful testing
		{
			fprintf(stderr, "Failed to load button texture! pathname: %s", path);
			return false;
		}
	}

	// The position corresponds to the center of the texture
	float wr = width / (double)2; // UserInterface_texture.width * 0.5f;
	float hr = height / (double)2; // UserInterface_texture.height * 0.5f;

	TexturedVertex vertices[4];
	vertices[0].position = { -wr, +hr, -0.01f };
	vertices[0].texcoord = { 0.f, 1.f };
	vertices[1].position = { +wr, +hr, -0.01f };
	vertices[1].texcoord = { 1.f, 1.f, };
	vertices[2].position = { +wr, -hr, -0.01f };
	vertices[2].texcoord = { 1.f, 0.f };
	vertices[3].position = { -wr, -hr, -0.01f };
	vertices[3].texcoord = { 0.f, 0.f };

	// counterclockwise as it's the default opengl front winding direction
	uint16_t indices[] = { 0, 3, 1, 1, 3, 2 };

	// Clearing errors
	gl_flush_errors();

	// Vertex Buffer creation
	glGenBuffers(1, &mesh.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(TexturedVertex) * 4, vertices, GL_STATIC_DRAW);

	// Index Buffer creation
	glGenBuffers(1, &mesh.ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint16_t) * 6, indices, GL_STATIC_DRAW);

	// Vertex Array (Container for Vertex + Index buffer)
	glGenVertexArrays(1, &mesh.vao);
	if (gl_has_errors()) {
		fprintf(stderr, "ERROR IN SCREENBUTTON INIT! GL HAS ERRORS");
		return false;
	}

	// Loading shaders
	if (!effect.load_from_file(shader_path("textured.vs.glsl"), shader_path("textured.fs.glsl")))
		return false;

	// Setting initial values
	m_scale.x = 1.f;
	m_scale.y = 1.f;
	m_position = { (float)w / 2.f, (float)h * 3.5f };
	m_rotation = 0.f;

	// Setting initial values, scale is negative to make it face the opposite way
	// 1.0 would be as big as the original texture
	set_color({ 1.0f,1.0f,1.0f });
	m_direction = { 0.f,0.f };
	m_light_up = 0;
	return true;
}



Button::~Button() {
}

void Button::destroy() {
	glDeleteBuffers(1, &mesh.vbo);
	glDeleteBuffers(1, &mesh.ibo);
	glDeleteBuffers(1, &mesh.vao);

	glDeleteShader(effect.vertex);
	glDeleteShader(effect.fragment);
	glDeleteShader(effect.program);
}

void Button::CheckClick(double mouse_x, double mouse_y) {
	if (mouse_x >= left_corner && mouse_x <= left_corner + width &&
		mouse_y >= top_corner && mouse_y <= top_corner + height) {
		onClick();
	}
}


void Button::set_color(vec3 in_color)
{
	float color[3] = { in_color.x,in_color.y,in_color.z };
	memcpy(m_color, color, sizeof(color));
}

/*
// Releases all associated resources
void destroy();

// If mouse is hovering above the button, we will light up the button.
void update(vec2 hp_mp, float zoom_factor);

// Renders the salmon
void draw(const mat3& projection)override;

// Light up the button when is_within_range()
void lightup();

// Checks to see if mouse is within range. To be used in update of mouse and mouse_onclick
bool is_within_range();
*/
