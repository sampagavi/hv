#pragma once
#include <vector>
#include <ftxui/component/component.hpp>
#include "../../include/types.hpp"

ftxui::Component MakeAnomalyPanel(const std::vector<AnomalyEvent>* events);
