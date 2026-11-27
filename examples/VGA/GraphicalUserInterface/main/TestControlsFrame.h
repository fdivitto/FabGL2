#pragma once


// standard controls: label, text edit, buttons, list box, combo boxes, check boxes, radio buttons, sliders
struct TestControlsFrame : public uiFrame {
  uiTextEdit * textEdit;
  uiButton * normalButton, * accentButton, * switchButton;
  uiListBox * listBox;
  uiComboBox * comboBox1, * comboBox2;
  uiCheckBox * checkBox;
  uiCheckBox * radio1, * radio2, * radio3;
  uiSlider * slider1, * slider2;
  uiLabel * sliderLabel1, * sliderLabel2;
  uiSplitButton * splitButton1;

  // label + control on the right side: both follow the right border
  template <typename T>
  T * rightAnchored(T * control) {
    control->anchors().left  = false;
    control->anchors().right = true;
    return control;
  }

  TestControlsFrame(uiFrame * parent)
    : uiFrame(parent, "Controls", Point(70, 24), Size(470, 296), false, uiThemeStyle::Frame) {

    int x = clientPos().X + 10;
    int y = clientPos().Y + 8;

    new uiStaticLabel(this, "Text Edit", Point(x, y + 4), true, uiThemeStyle::Label);
    textEdit = new uiTextEdit(this, "Type something here", Point(x + 80, y), Size(366, 24), true, uiThemeStyle::Edit);
    textEdit->anchors().right = true;

    // buttons
    y += 34;
    normalButton = new uiButton(this, "Button", Point(x, y), Size(80, 26), uiButtonKind::Button, true, uiThemeStyle::Button);
    normalButton->onClick = [&]() { app()->messageBox("", "Button Pressed!", "OK", nullptr, nullptr, uiMessageBoxIcon::Info); };

    accentButton = new uiButton(this, "Accent", Point(x + 88, y), Size(80, 26), uiButtonKind::Button, true, uiThemeStyle::ButtonAccent);
    accentButton->onClick = [&]() { app()->messageBox("Accent Button", "This is the primary action", "OK", nullptr, nullptr, uiMessageBoxIcon::Info); };

    switchButton = new uiButton(this, "Switch OFF", Point(x + 176, y), Size(100, 26), uiButtonKind::Switch, true, uiThemeStyle::Button);
    switchButton->onChange = [&]() { switchButton->setText( switchButton->down() ? "Switch ON" : "Switch OFF" ); };

    splitButton1 = new uiSplitButton(this, "Split Button", Point(x + 284, y), Size(110, 26), 100, "Option 1;Option 2;Option 3;Option 4;Option 5;Option 6", ';', true, uiThemeStyle::SplitButton);
    splitButton1->onSelect = [&](int index) { app()->messageBox("Split Button", splitButton1->items().get(index), "OK", nullptr, nullptr, uiMessageBoxIcon::Info); };

    // list box (left) and combo boxes, check box (right)
    y += 38;
    listBox = new uiListBox(this, Point(x, y), Size(170, 112), true, uiThemeStyle::ListBox);
    listBox->anchors().right = true;
    for (int i = 0; i < 10; ++i)
      listBox->items().appendFmt("List Box Row %d", i);
    listBox->selectItem(0);

    int rx = x + 186;
    rightAnchored(new uiStaticLabel(this, "Combo Box", Point(rx, y + 4), true, uiThemeStyle::Label));
    comboBox1 = rightAnchored(new uiComboBox(this, Point(rx + 110, y), Size(150, 24), 110, true, uiThemeStyle::ComboBox));
    for (int i = 0; i < 10; ++i)
      comboBox1->items().appendFmt("Combo Row %d", i);
    comboBox1->selectItem(0);

    rightAnchored(new uiStaticLabel(this, "Editable Combo", Point(rx, y + 34), true, uiThemeStyle::Label));
    comboBox2 = rightAnchored(new uiComboBox(this, Point(rx + 110, y + 30), Size(150, 24), 110, true, uiThemeStyle::ComboBox));
    comboBox2->comboBoxProps().openOnFocus = false;
    comboBox2->textEditProps().hasCaret  = true;
    comboBox2->textEditProps().allowEdit = true;
    comboBox2->items().copyFrom(comboBox1->items());

    checkBox = rightAnchored(new uiCheckBox(this, Point(rx, y + 64), Size(16, 16), uiCheckBoxKind::CheckBox, true, uiThemeStyle::CheckBox));
    checkBox->setChecked(true);
    rightAnchored(new uiStaticLabel(this, "Check Box", Point(rx + 22, y + 64), true, uiThemeStyle::Label));

    // vertical slider
    slider2 = rightAnchored(new uiSlider(this, Point(rx + 236, y + 64), Size(20, 96), uiOrientation::Vertical, true, uiThemeStyle::Slider));
    sliderLabel2 = rightAnchored(new uiLabel(this, "0", Point(rx + 196, y + 64), Size(36, 16), true, uiThemeStyle::LabelValue));
    slider2->onChange = [&]() {
      sliderLabel2->setTextFmt("%d", slider2->position());
    };

    // radio buttons
    y += 124;
    char const * RadioText[3] = { "Radio 1", "Radio 2", "Radio 3" };
    uiCheckBox * * radios[3]  = { &radio1, &radio2, &radio3 };
    for (int i = 0; i < 3; ++i) {
      *radios[i] = new uiCheckBox(this, Point(x + i * 90, y), Size(16, 16), uiCheckBoxKind::RadioButton, true, uiThemeStyle::CheckBox);
      (*radios[i])->setGroupIndex(1);
      new uiStaticLabel(this, RadioText[i], Point(x + i * 90 + 22, y), true, uiThemeStyle::Label);
    }
    radio1->setChecked(true);

    // horizontal slider
    y += 30;
    sliderLabel1 = new uiLabel(this, "0", Point(x, y + 2), Size(30, 16), true, uiThemeStyle::LabelValue);
    slider1 = new uiSlider(this, Point(x + 36, y), Size(300, 20), uiOrientation::Horizontal, true, uiThemeStyle::Slider);
    slider1->anchors().right = true;
    slider1->onChange = [&]() {
      sliderLabel1->setTextFmt("%d", slider1->position());
    };

  }
};
