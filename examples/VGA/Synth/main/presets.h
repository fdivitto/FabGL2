/*
  Created by Fabrizio Di Vittorio (fdivitto2013@gmail.com) - <http://www.fabgl.com>
  Copyright (c) 2019-2026 Fabrizio Di Vittorio.
  All rights reserved.


* Please contact fdivitto2013@gmail.com if you need a commercial license.


* This library and related software is available under GPL v3.

  FabGL is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  FabGL is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with FabGL.  If not, see <http://www.gnu.org/licenses/>.
 */


#pragma once


// Synth presets: the factory sounds and the preset data, saved to NVS by the application



static constexpr int PRESETS_COUNT    = 64;
static constexpr int PRESET_NAME_SIZE = 17;   // 16 characters + terminator


// A preset stores the values of the controls: times in ms, levels and depths in percent, cutoff, resonance and
// LFO rate as knob positions (0..100, mapped by ResonantLowPassFilter::cutoffFromPosition() etc.)
struct Preset {
  char    name[PRESET_NAME_SIZE];
  uint8_t wave;                  // SynthWaveform
  int16_t attack, decay;         // amplitude envelope
  uint8_t sustain;
  int16_t release;
  uint8_t filterOn;
  uint8_t cutoff;                // knob position
  uint8_t resonance;             // knob position
  int8_t  envAmount;             // -100..100
  int16_t fAttack, fDecay;       // filter envelope
  uint8_t fSustain;
  int16_t fRelease;
  uint8_t lfoWave;               // LFOWaveform
  uint8_t lfoRate;               // knob position
  uint8_t lfoCutoff;             // depth, percent
  uint8_t lfoAmp;                // depth, percent

  // same sound (the name is not compared)
  bool sameSound(Preset const & p) const
  {
    return wave == p.wave && attack == p.attack && decay == p.decay && sustain == p.sustain && release == p.release &&
           filterOn == p.filterOn && cutoff == p.cutoff && resonance == p.resonance && envAmount == p.envAmount &&
           fAttack == p.fAttack && fDecay == p.fDecay && fSustain == p.fSustain && fRelease == p.fRelease &&
           lfoWave == p.lfoWave && lfoRate == p.lfoRate && lfoCutoff == p.lfoCutoff && lfoAmp == p.lfoAmp;
  }
};


// oscillator waveforms (SynthWaveform)
enum { SIN, SQR, PLS, TRI, SAW };

// LFO waveforms (LFOWaveform)
enum { L_TRI, L_SIN, L_SQR, L_SAW, L_RND };


// Knob positions, for reference:
//   cutoff     10 = 82 Hz, 20 = 134 Hz, 30 = 220 Hz, 40 = 361 Hz, 50 = 592 Hz, 60 = 970 Hz, 70 = 1.6 KHz,
//              80 = 2.6 KHz, 90 = 4.3 KHz, 100 = 7 KHz
//   resonance  above 93 the filter self-oscillates
//   LFO rate   30 = 0.5 Hz, 40 = 0.8 Hz, 50 = 1.4 Hz, 60 = 2.4 Hz, 70 = 4.1 Hz, 75 = 5.3 Hz, 80 = 6.9 Hz, 90 = 11.7 Hz

static const Preset FactoryPresets[PRESETS_COUNT] = {
  //  name                wave   A     D    S     R  filt cut res  env    fA    fD  fS    fR   lfo  rate lcut lamp

  // brass
  { "BRASS SECTION",      SAW,  30,  300,  80,  250, 1, 35, 20,  45,   60,  500, 40,  300, L_SIN, 70,  0,  0 },
  { "SYNTH BRASS",        SAW,  10,  400,  70,  300, 1, 30, 30,  55,   20,  350, 30,  300, L_SIN, 70,  0,  0 },
  { "SOFT HORNS",         SAW,  80,  500,  85,  400, 1, 32, 10,  35,  150,  600, 45,  400, L_SIN, 70,  0,  0 },
  { "TRUMPET",            SAW,  15,  200,  85,  150, 1, 45, 15,  35,   40,  250, 50,  150, L_SIN, 75,  4,  0 },
  { "BRASS STAB",         SAW,   2,  250,   0,  200, 1, 30, 35,  55,    1,  200,  0,  150, L_SIN, 70,  0,  0 },
  { "POLY BRASS",         SAW,  20,  300,  75,  350, 1, 40, 25,  40,   30,  450, 35,  350, L_SIN, 72,  3,  0 },
  { "FANFARE",            SAW,   5,  150,  90,  300, 1, 50, 20,  40,   10,  200, 60,  300, L_SIN, 70,  0,  0 },
  { "LOW BRASS",          SAW,  40,  400,  80,  300, 1, 25, 20,  40,   80,  500, 40,  300, L_SIN, 70,  0,  0 },

  // strings and pads
  { "STRING ENSEMBLE",    SAW, 400,  500,  90,  900, 1, 55,  5,  10,  500,  800, 80,  900, L_TRI, 60,  0,  8 },
  { "SLOW STRINGS",       SAW, 900,  600,  90, 1500, 1, 50,  5,  15,  900,  900, 70, 1500, L_TRI, 60,  0,  5 },
  { "WARM PAD",           SAW, 700,  800,  85, 1500, 1, 35, 25,  20,  900, 1000, 60, 1500, L_SIN, 45,  8,  0 },
  { "GLASS PAD",          TRI, 600,  700,  80, 1800, 1, 60, 40,  15,  700,  900, 50, 1800, L_SIN, 50,  6,  0 },
  { "SWEEP PAD",          SAW, 600, 1000,  80, 1800, 1, 25, 55,   0,  600, 1000, 50, 1800, L_TRI, 30, 35,  0 },
  { "CHOIR",              TRI, 500,  600,  85, 1200, 1, 45, 45,  10,  500,  800, 70, 1200, L_SIN, 70,  3,  5 },
  { "DARK PAD",           SAW, 800,  900,  90, 2000, 1, 25, 15,  10,  900, 1000, 60, 2000, L_SIN, 40,  5,  0 },
  { "PWM PAD",            PLS, 500,  700,  85, 1500, 1, 50, 10,  15,  600,  900, 60, 1500, L_TRI, 50,  0, 10 },
  { "SPACE PAD",          SQR, 900, 1000,  80, 2000, 1, 40, 60,  25, 1000, 1000, 40, 2000, L_SIN, 35, 25,  0 },
  { "VOX HUMANA",         TRI, 300,  500,  85,  800, 1, 40, 50,  15,  400,  600, 60,  800, L_SIN, 72,  6,  6 },

  // leads
  { "SAW LEAD",           SAW,   5,  200,  90,  150, 1, 70, 30,  15,    5,  300, 70,  150, L_SIN, 75,  0,  0 },
  { "SQUARE LEAD",        SQR,   5,  200,  90,  150, 1, 65, 25,  15,    5,  300, 70,  150, L_SIN, 75,  0,  0 },
  { "RESO LEAD",          SAW,   5,  200,  90,  200, 1, 40, 75,  40,    5,  400, 40,  200, L_SIN, 75,  0,  0 },
  { "MINI LEAD",          SAW,  10,  300,  85,  200, 1, 50, 45,  35,   10,  350, 50,  200, L_SIN, 76,  3,  0 },
  { "SINE WHISTLE",       SIN,  30,  100,  95,  300, 0, 70,  0,   0,    5,  300, 50,  300, L_SIN, 75,  0, 10 },
  { "FLUTE",              TRI,  80,  150,  85,  200, 1, 60, 10,  10,   80,  200, 80,  200, L_SIN, 72,  0,  8 },
  { "OBOE",               PLS,  30,  200,  85,  150, 1, 55, 30,  10,   30,  200, 80,  150, L_SIN, 74,  0,  4 },
  { "CLARINET",           SQR,  40,  150,  90,  150, 1, 50, 15,   5,   40,  200, 80,  150, L_SIN, 74,  0,  3 },
  { "SCREAM LEAD",        SAW,   5,  300,  90,  200, 1, 55, 85,  30,    5,  500, 50,  200, L_SIN, 76,  4,  0 },
  { "ACID LEAD",          SAW,   2,  200,  60,  100, 1, 35, 80,  50,    2,  250, 10,  100, L_SIN, 70,  0,  0 },

  // bass
  { "MOOG BASS",          SAW,   2,  300,  70,   80, 1, 25, 35,  45,    2,  250, 10,   80, L_SIN, 70,  0,  0 },
  { "ACID BASS",          SAW,   2,  200,  60,   60, 1, 20, 88,  60,    1,  180,  0,   60, L_SIN, 70,  0,  0 },
  { "SUB BASS",           SIN,   5,  200,  95,  100, 0, 50,  0,   0,    5,  200, 50,  100, L_SIN, 70,  0,  0 },
  { "SQUARE BASS",        SQR,   2,  250,  70,   80, 1, 30, 20,  35,    2,  200, 10,   80, L_SIN, 70,  0,  0 },
  { "PLUCK BASS",         SAW,   1,  350,   0,  120, 1, 22, 30,  55,    1,  250,  0,  120, L_SIN, 70,  0,  0 },
  { "FAT BASS",           SAW,   5,  500,  80,  150, 1, 30, 25,  30,    5,  400, 20,  150, L_SIN, 70,  0,  0 },
  { "WOBBLE BASS",        SAW,   5,  300,  90,  100, 1, 20, 60,  10,    5,  300, 50,  100, L_SIN, 65, 40,  0 },
  { "RUBBER BASS",        PLS,   2,  300,  50,  100, 1, 25, 55,  50,    2,  300,  5,  100, L_SIN, 70,  0,  0 },
  { "FUNK BASS",          SAW,   1,  200,  40,   80, 1, 30, 50,  50,    1,  150,  0,   80, L_SIN, 70,  0,  0 },
  { "DEEP BASS",          TRI,   5,  400,  85,  200, 1, 25, 10,  15,    5,  400, 30,  200, L_SIN, 70,  0,  0 },

  // keys and organs
  { "ORGAN",              SQR,   5,   50, 100,   60, 0, 70,  0,   0,    5,   50, 50,   60, L_SIN, 75,  0,  5 },
  { "JAZZ ORGAN",         TRI,   5,   80, 100,   80, 0, 70,  0,   0,    5,   80, 50,   80, L_SIN, 74,  0,  8 },
  { "ELECTRIC PIANO",     TRI,   2, 1000,   0,  400, 1, 55, 10,  20,    1,  800, 10,  400, L_SIN, 70,  0, 10 },
  { "CLAVINET",           PLS,   1,  500,   0,  100, 1, 60, 35,  30,    1,  250, 10,  100, L_SIN, 70,  0,  0 },
  { "HARPSICHORD",        SAW,   1,  800,   0,  300, 1, 75, 10,  15,    1,  500,  0,  300, L_SIN, 70,  0,  0 },
  { "SYNTH PIANO",        SAW,   2, 1000,   0,  500, 1, 45, 10,  35,    1,  700,  0,  500, L_SIN, 70,  0,  0 },
  { "VIBES",              SIN,   2, 1000,   0,  800, 1, 70,  0,   0,    1,  500, 50,  800, L_SIN, 70,  0, 25 },
  { "ACCORDION",          PLS,  30,  100,  95,  150, 1, 65, 10,   5,   30,  150, 80,  150, L_SIN, 76,  0, 10 },

  // plucks and bells
  { "PLUCK",              SAW,   1,  400,   0,  300, 1, 30, 30,  60,    1,  250,  0,  300, L_SIN, 70,  0,  0 },
  { "HARP",               TRI,   1, 1000,   0,  700, 1, 50, 15,  30,    1,  600,  0,  700, L_SIN, 70,  0,  0 },
  { "KOTO",               PLS,   1,  600,   0,  400, 1, 40, 40,  45,    1,  300,  0,  300, L_SIN, 70,  0,  0 },
  { "MARIMBA",            SIN,   1,  400,   0,  300, 1, 60, 20,  20,    1,  150,  0,  200, L_SIN, 70,  0,  0 },
  { "BELL",               TRI,   1, 1000,   0, 1500, 1, 80, 50,  10,    1,  800, 30, 1500, L_SIN, 70,  0,  0 },
  { "MUSIC BOX",          SIN,   1,  800,   0,  900, 0, 70,  0,   0,    1,  500,  0,  900, L_SIN, 70,  0,  0 },
  { "STEEL DRUM",         TRI,   1,  600,   0,  400, 1, 45, 60,  30,    1,  250,  0,  300, L_SIN, 70,  0,  0 },
  { "PIZZICATO",          SAW,   1,  250,   0,  150, 1, 50, 10,  30,    1,  150,  0,  150, L_SIN, 70,  0,  0 },

  // sequencer sounds and effects
  { "ARP SYNTH",          SQR,   1,  200,  30,  150, 1, 45, 40,  35,    1,  200, 10,  150, L_SIN, 70,  0,  0 },
  { "BLIP",               SQR,   1,   80,   0,   60, 1, 70, 30,  20,    1,   80,  0,   60, L_SIN, 70,  0,  0 },
  { "SAMPLE AND HOLD",    SAW,   5,  300,  90,  300, 1, 40, 70,   0,    5,  300, 50,  300, L_RND, 75, 35,  0 },
  { "TREMOLO KEYS",       TRI,   2,  900,  40,  400, 1, 60, 10,  15,    2,  600, 20,  400, L_SIN, 72,  0, 60 },
  { "WAH SYNTH",          SAW,  10,  300,  85,  200, 1, 30, 70,   0,   10,  300, 50,  200, L_TRI, 55, 45,  0 },
  { "SIREN",              SQR,  50,  100, 100,  400, 1, 40, 80,   0,   50,  100, 50,  400, L_TRI, 35, 60,  0 },
  { "SPACE WIND",         SAW, 900, 1000,  90, 2000, 1, 30, 92,   0,  900, 1000, 50, 2000, L_RND, 50, 40,  0 },
  { "SCI-FI DRONE",       SQR, 1000, 1000, 100, 2000, 1, 25, 95,  0, 1000, 1000, 50, 2000, L_SIN, 30, 30,  0 },
  { "HELICOPTER",         SQR,   1,  100, 100,  100, 1, 40, 30,   0,    1,  100, 50,  100, L_SQR, 80,  0,100 },
  { "INIT",               SAW,   5,  200,  80,  300, 1, 70, 30,   0,    5,  400, 30,  300, L_TRI, 60,  0,  0 },
};
