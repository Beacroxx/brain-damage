#pragma once

#include "main.hpp"

template <typename EventType>
dpp::task<void> updateStarboardMessage(custom_cluster &bot, const EventType &event);