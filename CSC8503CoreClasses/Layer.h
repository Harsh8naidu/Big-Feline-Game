#pragma once

namespace NCL {
	enum class Layer {

		// these enums are now used for tutorials to check whether the layering system works with simple shapes
		Floor = 0,
		Sphere,
		Box,

		// these enums are now used for tutorials to check whether the layering system works with simple shapes
		Player,
		Enemy,
		Collectible,
		Obstacle,

		IgnoreRaycast = 2,
		Default = -1
	};

	// Define layer mask for raycasting
	using LayerMask = unsigned int;

	constexpr LayerMask DefaultLayerMask = static_cast<LayerMask>(-1); // Default layer mask is all layers

	// Define layer mask for raycasting
	constexpr LayerMask IgnoreRaycastLayerMask = ~(1 << static_cast<int>(Layer::IgnoreRaycast));
}