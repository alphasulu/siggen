clc;clear all;close all;

% Direct digital synthesizer (DDS) is a type of frequency synthesizer
% used for creating arbitrary waveforms from a single,
% fixed-frequency reference clock.
% Applications of DDS include: signal generation, local oscillators
% in communication systems, function generators, mixers, modulators,
% sound synthesizers and as part of a digital phase-locked loop


sintablen = 100000;
SINTAB = sin(2*pi*(0:sintablen-1)./sintablen);
fs = 44100;

% the sintable consists of one cycle of sine wave with 100000 samples
% if you access 100000 samples/sec (fs) from the above sintable,
% it will generate one complete cycle of sine wave in one sec.

% we need to generate 100Hz-10kHz sine waves.

for F_required = 100:10000

  index = 1; step = (F_required/fs)*sintablen;
  for i = 1:100000
      sin1Hz(i) = SINTAB(round(index));
      index = index+step;
      if index>sintablen
          index = index-sintablen;
      end
  end
  plot(sin1Hz); hold on;
end  