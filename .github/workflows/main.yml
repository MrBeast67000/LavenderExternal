// bridge.cpp | C++17 | lavender.gg
#include "bridge.h"

RobloxBridge::RobloxBridge(std::string host, int port)
    : host_(std::move(host)), port_(port) {
    visuals_ = {
        {"playerLabels", true},
        {"objectHighlight", true},
        {"tracers", false},
        {"tracerColor", {255, 166, 255}},
        {"highlightColor", {189, 140, 255}},
        {"opacity", 0.85},
        {"showDistance", true},
        {"fov", 70.0}
    };
}

RobloxBridge::~RobloxBridge() { stop(); }

void RobloxBridge::start() {
    if (running_) return;
    running_ = true;
    thread_ = std::thread(&RobloxBridge::server_thread_fn, this);
}

void RobloxBridge::stop() {
    if (!running_) return;
    svr_.stop();
    if (thread_.joinable()) thread_.join();
    running_ = false;
}

void RobloxBridge::server_thread_fn() {
    // Roblox LocalScript pushes its own observed state (player info, session info,
    // notifications) here every poll interval. this is the ONLY way the overlay knows
    // anything about the live game — nothing is read from Roblox's process.
    svr_.Post("/api/state", [this](const httplib::Request& req, httplib::Response& res) {
        try {
            auto body = nlohmann::json::parse(req.body);
            std::lock_guard<std::mutex> lk(state_mutex_);
            latest_state_ = body;
            last_state_time_ = std::chrono::steady_clock::now();
            res.set_content(R"({"ok":true})", "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(
                std::string(R"({"ok":false,"error":")") + e.what() + "\"}",
                "application/json");
        }
    });

    svr_.Get("/api/visuals", [this](const httplib::Request&, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(visuals_mutex_);
        res.set_content(visuals_.dump(), "application/json");
    });

    // one-shot commands (heal, respawn, teleport, setwalkspeed, ...). cleared on read
    // so a command never fires twice even if Roblox's poll loop jitters.
    svr_.Get("/api/commands", [this](const httplib::Request&, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(cmd_mutex_);
        nlohmann::json out = command_queue_;
        command_queue_.clear();
        res.set_content(out.dump(), "application/json");
    });

    svr_.listen(host_.c_str(), port_);
}

void RobloxBridge::push_command(const nlohmann::json& cmd) {
    std::lock_guard<std::mutex> lk(cmd_mutex_);
    command_queue_.push_back(cmd);
}

void RobloxBridge::set_visuals(const nlohmann::json& visuals) {
    std::lock_guard<std::mutex> lk(visuals_mutex_);
    visuals_ = visuals;
}

nlohmann::json RobloxBridge::get_visuals() const {
    std::lock_guard<std::mutex> lk(visuals_mutex_);
    return visuals_;
}

nlohmann::json RobloxBridge::get_latest_state() const {
    std::lock_guard<std::mutex> lk(state_mutex_);
    return latest_state_;
}

bool RobloxBridge::has_fresh_state() const {
    std::lock_guard<std::mutex> lk(state_mutex_);
    if (latest_state_.empty()) return false;
    auto age = std::chrono::steady_clock::now() - last_state_time_;
    return age < std::chrono::seconds(5);
}
