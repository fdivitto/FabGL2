#pragma once


// modal window with a form: the rest of the UI is blocked until Close is pressed
struct TestModalDialog : public uiFrame {
  uiTextEdit * textEdit1, * textEdit2, * textEdit3, * textEdit4;
  uiButton * button1, * button2;
  uiPanel * card, * footer;

  static constexpr int W = 340;
  static constexpr int H = 262;

  TestModalDialog(uiFrame * parent)
    : uiFrame(parent, "Modal Dialog", Point((640 - W) / 2, 70), Size(W, H), true, uiThemeStyle::Dialog) {
    frameProps().resizeable        = false;
    frameProps().hasCloseButton    = false;
    frameProps().hasMaximizeButton = false;
    frameProps().hasMinimizeButton = false;

    int x = clientPos().X;
    int y = clientPos().Y;
    int w = clientSize().width;

    new uiStaticLabel(this, "New contact", Point(x + 14, y + 6), true, uiThemeStyle::LabelTitle);
    new uiStaticLabel(this, "This is a modal window: click Close to continue", Point(x + 14, y + 28), true, uiThemeStyle::LabelDim);

    card = new uiPanel(this, Point(x + 12, y + 48), Size(w - 24, 132), true, uiThemeStyle::Card);

    char const * Labels[4] = { "First name", "Last name", "Address", "Phone" };
    uiTextEdit * * edits[4] = { &textEdit1, &textEdit2, &textEdit3, &textEdit4 };
    for (int i = 0; i < 4; ++i) {
      new uiStaticLabel(card, Labels[i], Point(10, 12 + i * 30), true, uiThemeStyle::Label);
      *edits[i] = new uiTextEdit(card, "", Point(90, 8 + i * 30), Size(card->size().width - 100, 24), true, uiThemeStyle::Edit);
    }

    // footer with the buttons (right aligned)
    int fh = 44;
    footer = new uiPanel(this, Point(x - 1, y + clientSize().height - fh), Size(w + 2, fh), true, uiThemeStyle::DialogFooter);

    button1 = new uiButton(footer, "Add Item", Point(footer->size().width - 192, 9), Size(86, 26), uiButtonKind::Button, true, uiThemeStyle::ButtonAccent);
    button1->onClick = [&]() {
      app()->messageBox("New Item", "Item added correctly", "OK", nullptr, nullptr, uiMessageBoxIcon::Info);
      textEdit1->setText("");
      textEdit2->setText("");
      textEdit3->setText("");
      textEdit4->setText("");
    };

    button2 = new uiButton(footer, "Close", Point(footer->size().width - 98, 9), Size(86, 26), uiButtonKind::Button, true, uiThemeStyle::Button);
    button2->onClick = [&]() {
      exitModal(0);
    };
  }
};
