// Header
#include "enemy_02.hpp"

#include <cmath>
#include <algorithm>

Texture Enemy_02::enemy_texture;

bool Enemy_02::init(int level)
{
	// Load shared texture
	if (!enemy_texture.is_valid())
	{
		if (!enemy_texture.load_from_file(textures_path("enemy_02.png")))
		{
			fprintf(stderr, "Failed to load enemy texture!");
			return false;
		}
	}

	// The position corresponds to the center of the texture
	float wr = enemy_texture.width * 0.5f;
	float hr = enemy_texture.height * 0.5f;

	TexturedVertex vertices[4];
	vertices[0].position = { -wr, +hr, -0.02f };
	vertices[0].texcoord = { 0.f, 1.f };
	vertices[1].position = { +wr, +hr, -0.02f };
	vertices[1].texcoord = { 1.f, 1.f };
	vertices[2].position = { +wr, -hr, -0.02f };
	vertices[2].texcoord = { 1.f, 0.f };
	vertices[3].position = { -wr, -hr, -0.02f };
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
	if (gl_has_errors())
		return false;

	// Loading shaders
	if (!effect.load_from_file(shader_path("textured.vs.glsl"), shader_path("textured.fs.glsl")))
		return false;

	// Setting initial values, scale is negative to make it face the opposite way
	// 1.0 would be as big as the original texture
	m_scale.x = 0.8f;
	m_scale.y = 0.8f;
	m_rotation = 0.f;
	enemyRandMoveAngle = 0.f;
	randMovementTime = clock();
	m_is_alive = true;

	float f = (float)rand() / RAND_MAX;
    float randAttributeFactor = 1.0f + f * (2.0f - 1.0f);

	m_speed = std::min(70.0f + (float)level * 0.8f * randAttributeFactor, 400.0f);
	randMovementCooldown = std::max(1000.0 - (double)level * 2.5 * randAttributeFactor, 200.0);
	hp = std::min(50.0f + (float)level * 0.2f * randAttributeFactor, 80.f);
	deceleration = 1.0f;
	momentum_factor = 1.3f;
	momentum.x = 0.f;
	momentum.y = 0.f;
	m_level = level;
	poweredup = false;
	variation = 0.f;
	speedBoost = false;

	return true;
}

// Call if init() was successful
// Releases all graphics resources
void Enemy_02::destroy()
{
	glDeleteBuffers(1, &mesh.vbo);
	glDeleteBuffers(1, &mesh.ibo);
	glDeleteVertexArrays(1, &mesh.vao);
	effect.release();
}


void Enemy_02::draw(const mat3& projection)
{
	// Transformation code, see Rendering and Transformation in the template specification for more info
	// Incrementally updates transformation matrix, thus ORDER IS IMPORTANT
	transform_begin();
	transform_translate(m_position);

	if(m_face_left_or_right == 1){
		m_scale.x = 0.8f;
	} else {
		m_scale.x = -0.8f;
	}

	transform_scale(m_scale);
	transform_end();

	// Setting shaders
	glUseProgram(effect.program);

	// Enabling alpha channel for textures
	glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);

	// Getting uniform locations for glUniform* calls
	GLint transform_uloc = glGetUniformLocation(effect.program, "transform");
	GLint color_uloc = glGetUniformLocation(effect.program, "fcolor");
	GLint projection_uloc = glGetUniformLocation(effect.program, "projection");

	// Setting vertices and indices
	glBindVertexArray(mesh.vao);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ibo);

	// Input data location as in the vertex buffer
	GLint in_position_loc = glGetAttribLocation(effect.program, "in_position");
	GLint in_texcoord_loc = glGetAttribLocation(effect.program, "in_texcoord");
	glEnableVertexAttribArray(in_position_loc);
	glEnableVertexAttribArray(in_texcoord_loc);
	glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)0);
	glVertexAttribPointer(in_texcoord_loc, 2, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)sizeof(vec3));

	// Enabling and binding texture to slot 0
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, enemy_texture.id);

	// Setting uniform values to the currently bound program
	glUniformMatrix3fv(transform_uloc, 1, GL_FALSE, (float*)&transform);
	float color[] = { 1.f, 1.f, 1.f };
	if (poweredup){
		switch (powerupType)
		{
			case 0:
				color[0] = 0.2f;
				color[2] = 0.2f;
				break;
			case 1:
				color[1] = 0.2f;
				color[2] = 0.2f;
				break;
			case 2:
				color[0] = 1.f;
				color[1] = 0.f;
				color[2] = 1.f;
				break;
			case 3:
				color[0] = 0.f;
				color[1] = 0.f;
				color[2] = 1.f;
				break;
		}
	}
	glUniform3fv(color_uloc, 1, color);
	glUniformMatrix3fv(projection_uloc, 1, GL_FALSE, (float*)&projection);

	// Drawing!
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);
}

void Enemy_02::update(float ms, vec2 target_pos)
{
	//momentum first
	m_position.x += momentum.x;
	m_position.y += momentum.y;

	if (momentum.x > 0.5f) {
		momentum.x = std::max(momentum.x - deceleration, 0.f);
	}
	if (momentum.x < -0.5f) {
		momentum.x = std::min(momentum.x + deceleration, 0.f);
	}

	if (momentum.y > 0.5f) {
		momentum.y = std::max(momentum.y - deceleration, 0.f);
	}
	if (momentum.y < -0.5f) {
		momentum.y = std::min(momentum.y + deceleration, 0.f);
	}

	// Move fish along -X based on how much time has passed, this is to (partially) avoid
	// having entities move at different speed based on the machine.
	float x_diff =  m_position.x - target_pos.x;
	float y_diff =  m_position.y - target_pos.y;
	float distance = std::sqrt(x_diff * x_diff + y_diff * y_diff);
	float enemy_angle = atan2(y_diff, x_diff);
	float m_speed_rand_LO = m_speed * 0.8f;
	float m_speed_rand_HI = m_speed * 1.2f;
	float m_speed_rand = m_speed_rand_LO + static_cast <float> (rand()) /( static_cast <float> (RAND_MAX/(m_speed_rand_HI-m_speed_rand_LO)));
	if (speedBoost) {
		m_speed_rand = m_speed_rand * 2.f;
	}
	int facing = 1;
	if (x_diff > 0.0) {
		facing = 0;
	}
	set_facing(facing);
	set_rotation(enemy_angle);
	clock_t currentTime = clock();
	if (distance <= 100.f) {
		float step = -1.2f * m_speed_rand * (ms / 1000);
		m_position.x += cos(enemy_angle)*step;
		m_position.y += sin(enemy_angle)*step;
	} else if (distance <= 500.f) {
		float step = -m_speed_rand * (ms / 1000);
		float enemyRandMoveAngle_after_variation = enemyRandMoveAngle + sinf((timePassed - clock()) / 200.f) * variation;
		m_position.x += cos(enemyRandMoveAngle_after_variation)*step;
		m_position.y += sin(enemyRandMoveAngle_after_variation)*step;
	} else {
		float step = -m_speed_rand * (ms / 1000);
		m_position.x += cos(enemy_angle)*step;
		m_position.y += sin(enemy_angle)*step;
	}
	if (checkIfCanChangeDirectionOfMove(currentTime)){
		float LO = enemy_angle - 0.9f;
		float HI = enemy_angle + 0.9f;
		enemyRandMoveAngle = LO + static_cast <float> (rand()) /( static_cast <float> (RAND_MAX/(HI-LO)));
		setRandMovementTime(currentTime);
		if (powerupType == 2 || powerupType == 3) {
			speedBoost = !speedBoost;
		}
	}

}


bool Enemy_02::collide_with(Projectile &projectile)
{
	float dx = m_position.x - projectile.get_position().x;
	float dy = m_position.y - projectile.get_position().y;
	float d_sq = dx * dx + dy * dy;
	float other_r = std::max(projectile.get_bounding_box().x, projectile.get_bounding_box().y);
	float my_r = std::max(m_scale.x, m_scale.y);
	float r = std::max(other_r, my_r);
	r *= 1.f;
	if (d_sq < r * r)
			return true;
	return false;

}

bool Enemy_02::checkIfCanFire(clock_t currentClock)
{
	return false;
}

void Enemy_02::powerup()
{
	if (!poweredup) {
		powerupType = 0 + ( std::rand() % ( 3 - 0 + 1 ));
		float f = (float)rand() / RAND_MAX;
		float randAttributeFactor = 1.0f + f * (2.0f - 1.0f);
		switch(powerupType){
			// green
			case 0:
				hp = hp * std::min(1.5f + (float)m_level * 0.05f * randAttributeFactor, 3.5f);
				break;
			// red
			case 1:
				m_speed = m_speed * 2.f;
				break;
			// purple
			case 2:
			 	timePassed = clock();
				variation = std::min(0.4f + (float)m_level * 0.05f * randAttributeFactor, 1.f);
				break;
			// blue
			case 3:
				m_speed = m_speed * 1.2f;
				randMovementCooldown = randMovementCooldown / 2;
				deceleration = deceleration / 2;
		}
		poweredup = true;
	}
}