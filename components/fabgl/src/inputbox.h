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


/**
 * @file
 *
 * @brief This file contains the InputBox class
 */



#include <stdint.h>
#include <stddef.h>

#include "fabglconf.h"
#include "fabui.h"
#include "fabutils.h"
#include "dispdrivers/vgabasecontroller.h"
#include "comdrivers/ps2controller.h"



namespace fabgl {



/** \ingroup Enumerations
 * @brief Result of InputBox dialogs
 */
enum class InputResult {
  None        = 0,       /**< Still running */
  ButtonExt0  = 1,       /**< Extended button 0 pressed */
  ButtonExt1  = 2,       /**< Extended button 1 pressed */
  ButtonExt2  = 3,       /**< Extended button 2 pressed */
  ButtonExt3  = 4,       /**< Extended button 3 pressed */
  Cancel      = 5,       /**< CANCEL button or ESC key pressed */
  ButtonLeft  = 5,       /**< Left button (cancel) or ESC key pressed */
  Enter       = 6,       /**< OK button, ENTER or RETURN key pressed */
  ButtonRight = 6,       /**< Right button (OK), ENTER or RETURN key pressed */
};



////////////////////////////////////////////////////////////////////////////////////////////////////
// InputForm


class InputBox;


struct InputForm {
  InputForm(InputBox * inputBox_)
    : inputBox(inputBox_)
  {
  }

  void init(uiApp * app_, bool modalDialog_);

  virtual void addControls()      = 0;
  virtual void calcRequiredSize() = 0;
  virtual void finalize()         { }
  virtual void show()             { }

  void doExit(int value);
  void defaultEnterHandler(uiKeyEventInfo const & key);
  void defaultEscapeHandler(uiKeyEventInfo const & key);


  static constexpr int BUTTONS  = 6;

  InputBox *       inputBox;

  uiApp *          app;

  char const *     titleText;
  int              autoOK;

  FontInfo const * font;
  int              requiredWidth;
  int              requiredHeight;

  uiFrame *        mainFrame;
  uiPanel *        panel;
  uiLabel *        autoOKLabel;

  InputResult      retval;
  int              buttonSubItem;    // in case of button with subitems, specifies the selected subitem

  uiWindow *       controlToFocus;

  bool             modalDialog;
};



////////////////////////////////////////////////////////////////////////////////////////////////////
// InputApp


struct InputApp : public uiApp {
  InputApp(InputForm * form_)      { form = form_; }
  virtual void init()              { form->init(this, false); }

  InputForm * form;
};




////////////////////////////////////////////////////////////////////////////////////////////////////
// TextInputForm


struct TextInputForm : public InputForm {
  TextInputForm(InputBox * inputBox_)
    : InputForm(inputBox_)
  {
  }

  void addControls();
  void calcRequiredSize();
  void finalize();

  char const *     labelText;
  char *           inOutString;
  int              maxLength;
  bool             passwordMode;

  int              editExtent;
  int              labelExtent;

  uiTextEdit *     edit;
};



////////////////////////////////////////////////////////////////////////////////////////////////////
// MessageForm


struct MessageForm : public InputForm {
  MessageForm(InputBox * inputBox_)
    : InputForm(inputBox_)
  {
  }

  void addControls();
  void calcRequiredSize();
  void finalize();

  char const *     messageText;

  int              messageExtent;
};



////////////////////////////////////////////////////////////////////////////////////////////////////
// SelectForm


struct SelectForm : public InputForm {
  SelectForm(InputBox * inputBox_)
    : InputForm(inputBox_)
  {
  }

  void addControls();
  void calcRequiredSize();
  void finalize();

  int countItems(size_t * maxLength);

  char const *     messageText;
  char const *     items;       // "separator" separated items (zero ends the list)
  char             separator;
  StringList *     itemsList;
  bool             menuMode;

  int              listBoxHeight;
  int              outSelected;

  uiListBox *      listBox;
};



////////////////////////////////////////////////////////////////////////////////////////////////////
// ProgressForm


struct ProgressForm : public InputForm {
  ProgressForm(InputBox * inputBox_)
    : InputForm(inputBox_)
  {
  }

  void addControls();
  void calcRequiredSize();
  void show();

  bool update(int percentage, char const * format, ...);

  static const int        progressBarHeight = 16;

  bool                    hasProgressBar;
  Delegate<ProgressForm*> execFunc;
  int                     width;

  uiLabel *               label;
  uiProgressBar *         progressBar;
};



////////////////////////////////////////////////////////////////////////////////////////////////////
// FileBrowserForm


struct FileBrowserForm : public InputForm {
  static constexpr int SIDE_BUTTONS_WIDTH  = 65;
  static constexpr int SIDE_BUTTONS_HEIGHT = 18;
  static constexpr int CTRLS_DIST          = 4;
  static constexpr int BROWSER_WIDTH       = 150;
  static constexpr int BROWSER_HEIGHT      = 242;
  static constexpr int MAXNAME             = 32;

  FileBrowserForm(InputBox * inputBox_)
    : InputForm(inputBox_)
  {
  }
  ~FileBrowserForm() {
    free(srcDirectory);
    free(srcFilename);
  }
  void doCopy();
  void doPaste();

  void addControls();
  void calcRequiredSize();
  void finalize();

  char const *    directory;

  char *          srcDirectory = nullptr;
  char *          srcFilename  = nullptr;

  uiFileBrowser * fileBrowser;
  uiButton *      newFolderButton;
  uiButton *      renameButton;
  uiButton *      deleteButton;
  uiButton *      copyButton;
  uiButton *      pasteButton;
};



////////////////////////////////////////////////////////////////////////////////////////////////////
// FileSelectorForm

struct FileSelectorForm : public InputForm {
  static constexpr int CTRLS_DIST          = 4;
  static constexpr int BROWSER_WIDTH       = 180;
  static constexpr int BROWSER_HEIGHT      = 150;
  static constexpr int MINIMUM_EDIT_WIDTH  = 64;

  FileSelectorForm(InputBox * inputBox_)
    : InputForm(inputBox_)
  {
  }

  void addControls();
  void calcRequiredSize();
  void finalize();

  char const *     labelText;
  char *           inOutDirectory;
  int              maxDirectoryLength;
  char *           inOutFilename;
  int              maxFilenameLength;

  int              editExtent;
  int              labelExtent;

  uiTextEdit *     edit;
  uiFileBrowser *  fileBrowser;
};



////////////////////////////////////////////////////////////////////////////////////////////////////
// InputBox


/**
 * @brief InputBox is a helper class to build simple user interfaces, such as wizards and input boxes
 *
 * @see @ref guide_gui
 *
 * @ingroup gui
 */
class InputBox {

public:

  /**
   * @brief Creates a new InputBox instance
   *
   * @param app Optional existing uiApp object. If specified, applications can use the InputBox helpers inside that uiApp.
   */
  InputBox(uiApp * app = nullptr);

  ~InputBox();

  /**
   * @brief Initializes InputBox from a VGA modeline, creating a VGA display controller
   *
   * @param modeline Optional modeline (640x480 if not specified)
   * @param viewPortWidth Viewport width (-1 = automatic)
   * @param viewPortHeight Viewport height (-1 = automatic)
   * @param displayColors Number of display colors (2, 4, 8 or 16)
   */
  void begin(char const * modeline = nullptr, int viewPortWidth = -1, int viewPortHeight = -1, int displayColors = 16);

  /**
   * @brief Initializes InputBox using an already initialized display controller
   *
   * @param displayController Active display controller
   */
  void begin(BitmappedDisplayController * displayController);

  /**
   * @brief Gets the created or assigned display controller
   *
   * @return Display controller object
   */
  BitmappedDisplayController * getDisplayController()                        { return m_dispCtrl; }

  /**
   * @brief Releases resources and, if it was created by InputBox, disables the display output
   */
  void end();

  /**
   * @brief Sets the background color
   *
   * @param value Background color
   */
  void setBackgroundColor(RGB888 const & value)   { m_backgroundColor = value; }

  RGB888 backgroundColor()                        { return m_backgroundColor; }

  /**
   * @brief Specifies a timeout for the dialog
   *
   * The countdown stops when the user moves the mouse or presses a key.
   *
   * @param timeout If >0, OK is automatically triggered after the specified number of seconds
   */
  void setAutoOK(int timeout)                     { m_autoOK = timeout; }

  /**
   * @brief Sets up an extended button or split button
   *
   * Extended button texts are cleared after every dialog.
   *
   * @param index A value from 0 to 3. 0 = leftmost button ... 3 = rightmost button
   * @param text Button text
   * @param subItems If specified, a split button is created. subItems is a semicolon-separated list of menu items
   * @param subItemsHeight Height of the split button menu, in pixels
   */
  void setupButton(int index, char const * text, char const * subItems = nullptr, int subItemsHeight = 80);

  /**
   * @brief Sets the minimum button size
   *
   * @param value Minimum button size in pixels
   */
  void setMinButtonsWidth(int value)                   { m_minButtonsWidth = value; }

  int minButtonsWidth()                                { return m_minButtonsWidth; }

  char const * buttonText(int index)                   { return m_buttonText[index]; }

  char const * buttonSubItems(int index)               { return m_buttonSubItems[index]; }

  int buttonsSubItemsHeight(int index)                 { return m_buttonSubItemsHeight[index]; }

  /**
   * @brief Gets the result of the last dialog
   *
   * @return Last result
   */
  InputResult getLastResult()                          { return m_lastResult; }

  /**
   * @brief Gets the selected item of a split button
   *
   * @return Selected sub-item
   */
  int selectedSubItem()                                { return m_buttonSubItem; }

  /**
   * @brief Shows a dialog with a label and a text edit box
   *
   * @param titleText Optional title of the dialog (nullptr = no title)
   * @param labelText Label text
   * @param inOutString Edit box string, both input and output
   * @param maxLength Maximum length of the edit box string (terminating zero excluded)
   * @param buttonCancelText Optional text for the CANCEL button (nullptr = no CANCEL button). Default is "Cancel".
   * @param buttonOKText Optional text for the OK button (nullptr = no OK button). Default is "OK".
   * @param passwordMode Optional password mode. If true, characters are shown as "*". Default is false.
   *
   * @return Dialog box result (Cancel or Enter)
   *
   * Example:
   *
   *     char wifiName[32];
   *     char wifiPsw[32];
   *     InputBox ib;
   *     ib.begin();
   *     if (ib.message("Network Configuration", "Configure WiFi network?", "No", "Yes") == InputResult::Enter) {
   *       ib.textInput("Network Configuration", "WiFi Name", wifiName, 31);
   *       ib.textInput("Network Configuration", "WiFi Password", wifiPsw, 31, "Cancel", "OK", true);
   *     }
   *     ib.end();
   */
  InputResult textInput(char const * titleText, char const * labelText, char * inOutString, int maxLength, char const * buttonCancelText = "Cancel", char const * buttonOKText = "OK", bool passwordMode = false);

  /**
   * @brief Shows a dialog with just a label
   *
   * @param titleText Optional title of the dialog (nullptr = no title)
   * @param messageText Message to show
   * @param buttonCancelText Optional text for the CANCEL button (nullptr = no CANCEL button). Default is nullptr.
   * @param buttonOKText Optional text for the OK button (nullptr = no OK button). Default is "OK".
   *
   * @return Dialog box result (Cancel or Enter)
   *
   * Example:
   *
   *     char wifiName[32];
   *     char wifiPsw[32];
   *     InputBox ib;
   *     ib.begin();
   *     if (ib.message("Network Configuration", "Configure WiFi network?", "No", "Yes") == InputResult::Enter) {
   *       ib.textInput("Network Configuration", "WiFi Name", wifiName, 31);
   *       ib.textInput("Network Configuration", "WiFi Password", wifiPsw, 31, "Cancel", "OK", true);
   *     }
   *     ib.end();
   */
  InputResult message(char const * titleText, char const * messageText, char const * buttonCancelText = nullptr, char const * buttonOKText = "OK");

  /**
   * @brief Shows a dialog with just a label, using printf-like formatted text
   *
   * @param titleText Optional title of the dialog (nullptr = no title)
   * @param buttonCancelText Optional text for the CANCEL button (nullptr = no CANCEL button)
   * @param buttonOKText Optional text for the OK button (nullptr = no OK button)
   * @param format printf-like format string
   * @param ... printf-like parameters
   *
   * @return Dialog box result (Cancel or Enter)
   *
   * Example:
   *
   *     InputBox ib;
   *     ib.begin();
   *     for (int i = 0; i < 3; ++i)
   *       if (ib.messageFmt(nullptr, "Abort", "Continue", "Iteration number %d of 3", i) == InputResult::Cancel)
   *        break;
   *     ib.end();
   */
  InputResult messageFmt(char const * titleText, char const * buttonCancelText, char const * buttonOKText, const char *format, ...);

  /**
   * @brief Shows a dialog with a label and a list box
   *
   * @param titleText Optional title of the dialog (nullptr = no title)
   * @param messageText Message to show
   * @param itemsText String containing the separator-delimited list of items to show in the list box
   * @param separator Optional item separator. Default is ';'
   * @param buttonCancelText Optional text for the CANCEL button (nullptr = no CANCEL button). Default is "Cancel".
   * @param buttonOKText Optional text for the OK button (nullptr = no OK button). Default is "OK".
   *
   * @return Index of the selected item, or -1 if the dialog has been canceled
   *
   * Example:
   *
   *     InputBox ib;
   *     ib.begin();
   *     int s = ib.select("Download Boot Disk", "Select boot disk to download", "FreeDOS;Minix 2.0;MS-DOS 3.3");
   *     ib.messageFmt("", nullptr, "OK", "You have selected %d", s);
   *     ib.end();
   */
  int select(char const * titleText, char const * messageText, char const * itemsText, char separator = ';', char const * buttonCancelText = "Cancel", char const * buttonOKText = "OK");

  /**
   * @brief Shows a dialog with a label and a list box
   *
   * @param titleText Optional title of the dialog (nullptr = no title)
   * @param messageText Message to show
   * @param items StringList object containing the items to show in the list box. Item selection is read before and updated after the dialog
   * @param buttonCancelText Optional text for the CANCEL button (nullptr = no CANCEL button). Default is "Cancel".
   * @param buttonOKText Optional text for the OK button (nullptr = no OK button). Default is "OK".
   *
   * @return Dialog box result (Cancel or Enter)
   *
   * Example:
   *
   *     InputBox ib;
   *     ib.begin();
   *     StringList list;
   *     list.append("Option 0");
   *     list.append("Option 1");
   *     list.append("Option 2");
   *     list.append("Option 3");
   *     list.select(1, true);    // initially item 1 (Option 1) is selected
   *     if (ib.select("Select values", "Please select one or more items", &list) == InputResult::Enter) {
   *       for (int i = 0; i < list.count(); ++i)
   *         if (list.selected(i))
   *           ib.messageFmt("", nullptr, "OK", "You have selected %d", i);
   *     }
   *     ib.end();
   */
  InputResult select(char const * titleText, char const * messageText, StringList * items, char const * buttonCancelText = "Cancel", char const * buttonOKText = "OK");

  /**
   * @brief Shows a dialog with a label and a list box. The dialog closes as soon as an item is selected, like a menu
   *
   * @param titleText Optional title of the dialog (nullptr = no title)
   * @param messageText Message to show
   * @param itemsText String containing the separator-delimited list of items to show in the list box
   * @param separator Optional item separator. Default is ';'
   *
   * @return Index of the selected item, or -1 if the dialog has been canceled
   *
   * Example:
   *
   *     InputBox ib;
   *     ib.begin();
   *     int s = ib.menu("Menu", "Click on an item", "Item number one;Item number two;Item number three;Item number four");
   *     ib.messageFmt("", nullptr, "OK", "You have selected %d", s);
   *     ib.end();
   */
  int menu(char const * titleText, char const * messageText, char const * itemsText, char separator = ';');

  /**
   * @brief Shows a dialog with a label and a list box. The dialog closes as soon as an item is selected, like a menu
   *
   * @param titleText Optional title of the dialog (nullptr = no title)
   * @param messageText Message to show
   * @param items StringList object containing the items to show in the list box
   *
   * @return Index of the selected item, or -1 if the dialog has been canceled
   *
   * Example:
   *
   *     InputBox ib;
   *     ib.begin();
   *     StringList list;
   *     list.append("Menu item 0");
   *     list.append("Menu item 1");
   *     list.append("Menu item 2");
   *     list.append("Menu item 3");
   *     int s = ib.menu("Menu", "Click on a menu item", &list);
   *     ib.messageFmt("", nullptr, "OK", "You have selected %d", s);
   *     ib.end();
   */
  int menu(char const * titleText, char const * messageText, StringList * items);

  /**
   * @brief Shows a dialog with a label and a progress bar, updated by a user function
   *
   * @param titleText Optional title of the dialog (nullptr = no title)
   * @param buttonCancelText Optional text for the CANCEL button (nullptr = no CANCEL button)
   * @param hasProgressBar If true, a progress bar is shown
   * @param width Dialog width in pixels
   * @param execFunc Function (typically a lambda) that updates the text and the progress bar by calling update() on the object it receives
   *
   * @return Dialog box result (Cancel or Enter)
   *
   * Example:
   *
   *     InputBox ib;
   *     ib.begin();
   *     auto r = ib.progressBox("This is the title", "Abort", true, 200, [&](fabgl::ProgressForm * form) {
   *       for (int i = 0; i <= 100; ++i) {
   *         if (!form->update(i, "Index is %d/100", i))
   *           break;
   *         vTaskDelay(pdMS_TO_TICKS(100));
   *       }
   *     });
   *     if (r == InputResult::Cancel)
   *       ib.message("", "Operation Aborted");
   *     ib.end();
   */
  template <typename Func>
  InputResult progressBox(char const * titleText, char const * buttonCancelText, bool hasProgressBar, int width, Func execFunc) {
    ProgressForm form(this);
    form.execFunc = execFunc;
    return progressBoxImpl(form, titleText, buttonCancelText, hasProgressBar, width);
  }

  /**
   * @brief Shows a dialog listing files and folders, with buttons to create folders and to delete and rename files and folders
   *
   * @param titleText Optional title of the dialog (nullptr = no title)
   * @param directory Initial directory. At least one filesystem must be mounted
   * @param buttonOKText Optional text for the OK button (nullptr = no OK button). Default is "Close".
   *
   * @return Dialog box result (Cancel or Enter)
   *
   * Example:
   *
   *     InputBox ib;
   *     ib.begin();
   *     if (FileBrowser::mountSDCard(false, "/SD"))
   *       ib.fileBrowser("File Browser", "/SD");
   *     ib.end();
   */
  InputResult folderBrowser(char const * titleText, char const * directory = "/", char const * buttonOKText = "Close");

  /**
   * @brief Lets the user select a file and a directory, starting from the specified path
   *
   * @param titleText Optional title of the dialog (nullptr = no title)
   * @param messageText Message to show
   * @param inOutDirectory Starting directory on input, selected directory on output
   * @param maxDirectoryLength Maximum length of the directory buffer (terminating zero excluded)
   * @param inOutFilename Initial file name on input, selected file name on output
   * @param maxFilenameLength Maximum length of the file name buffer (terminating zero excluded)
   * @param buttonCancelText Optional text for the CANCEL button (nullptr = no CANCEL button). Default is "Cancel".
   * @param buttonOKText Optional text for the OK button (nullptr = no OK button). Default is "OK".
   *
   * @return Dialog box result (Cancel or Enter)
   *
   * Example:
   *
   *     InputBox ib;
   *     ib.begin();
   *     if (FileBrowser::mountSDCard(false, "/SD")) {
   *       char filename[16] = "";
   *       char directory[32] = "/SD";
   *       if (ib.fileSelector("File Select", "Filename: ", directory, sizeof(directory) - 1, filename, sizeof(filename) - 1) == InputResult::Enter)
   *         ib.messageFmt("", nullptr, "OK", "Folder = %s, File = %s", directory, filename);
   *     }
   *     ib.end();
   */
  InputResult fileSelector(char const * titleText, char const * messageText, char * inOutDirectory, int maxDirectoryLength, char * inOutFilename, int maxFilenameLength, char const * buttonCancelText = "Cancel", char const * buttonOKText = "OK");


  // delegates

  /**
   * @brief Paint event delegate
   */
  Delegate<Canvas *> onPaint;
  


private:

  InputResult progressBoxImpl(ProgressForm & form, char const * titleText, char const * buttonCancelText, bool hasProgressBar, int width);

  void exec(InputForm * form);
  void resetButtons();

  BitmappedDisplayController *        m_dispCtrl;
  VGABaseController *                 m_vgaCtrl;
  RGB888                              m_backgroundColor;
  uiApp *                             m_existingApp;                               // uiApp in case of running on existing app
  uint16_t                            m_autoOK;                                    // auto ok in seconds
  int16_t                             m_buttonSubItem;                             // in case of button with subitems, specifies the selected subitem                     //
  char const *                        m_buttonText[InputForm::BUTTONS]     = { };
  char const *                        m_buttonSubItems[InputForm::BUTTONS] = { };  // ext button is uiButton if nullptr, uiSplitButton otherwise
  uint16_t                            m_buttonSubItemsHeight[InputForm::BUTTONS] = { };
  InputResult                         m_lastResult = InputResult::None;
  int16_t                             m_minButtonsWidth;
};



}   // namespace fabgl
