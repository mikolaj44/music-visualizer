#pragma once

#include <chrono>
#include <thread>

#include <AudioFile/AudioFile.h>

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <cstddef>

enum class visualization_type : uint8_t
{
    WAVEFORM,
    BAR,
    CIRCLE
};

struct visualizer_config
{
    size_t framerate = 60;

    visualization_type type = visualization_type::CIRCLE;

    // BAR / WAVEFORM params
    size_t bar_num_bars = 100;
    float vertical_scale = 30.0;

    // CIRCLE params
    float radial_scale = 5.0;
    float base_radius = 100.0;
    float circle_angle_offset = 0;
    float circle_spin_angle_increment = 0.01;
    float inner_circle_spike_height_div = 2.0;

    size_t channel_idx = 0;

    unsigned int window_width = 640;
    unsigned int window_height = 480;

    bool center_symmetric_waveform = true;

    sf::Color foreground_color = {255, 255, 255};
    sf::Color background_color = {0, 0, 0};
    sf::Color dynamic_background_color = background_color;

    size_t dynamic_background_ampltude_threshold = 10;

    std::filesystem::path audio_path = "";
};

class music_visualizer
{
public:
    music_visualizer(const visualizer_config& config);

    ~music_visualizer();

    void start();

    void stop();

    bool is_running();

private:
    void init_visualizer();

    void run();

    bool try_window_close();

    void draw_frame(const std::span<std::complex<double>> dft_result, size_t frame_index);

    void draw_frame_waveform(const std::span<std::complex<double>> dft_result, size_t frame_index);
    void draw_frame_bar(const std::span<std::complex<double>> dft_result, size_t frame_index);
    void draw_frame_circle(const std::span<std::complex<double>> dft_result, size_t frame_index);

    void clear_background(size_t num_amplitudes_above_threshold);

    AudioFile<double> audio_file;

    sf::RenderWindow window;
    sf::Texture texture;
    sf::Music music;

    std::chrono::system_clock::time_point previous_frame_ts;
    std::chrono::system_clock::time_point next_frame_ts;

    visualizer_config config;

    std::filesystem::path converted_path;
    bool conversion_happened = false;

    std::thread main_thread;

    std::atomic<bool> is_running_ = false;
    std::atomic<bool> is_destroyed_ = false;
};
