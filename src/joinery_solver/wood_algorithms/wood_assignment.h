#pragma once

#include "pch.h"

#include "wood_element_plate.h"
#include "wood_settings.h"

namespace wood_session {

/// Every plate's feature_types reset to -1 per slot (bottom, top, one per side), then each point's type written into the slot of every plate whose nearest outline segment lies within 10 x settings.distance: a negative type names the bottom or top face, a positive one the side; the absolute value is stored.
void assign_feature_types(const std::vector<std::shared_ptr<Plate>>& plates, const Settings& settings, const std::vector<session_cpp::Point>& points, const std::vector<int>& types);

/// Every plate's insertion vectors reset to zero per slot, then each line's vector written into the side slot of every plate whose outline segment nearest the line start lies within 10 x settings.distance.
void assign_insertion_vectors(const std::vector<std::shared_ptr<Plate>>& plates, const Settings& settings, const std::vector<session_cpp::Line>& lines);

} // namespace wood_session
