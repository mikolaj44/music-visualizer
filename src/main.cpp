#include <csignal>
#include <memory>
#include <stdexcept>

#include <AudioFile/AudioFile.h>
#include <CLI/CLI.hpp>

#include "music_visualizer.hpp"

std::unique_ptr<music_visualizer> visualizer;

volatile bool should_exit = false;

void handler(int)
{
	should_exit = true;
};

int main(int argc, char* argv[])
{
	CLI::App app{"Simple music visualizer for audio files"};
    argv = app.ensure_utf8(argv);

	visualizer_config config;

	std::map<std::string, visualization_type> map{{"WAVEFORM", visualization_type::WAVEFORM}, {"BAR", visualization_type::BAR}, {"CIRCLE", visualization_type::CIRCLE}};

	std::array<unsigned int, 3> foreground_color{255, 255, 255};
	std::array<unsigned int, 3> background_color{0, 0, 0};
	std::array<unsigned int, 3> dynamic_background_color{0, 0, 0};

    app.add_option("path", config.audio_path, "Path to the audio file");

	app.add_option("--type", config.type, "Visualizer type")->transform(CLI::CheckedTransformer(map, CLI::ignore_case));;
    app.add_option("--fps", config.framerate, "Framerate (integer)")->default_val(60);

	app.add_option("--window-width", config.window_width, "Window width")->default_val(640);
    app.add_option("--window-height", config.window_height, "Window height")->default_val(480);

	app.add_option("--fg-color", foreground_color, "Foreground color")->capture_default_str();
    app.add_option("--bg-color", background_color, "Background color")->capture_default_str();

	app.add_option("--center-symmetric", config.center_symmetric_waveform, "Make peaks focus in the center (visible on WAVEFORM or BAR)")->default_val(true);
    app.add_option("--channel-idx", config.channel_idx, "Audio channel index")->default_val(0);

	app.add_option("--num-bars", config.bar_num_bars, "Number of bars in BAR")->default_val(100);
    app.add_option("--vertical-scale", config.vertical_scale, "Vertical size multiplier for BAR and WAVEFORM")->default_val(30.0);

	app.add_option("--radial-scale", config.radial_scale, "Spike size multiplier for CIRCLE")->default_val(5.0);
    app.add_option("--base-radius", config.base_radius, "Base circle radius for CIRCLE")->default_val(100.0);
	app.add_option("--circle-initial-angle", config.circle_angle_offset, "Starting angle of CIRCLE")->default_val(0);
	app.add_option("--circle-spin-increment", config.circle_spin_angle_increment, "Angle added in CIRCLE each frame to make it spin")->default_val(0.01);
    app.add_option("--inner-circle-spike-div", config.inner_circle_spike_height_div, "Ratio between the top and bottom spikes in CIRCLE")->default_val(2.0);

	app.add_option("--dynamic-bg-color", dynamic_background_color, "The color that starts appearing when the amplitude threshold is exceeded")->capture_default_str();
    app.add_option("--dynamic-bg-amp-threshold", config.dynamic_background_ampltude_threshold,
		"The dft amplitude that has to be exceeded in order to increment the background color towards the dynamic background color (each value from dft that exceeds adds or subtracts 1 from that color to move it towards the dynamic color)"
	)->default_val(10);

    CLI11_PARSE(app, argc, argv);

	config.foreground_color = sf::Color{static_cast<uint8_t>(foreground_color[0]), static_cast<uint8_t>(foreground_color[1]), static_cast<uint8_t>(foreground_color[2])};
	config.background_color = sf::Color{static_cast<uint8_t>(background_color[0]), static_cast<uint8_t>(background_color[1]), static_cast<uint8_t>(background_color[2])};
	config.dynamic_background_color = sf::Color{static_cast<uint8_t>(dynamic_background_color[0]), static_cast<uint8_t>(dynamic_background_color[1]), static_cast<uint8_t>(dynamic_background_color[2])};

	signal(SIGINT, &handler);
	signal(SIGTERM, &handler);

	try
	{
		visualizer = std::make_unique<music_visualizer>(config);

		visualizer->start();
	}
	catch(const std::runtime_error& error)
	{
		std::cout << error.what() << "\n";

		return 1;
	}

	while(!should_exit && visualizer->is_running()) {};

	return 0;
}
