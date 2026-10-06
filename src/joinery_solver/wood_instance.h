#pragma once

#include "pch.h"

using namespace session_cpp;

namespace wood_session {

/// The class key of a column, beam, block or plate, and the right-handed frame that maps its canonical local copy onto it; numbers in thousandths, so a mirror or a near miss keys apart; nullopt for any other type or a degenerate frame.
std::optional<std::pair<std::string, Xform>> element_key(const Element& element);

} // namespace wood_session
