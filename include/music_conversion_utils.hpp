#pragma once

#include <stdlib.h>
#include <filesystem>

void convert_audio_file(const std::filesystem::path& from_path, const std::filesystem::path& to_path)
{
    int value = system((std::string("ffmpeg -hide_banner -loglevel error -y -i '") + from_path.c_str() + "' '" + to_path.c_str() + "'").c_str());

    (void)value;
}