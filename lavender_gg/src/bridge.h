// bridge.h | C++17 | lavender.gg | localhost HTTP bridge between the overlay and the
// Roblox Studio client script. Roblox POSTs its own state here (HttpService); the overlay
// reads it. The overlay queues admin/movement commands here; Roblox GETs and clears them.
#pragma once
#include <nlohmann/json.hpp>
#include <httplib.h>
#include <thread>
#include <mutex>
#include <atomic>
#include <vector>
#include <string>
#include <chrono>

class RobloxBridge {
public:
    explicit RobloxBridge(std::string host = "127.0.0.1", int port = 7777);
    ~RobloxBridge();

    void start();
    void stop();
    bool is_running() const { return running_; }

    // overlay -> roblox
    void push_command(const nlohmann::json& cmd);
    void set_visuals(const nlohmann::json& visuals);
    nlohmann::json get_visuals() const;

    // roblox -> overlay
    nlohmann::json get_latest_state() const;
    bool has_fresh_state() const;

private:
    void server_thread_fn();

    std::string host_;
    int port_;
    std::atomic<bool> running_{false};
    std::thread thread_;
    httplib::Server svr_;

    mutable std::mutex state_mutex_;
    nlohmann::json latest_state_ = nlohmann::json::object();
    std::chrono::steady_clock::time_point last_state_time_{};

    mutable std::mutex visuals_mutex_;
    nlohmann::json visuals_ = nlohmann::json::object();

    mutable std::mutex cmd_mutex_;
    std::vector<nlohmann::json> command_queue_;
};
