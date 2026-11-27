#pragma once

#include <math.h>


/* The newer controls, connected to each other:

  - uiTileSelector: waveform of the graph (tiles painted with onPaintTile)
  - uiGraph: the waveform, with filled area
  - uiKnob: cycles and amplitude of the waveform
  - uiToggleSwitch: settings, "Animation" runs the activity leds and the progress bar
  - uiStepper: octave of the piano keyboard
  - uiLevelBar: volume (editable with mouse, wheel and keys)
  - uiLedBar: activity leds (4 states per led)
  - uiProgressBar
  - uiPianoKeyboard: play it with the mouse, the played note is shown on the right
*/


struct TestNewControlsFrame : public uiFrame {

  static constexpr int GraphPoints = 120;
  static constexpr int Leds        = 10;

  uiTileSelector * waveTiles;
  uiGraph *        graph;
  uiKnob *         cyclesKnob, * amplitudeKnob;
  uiToggleSwitch * fillSwitch, * wifiSwitch, * bluetoothSwitch, * animationSwitch;
  uiStepper *      octaveStepper;
  uiLevelBar *     volumeBar;
  uiLedBar *       activityLeds;
  uiProgressBar *  progressBar;
  uiLabel *        noteLabel;
  uiPianoKeyboard * piano;

  uiTimerHandle    timer;
  int              octave;
  int              progress;
  int              ledPhase;

  TestNewControlsFrame(uiFrame * parent)
    : uiFrame(parent, "New Controls", Point(20, 14), Size(600, 408), false, uiThemeStyle::Frame),
      piano(nullptr),
      timer(nullptr),
      octave(4),
      progress(0),
      ledPhase(0)
  {
    int x0 = clientPos().X + 10;
    int y0 = clientPos().Y + 8;

    createWaveformCard(x0, y0);
    createSettingsCard(x0 + 386, y0);
    createStatusCard(x0, y0 + 224);

    // piano keyboard, 3 octaves
    piano = new uiPianoKeyboard(this, Point(x0, y0 + 274), Size(576, 96), 36, true, uiThemeStyle::PianoKeyboard);
    piano->setBaseNote(octaveBaseNote());
    piano->onNoteOn = [&](int note) {
      char name[8];
      noteLabel->setTextFmt("%s  (%d)", fabgl::midiNoteName(note, name, sizeof(name)), note);
    };
    piano->onNoteOff = [&](int note) {
      noteLabel->setText("-");
    };

    updateGraph();

    // the animation runs only while the window is visible
    onShow = [&]() { timer = app()->setTimer(this, 80); };
    onHide = [&]() {
      if (timer)
        app()->killTimer(timer);
      timer = nullptr;
    };
    onTimer = [&](uiTimerHandle tHandle) { animate(); };
  }

  uiPanel * createCard(int x, int y, int w, int h, char const * title) {
    auto card = new uiPanel(this, Point(x, y), Size(w, h), true, uiThemeStyle::Card);
    new uiStaticLabel(card, title, Point(10, 6), true, uiThemeStyle::LabelTitle);
    return card;
  }

  void createWaveformCard(int x, int y) {
    auto card = createCard(x, y, 378, 216, "Waveform");

    waveTiles = new uiTileSelector(card, Point(10, 30), Size(356, 48), true, uiThemeStyle::TileSelector);
    waveTiles->items().appendSepList("Sine;Square;Triangle;Sawtooth", ';');
    waveTiles->selectItem(0);
    waveTiles->onPaintTile = [&](int index, Rect const & r, bool selected) { paintWaveIcon(index, r, selected); };
    waveTiles->onChange = [&]() { updateGraph(); };

    graph = new uiGraph(card, Point(10, 86), Size(228, 120), true, uiThemeStyle::Graph);
    graph->graphProps().fillArea   = true;
    graph->graphProps().hGridLines = 3;
    graph->graphProps().vGridLines = 5;
    graph->setRange(-110, 110);

    cyclesKnob = new uiKnob(card, "CYCLES", Point(246, 84), Size(58, 76), true, uiThemeStyle::Knob);
    cyclesKnob->setup(1, 8, 2);
    cyclesKnob->onChange = [&]() {
      cyclesKnob->setValueTextFmt("%d", cyclesKnob->position());
      updateGraph();
    };
    cyclesKnob->setValueTextFmt("%d", cyclesKnob->position());

    amplitudeKnob = new uiKnob(card, "LEVEL", Point(308, 84), Size(58, 76), true, uiThemeStyle::Knob);
    amplitudeKnob->setup(0, 100, 80);
    amplitudeKnob->onChange = [&]() {
      amplitudeKnob->setValueTextFmt("%d%%", amplitudeKnob->position());
      updateGraph();
    };
    amplitudeKnob->setValueTextFmt("%d%%", amplitudeKnob->position());

    new uiStaticLabel(card, "Fill area", Point(250, 182), true, uiThemeStyle::Label);
    fillSwitch = new uiToggleSwitch(card, Point(300, 180), Size(66, 20), true, true, uiThemeStyle::ToggleSwitch);
    fillSwitch->onChange = [&]() {
      graph->graphProps().fillArea = fillSwitch->checked();
      graph->repaint();
    };
  }

  void createSettingsCard(int x, int y) {
    auto card = createCard(x, y, 190, 216, "Settings");

    auto row = [&](char const * text, int ry, bool checked) {
      new uiStaticLabel(card, text, Point(10, ry + 3), true, uiThemeStyle::Label);
      return new uiToggleSwitch(card, Point(104, ry), Size(76, 20), checked, true, uiThemeStyle::ToggleSwitch);
    };
    wifiSwitch      = row("Wi-Fi",     32, true);
    bluetoothSwitch = row("Bluetooth", 58, false);
    animationSwitch = row("Animation", 84, true);

    new uiStaticLabel(card, "Piano octave", Point(10, 114), true, uiThemeStyle::LabelDim);
    octaveStepper = new uiStepper(card, Point(10, 130), Size(170, 26), true, uiThemeStyle::Stepper);
    octaveStepper->onStep = [&](int delta) {
      octave = fabgl::iclamp(octave + delta, 1, 6);
      updateOctave();
    };
    updateOctave();

    new uiStaticLabel(card, "Volume", Point(10, 164), true, uiThemeStyle::LabelDim);
    volumeBar = new uiLevelBar(card, Point(10, 182), Size(170, 22), true, uiThemeStyle::LevelBar);
    volumeBar->levelBarProps().segments     = 16;
    volumeBar->levelBarProps().highSegments = 3;
    volumeBar->setRange(0, 100);
    volumeBar->setPosition(60);
  }

  void createStatusCard(int x, int y) {
    auto card = new uiPanel(this, Point(x, y), Size(576, 42), true, uiThemeStyle::Card);

    new uiStaticLabel(card, "Activity", Point(10, 13), true, uiThemeStyle::LabelDim);
    activityLeds = new uiLedBar(card, Point(64, 13), Size(130, 16), Leds, true, uiThemeStyle::LedBar);

    new uiStaticLabel(card, "Progress", Point(212, 13), true, uiThemeStyle::LabelDim);
    progressBar = new uiProgressBar(card, Point(272, 13), Size(160, 16), true, uiThemeStyle::ProgressBar);

    new uiStaticLabel(card, "Note", Point(450, 13), true, uiThemeStyle::LabelDim);
    noteLabel = new uiLabel(card, "-", Point(486, 12), Size(80, 18), true, uiThemeStyle::Label);
  }

  int octaveBaseNote() { return 12 * (octave + 1) - 12; }   // the keyboard starts one octave below

  void updateOctave() {
    char text[32];
    snprintf(text, sizeof(text), "C%d - B%d", octave - 1, octave + 1);
    octaveStepper->setText(text);
    if (piano)
      piano->setBaseNote(octaveBaseNote());
  }

  // value of the selected waveform at "phase" (0..1), range -1..1
  float waveValue(float phase) {
    switch (waveTiles->selectedItem()) {
      case 1:  return phase < 0.5f ? 1.0f : -1.0f;                                     // square
      case 2:  return phase < 0.5f ? 4.0f * phase - 1.0f : 3.0f - 4.0f * phase;        // triangle
      case 3:  return 2.0f * phase - 1.0f;                                             // sawtooth
      default: return sinf(2.0f * (float) M_PI * phase);                                // sine
    }
  }

  void updateGraph() {
    int16_t values[GraphPoints];
    int cycles = cyclesKnob->position();
    float amplitude = amplitudeKnob->position();
    for (int i = 0; i < GraphPoints; ++i) {
      float phase = (float) i * cycles / GraphPoints;
      values[i] = (int16_t) (waveValue(phase - floorf(phase)) * amplitude);
    }
    graph->setValues(values, GraphPoints);
  }

  // waveform icon inside a tile
  void paintWaveIcon(int index, Rect const & r, bool selected) {
    auto cv = waveTiles->canvas();
    cv->setPenColor(selected ? ink(Ink::Accent) : ink(Ink::TextDim));
    int x1 = r.X1 + 8, x2 = r.X2 - 8;
    int y1 = r.Y1 + 6, y2 = r.Y2 - 6;
    int ym = (y1 + y2) / 2;
    int w  = x2 - x1;
    switch (index) {
      case 0:
      {
        int px = x1, py = ym;
        for (int x = x1; x <= x2; ++x) {
          int y = ym - (int) (sinf(2.0f * (float) M_PI * (x - x1) / w) * (ym - y1));
          cv->drawLine(px, py, x, y);
          px = x;
          py = y;
        }
        break;
      }
      case 1:
        cv->moveTo(x1, ym);
        cv->lineTo(x1, y1);
        cv->lineTo(x1 + w / 2, y1);
        cv->lineTo(x1 + w / 2, y2);
        cv->lineTo(x2, y2);
        cv->lineTo(x2, ym);
        break;
      case 2:
        cv->moveTo(x1, ym);
        cv->lineTo(x1 + w / 4, y1);
        cv->lineTo(x1 + w * 3 / 4, y2);
        cv->lineTo(x2, ym);
        break;
      case 3:
        cv->moveTo(x1, y2);
        cv->lineTo(x2, y1);
        cv->lineTo(x2, y2);
        break;
    }
  }

  void animate() {
    if (!animationSwitch->checked())
      return;

    // progress bar: 0..100 then again
    progress = (progress + 1) % 101;
    progressBar->setPercentage(progress);

    // leds: a "VU meter" whose height follows the volume, with a moving wobble
    ledPhase = (ledPhase + 1) % 32;
    int wobble = ledPhase < 16 ? ledPhase : 31 - ledPhase;   // 0..15..0
    int lit    = volumeBar->position() * Leds / 100 * (8 + wobble) / 23;
    for (int i = 0; i < Leds; ++i) {
      int state = 0;
      if (i < lit)
        state = i < Leds * 6 / 10 ? 1 : (i < Leds * 9 / 10 ? 2 : 3);
      if (activityLeds->led(i) != state)
        activityLeds->setLed(i, state);
    }
  }

};
