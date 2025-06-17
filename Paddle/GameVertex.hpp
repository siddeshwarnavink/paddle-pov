#pragma once

#include <glm/glm.hpp>

struct Vertex {
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec3 normal;
	glm::vec2 uv;

	bool operator==(const Vertex& other) const {
		return pos == other.pos && normal == other.normal && uv == other.uv;
	}
};

struct VertexHasher {
	std::size_t operator()(const Vertex& k) const noexcept {
		std::size_t h1 = std::hash<float>()(k.pos.x) ^ std::hash<float>()(k.pos.y) << 1 ^ std::hash<float>()(k.pos.z) << 2;
		std::size_t h2 = std::hash<float>()(k.normal.x) ^ std::hash<float>()(k.normal.y) << 1 ^ std::hash<float>()(k.normal.z) << 2;
		std::size_t h3 = std::hash<float>()(k.uv.x) ^ std::hash<float>()(k.uv.y) << 1;

		return h1 ^ (h2 << 1) ^ (h3 << 2);
	}
};