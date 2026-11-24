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


/*
 * FabGL Synth: a polyphonic synthesizer played with the PS/2 keyboard (or the mouse).
 *
 * Keyboard layout (like most music trackers):
 *
 *      W   E       T   Y   U       O   P       ]/+
 *    A   S   D   F   G   H   J   K   L   ;/ò  '/à
 *    C   D   E   F   G   A   B   C   D   E    F
 *
 *    Z / X  : octave down / up
 *    1 .. 5 : select waveform
 *
 * Each note is played by a voice: a WaveformGenerator followed by a resonant low-pass filter
 * (adjustable cutoff and resonance) and shaped by an ADSR envelope.
 * Up to MAX_VOICES notes can sound at the same time.
 */


#include <math.h>

#include "fabgl.h"
#include "fabui.h"


using fabgl::uiControl;
using fabgl::uiEvent;
using fabgl::uiOrigin;
using fabgl::Delegate;


fabgl::VGA16Controller DisplayController;
fabgl::PS2Controller   PS2Controller;
SoundGenerator         soundGenerator;



////////////////////////////////////////////////////////////////////////////////////////////////////
// ADSR envelope parameters (shared by all voices)


static constexpr int ENV_MAX = 1 << 24;   // envelope full level (fixed point)


struct Envelope {
  volatile int attackInc  = ENV_MAX;
  volatile int decayInc   = ENV_MAX;
  volatile int sustain    = ENV_MAX;
  volatile int releaseInc = ENV_MAX;

  // ms -> level increment per sample (from 0 to ENV_MAX)
  static int msToInc(int ms) {
    int samples = ms * FABGL_SOUNDGEN_DEFAULT_SAMPLE_RATE / 1000;
    return samples > 0 ? ENV_MAX / samples : ENV_MAX;
  }

  void setAttack(int ms)          { attackInc  = msToInc(ms); }
  void setDecay(int ms)           { decayInc   = msToInc(ms); }
  void setSustain(int percent)    { sustain    = (int64_t) ENV_MAX * percent / 100; }
  void setRelease(int ms)         { releaseInc = msToInc(ms); }
};

Envelope envelope;



////////////////////////////////////////////////////////////////////////////////////////////////////
// Resonant low-pass filter parameters (shared by all voices)
//
// Second order (12 dB/octave) "RBJ cookbook" biquad. Coefficients are computed in floating point by
// the UI task and used in fixed point (Q28) by the sound generator interrupt, where the FPU can't be used.
// Two coefficient sets are kept: the UI writes the inactive one and then swaps them, so the interrupt
// never sees a half-updated set.


struct LowPassFilter {

  static constexpr int COEFF_SHIFT = 28;

  static constexpr int MIN_CUTOFF  = 50;     // Hz
  static constexpr int MAX_CUTOFF  = 7000;   // Hz (sample rate is 16384 Hz, Nyquist is 8192 Hz)
  static constexpr float MIN_Q     = 0.707f; // no resonance (Butterworth)
  static constexpr float MAX_Q     = 12.0f;

  struct Coeffs {
    int32_t b0, b1, b2, a1, a2;
  };

  Coeffs       sets[2];
  volatile int active  = 0;
  volatile bool enabled = true;
  int          cutoff  = 2000;  // Hz
  float        q       = 2.0f;

  LowPassFilter() { update(); }

  void setCutoff(int value)    { cutoff = value; update(); }
  void setResonance(float value) { q = value; update(); }

  void update()
  {
    float w0    = 2.0f * (float) M_PI * cutoff / FABGL_SOUNDGEN_DEFAULT_SAMPLE_RATE;
    float cosw  = cosf(w0);
    float alpha = sinf(w0) / (2.0f * q);
    float a0    = 1.0f + alpha;
    float scale = (float) (1 << COEFF_SHIFT) / a0;

    int next = active ^ 1;
    Coeffs & c = sets[next];
    c.b0 = (int32_t) lroundf((1.0f - cosw) * 0.5f * scale);
    c.b1 = (int32_t) lroundf((1.0f - cosw) * scale);
    c.b2 = c.b0;
    c.a1 = (int32_t) lroundf(-2.0f * cosw * scale);
    c.a2 = (int32_t) lroundf((1.0f - alpha) * scale);
    active = next;
  }

  // slider position (0..100) <-> logarithmic scales
  static int cutoffFromSlider(int pos)  { return (int) lroundf(MIN_CUTOFF * powf((float) MAX_CUTOFF / MIN_CUTOFF, pos / 100.0f)); }
  static float qFromSlider(int pos)     { return MIN_Q * powf(MAX_Q / MIN_Q, pos / 100.0f); }
};

LowPassFilter lowPassFilter;



////////////////////////////////////////////////////////////////////////////////////////////////////
// Voice: a waveform generator (sine, square, ...) shaped by the ADSR envelope


enum class Waveform { Sine, Square, Pulse, Triangle, Sawtooth };

static const char * WaveformNames[] = { "Sine", "Square", "Pulse 25%", "Triangle", "Sawtooth" };
static constexpr int WaveformsCount = sizeof(WaveformNames) / sizeof(WaveformNames[0]);


class Voice : public WaveformGenerator {

public:

  enum class State { Idle, Attack, Decay, Sustain, Release };

  Voice()
  {
    m_pulse.setDutyCycle(64);
    m_gen = &m_sine;
  }

  // must be called after attaching the voice to the sound generator (sample rate is known)
  void setWaveform(Waveform waveform)
  {
    WaveformGenerator * gens[] = { &m_sine, &m_square, &m_pulse, &m_triangle, &m_sawtooth };
    auto gen = gens[(int)waveform];
    gen->setSampleRate(sampleRate());
    gen->setVolume(127);
    gen->setFrequency(m_frequency);
    m_gen = gen;
  }

  void setFrequency(int value)
  {
    m_frequency = value;
    m_gen->setFrequency(value);
  }

  void noteOn(int note, int frequency)
  {
    m_note = note;
    setFrequency(frequency);
    if (m_state == State::Idle)
      m_x1 = m_x2 = m_y1 = m_y2 = 0;  // clear filter state (safe: the interrupt doesn't touch idle voices)
    m_state = State::Attack;  // a retriggered voice restarts from its current level (no clicks)
    enable(true);
  }

  void noteOff()
  {
    if (m_state != State::Idle)
      m_state = State::Release;
  }

  int note()      { return m_note; }
  State state()   { return m_state; }
  bool isIdle()   { return m_state == State::Idle; }

  int getSample()
  {
    switch (m_state) {
      case State::Attack:
        m_level += envelope.attackInc;
        if (m_level >= ENV_MAX) {
          m_level = ENV_MAX;
          m_state = State::Decay;
        }
        break;
      case State::Decay:
        m_level -= envelope.decayInc;
        if (m_level <= envelope.sustain) {
          m_level = envelope.sustain;
          m_state = State::Sustain;
        }
        break;
      case State::Sustain:
        m_level = envelope.sustain;   // follows the sustain slider while the key is held
        break;
      case State::Release:
        m_level -= envelope.releaseInc;
        if (m_level <= 0) {
          m_level = 0;
          m_state = State::Idle;
          m_note  = -1;
          enable(false);
        }
        break;
      case State::Idle:
        return 0;
    }

    int sample = m_gen->getSample();                  // -128..127 (generator volume = 127)
    if (lowPassFilter.enabled)
      sample = filter(sample);
    sample = sample * volume() / 127;                 // voice volume
    return (sample * (m_level >> 16)) >> 8;           // envelope (0..256)
  }

private:

  // biquad low-pass filter, direct form I. Samples are scaled by 256 to keep precision.
  int filter(int in)
  {
    auto const & c = lowPassFilter.sets[lowPassFilter.active];
    int32_t x0 = in << 8;
    int64_t acc = (int64_t) c.b0 * x0 + (int64_t) c.b1 * m_x1 + (int64_t) c.b2 * m_x2
                - (int64_t) c.a1 * m_y1 - (int64_t) c.a2 * m_y2;
    int32_t y0 = (int32_t) (acc >> LowPassFilter::COEFF_SHIFT);
    y0 = fabgl::iclamp(y0, -(1 << 20), (1 << 20));         // safety net against runaway (never reached when stable)
    m_x2 = m_x1;
    m_x1 = x0;
    m_y2 = m_y1;
    m_y1 = y0;
    return fabgl::iclamp(y0 >> 8, -128, 127);              // resonance can exceed the 8 bit range: clip
  }

  SineWaveformGenerator     m_sine;
  SquareWaveformGenerator   m_square;
  SquareWaveformGenerator   m_pulse;
  TriangleWaveformGenerator m_triangle;
  SawtoothWaveformGenerator m_sawtooth;

  WaveformGenerator * m_gen;
  int                 m_frequency = 0;
  int                 m_note      = -1;
  volatile State      m_state     = State::Idle;
  int                 m_level     = 0;

  // filter state
  int32_t             m_x1 = 0, m_x2 = 0, m_y1 = 0, m_y2 = 0;
};



////////////////////////////////////////////////////////////////////////////////////////////////////
// Synth: a pool of voices


class Synth {

public:

  static constexpr int MAX_VOICES = 8;

  void begin()
  {
    for (auto & v : m_voices) {
      soundGenerator.attach(&v);
      v.setWaveform(m_waveform);
    }
    soundGenerator.play(true);
  }

  void setWaveform(Waveform value)
  {
    m_waveform = value;
    bool playing = soundGenerator.play(false);
    for (auto & v : m_voices)
      v.setWaveform(value);
    soundGenerator.play(playing);
  }

  Waveform waveform() { return m_waveform; }

  // note: MIDI note number (60 = C4, 69 = A4 = 440 Hz)
  void noteOn(int note)
  {
    Voice * voice = nullptr;
    // same note still sounding (releasing)? reuse its voice
    for (auto & v : m_voices)
      if (v.note() == note) {
        voice = &v;
        break;
      }
    // free voice?
    if (!voice)
      for (auto & v : m_voices)
        if (v.isIdle()) {
          voice = &v;
          break;
        }
    // all voices busy: steal a releasing one, otherwise the oldest one
    if (!voice)
      for (auto & v : m_voices)
        if (v.state() == Voice::State::Release) {
          voice = &v;
          break;
        }
    if (!voice)
      voice = &m_voices[m_nextSteal++ % MAX_VOICES];

    voice->noteOn(note, noteFrequency(note));
  }

  void noteOff(int note)
  {
    for (auto & v : m_voices)
      if (v.note() == note && v.state() != Voice::State::Release)
        v.noteOff();
  }

  static int noteFrequency(int note)
  {
    return (int) lroundf(440.0f * powf(2.0f, (note - 69) / 12.0f));
  }

private:

  Voice    m_voices[MAX_VOICES];
  Waveform m_waveform  = Waveform::Sawtooth;
  int      m_nextSteal = 0;
};

Synth synth;



////////////////////////////////////////////////////////////////////////////////////////////////////
// Note names


static const char * NoteNames[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

// MIDI note -> "C4"
static void noteName(int note, char * str, size_t len)
{
  snprintf(str, len, "%s%d", NoteNames[note % 12], note / 12 - 1);
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// PianoKeyboard: on screen keyboard control (two octaves plus the final C)


class PianoKeyboard : public uiControl {

public:

  static constexpr int KEYS        = 25;   // semitones displayed (C..C'')
  static constexpr int WHITE_KEYS  = 15;

  // keyboard source flags
  static constexpr uint8_t SRC_KEYBOARD = 1;
  static constexpr uint8_t SRC_MOUSE    = 2;

  PianoKeyboard(uiWindow * parent, const Point & pos, const Size & size)
    : uiControl(parent, pos, size, true)
  {
    windowStyle().borderSize  = 0;
    windowProps().focusable   = false;   // keyboard events are handled by the main frame
    memset(m_pressed, 0, sizeof(m_pressed));
  }

  // base note (the leftmost C)
  void setBaseNote(int value)
  {
    releaseAll();
    m_baseNote = value;
    repaint();
  }

  int baseNote() { return m_baseNote; }

  // index: 0..KEYS-1
  void press(int index, uint8_t source)
  {
    if (index < 0 || index >= KEYS || (m_pressed[index] & source))
      return; // ignore typematic repeats
    bool wasPressed = m_pressed[index];
    m_pressed[index] |= source;
    if (!wasPressed) {
      synth.noteOn(m_baseNote + index);
      repaintKey(index);
      onNote(m_baseNote + index);
    }
  }

  void release(int index, uint8_t source)
  {
    if (index < 0 || index >= KEYS || !(m_pressed[index] & source))
      return;
    m_pressed[index] &= ~source;
    if (!m_pressed[index]) {
      synth.noteOff(m_baseNote + index);
      repaintKey(index);
    }
  }

  void releaseAll()
  {
    for (int i = 0; i < KEYS; ++i)
      if (m_pressed[i]) {
        m_pressed[i] = 0;
        synth.noteOff(m_baseNote + i);
      }
    m_mouseKey = -1;
    repaint();
  }

  // keyboard letters to show on the keys (nullptr = none)
  void setKeyLabels(char const * const * labels) { m_keyLabels = labels; }

  // called when a note starts playing
  Delegate<int> onNote;

  virtual void processEvent(uiEvent * event)
  {
    uiControl::processEvent(event);

    switch (event->id) {

      case fabgl::UIEVT_PAINT:
        beginPaint(event, uiControl::clientRect(uiOrigin::Window));
        paintKeyboard();
        break;

      case fabgl::UIEVT_MOUSEBUTTONDOWN:
        if (event->params.mouse.changedButton == 1) {
          m_mouseKey = keyAt(event->params.mouse.status.X, event->params.mouse.status.Y);
          press(m_mouseKey, SRC_MOUSE);
        }
        break;

      case fabgl::UIEVT_MOUSEMOVE:
        // glissando: dragging the mouse with the button pressed changes note
        if (m_mouseKey >= 0 && app()->capturedMouseWindow() == this) {
          int key = keyAt(event->params.mouse.status.X, event->params.mouse.status.Y);
          if (key != m_mouseKey) {
            release(m_mouseKey, SRC_MOUSE);
            m_mouseKey = key;
            press(m_mouseKey, SRC_MOUSE);
          }
        }
        break;

      case fabgl::UIEVT_MOUSEBUTTONUP:
        if (event->params.mouse.changedButton == 1) {
          release(m_mouseKey, SRC_MOUSE);
          m_mouseKey = -1;
        }
        break;

      default:
        break;
    }
  }

private:

  static bool isBlack(int index)
  {
    static const bool black[12] = { false, true, false, true, false, false, true, false, true, false, true, false };
    return black[index % 12];
  }

  int whiteWidth()  { return clientSize().width / WHITE_KEYS; }
  int blackWidth()  { return whiteWidth() * 2 / 3; }
  int blackHeight() { return clientSize().height * 6 / 10; }

  // number of white keys before "index"
  static int whiteIndex(int index)
  {
    static const int whites[12] = { 0, 1, 1, 2, 2, 3, 4, 4, 5, 5, 6, 6 };
    return index / 12 * 7 + whites[index % 12];
  }

  Rect keyRect(int index)
  {
    int ww = whiteWidth();
    if (isBlack(index)) {
      int x = whiteIndex(index) * ww - blackWidth() / 2;
      return Rect(x, 0, x + blackWidth() - 1, blackHeight() - 1);
    } else {
      int x = whiteIndex(index) * ww;
      return Rect(x, 0, x + ww - 1, clientSize().height - 1);
    }
  }

  int keyAt(int x, int y)
  {
    // black keys are over white keys: check them first
    for (int i = 0; i < KEYS; ++i)
      if (isBlack(i) && keyRect(i).contains(x, y))
        return i;
    for (int i = 0; i < KEYS; ++i)
      if (!isBlack(i) && keyRect(i).contains(x, y))
        return i;
    return -1;
  }

  void repaintKey(int index)
  {
    // a white key is partially covered by black keys: repaint the whole keyboard area of the key
    repaint(keyRect(index));
  }

  void paintKey(int index)
  {
    Rect r = keyRect(index);
    bool black   = isBlack(index);
    bool pressed = m_pressed[index] != 0;

    if (black) {
      canvas()->setBrushColor(pressed ? RGB888(0, 0, 255) : RGB888(0, 0, 0));
      canvas()->fillRectangle(r);
      canvas()->setPenColor(RGB888(64, 64, 64));
      canvas()->drawRectangle(r);
    } else {
      canvas()->setBrushColor(pressed ? RGB888(0, 255, 255) : RGB888(255, 255, 255));
      canvas()->fillRectangle(r);
      canvas()->setPenColor(RGB888(0, 0, 0));
      canvas()->drawRectangle(r);
    }

    auto font = &fabgl::FONT_std_12;
    canvas()->setGlyphOptions(GlyphOptions().FillBackground(false));

    // keyboard letter
    if (m_keyLabels && m_keyLabels[index]) {
      char const * lbl = m_keyLabels[index];
      int tx = r.X1 + (r.width() - canvas()->textExtent(font, lbl)) / 2;
      canvas()->setPenColor(black ? RGB888(255, 255, 255) : RGB888(0, 0, 255));
      canvas()->drawText(font, tx, r.Y2 - font->height - (black ? 4 : 24), lbl);
    }

    // note name on white keys
    if (!black) {
      char name[16];
      noteName(m_baseNote + index, name, sizeof(name));
      int tx = r.X1 + (r.width() - canvas()->textExtent(font, name)) / 2;
      canvas()->setPenColor(RGB888(128, 128, 128));
      canvas()->drawText(font, tx, r.Y2 - font->height - 4, name);
    }
  }

  void paintKeyboard()
  {
    // background (area not covered by keys)
    canvas()->setBrushColor(RGB888(64, 64, 64));
    canvas()->fillRectangle(uiControl::clientRect(uiOrigin::Window));
    // white keys first, then black keys on top
    for (int i = 0; i < KEYS; ++i)
      if (!isBlack(i))
        paintKey(i);
    for (int i = 0; i < KEYS; ++i)
      if (isBlack(i))
        paintKey(i);
  }

  int                  m_baseNote  = 60;   // C4
  uint8_t              m_pressed[KEYS];    // SRC_xxx flags
  int                  m_mouseKey  = -1;
  char const * const * m_keyLabels = nullptr;
};



////////////////////////////////////////////////////////////////////////////////////////////////////
// PS/2 keyboard mapping


// virtual key -> keyboard index (semitones from the leftmost C), -1 = not a note key
static int keyToIndex(VirtualKey vk)
{
  using namespace fabgl;
  switch (vk) {
    case VK_a: case VK_A:                   return 0;   // C
    case VK_w: case VK_W:                   return 1;   // C#
    case VK_s: case VK_S:                   return 2;   // D
    case VK_e: case VK_E:                   return 3;   // D#
    case VK_d: case VK_D:                   return 4;   // E
    case VK_f: case VK_F:                   return 5;   // F
    case VK_t: case VK_T:                   return 6;   // F#
    case VK_g: case VK_G:                   return 7;   // G
    case VK_y: case VK_Y:                   return 8;   // G#
    case VK_h: case VK_H:                   return 9;   // A
    case VK_u: case VK_U:                   return 10;  // A#
    case VK_j: case VK_J:                   return 11;  // B
    case VK_k: case VK_K:                   return 12;  // C
    case VK_o: case VK_O:                   return 13;  // C#
    case VK_l: case VK_L:                   return 14;  // D
    case VK_p: case VK_P:                   return 15;  // D#
    case VK_SEMICOLON: case VK_COLON:       return 16;  // E  (US layout)
    case VK_GRAVE_o:                        return 16;  // E  (IT layout)
    case VK_QUOTE: case VK_QUOTEDBL:        return 17;  // F  (US layout)
    case VK_GRAVE_a:                        return 17;  // F  (IT layout)
    case VK_RIGHTBRACKET:                   return 18;  // F# (US layout)
    case VK_PLUS:                           return 18;  // F# (IT layout)
    default:                                return -1;
  }
}


// labels drawn on the on-screen keys
static char const * KeyLabels[PianoKeyboard::KEYS] = {
  "A", "W", "S", "E", "D", "F", "T", "G", "Y", "H", "U", "J",
  "K", "O", "L", "P", ";", "'", "]",
  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
};



////////////////////////////////////////////////////////////////////////////////////////////////////
// Main frame


class SynthFrame : public uiFrame {

public:

  SynthFrame(uiWindow * parent)
    : uiFrame(parent, "FabGL Synth", Point(0, 0), parent->clientSize())
  {
    frameProps().resizeable        = false;
    frameProps().moveable          = false;
    frameProps().hasCloseButton    = false;
    frameProps().hasMaximizeButton = false;
    frameProps().hasMinimizeButton = false;
    frameStyle().backgroundColor   = RGB888(255, 255, 255);

    int y = 30;

    // waveform
    new uiStaticLabel(this, "Waveform", Point(10, y + 1));
    m_waveformCombo = new uiComboBox(this, Point(80, y), Size(110, 20), 85);
    for (int i = 0; i < WaveformsCount; ++i)
      m_waveformCombo->items().appendFmt("%d - %s", i + 1, WaveformNames[i]);
    m_waveformCombo->selectItem((int) synth.waveform());
    m_waveformCombo->onChange = [&]() {
      synth.setWaveform((Waveform) m_waveformCombo->selectedItem());
    };

    // octave
    new uiStaticLabel(this, "Octave", Point(220, y + 1));
    auto octDown = new uiButton(this, "Z  <", Point(270, y), Size(40, 20));
    octDown->onClick = [&]() { changeOctave(-1); };
    m_octaveLabel = new uiLabel(this, "", Point(318, y + 1), Size(30, 18));
    m_octaveLabel->labelStyle().textAlign = uiHAlign::Center;
    auto octUp = new uiButton(this, ">  X", Point(352, y), Size(40, 20));
    octUp->onClick = [&]() { changeOctave(+1); };

    // master volume
    m_volumeLabel = new uiLabel(this, "", Point(420, y + 1), Size(80, 18));
    auto volumeSlider = new uiSlider(this, Point(500, y + 1), Size(125, 18), uiOrientation::Horizontal);
    volumeSlider->onChange = [=, this]() {
      m_volumeLabel->setTextFmt("Volume %d", volumeSlider->position());
      soundGenerator.setVolume(volumeSlider->position());
    };
    volumeSlider->setup(0, 127, 16);
    volumeSlider->setPosition(100);

    y = 70;

    // ADSR envelope
    auto envPanel = addPanel("Envelope (ADSR)", Point(10, y), Size(250, 150));
    addSlider(envPanel, 10,  "Attack",  0, 1000,  10, [](int v, uiLabel * l) { l->setTextFmt("%d ms", v); envelope.setAttack(v); });
    addSlider(envPanel, 70,  "Decay",   0, 1000, 200, [](int v, uiLabel * l) { l->setTextFmt("%d ms", v); envelope.setDecay(v); });
    addSlider(envPanel, 130, "Sustain", 0, 100,   70, [](int v, uiLabel * l) { l->setTextFmt("%d %%", v); envelope.setSustain(v); });
    addSlider(envPanel, 190, "Release", 0, 2000, 300, [](int v, uiLabel * l) { l->setTextFmt("%d ms", v); envelope.setRelease(v); });

    // resonant low-pass filter (sliders 0..100 on logarithmic scales)
    auto filterPanel = addPanel("Low-pass filter", Point(270, y), Size(190, 150));
    addSlider(filterPanel, 12, "Cutoff", 0, 100, 70, [](int v, uiLabel * l) {
      int cutoff = LowPassFilter::cutoffFromSlider(v);
      l->setTextFmt("%d Hz", cutoff);
      lowPassFilter.setCutoff(cutoff);
    });
    addSlider(filterPanel, 72, "Resonance", 0, 100, 37, [](int v, uiLabel * l) {
      float q = LowPassFilter::qFromSlider(v);
      int q10 = (int) lroundf(q * 10);
      l->setTextFmt("Q %d.%d", q10 / 10, q10 % 10);
      lowPassFilter.setResonance(q);
    });
    new uiStaticLabel(filterPanel, "On", Point(150, 24));
    auto filterCheck = new uiCheckBox(filterPanel, Point(149, 44), Size(17, 17));
    filterCheck->setChecked(lowPassFilter.enabled);
    filterCheck->onChange = [=]() {
      lowPassFilter.enabled = filterCheck->checked();
    };

    // info panel
    auto infoPanel = addPanel("Now playing", Point(470, y), Size(160, 150));
    m_noteLabel = new uiLabel(infoPanel, "-", Point(10, 28), Size(140, 36));
    m_noteLabel->labelStyle().textFont  = &fabgl::FONT_std_24;
    m_noteLabel->labelStyle().textAlign = uiHAlign::Center;
    m_noteLabel->labelStyle().textColor = RGB888(0, 0, 255);
    m_freqLabel = new uiLabel(infoPanel, "", Point(10, 66), Size(140, 20));
    m_freqLabel->labelStyle().textAlign = uiHAlign::Center;
    new uiStaticLabel(infoPanel, "Keys: A W S E D F ...", Point(10, 94));
    new uiStaticLabel(infoPanel, "Z/X: octave  1-5: wave", Point(10, 110));
    new uiStaticLabel(infoPanel, "or click with the mouse", Point(10, 126));

    // on screen keyboard
    m_piano = new PianoKeyboard(this, Point(20, 235), Size(600, 230));
    m_piano->setKeyLabels(KeyLabels);
    m_piano->onNote = [&](int note) {
      char name[16];
      noteName(note, name, sizeof(name));
      m_noteLabel->setText(name);
      m_freqLabel->setTextFmt("%d Hz", Synth::noteFrequency(note));
    };

    updateOctaveLabel();

    // PS/2 keyboard
    onKeyDown = [&](uiKeyEventInfo const & key) {
      int index = keyToIndex(key.VK);
      if (index >= 0) {
        m_piano->press(index, PianoKeyboard::SRC_KEYBOARD);
        return;
      }
      switch (key.VK) {
        case fabgl::VK_z: case fabgl::VK_Z:
          changeOctave(-1);
          break;
        case fabgl::VK_x: case fabgl::VK_X:
          changeOctave(+1);
          break;
        case fabgl::VK_1: case fabgl::VK_2: case fabgl::VK_3: case fabgl::VK_4: case fabgl::VK_5:
          selectWaveform(key.VK - fabgl::VK_1);
          break;
        default:
          break;
      }
    };

    onKeyUp = [&](uiKeyEventInfo const & key) {
      int index = keyToIndex(key.VK);
      if (index >= 0)
        m_piano->release(index, PianoKeyboard::SRC_KEYBOARD);
    };
  }

private:

  // a fixed sub-frame that groups controls
  uiFrame * addPanel(char const * title, Point const & pos, Size const & size)
  {
    auto panel = new uiFrame(this, title, pos, size);
    panel->frameProps().resizeable        = false;
    panel->frameProps().moveable          = false;
    panel->frameProps().hasCloseButton    = false;
    panel->frameProps().hasMaximizeButton = false;
    panel->frameProps().hasMinimizeButton = false;
    panel->windowProps().activable        = false;   // keep the main frame active (it receives the keyboard)
    return panel;
  }

  // vertical slider with title and value label. "apply" receives the slider position and the value label
  void addSlider(uiWindow * parent, int x, char const * title, int min, int max, int value, void (*apply)(int, uiLabel *))
  {
    new uiStaticLabel(parent, title, Point(x, 24));
    auto label  = new uiLabel(parent, "", Point(x, 126), Size(56, 18));
    auto slider = new uiSlider(parent, Point(x + 12, 42), Size(17, 80), uiOrientation::Vertical);
    slider->setup(min, max, (max - min) / 10);
    slider->setPosition(value);
    slider->onChange = [=]() {
      apply(slider->position(), label);
    };
    apply(value, label);
  }

  void selectWaveform(int index)
  {
    if (index >= 0 && index < WaveformsCount) {
      m_waveformCombo->selectItem(index);   // selectItem() doesn't fire onChange
      synth.setWaveform((Waveform) index);
    }
  }

  void changeOctave(int delta)
  {
    int base = m_piano->baseNote() + delta * 12;
    // leftmost C from C1 (MIDI 24) to C6 (MIDI 84): the rightmost key reaches C8
    if (base >= 24 && base <= 84) {
      m_piano->setBaseNote(base);
      updateOctaveLabel();
    }
  }

  void updateOctaveLabel()
  {
    m_octaveLabel->setTextFmt("%d", m_piano->baseNote() / 12 - 1);
  }

  PianoKeyboard * m_piano;
  uiComboBox *    m_waveformCombo;
  uiLabel *       m_octaveLabel;
  uiLabel *       m_volumeLabel;
  uiLabel *       m_noteLabel;
  uiLabel *       m_freqLabel;
};



////////////////////////////////////////////////////////////////////////////////////////////////////
// Application


class SynthApp : public uiApp {

  void init()
  {
    synth.begin();

    auto frame = new SynthFrame(rootWindow());
    showWindow(frame, true);
    setActiveWindow(frame);
    setFocusedWindow(frame);
  }

} app;


extern "C" void app_main()
{
  PS2Controller.begin(PS2Preset::KeyboardPort0_MousePort1, KbdMode::GenerateVirtualKeys);

  DisplayController.begin();
  DisplayController.setResolution(VGA_640x480_60Hz);

  // adjust this to center screen in your monitor
  //DisplayController.moveScreen(20, -2);

  while (true)
    app.run(&DisplayController);
}
