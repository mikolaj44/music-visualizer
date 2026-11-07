#include <chrono>
#include <filesystem>
#include <numbers>
#include <stdexcept>

#include <SFML/Audio/SoundSource.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/System/Vector2.hpp>

#include "music_visualizer.hpp"
#include "dft_utils.hpp"
#include "music_conversion_utils.hpp"

namespace
{
    sf::Color move_to_color(const sf::Color& from, const sf::Color& to, size_t step)
    {
        uint8_t r = (from.r < to.r ? std::min<uint16_t>(255, (static_cast<uint16_t>(from.r) + static_cast<uint16_t>(step))) : std::max<int16_t>(0, static_cast<int16_t>(from.r) - static_cast<int16_t>(step)));
        uint8_t g = (from.g < to.g ? std::min<uint16_t>(255, (static_cast<uint16_t>(from.g) + static_cast<uint16_t>(step))) : std::max<int16_t>(0, static_cast<int16_t>(from.g) - static_cast<int16_t>(step)));
        uint8_t b = (from.b < to.b ? std::min<uint16_t>(255, (static_cast<uint16_t>(from.b) + static_cast<uint16_t>(step))) : std::max<int16_t>(0, static_cast<int16_t>(from.b) - static_cast<int16_t>(step)));

        return {r, g, b};
    }
}

music_visualizer::music_visualizer(const visualizer_config& config) : config(config)
{
    init_visualizer();
}

music_visualizer::~music_visualizer()
{
    if(conversion_happened)
    {
        std::filesystem::remove(converted_path);
    }

    is_destroyed_ = true;

    main_thread.join();
}

void music_visualizer::init_visualizer()
{
    const auto& from_path = config.audio_path;

    constexpr auto wav_extension = ".wav";
    constexpr auto filename_suffix = "_audio_visualizer";

    if(from_path.extension() != wav_extension)
    {
        auto filename = from_path.filename();

        filename = from_path.filename().replace_filename(from_path.stem().string() + filename_suffix);

        converted_path = std::filesystem::temp_directory_path() / filename.replace_extension(wav_extension);

        conversion_happened = true;

        convert_audio_file(from_path, converted_path);
    }
    else
    {
        converted_path = from_path;
    }

    if(!audio_file.load(converted_path))
    {
        throw std::runtime_error{"[AudioFile] failed to load audio file"};
    }

    window = {sf::VideoMode({config.window_width, config.window_height}), ""};
    texture = sf::Texture{{config.window_width, config.window_height}};

    if(!window.setActive(false))
    {
        throw std::runtime_error{"[SFML] failed to deactivate the window"};
    }

    if(!music.openFromFile(converted_path))
    {
        throw std::runtime_error{"[SFML] failed to load audio file"};
    }

    main_thread = std::thread([this] {this->run();});
}

void music_visualizer::start()
{
    is_running_ = true;
}

void music_visualizer::stop()
{
    is_running_ = false;

    music.pause();
}

bool music_visualizer::is_running()
{
    return is_running_;
}

bool music_visualizer::try_window_close()
{
    while(const std::optional event = window.pollEvent())
    {
        if(event->is<sf::Event::Closed>())
        {
            window.close();

            return true;
        }
    }

    return false;
}

void music_visualizer::run()
{
    using namespace std::chrono;

	size_t total_num_samples = static_cast<size_t>(audio_file.getNumSamplesPerChannel());

    size_t frame_index = 0;
	uint32_t sample_rate = audio_file.getSampleRate();
	uint32_t fft_size = round_up_to_pow_2(sample_rate / config.framerate);

    auto frame_duration = microseconds(static_cast<size_t>((static_cast<double>(fft_size) / sample_rate) * 1e6));

    if(!window.setActive(true))
    {
        throw std::runtime_error{"[SFML] failed to activate the window"};
    }

    music.play();

    previous_frame_ts = system_clock::now();
    next_frame_ts = previous_frame_ts + frame_duration;

    size_t sample_idx = 0;

    bool window_is_closed = false;

    while(window.isOpen() && !is_destroyed_)
    {
        if(try_window_close())
        {
            break;
        }

        for(sample_idx = 0; sample_idx < total_num_samples; sample_idx += fft_size)
        {
            if(try_window_close())
            {
                window_is_closed = true;

                break;
            }

            if(!is_running_ || is_destroyed_)
            {
                break;
            }

            if(music.getStatus() == sf::SoundSource::Status::Paused)
            {
                music.play();
            }

            auto chunk = std::span<double>(audio_file.samples[config.channel_idx].begin() + sample_idx, fft_size);

            while(system_clock::now() < next_frame_ts) {};

            auto result = fft(chunk);

            if(config.center_symmetric_waveform)
            {
                swap_halves(result);
            }

            draw_frame(result, frame_index++);

            next_frame_ts += frame_duration;
        }

        if(sample_idx >= total_num_samples || is_destroyed_ || window_is_closed)
        {
            break;
        }
    }

    music.stop();
    window.close();

    is_running_ = false;
}

void music_visualizer::draw_frame(const std::span<std::complex<double>> dft_result, [[maybe_unused]] size_t frame_index)
{
    if(is_destroyed_ || !is_running_)
    {
        return;
    }

    if(config.type == visualization_type::WAVEFORM)
    {
        draw_frame_waveform(dft_result, frame_index);
    }
    else if(config.type == visualization_type::BAR)
    {
        draw_frame_bar(dft_result, frame_index);
    }
    else if(config.type == visualization_type::CIRCLE)
    {
        draw_frame_circle(dft_result, frame_index);
    }
}

void music_visualizer::draw_frame_waveform(const std::span<std::complex<double>> dft_result, [[maybe_unused]] size_t frame_index)
{
    sf::VertexArray lines(sf::PrimitiveType::LineStrip, dft_result.size());

    float x_scale = static_cast<float>(config.window_width) / dft_result.size();

    size_t num_amplitudes_above_threshold = 0;

    for(size_t i = 0; i < dft_result.size(); ++i)
    {
        float amplitude = std::abs(dft_result[i]);

        if(amplitude > config.dynamic_background_ampltude_threshold)
        {
            num_amplitudes_above_threshold++;
        }

        lines[i].color = config.foreground_color;
        lines[i].position = sf::Vector2f(i * x_scale, config.window_height - (amplitude * config.vertical_scale));
    }

    clear_background(num_amplitudes_above_threshold);

    window.draw(lines);
    window.display();
}

void music_visualizer::draw_frame_bar(const std::span<std::complex<double>> dft_result, [[maybe_unused]] size_t frame_index)
{
    size_t samples_per_bar = (dft_result.size() + config.bar_num_bars - 1) / config.bar_num_bars;

    float bar_width = static_cast<float>(config.window_width) / (static_cast<float>(dft_result.size()) / samples_per_bar);

    std::vector<sf::RectangleShape> rects;
    rects.resize(config.bar_num_bars);

    window.clear();

    size_t rect_index = 0;
    size_t num_amplitudes_above_threshold = 0;

    for(size_t i = 0; i < dft_result.size() - samples_per_bar; i += samples_per_bar)
    {
        size_t max_amplitude = 0;

        for(size_t j = i; j < i + samples_per_bar; ++j)
        {
            float amplitude = std::abs(dft_result[j]);

            if(amplitude > config.dynamic_background_ampltude_threshold)
            {
                num_amplitudes_above_threshold++;
            }

            max_amplitude = std::max<size_t>(max_amplitude, amplitude);
        }

        if(is_destroyed_ || !is_running_)
        {
            return;
        }

        rects[rect_index].setFillColor(config.foreground_color);
        rects[rect_index].setOutlineColor(config.foreground_color);

        rects[rect_index].setSize({bar_width, -static_cast<float>(max_amplitude * config.vertical_scale)});
        rects[rect_index].setPosition({static_cast<float>(rect_index * bar_width), static_cast<float>(config.window_height)});

        rect_index++;
    }

    clear_background(num_amplitudes_above_threshold);

    for(const auto& rect : rects)
    {
        window.draw(rect);
    }

    window.display();
}

void music_visualizer::draw_frame_circle(const std::span<std::complex<double>> dft_result, [[maybe_unused]] size_t frame_index)
{
    static float angle_offset = 0;

    float angle_step = 2 * std::numbers::pi / dft_result.size();

    sf::VertexArray outer_circle(sf::PrimitiveType::LineStrip, dft_result.size());
    sf::VertexArray inner_circle(sf::PrimitiveType::LineStrip, dft_result.size());

    size_t vertex_index = 0;
    size_t num_amplitudes_above_threshold = 0;

    for(float angle = 0; angle < 2 * std::numbers::pi; angle += angle_step)
    {
        float curr_angle = angle + config.circle_angle_offset + angle_offset;

        float amplitude = std::abs(dft_result[vertex_index]);

        if(amplitude > config.dynamic_background_ampltude_threshold)
        {
            num_amplitudes_above_threshold++;
        }

        // outer circle
        float spike = amplitude * config.radial_scale;
        float radius = config.base_radius + spike;

        float x = radius * cos(curr_angle) + config.window_width / 2;
        float y = radius * sin(curr_angle) + config.window_height / 2;

        outer_circle[vertex_index].color = config.foreground_color;
        outer_circle[vertex_index].position = sf::Vector2f(x, y);

        // inner circle
        radius = config.base_radius - spike / config.inner_circle_spike_height_div;

        x = radius * cos(curr_angle) + config.window_width / 2;
        y = radius * sin(curr_angle) + config.window_height / 2;

        inner_circle[vertex_index].color = config.foreground_color;
        inner_circle[vertex_index].position = sf::Vector2f(x, y);

        vertex_index++;
    }

    clear_background(num_amplitudes_above_threshold);

    window.draw(outer_circle);
    window.draw(inner_circle);
    window.display();

    angle_offset += config.circle_spin_angle_increment;
}

void music_visualizer::clear_background(size_t num_amplitudes_above_threshold)
{
    window.clear(move_to_color(config.background_color, config.dynamic_background_color, num_amplitudes_above_threshold));
}
