### Music visualizer

Small weekend project, works on any audio file (converts to `.wav` on input based on the file extension), performs FFT on audio chunks on the fly and displays 3 different types of visualizations (waveform, bars and circle).

#### Dependencies:

- [SFML](https://www.sfml-dev.org/) for displying the visualizations and playing audio
- [AudioFile](https://github.com/adamstark/AudioFile) for reading audio samples from WAV files
- [cli11](https://github.com/cliutils/cli11) for the command line interface
- [ffmpeg](https://www.ffmpeg.org/) for creating a temporary `.wav` file if the one provided is not in the right fornat, if it is then it's not needed

#### Usage:

Display help:

```bash
./music_visualizer --help
./music_visualizer -h
```

#### Example visualizations:

Circle

<img width="767" height="597" src="https://i.imgur.com/jyp90KT.png"/>

```bash
./music-visualizer path/to/song.ext --type CIRCLE --fg-color 255 0 255 --inner-circle-spike-div 20 --radial-scale 3.5 --circle-spin-increment 3.14159265359 --circle-initial-angle 1.57079632679 --window-width 1920 --window-height 1080
```

Waveform

<img width="624" height="474" src="https://i.imgur.com/3PopmzP.png"/>

```bash
./music-visualizer path/to/song.ext --type WAVEFORM --vertical-scale 6
```

Bar

<img width="634" height="480" src="https://i.imgur.com/ducHlAF.png"/>

```bash
./music-visualizer path/to/song.ext --type BAR --vertical-scale 8 --num-bars 200 --fg-color 166 214 8
```

