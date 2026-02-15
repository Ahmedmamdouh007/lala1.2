#pragma once

#include "crow.h"

namespace lab_routes {
    /// Register lab routes. Only responds when lab_mode_enabled is true; guard returns 404/403 otherwise.
    void register_routes(crow::SimpleApp& app, bool lab_mode_enabled);
}
