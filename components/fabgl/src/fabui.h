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
 * @brief This file contains all classes related to FabGL Graphical User Interface
 */



#include <stdint.h>
#include <stddef.h>

#include <list>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/timers.h"

#include "fabglconf.h"
#include "fabutils.h"
#include "displaycontroller.h"
#include "canvas.h"
#include "fabfonts.h"
#include "codepages.h"



/*

  *uiObject
    *uiEvtHandler
      *uiApp
      *uiWindow
        *uiFrame
        *uiControl
          *uiButton
          *uiLabel
          *uiStaticLabel
          *uiImage
          *uiPanel
          *uiTextEdit
          *uiScrollableControl
            *uiPaintBox
              *uiCustomListBox
                *uiListBox
                *uiColorListBox
                *uiFileBrowser
                *uiSimpleMenu
            uiMemoEdit
          *uiCheckBox
          *uiCustomComboBox
            *uiComboBox
            *uiColorComboBox
            *uiSplitButton
          uiMenu
          uiGauge
          *uiSlider
          uiSpinButton
          *uiColorBox
          *uiProgressBar
          *uiKnob
          *uiToggleSwitch
          *uiLevelBar
          *uiLedBar
          *uiStepper
          *uiTileSelector
          *uiGraph
          *uiPianoKeyboard

*/


namespace fabgl {



// increase in case of garbage between windows!
#define FABGLIB_UI_EVENTS_QUEUE_SIZE 300


using std::list;
using std::pair;


////////////////////////////////////////////////////////////////////////////////////////////////////
// uiEvent

enum uiEventID {
  UIEVT_NULL,
  UIEVT_DEBUGMSG,
  UIEVT_APPINIT,
  UIEVT_GENPAINTEVENTS,
  UIEVT_PAINT,
  UIEVT_ACTIVATE,
  UIEVT_DEACTIVATE,
  UIEVT_MOUSEMOVE,
  UIEVT_MOUSEWHEEL,
  UIEVT_MOUSEBUTTONDOWN,
  UIEVT_MOUSEBUTTONUP,
  UIEVT_SETPOS,
  UIEVT_SETSIZE,
  UIEVT_RESHAPEWINDOW,
  UIEVT_MOUSEENTER,
  UIEVT_MOUSELEAVE,
  UIEVT_MAXIMIZE,   // Request for maximize
  UIEVT_MINIMIZE,   // Request for minimize
  UIEVT_RESTORE,    // Restore from UIEVT_MAXIMIZE or UIEVT_MINIMIZE
  UIEVT_SHOW,
  UIEVT_HIDE,
  UIEVT_SETFOCUS,
  UIEVT_KILLFOCUS,
  UIEVT_KEYDOWN,
  UIEVT_KEYUP,
  UIEVT_KEYTYPE,
  UIEVT_TIMER,
  UIEVT_CLICK,
  UIEVT_DBLCLICK,
  UIEVT_EXITMODAL,
  UIEVT_DESTROY,
  UIEVT_CLOSE,      // Request to close (frame Close button)
  UIEVT_QUIT,       // Quit the application
  UIEVT_CREATE,
  UIEVT_CHILDSETFOCUS,  // a UIEVT_SETFOCUS has been sent to a child
  UIEVT_CHILDKILLFOCUS, // a UIEVT_KILLFOCUS has been sent to a child
};


class uiEvtHandler;
class uiApp;
class uiWindow;


typedef TimerHandle_t uiTimerHandle;


/** @brief Contains details about the key event */
struct uiKeyEventInfo {
  VirtualKey VK;         /**< Virtual key */
  uint8_t    ASCII;      /**< ASCII value (when applicable) */
  uint8_t    LALT  : 1;  /**< Status of left-ALT key */
  uint8_t    RALT  : 1;  /**< Status of right-ALT key */
  uint8_t    CTRL  : 1;  /**< Status of CTRL (left or right) key */
  uint8_t    SHIFT : 1;  /**< Status of SHIFT (left or right) key */
  uint8_t    GUI   : 1;  /**< Status of GUI (Windows logo) key */
};


/** @brief Contains details about the mouse event */
struct uiMouseEventInfo {
  MouseStatus status;         /**< Mouse position, wheel delta and buttons status */
  uint8_t     changedButton;  /**< Specifies which mouse button has changed 0 = none, 1 = left, 2 = middle, 3 = right */
};


struct uiFocusInfo {
  uiWindow * oldFocused;
  uiWindow * newFocused;
};


struct uiEvent {
  uiEvtHandler * dest;
  uiEventID      id;

  union uiEventParams {
    // event: UIEVT_MOUSEMOVE, UIEVT_MOUSEWHEEL, UIEVT_MOUSEBUTTONDOWN, UIEVT_MOUSEBUTTONUP, UIEVT_CLICK, UIEVT_DBLCLICK
    uiMouseEventInfo mouse;
    // event: UIEVT_PAINT, UIEVT_GENPAINTEVENTS, UIEVT_RESHAPEWINDOW
    Rect rect;
    // event: UIEVT_SETPOS
    Point pos;
    // event: UIEVT_SETSIZE
    Size size;
    // event: UIEVT_DEBUGMSG
    char const * debugMsg;
    // event: UIEVT_KEYDOWN, UIEVT_KEYUP
    uiKeyEventInfo key;
    // event: UIEVT_TIMER
    uiTimerHandle timerHandle;
    // event: UIEVT_EXITMODAL
    int modalResult;
    // event: UIEVT_QUIT
    int exitCode;
    // event: UIEVT_SETFOCUS, UIEVT_KILLFOCUS, UIEVT_CHILDKILLFOCUS, UIEVT_CHILDSETFOCUS
    uiFocusInfo focusInfo;

    uiEventParams() { memset((void *) this, 0, sizeof(uiEventParams)); }
  } params;

  uiEvent() : dest(nullptr), id(UIEVT_NULL) { }
  uiEvent(uiEvent const & e) { dest = e.dest; id = e.id; params = e.params; }
  uiEvent & operator=(uiEvent const & e) { dest = e.dest; id = e.id; params = e.params; return *this; }
  uiEvent(uiEvtHandler * dest_, uiEventID id_) : dest(dest_), id(id_) { }
};



/** \ingroup Enumerations
 * @brief Item direction/orientation
 */
enum class uiOrientation {
  Vertical,          /**< Vertical orientation */
  Horizontal,        /**< Horizontal orientation */
};


/** \ingroup Enumerations
 * @brief Text horizontal alignment
 */
enum class uiHAlign {
  Left,             /**< Left align */
  Right,            /**< Right align */
  Center,           /**< Center align */
};



////////////////////////////////////////////////////////////////////////////////////////////////////
// uiObject


/** @brief Specifies the object type */
struct uiObjectType {
  uint64_t uiApp               : 1;
  uint64_t uiEvtHandler        : 1;
  uint64_t uiWindow            : 1;
  uint64_t uiFrame             : 1;
  uint64_t uiControl           : 1;
  uint64_t uiScrollableControl : 1;
  uint64_t uiButton            : 1;
  uint64_t uiTextEdit          : 1;
  uint64_t uiLabel             : 1;
  uint64_t uiStaticLabel       : 1;
  uint64_t uiImage             : 1;
  uint64_t uiPanel             : 1;
  uint64_t uiPaintBox          : 1;
  uint64_t uiCustomListBox     : 1;
  uint64_t uiListBox           : 1;
  uint64_t uiFileBrowser       : 1;
  uint64_t uiComboBox          : 1;
  uint64_t uiCheckBox          : 1;
  uint64_t uiSlider            : 1;
  uint64_t uiColorListBox      : 1;
  uint64_t uiCustomComboBox    : 1;
  uint64_t uiColorBox          : 1;
  uint64_t uiColorComboBox     : 1;
  uint64_t uiProgressBar       : 1;
  uint64_t uiSplitButton       : 1;
  uint64_t uiSimpleMenu        : 1;
  uint64_t uiKnob              : 1;
  uint64_t uiToggleSwitch      : 1;
  uint64_t uiLevelBar          : 1;
  uint64_t uiLedBar            : 1;
  uint64_t uiStepper           : 1;
  uint64_t uiTileSelector      : 1;
  uint64_t uiGraph             : 1;
  uint64_t uiPianoKeyboard     : 1;

  uiObjectType() : uiApp(0), uiEvtHandler(0), uiWindow(0), uiFrame(0), uiControl(0), uiScrollableControl(0), uiButton(0), uiTextEdit(0),
                   uiLabel(0), uiStaticLabel(0), uiImage(0), uiPanel(0), uiPaintBox(0), uiCustomListBox(0), uiListBox(0), uiFileBrowser(0), uiComboBox(0),
                   uiCheckBox(0), uiSlider(0), uiColorListBox(0), uiCustomComboBox(0), uiColorBox(0), uiColorComboBox(0), uiProgressBar(0),
                   uiSplitButton(0), uiSimpleMenu(0), uiKnob(0), uiToggleSwitch(0), uiLevelBar(0), uiLedBar(0), uiStepper(0),
                   uiTileSelector(0), uiGraph(0), uiPianoKeyboard(0)
    { }
};


/**
 * @brief Base class of all UI elements like windows and controls
 *
 * @ingroup gui
 */
class uiObject {

public:

  uiObject();

  virtual ~uiObject();

  /**
   * @brief Determines the object type.
   *
   * @return Object type.
   */
  uiObjectType & objectType() { return m_objectType; }

private:
  uiObjectType m_objectType;
};



////////////////////////////////////////////////////////////////////////////////////////////////////
// uiEvtHandler


/**
 * @brief Base class of all UI elements that can receive events.
 *
 * @ingroup gui
 */
class uiEvtHandler : public uiObject {

public:

  uiEvtHandler(uiApp * app);

  virtual ~uiEvtHandler();

  virtual void processEvent(uiEvent * event);

  /**
   * @brief Determines the app that owns this object.
   *
   * @return An uiApp object.
   */
  uiApp * app() { return m_app; }


protected:

  void setApp(uiApp * value) { m_app = value; }


private:

  uiApp * m_app;
};



////////////////////////////////////////////////////////////////////////////////////////////////////
// uiWindow

/** \ingroup Enumerations
 * @brief Specifies window rectangle origin
 */
enum class uiOrigin {
  Screen,   /**< Rectangle coordinates relative to the screen (that is the root window) */
  Parent,   /**< Rectangle coordinates relative to the parent window */
  Window,   /**< Rectangle coordinates relative to the window itself */
};


/** @brief Specifies current window state */
struct uiWindowState {
  uint8_t visible   : 1;  /**< 0 = hidden,   1 = visible   */
  uint8_t active    : 1;  /**< 0 = inactive, 1 = active    */
};


/** @brief Contains some window options */
struct uiWindowProps {
  uint8_t activable  : 1;  /**< The window is activable (default for windows)  */
  uint8_t focusable  : 1;  /**< The window is focusable (default for controls) */
  uint8_t activeLook : 1;  /**< Maintain active style even when not active */

  uiWindowProps() :
    activable(true),
    focusable(false),
    activeLook(false)
  { }
};


/** @brief Contains the window style */
struct uiWindowStyle {
  CursorName    defaultCursor      = CursorName::CursorPointerSimpleReduced;  /**< Default window mouse cursor */
  RGB888        borderColor        = RGB888(128, 128, 128);                   /**< Border color */
  RGB888        activeBorderColor  = RGB888(128, 128, 255);                   /**< Border color when active */
  RGB888        focusedBorderColor = RGB888(0, 0, 255);                       /**< Border color when focused */
  uint8_t       borderSize         = 3;                                       /**< Border size in pixels. This determines also the resize grips area. */
  uint8_t       focusedBorderSize  = 1;                                       /**< Border size when focused */

  void adaptToDisplayColors(int displayColors) {
    if (displayColors < 4) {
      borderColor        = RGB888(0, 0, 0);
      activeBorderColor  = RGB888(0, 0, 0);
      focusedBorderColor = RGB888(0, 0, 0);
    } else if (displayColors < 16) {
      borderColor = RGB888(0, 0, 0);
    }
  }
};


/** @brief Contains anchors enable/disable switches */
struct uiAnchors {
  uint8_t left   : 1;     /**< Left anchor enable/disable switch */
  uint8_t top    : 1;     /**< Top anchor enable/disable switch */
  uint8_t right  : 1;     /**< Right anchor enable/disable switch */
  uint8_t bottom : 1;     /**< Bottom anchor enable/disable switch */

  uiAnchors() : left(true), top(true), right(false), bottom(false) { }
};


#define UIWINDOW_PARENTCENTER Point(-1000, -1000)


/**
 * @brief Base class for all visible UI elements (Frames and Controls)
 *
 * @see @ref guide_gui
 *
 * @ingroup gui
 */
class uiWindow : public uiEvtHandler {

friend class uiApp;

public:

  /**
   * @brief Creates an instance of the object
   *
   * @param parent The parent window. A window must always have a parent window
   * @param pos Top-left coordinates of the window relative to the parent. Use UIWINDOW_PARENTCENTER to center it over the parent window
   * @param size The window size
   * @param visible If true, the window is immediately visible
   * @param styleClassID Optional style class identifier
   */
  uiWindow(uiWindow * parent, const Point & pos, const Size & size, bool visible, uint32_t styleClassID = 0);

  virtual ~uiWindow();

  virtual void processEvent(uiEvent * event);

  /**
   * @brief Gets next sibling
   *
   * Next sibling is more visible than calling object.
   *
   * @return Next uiWindow object
   */
  uiWindow * next()  { return m_next; }

  /**
   * @brief Gets previous sibling
   *
   * Previous sibling is less visible than calling object.
   *
   * @return Previous uiWindow object
   */
  uiWindow * prev()  { return m_prev; }

  /**
   * @brief Gets first child
   *
   * @return First uiWindow object
   */
  uiWindow * firstChild() { return m_firstChild; }

  /**
   * @brief Gets last child
   *
   * @return Last uiWindow object
   */
  uiWindow * lastChild() { return m_lastChild; }

  /**
   * @brief Determines whether this window has children
   *
   * @return True if window has one o more children
   */
  bool hasChildren() { return m_firstChild != nullptr; }

  /**
   * @brief Brings this window on top
   */
  void bringOnTop();

  /**
   * @brief Brings this window after another one
   *
   * @param insertionPoint This window will be placed after insertionPoint window. If insertionPoint is nullptr this window will be placed at the first position (location of less visibility.).
   */
  void bringAfter(uiWindow * insertionPoint);

  /**
   * @brief Determines the window position relative to parent window
   *
   * To change window position use uiApp::moveWindow().
   *
   * @return The top left point where this window is located
   */
  Point pos() { return m_pos; }

  /**
   * @brief Determines position of the client area
   *
   * @return The top left point where the client area starts
   */
  Point clientPos();

  /**
   * @brief Determines the window size
   *
   * To set window size use uiApp::resizeWindow().
   *
   * @return The window size
   */
  Size size() { return m_size; }

  /**
   * @brief Determines the client area size
   *
   * @return The client area size
   */
  Size clientSize();

  /**
   * @brief Determines the window bounding box
   *
   * To set window position use uiApp::moveWindow(), to set window size use uiApp::resizeWindow() or uiApp::reshapeWindow().
   *
   * @param origin Decides the origin of returned rectangle
   *
   * @return The window bounding box
   */
  Rect rect(uiOrigin origin);

  /**
   * @brief Determines the client area bounding box
   *
   * @param origin Decides the origin of returned rectangle
   *
   * @return The window client area bounding box
   */
  virtual Rect clientRect(uiOrigin origin);

  /**
   * @brief Determines the window state
   *
   * To set window state (hidden, visible) use uiApp::showWindow().
   *
   * @return Current window state
   */
  uiWindowState state() { return m_state; }

  /**
   * @brief Sets or gets window properties
   *
   * @return L-value representing some window properties
   */
  uiWindowProps & windowProps() { return m_windowProps; }

  /**
   * @brief Sets or gets window style
   *
   * @return L-value representing window style (colors, border size, etc.)
   */
  uiWindowStyle & windowStyle() { return m_windowStyle; }

  /**
   * @brief Determines the parent window
   *
   * @return Parent window
   */
  uiWindow * parent() { return m_parent; }

  /**
   * @brief Determines the parent frame
   *
   * @return Parent frame
   */
  uiWindow * parentFrame();

  /**
   * @brief Transforms rectangle origins from current window to another one
   *
   * @param rect Rectangle to transform
   * @param baseWindow Window where the output rectangle will be relative to
   *
   * @return The translated rectangle
   */
  Rect transformRect(Rect const & rect, uiWindow * baseWindow);

  /**
   * @brief Repaints a rectangle of this window
   *
   * @param rect Rectangle to repaint
   */
  void repaint(Rect const & rect);

  /**
   * @brief Repaints this window
   */
  void repaint();

  /**
   * @brief Determines whether the mouse is over this window
   *
   * This methods returns True also when the mouse is captured.
   *
   * @return True if the mouse is over the window
   */
  bool isMouseOver() { return m_isMouseOver; }

  /**
   * @brief Exits from a modal window
   *
   * To open a modal window use uiApp::showModalWindow().
   *
   * @param modalResult An integer value to return to uiApp::showModalWindow().
   */
  void exitModal(int modalResult);

  /**
   * @brief Determines whether this window or control has focus
   *
   * To set currently focused window use uiApp::setFocusedWindow().
   *
   * @return True if this window has focus
   */
  bool hasFocus();

  /**
   * @brief Determines whether this window is the active one
   *
   * @return True if this window is the active window
   */
  bool isActiveWindow();

  /**
   * @brief Allows to switch on or off anchors
   *
   * @return An L-value used to switch on or off anchors
   */
  uiAnchors & anchors() { return m_anchors; }

  /**
   * @brief Sets the focus index (aka tab-index)
   *
   * @param value The focus index. -1 = control isn't included into the focusable controls. 0 = first focusable control.
   */
  void setFocusIndex(int value) { m_focusIndex = value; }

  /**
   * @brief Determines the focus index (aka tab-index)
   *
   * To set focus index use uiWindow::setFocusIndex().
   *
   * @return The focus index
   */
  int focusIndex()                       { return m_focusIndex; }

  Canvas * canvas();

  /**
   * @brief Sets style class for this UI element
   *
   * @param value Style class identifier
   */
  void setStyleClassID(uint16_t value)   { m_styleClassID = value; }

  /**
   * @brief Determines current style class for this UI element
   *
   * @return Style class ID
   */
  uint16_t styleClassID()                { return m_styleClassID; }

  /**
   * @brief Enables a child window to send keyboard events to its parent
   *
   * Events aren't posted, but processed instantly.
   *
   * @param value When true parent processes keyboard events
   */
  void setParentProcessKbdEvents(bool value) { m_parentProcessKbdEvents = value; }


protected:

  void addChild(uiWindow * child);
  void insertAfter(uiWindow * child, uiWindow * underlyingChild);
  void freeChildren();
  void removeChild(uiWindow * child, bool freeChild = true);
  void moveChildOnTop(uiWindow * child);
  void moveAfter(uiWindow * child, uiWindow * underlyingChild);
  bool isChild(uiWindow * window);

  virtual Size minWindowSize()        { return Size(0, 0); }

  void beginPaint(uiEvent * paintEvent, Rect const & clippingRect);

  void generatePaintEvents(Rect const & paintRect);
  void reshape(Rect const & r);

  bool isFocusable();

private:

  void paintWindow();

  uiWindow * findChildWithFocusIndex(int focusIndex, int * maxIndex);


  uiWindow *    m_parent;

  Point         m_pos;
  Size          m_size;

  // double linked list, order is: bottom (first items) -> up (last items)
  uiWindow *    m_next;
  uiWindow *    m_prev;
  uiWindow *    m_firstChild;
  uiWindow *    m_lastChild;

  uiWindowStyle m_windowStyle;

  uiWindowProps m_windowProps;

  uiWindowState m_state;

  uiAnchors     m_anchors;

  int8_t        m_focusIndex;      // -1 = doesn't participate to focus trip

  uint8_t       m_styleClassID;

  uint8_t       m_isMouseOver;     // 1 after mouse entered, 0 after mouse left
  
  uint8_t       m_parentProcessKbdEvents; // if 1 parent processes keyboard events
};



////////////////////////////////////////////////////////////////////////////////////////////////////
// uiFrame


/**
 * Specifies frame style (colors, title font, etc.)
 */
struct uiFrameStyle {
  RGB888              backgroundColor                = RGB888(255, 255, 255);  /**< Frame background color */
  RGB888              titleBackgroundColor           = RGB888(128, 128, 128);  /**< Title background color */
  RGB888              activeTitleBackgroundColor     = RGB888(128, 128, 255);  /**< Title background color when active */
  RGB888              titleColor                     = RGB888(0, 0, 0);        /**< Title color */
  RGB888              activeTitleColor               = RGB888(255, 255, 255);  /**< Title color when active */
  FontInfo const *    titleFont                      = &FONT_std_12;           /**< Title font */
  RGB888              buttonColor                    = RGB888(64, 64, 64);     /**< Color used to draw Close, Maximize and Minimize buttons */
  RGB888              activeButtonColor              = RGB888(255, 255, 255);  /**< Color used to draw Close, Maximize and Minimize buttons when active */
  RGB888              mouseOverBackgroundButtonColor = RGB888(0, 0, 255);      /**< Background color of the Close, Maximize and Minimize buttons when the mouse is over them */
  RGB888              mouseOverButtonColor           = RGB888(255, 255, 255);  /**< Pen color of the Close, Maximize and Minimize buttons when the mouse is over them */

  void adaptToDisplayColors(int displayColors) {
    if (displayColors < 4) {
      titleBackgroundColor           = RGB888(255, 255, 255);
      titleColor                     = RGB888(0, 0, 0);
      buttonColor                    = RGB888(0, 0, 0);
      activeTitleBackgroundColor     = RGB888(0, 0, 0);
      activeTitleColor               = RGB888(255, 255, 255);
      activeButtonColor              = RGB888(255, 255, 255);
      mouseOverButtonColor           = RGB888(0, 0, 0);
      mouseOverBackgroundButtonColor = RGB888(255, 255, 255);
    } else if (displayColors < 16) {
      titleBackgroundColor = RGB888(0, 0, 0);
      titleColor           = RGB888(255, 255, 255);
      buttonColor          = RGB888(255, 255, 255);
      activeButtonColor    = RGB888(0, 0, 0);
    }
  }
};


/**
 * @brief Properties of the frame
 */
struct uiFrameProps {
  uint8_t resizeable        : 1; /**< Frame is resizable. Make sure the window border has enough thickness setting also uiWindowStyle.borderSize */
  uint8_t moveable          : 1; /**< Frame is movable. Make sure the window has a title bar setting window title */
  uint8_t hasCloseButton    : 1; /**< Frame has close button. Make sure the window has a title bar setting window title */
  uint8_t hasMaximizeButton : 1; /**< Frame has maximize button. Make sure the window has a title bar setting window title */
  uint8_t hasMinimizeButton : 1; /**< Frame has minimize button. Make sure the window has a title bar setting window title */
  uint8_t fillBackground    : 1; /**< Frame has filled background */

  uiFrameProps() :
    resizeable(true),
    moveable(true),
    hasCloseButton(true),
    hasMaximizeButton(true),
    hasMinimizeButton(true),
    fillBackground(true)
  { }
};


/** @brief Specifies current frame state */
struct uiFrameState {
  uint8_t maximized : 1;  /**< 0 = normal,   1 = maximized */
  uint8_t minimized : 1;  /**< 0 = normal,   1 = minimized */
};


/** \ingroup Enumerations
 * @brief Frame items
 */
enum class uiFrameItem : uint8_t {
  None,                 /**< No frame item */
  MoveArea,             /**< Title bar area used to move the frame */
  TopLeftResize,        /**< Top-left resize corner */
  TopCenterResize,      /**< Top resize edge */
  TopRightResize,       /**< Top-right resize corner */
  CenterLeftResize,     /**< Left resize edge */
  CenterRightResize,    /**< Right resize edge */
  BottomLeftResize,     /**< Bottom-left resize corner */
  BottomCenterResize,   /**< Bottom resize edge */
  BottomRightResize,    /**< Bottom-right resize corner */
  CloseButton,          /**< Close button */
  MaximizeButton,       /**< Maximize button */
  MinimizeButton,       /**< Minimize button */
};


/**
 * @brief A frame is a window with a title bar, maximize/minimize/close buttons and that is resizable or movable
 *
 * A frame is the unique container that can handle focus travel (TAB and SHIFT-TAB) among child controls.
 *
 * @ingroup gui
 */
class uiFrame : public uiWindow {

public:

  /**
   * @brief Creates an instance of the object
   *
   * @param parent Parent window
   * @param title Title of the frame. An empty string hides the title bar
   * @param pos Top-left coordinates of the frame relative to the parent. Use UIWINDOW_PARENTCENTER to center it over the parent window
   * @param size The frame size
   * @param visible If true, the frame is immediately visible
   * @param styleClassID Optional style class identifier
   */
  uiFrame(uiWindow * parent, char const * title, const Point & pos, const Size & size, bool visible = true, uint32_t styleClassID = 0);

  virtual ~uiFrame();

  virtual void processEvent(uiEvent * event);

  /**
   * @brief Determines the window title
   *
   * @return The window title
   */
  char const * title() { return m_title; }

  /**
   * @brief Sets window title
   *
   * Window must be repainted to display the new title.
   *
   * @param value The window title. nullptr hides the title bar.
   */
  void setTitle(char const * value);

  /**
   * @brief Sets window title as formatted text
   *
   * Window must be repainted to display the new title.
   *
   * @param format printf-like format string.
   */
  void setTitleFmt(const char *format, ...);

  /**
   * @brief Sets or gets frame style
   *
   * @return L-value representing frame style (colors, border size, etc.)
   */
  uiFrameStyle & frameStyle() { return m_frameStyle; }

  /**
   * @brief Sets or gets frame properties
   *
   * @return L-value representing some frame properties
   */
  uiFrameProps & frameProps() { return m_frameProps; }

  Rect clientRect(uiOrigin origin);

  int getNextFreeFocusIndex() { return m_nextFreeFocusIndex++; }

  /**
   * @brief Determines the frame state
   *
   * To set frame state (maximized, minimized) use uiApp::maximizeFrame() or uiApp::minimizeFrame().
   *
   * @return Current window state
   */
  uiFrameState frameState() { return m_frameState; }



  // Delegates

  /**
   * @brief Show window event delegate
   *
   * This delegate is called when the window is about to be shown.
   */
  Delegate<> onShow;

  /**
   * @brief Hide window event delegate
   *
   * This delegate is called when the window is about to be hidden.
   */
  Delegate<> onHide;

  /**
   * @brief Resize window event delegate
   *
   * This delegate is called after its size has changed.
   */
  Delegate<> onResize;

  /**
   * @brief Timer event delegate
   *
   * This delegate is called when the timer expires.
   * To create a timer, call uiApp::setTimer().
   */
  Delegate<uiTimerHandle> onTimer;

  /**
   * @brief Key-down event delegate
   */
  Delegate<uiKeyEventInfo const &> onKeyDown;

  /**
   * @brief Key-up event delegate
   */
  Delegate<uiKeyEventInfo const &> onKeyUp;

  /**
   * @brief Paint event delegate
   */
  Delegate<> onPaint;


protected:

  Size minWindowSize();
  int titleBarHeight();
  Rect titleBarRect();

private:

  void paintFrame();
  int paintButtons(Rect const & bkgRect);
  void movingCapturedMouse(int mouseX, int mouseY, bool mouseIsDown);
  void movingFreeMouse(int mouseX, int mouseY);
  uiFrameItem getFrameItemAt(int x, int y);
  Rect getBtnRect(int buttonIndex);
  void handleButtonsClick(int x, int y, bool doubleClick);
  void drawTextWithEllipsis(FontInfo const * fontInfo, int X, int Y, char const * text, int maxX);
  void drawReshapingBox(Rect boxRect);


  static constexpr int CORNERSENSE = 10;


  uiFrameStyle       m_frameStyle;

  uiFrameProps       m_frameProps;

  char *             m_title;
  int                m_titleLength;

  uiFrameItem        m_mouseDownFrameItem;  // frame item on mouse down
  uiFrameItem        m_mouseMoveFrameItem;  // frame item on mouse move

  Rect               m_lastReshapingBox;    // last reshaping box painted by drawReshapingBox(), (0,0,0,0) if there isn't any

  int                m_nextFreeFocusIndex;

  Point              m_mouseDownPos;        // mouse position when mouse down event has been received

  Rect               m_savedScreenRect;     // saved screen rect before Maximize or Minimize

  Size               m_sizeAtMouseDown;     // used to resize

  uiFrameState       m_frameState;
};



////////////////////////////////////////////////////////////////////////////////////////////////////
// uiControl


/**
 * @brief This is the base class for all controls. A control can have focus and is not activable.
 *
 * @ingroup gui
 */
class uiControl : public uiWindow {

public:

  /**
   * @brief Creates an instance of the object
   *
   * @param parent Parent window
   * @param pos Top-left coordinates of the control relative to the parent
   * @param size The control size
   * @param visible If true, the control is immediately visible
   * @param styleClassID Optional style class identifier
   */
  uiControl(uiWindow * parent, const Point & pos, const Size & size, bool visible, uint32_t styleClassID = 0);

  virtual ~uiControl();

  virtual void processEvent(uiEvent * event);
};



////////////////////////////////////////////////////////////////////////////////////////////////////
// uiScrollableControl


/** @brief Contains the scrollable control style */
struct uiScrollableControlStyle {
  RGB888  scrollBarBackgroundColor          = RGB888(64, 64, 64);     /**< Background color of the scrollbar */
  RGB888  scrollBarForegroundColor          = RGB888(128, 128, 128);  /**< Foreground color of the scrollbar */
  RGB888  mouseOverScrollBarForegroundColor = RGB888(255, 255, 255);  /**< Foreground color of the scrollbar when mouse is over it */
  uint8_t scrollBarSize                     = 11;                     /**< Width of the vertical scrollbar and height of the horizontal scrollbar */

  void adaptToDisplayColors(int displayColors) {
    if (displayColors < 16) {
      mouseOverScrollBarForegroundColor = RGB888(0, 0, 255);
    }
  }
};


/** \ingroup Enumerations
 * @brief Scrollbar items
 */
enum class uiScrollBarItem {
  None,           /**< No scrollbar item */
  LeftButton,     /**< Left arrow button (horizontal scrollbar) */
  RightButton,    /**< Right arrow button (horizontal scrollbar) */
  TopButton,      /**< Up arrow button (vertical scrollbar) */
  BottomButton,   /**< Down arrow button (vertical scrollbar) */
  HBar,           /**< Horizontal scrollbar thumb */
  VBar,           /**< Vertical scrollbar thumb */
  PageUp,         /**< Vertical track area above the thumb */
  PageDown,       /**< Vertical track area below the thumb */
  PageLeft,       /**< Horizontal track area left of the thumb */
  PageRight,      /**< Horizontal track area right of the thumb */
};


/**
 * @brief A scrollable control is a control with optionally vertical and/or horizontal scrollbars
 *
 * @ingroup gui
 */
class uiScrollableControl : public uiControl {

public:

  /**
   * @brief Creates an instance of the object
   *
   * @param parent The parent window. A control must always have a parent window
   * @param pos Top-left coordinates of the control relative to the parent
   * @param size The control size
   * @param visible If true, the control is immediately visible
   * @param styleClassID Optional style class identifier
   */
  uiScrollableControl(uiWindow * parent, const Point & pos, const Size & size, bool visible = true, uint32_t styleClassID = 0);

  virtual ~uiScrollableControl();

  virtual void processEvent(uiEvent * event);

  Rect clientRect(uiOrigin origin);

  /**
   * @brief Sets or gets control style
   *
   * @return L-value representing control style (colors, scrollbars width, etc.)
   */
  uiScrollableControlStyle & scrollableControlStyle() { return m_scrollableControlStyle; }

  /**
   * @brief Determines position of the horizontal scrollbar thumb
   *
   * Minimum value is 0, maximum value is HScrollBarRange() - HScrollBarVisible().
   * Position is measured in "scroll units". A scroll unit is determined by scrollbar size.
   *
   * @return Scrollbar position in scroll units
   */
  int HScrollBarPos()     { return m_HScrollBarPosition; }

  /**
   * @brief Determines horizontal scrollbar visible portion (aka thumb size) of the scrollable content
   *
   * Visible portion is measured in "scroll units". A scroll unit is determined by scrollbar size.
   *
   * @return Scrollbar visible portion in scroll units
   */
  int HScrollBarVisible() { return m_HScrollBarVisible; }

  /**
   * @brief Determines horizontal scrollbar range
   *
   * Range is the maximum position of the scrollbar. Maximum value of scrollbar position will HScrollBarRange() - HScrollBarVisible().
   * Range is measured in "scroll units". A scroll unit is determined by scrollbar size.
   *
   * @return Scrollbar range in scroll units.
   */
  int HScrollBarRange()   { return m_HScrollBarRange; }

  /**
   * @brief Determines position of the vertical scrollbar thumb
   *
   * Minimum value is 0, maximum value is VScrollBarRange() - VScrollBarVisible().
   * Position is measured in "scroll units". A scroll unit is determined by scrollbar size.
   *
   * @return Scrollbar position in scroll units
   */
  int VScrollBarPos()     { return m_VScrollBarPosition; }

  /**
   * @brief Determines vertical scrollbar visible portion (aka thumb size) of the scrollable content
   *
   * Visible portion is measured in "scroll units". A scroll unit is determined by scrollbar size.
   *
   * @return Scrollbar visible portion in scroll units
   */
  int VScrollBarVisible() { return m_VScrollBarVisible; }

  /**
   * @brief Determines vertical scrollbar range
   *
   * Range is the maximum position of the scrollbar. Maximum value of scrollbar position will VScrollBarRange() - VScrollBarVisible().
   * Range is measured in "scroll units". A scroll unit is determined by scrollbar size.
   *
   * @return Scrollbar range in scroll units.
   */
  int VScrollBarRange()  { return m_VScrollBarRange; }


  // Delegates

  /**
   * @brief Horizontal scrollbar change event delegate
   */
  Delegate<> onChangeHScrollBar;

  /**
   * @brief Vertical scrollbar change event delegate
   */
  Delegate<> onChangeVScrollBar;


protected:

  /**
   * @brief Sets scrollbar position, visible portion and range
   *
   * @param orientation Specifies which scrollbar to update
   * @param position Sets position of the scrollbar thumb. Minimum value is 0, maximum value is range - visible. Position is measured in "scroll units". A scroll unit is determined by scrollbar size.
   * @param visible Sets scrollbar visible portion (aka thumb size) of the scrollable content. Visible portion is measured in "scroll units". A scroll unit is determined by scrollbar size.
   * @param range Sets vertical scrollbar range. Range is the maximum position of the scrollbar. Maximum value of scrollbar position will range - visible.
   * @param repaintScrollbar If true, the scrollbar is repainted immediately.
   */
  virtual void setScrollBar(uiOrientation orientation, int position, int visible, int range, bool repaintScrollbar = true);


private:

  void paintScrollableControl();
  void paintScrollBars();
  Rect getVScrollBarRects(Rect * topButton = nullptr, Rect * bottonButton = nullptr, Rect * bar = nullptr);
  Rect getHScrollBarRects(Rect * leftButton = nullptr, Rect * rightButton = nullptr, Rect * bar = nullptr);
  uiScrollBarItem getItemAt(int x, int y);
  void repaintScrollBar(uiOrientation orientation);
  void handleFreeMouseMove(int mouseX, int mouseY);
  void handleCapturedMouseMove(int mouseX, int mouseY);
  void handleButtonsScroll();
  void handlePageScroll();

  uiScrollableControlStyle m_scrollableControlStyle;

  int16_t         m_HScrollBarPosition;
  int16_t         m_HScrollBarVisible;    // it means the "visible" area (how big is the bar)
  int16_t         m_HScrollBarRange;
  int16_t         m_VScrollBarPosition;
  int16_t         m_VScrollBarVisible;    // it means the "visible" area (how big is the bar)
  int16_t         m_VScrollBarRange;

  // values updated by getVScrollBarRects() and getHScrollBarRects()
  int16_t         m_HBarArea;
  int16_t         m_VBarArea;

  int16_t         m_mouseDownHScrollBarPosition;
  int16_t         m_mouseDownVScrollBarPosition;

  uiScrollBarItem m_mouseOverItem;

  // a timer is active while mouse is down and the mouse is over a button
  uiTimerHandle   m_scrollTimer;

  Point           m_mouseDownPos;    // mouse position when mouse down event has been received
};



////////////////////////////////////////////////////////////////////////////////////////////////////
// uiButton


/** @brief Contains the button style */
struct uiButtonStyle {
  RGB888           backgroundColor          = RGB888(128, 128, 128); /**< Background color */
  RGB888           downBackgroundColor      = RGB888(0, 255, 0);     /**< Background color when button is down */
  RGB888           mouseOverBackgroundColor = RGB888(255, 255, 255); /**< Background color when mouse is over */
  RGB888           mouseDownBackgroundColor = RGB888(128, 128, 255); /**< Background color when mouse is down */
  RGB888           mouseOverTextColor       = RGB888(0, 0, 0);       /**< Text color when mouse is over */
  RGB888           textColor                = RGB888(0, 0, 0);       /**< Text color */
  RGB888           downTextColor            = RGB888(0, 0, 0);       /**< Down text color */
  FontInfo const * textFont                 = &FONT_std_14;          /**< Text font */
  uint8_t          bitmapTextSpace          = 4;                     /**< Spaces between image and text */
  Bitmap const *   bitmap                   = nullptr;               /**< Bitmap to display */
  Bitmap const *   downBitmap               = nullptr;               /**< Bitmap to display when button is down */

  void adaptToDisplayColors(int displayColors) {
    if (displayColors < 4) {
      mouseOverBackgroundColor = RGB888(0, 0, 0);
      mouseOverTextColor       = RGB888(255, 255, 255);
      downTextColor            = RGB888(255, 255, 255);
      downBackgroundColor      = RGB888(0, 0, 0);
    } else if (displayColors < 16) {
      mouseOverBackgroundColor = RGB888(255, 255, 255);
      mouseDownBackgroundColor = RGB888(255, 255, 255);
      backgroundColor          = RGB888(0, 0, 255);
      downBackgroundColor      = RGB888(0, 128, 0);
      downTextColor            = displayColors < 8 ? RGB888(0, 0, 0) : RGB888(255, 255, 255);
      textColor                = RGB888(255, 255, 255);
      mouseOverTextColor       = RGB888(0, 0, 0);
    }
  }
};


/** \ingroup Enumerations
 * @brief Specifies the button kind
 */
enum class uiButtonKind {
  Button,   /**< Normal button - can have a single state. Generates only click events */
  Switch,   /**< Switch - can have two states. Generates onChange events */
};


/**
 * @brief Represents a button control. A button can have text and optionally a bitmap
 *
 * @ingroup gui
 */
class uiButton : public uiControl {

public:

  /**
   * @brief Creates an instance of the object
   *
   * @param parent The parent window. A button must always have a parent window
   * @param text The text of the button
   * @param pos Top-left coordinates of the button relative to the parent
   * @param size The button size
   * @param kind The button kind (button or switch)
   * @param visible If true, the button is immediately visible
   * @param styleClassID Optional style class identifier
   */
  uiButton(uiWindow * parent, char const * text, const Point & pos, const Size & size, uiButtonKind kind = uiButtonKind::Button, bool visible = true, uint32_t styleClassID = 0);

  virtual ~uiButton();

  virtual void processEvent(uiEvent * event);

  /**
   * @brief Sets button text
   *
   * Button must be repainted to display the new text.
   *
   * @param value Button text
   */
  void setText(char const * value);

  /**
   * @brief Determines button text
   *
   * @return Button text
   */
  char const * text() { return m_text; }

  /**
   * @brief Sets or gets button style
   *
   * @return L-value representing button style (colors, bitmaps, etc.)
   */
  uiButtonStyle & buttonStyle() { return m_buttonStyle; }

  /**
   * @brief Determines whether the switch button is down or up
   *
   * Only a switch button can be down.
   *
   * @return True if the button is down
   */
  bool down() { return m_down; }

  /**
   * @brief Sets button state of a switch button
   *
   * Only a switch button can be down.
   *
   * @param value Button state (true = down, false = up)
   */
  void setDown(bool value);


  // Delegates

  /**
   * @brief Button changed event delegate
   *
   * This delegate is called whenever a switch button change its state
   */
  Delegate<> onChange;

  /**
   * @brief Mouse click event delegate
   *
   * This delegate is called when the mouse button is pressed and released on the same position.
   */
  Delegate<> onClick;

  /**
   * @brief Mouse down event delegate
   *
   * This delegate is called when the mouse button is pressed
   */
  Delegate<uiMouseEventInfo const&> onMouseDown;

  /**
   * @brief Mouse up event delegate
   *
   * This delegate is called when the mouse button is released
   */
  Delegate<uiMouseEventInfo const&> onMouseUp;


private:

  void paintButton();
  void paintContent(Rect const & rect);

  void trigger();


  uiButtonStyle  m_buttonStyle;

  char *         m_text;
  int            m_textExtent;  // calculated by setText(). TODO: changing font doesn't update m_textExtent!

  bool           m_down;

  uiButtonKind   m_kind;

};



////////////////////////////////////////////////////////////////////////////////////////////////////
// uiTextEdit
// single line text edit


/**
 * @brief Represents text edit style
 */
struct uiTextEditStyle {
  RGB888           backgroundColor            = RGB888(128, 128, 128);     /**< Background color */
  RGB888           mouseOverBackgroundColor   = RGB888(255, 255, 255);     /**< Background color when mouse is over */
  RGB888           focusedBackgroundColor     = RGB888(255, 255, 255);     /**< Background color when focused */
  RGB888           textColor                  = RGB888(0, 0, 0);           /**< Text color */
  FontInfo const * textFont                   = &FONT_std_14;              /**< Text font */

  void adaptToDisplayColors(int displayColors) {
    if (displayColors < 16) {
    }
  }
};


/**
 * @brief Properties of the text edit
 */
struct uiTextEditProps {
  uint8_t hasCaret     : 1;   /**< If true, the edit box has a blinking caret */
  uint8_t allowEdit    : 1;   /**< If true, the edit box allows edit */
  uint8_t passwordMode : 1;   /**< If true, the edit box shows '*' in place of actual characters */

  uiTextEditProps()
    : hasCaret(true),
      allowEdit(true),
      passwordMode(false)
    {
    }
};


/**
 * @brief Represents a text edit control.
 *
 * A text edit is a single line editor, horizontally scrollable, text selection and replacement.
 *
 * @ingroup gui
 */
class uiTextEdit : public uiControl {

public:

  /**
   * @brief Creates an instance of the object
   *
   * @param parent The parent window. A text edit must always have a parent window
   * @param text Optional initial text
   * @param pos Top-left coordinates of the text edit relative to the parent
   * @param size The text edit size
   * @param visible If true, the button is immediately visible
   * @param styleClassID Optional style class identifier
   */
  uiTextEdit(uiWindow * parent, char const * text, const Point & pos, const Size & size, bool visible = true, uint32_t styleClassID = 0);

  virtual ~uiTextEdit();

  virtual void processEvent(uiEvent * event);

  /**
   * @brief Sets or gets text edit style
   *
   * @return L-value representing text edit style (colors, font, etc.)
   */
  uiTextEditStyle & textEditStyle() { return m_textEditStyle; }

  /**
   * @brief Sets or gets text edit properties
   *
   * @return L-value representing some text edit properties
   */
  uiTextEditProps & textEditProps() { return m_textEditProps; }

  /**
   * @brief Replaces current text
   *
   * Text edit must be repainted to display the new text.
   *
   * @param value Text to set
   */
  void setText(char const * value);

  /**
   * @brief Replaces current text
   *
   * Text edit must be repainted to display the new text.
   *
   * @param format printf-like format string.
   */
  void setTextFmt(const char *format, ...);

  /**
   * @brief Gets current content of the text edit
   *
   * @return Text edit content
   */
  char const * text() { return m_text; }


  // Delegates

  /**
   * @brief Text edit event delegate
   */
  Delegate<> onChange;

  /**
   * @brief Key-type event delegate
   */
  Delegate<uiKeyEventInfo const &> onKeyType;
  


protected:

  virtual Rect getEditRect();

private:

  void paintTextEdit();
  void paintContent();

  uint8_t const * getCharInfo(char ch, int * width);
  int charColumnToWindowX(int col);
  void updateCaret();
  void moveCursor(int col, int selCol);
  int getColFromMouseX(int mouseX);
  void handleKeyDown(uiKeyEventInfo const & key);
  void checkAllocatedSpace(int requiredLength);
  void insert(char c);
  void removeSel();
  int getWordPosAtLeft();
  int getWordPosAtRight();
  void selectWordAt(int mouseX);
  int keyToASCII(uiKeyEventInfo const & key);


  uiTextEditStyle m_textEditStyle;
  uiTextEditProps m_textEditProps;

  char *           m_text;
  int              m_textLength; // text length NOT including ending zero
  int              m_textSpace;  // actual space allocated including ending zero

  // rectangle where text will be painted (this is also the text clipping rect)
  Rect             m_contentRect;  // updated on painting

  // where text starts to be painted. Values less than m_contentRect.X1 are used to show characters which do not fit in m_contentRect
  int              m_viewX;

  // character index of cursor position (0 = at first char)
  int              m_cursorCol;

  // character index at start of selection (not included if < m_cursorCol, included if > m_cursorCol)
  int              m_selCursorCol;

  CodePage const * m_codepage;

};



////////////////////////////////////////////////////////////////////////////////////////////////////
// uiLabel


/** @brief Contains the label style */
struct uiLabelStyle {
  FontInfo const * textFont                 = &FONT_std_14;              /**< Text font */
  RGB888           backgroundColor          = RGB888(255, 255, 255);     /**< Background color */
  RGB888           textColor                = RGB888(0, 0, 0);           /**< Text color */
  uiHAlign         textAlign                = uiHAlign::Left;            /**< Text horizontal alignment */

  void adaptToDisplayColors(int displayColors) {
    if (displayColors < 16) {
    }
  }
};


/**
 * @brief A label is a static text UI element
 *
 * @ingroup gui
 */
class uiLabel : public uiControl {

public:

  /**
   * @brief Creates an instance of the object
   *
   * @param parent The parent window. A label must always have a parent window
   * @param text Text of the label
   * @param pos Top-left coordinates of the label relative to the parent
   * @param size The label size. If Size(0, 0) then size is automatically calculated
   * @param visible If true, the label is immediately visible
   * @param styleClassID Optional style class identifier
   */
  uiLabel(uiWindow * parent, char const * text, const Point & pos, const Size & size = Size(0, 0), bool visible = true, uint32_t styleClassID = 0);

  virtual ~uiLabel();

  virtual void processEvent(uiEvent * event);

  /**
   * @brief Sets label text
   *
   * To update the label content and size, call uiStaticLabel::update().
   *
   * @param value Label text
   */
  void setText(char const * value);

  /**
   * @brief Sets label formatted text
   *
   * To update the label content and size, call uiStaticLabel::update().
   *
   * @param format printf-like format string.
   */
  void setTextFmt(const char *format, ...);

  /**
   * @brief Determines label text
   *
   * @return Label text
   */
  char const * text() { return m_text; }

  /**
   * @brief Sets or gets label style
   *
   * @return L-value representing label style (colors, font, etc.)
   */
  uiLabelStyle & labelStyle() { return m_labelStyle; }

  /**
   * @brief Updates the label content
   *
   * Call this method whenever text or font changes.
   */
  void update();

  /**
   * @brief Mouse click event delegate
   *
   * This delegate is called when the mouse button is pressed and released on the same position.
   */
  Delegate<> onClick;


private:

  void paintLabel();


  char *         m_text;

  uiLabelStyle   m_labelStyle;

  uint8_t        m_autoSize;

};



////////////////////////////////////////////////////////////////////////////////////////////////////
// uiStaticLabel


/** @brief Contains the label style */
struct uiStaticLabelStyle {
  FontInfo const * textFont                 = &FONT_std_14;              /**< Text font */
  RGB888           backgroundColor          = RGB888(255, 255, 255);     /**< Background color */
  RGB888           textColor                = RGB888(0, 0, 0);           /**< Text color */

  void adaptToDisplayColors(int displayColors) {
    if (displayColors < 16) {
    }
  }
};


/**
 * @brief A staticlabel is a light version of uiLabel (text must be static). uiStaticLabel has lower memory footprint than uiLabel
 *
 * @ingroup gui
 */
class uiStaticLabel : public uiControl {

public:

  /**
   * @brief Creates an instance of the object
   *
   * @param parent The parent window. A label must always have a parent window
   * @param text Text of the label. Must be Static. This is not owned by the object
   * @param pos Top-left coordinates of the label relative to the parent
   * @param visible If true, the label is immediately visible
   * @param styleClassID Optional style class identifier
   */
  uiStaticLabel(uiWindow * parent, char const * text, const Point & pos, bool visible = true, uint32_t styleClassID = 0);
  
  virtual void processEvent(uiEvent * event);

  /**
   * @brief Sets label text
   *
   * To update the label content and size, call uiStaticLabel::update().
   * Text is not owned by the object (must be static or deleted by the application)
   *
   * @param value Label text
   */
  void setText(char const * value);

  /**
   * @brief Determines label text
   *
   * @return Label text
   */
  char const * text() { return m_text; }

  /**
   * @brief Sets or gets label style
   *
   * @return L-value representing label style (colors, font, etc.)
   */
  uiStaticLabelStyle & labelStyle() { return m_staticLabelStyle; }

  /**
   * @brief Updates the label content
   *
   * Call this method whenever text or font changes.
   */
  void update();


private:

  void paintLabel();


  char const *       m_text;

  uiStaticLabelStyle m_staticLabelStyle;

};



////////////////////////////////////////////////////////////////////////////////////////////////////
// uiImage


/** @brief Contains the image style */
struct uiImageStyle {
  RGB888 backgroundColor = RGB888(255, 255, 255);   /**< Background color */

  void adaptToDisplayColors(int displayColors) {
    if (displayColors < 16) {
    }
  }
};


/**
 * @brief Image control to display a static bitmap
 *
 * @ingroup gui
 */
class uiImage : public uiControl {

public:

  /**
   * @brief Creates an instance of the object
   *
   * @param parent The parent window. An image must always have a parent window
   * @param bitmap Bitmap to display
   * @param pos Top-left coordinates of the image relative to the parent
   * @param size The image size. If Size(0, 0) then size is automatically calculated
   * @param visible If true, the image is immediately visible
   * @param styleClassID Optional style class identifier
   */
  uiImage(uiWindow * parent, Bitmap const * bitmap, const Point & pos, const Size & size = Size(0, 0), bool visible = true, uint32_t styleClassID = 0);

  virtual ~uiImage();

  virtual void processEvent(uiEvent * event);

  /**
   * @brief Sets image bitmap
   *
   * Image must be repainted to display the new bitmap.
   *
   * @param bitmap Image bitmap
   */
  void setBitmap(Bitmap const * bitmap);

  /**
   * @brief Gets image bitmap
   *
   * @return Image bitmap
   */
  Bitmap const * bitmap() { return m_bitmap; }

  /**
   * @brief Sets or gets image style
   *
   * @return L-value representing image style
   */
  uiImageStyle & imageStyle() { return m_imageStyle; }


private:

  void paintImage();


  Bitmap const * m_bitmap;

  uiImageStyle   m_imageStyle;

  bool           m_autoSize;

};



////////////////////////////////////////////////////////////////////////////////////////////////////
// uiPanel


/** @brief Contains the panel style */
struct uiPanelStyle {
  RGB888 backgroundColor = RGB888(128, 128, 128);    /**< Panel background color */

  void adaptToDisplayColors(int displayColors) {
    if (displayColors < 16) {
    }
  }
};


/**
 * @brief A panel is used to contain and to group some controls
 *
 * @ingroup gui
 */
class uiPanel : public uiControl {

public:

  /**
   * @brief Creates an instance of the object
   *
   * @param parent The parent window. A panel must always have a parent window
   * @param pos Top-left coordinates of the panel relative to the parent
   * @param size The panel size
   * @param visible If true, the panel is immediately visible
   * @param styleClassID Optional style class identifier
   */
  uiPanel(uiWindow * parent, const Point & pos, const Size & size, bool visible = true, uint32_t styleClassID = 0);

  virtual ~uiPanel();

  virtual void processEvent(uiEvent * event);

  /**
   * @brief Sets or gets panel style
   *
   * @return L-value representing panel style
   */
  uiPanelStyle & panelStyle() { return m_panelStyle; }


private:

  void paintPanel();


  uiPanelStyle   m_panelStyle;
};



////////////////////////////////////////////////////////////////////////////////////////////////////
// uiPaintBox


/** @brief Contains the paintbox style */
struct uiPaintBoxStyle {
  RGB888 backgroundColor = RGB888(128, 128, 128);   /**< Paintbox background color */

  void adaptToDisplayColors(int displayColors) {
    if (displayColors < 16) {
    }
  }
};


/**
 * @brief A paintbox control allows applications to perform custom drawings providing uiPaintBox.onPaint delegate. A paintbox can have horizontal and vertical scrollbars.
 *
 * @ingroup gui
 */
class uiPaintBox : public uiScrollableControl {

public:

  /**
   * @brief Creates an instance of the object
   *
   * @param parent The parent window. A paintbox must always have a parent window
   * @param pos Top-left coordinates of the paintbox relative to the parent
   * @param size The paintbox size
   * @param visible If true, the paintbox is immediately visible
   * @param styleClassID Optional style class identifier
   */
  uiPaintBox(uiWindow * parent, const Point & pos, const Size & size, bool visible = true, uint32_t styleClassID = 0);

  virtual ~uiPaintBox();

  virtual void processEvent(uiEvent * event);

  /**
   * @brief Sets or gets paintbox style
   *
   * @return L-value representing paintbox style
   */
  uiPaintBoxStyle & paintBoxStyle() { return m_paintBoxStyle; }

  using uiScrollableControl::setScrollBar;

  // Delegates

  /**
   * @brief Paint event delegate
   *
   * Applications use this delegate to perform custom drawings.
   */
  Delegate<Rect const &> onPaint;


private:

  void paintPaintBox();


  uiPaintBoxStyle m_paintBoxStyle;
};



////////////////////////////////////////////////////////////////////////////////////////////////////
// uiColorBox

/**
 * @brief A color box is a control that displays a single color
 *
 * @ingroup gui
 */
class uiColorBox : public uiControl {

public:

  /**
   * @brief Creates an instance of the object
   *
   * @param parent The parent window. A panel must always have a parent window
   * @param pos Top-left coordinates of the panel relative to the parent
   * @param size The panel size
   * @param color Initial color
   * @param visible If true, the panel is immediately visible
   * @param styleClassID Optional style class identifier
   */
  uiColorBox(uiWindow * parent, const Point & pos, const Size & size, Color color = Color::BrightWhite, bool visible = true, uint32_t styleClassID = 0);

  virtual ~uiColorBox();

  virtual void processEvent(uiEvent * event);

  /**
   * @brief Gets current colorbox color
   *
   * @return Current color
   */
  Color color() { return m_color; }

  /**
   * @brief Sets current colorbox color
   *
   * @param value Color to set
   */
  void setColor(Color value);

private:

  void paintColorBox();


  Color m_color;
};



////////////////////////////////////////////////////////////////////////////////////////////////////
// uiCustomListBox


/** @brief Contains the listbox style */
struct uiListBoxStyle {
  RGB888           backgroundColor                = RGB888(128, 128, 128);   /**< Background color */
  RGB888           focusedBackgroundColor         = RGB888(255, 255, 255);   /**< Background color when focused */
  RGB888           selectedBackgroundColor        = RGB888(0, 0, 255);       /**< Background color when selected */
  RGB888           focusedSelectedBackgroundColor = RGB888(0, 0, 255);       /**< Background color when selected and focused */
  int              itemHeight                     = 16;                      /**< Item height in pixels */
  FontInfo const * textFont                       = &FONT_std_14;            /**< Text font */
  RGB888           textColor                      = RGB888(0, 0, 0);         /**< Text foreground color */
  RGB888           selectedTextColor              = RGB888(255, 255, 255);   /**< Text foreground color when selected */

  void adaptToDisplayColors(int displayColors) {
    if (displayColors < 4) {
      selectedBackgroundColor        = RGB888(0, 0, 0);
      focusedSelectedBackgroundColor = RGB888(0, 0, 0);
      selectedTextColor              = RGB888(255, 255, 255);
    }
  }
};


/**
 * @brief Properties of the list box
 */
struct uiListBoxProps {
  uint8_t allowMultiSelect  : 1;   /**< If true, the listbox allows to select multiple items */
  uint8_t selectOnMouseOver : 1;   /**< If true, an item is selected when the mouse is over it */

  uiListBoxProps()
    : allowMultiSelect(true),
      selectOnMouseOver(false)
    {
    }
};


/**
 * @brief Base class for list controls that display a list of selectable items
 *
 * @ingroup gui
 */
class uiCustomListBox : public uiScrollableControl {

public:

  /**
   * @brief Creates an instance of the object
   *
   * @param parent The parent window. A listbox must always have a parent window
   * @param pos Top-left coordinates of the listbox relative to the parent
   * @param size The listbox size
   * @param visible If true, the listbox is immediately visible
   * @param styleClassID Optional style class identifier
   */
  uiCustomListBox(uiWindow * parent, const Point & pos, const Size & size, bool visible = true, uint32_t styleClassID = 0);

  virtual ~uiCustomListBox();

  virtual void processEvent(uiEvent * event);

  /**
   * @brief Sets or gets listbox style
   *
   * @return L-value representing listbox style
   */
  uiListBoxStyle & listBoxStyle() { return m_listBoxStyle; }

  /**
   * @brief Sets or gets list box properties
   *
   * @return L-value representing some list box properties
   */
  uiListBoxProps & listBoxProps() { return m_listBoxProps; }

  /**
   * @brief Gets the first selected item
   *
   * @return Index of selected item or -1 if no item is selected
   */
  int firstSelectedItem();

  /**
   * @brief Gets the last selected item
   *
   * @return Index of selected item or -1 if no item is selected
   */
  int lastSelectedItem();

  /**
   * @brief Selects a listbox item
   *
   * @param index Index of item to select
   * @param add If true, this item is added to the selected items, otherwise all other items are deselected
   * @param range If true, selects a range of items
   */
  void selectItem(int index, bool add = false, bool range = false);

  /**
   * @brief Deselects all selected items
   */
  void deselectAll();


  // Delegates

  /**
   * @brief Change event delegate
   *
   * This delegate is called whenever an item is selected or deselected.
   */
  Delegate<> onChange;

  /**
   * @brief Kill focus event delegate
   */
  Delegate<> onKillFocus;

  /**
   * @brief Key-type event delegate
   */
  Delegate<uiKeyEventInfo const &> onKeyType;

  /**
   * @brief Key-up event delegate
   */
  Delegate<uiKeyEventInfo const &> onKeyUp;

  /**
   * @brief Mouse click event delegate
   *
   * This delegate is called when the mouse button is pressed and released on the same position.
   */
  Delegate<> onClick;

  /**
   * @brief Mouse double click event delegate
   *
   * This delegate is called when the mouse button is double pressed and released on the same position.
   * To change double click time use uiAppProps.doubleClickTime of uiApp::appProps().
   */
  Delegate<> onDblClick;



protected:

  void setScrollBar(uiOrientation orientation, int position, int visible, int range, bool repaintScrollbar);
  int getItemAtMousePos(int mouseX, int mouseY);

  // must be implemented by inherited class
  virtual int items_getCount()                              = 0;
  virtual void items_deselectAll()                          = 0;
  virtual void items_select(int index, bool select)         = 0;
  virtual bool items_selected(int index)                    = 0;
  virtual void items_draw(int index, const Rect & itemRect) = 0;

private:

  void paintListBox();
  void mouseDownSelect(int mouseX, int mouseY);
  void mouseMoveSelect(int mouseX, int mouseY);
  void handleKeyDown(uiKeyEventInfo key);
  void makeItemVisible(int index);


  uiListBoxStyle m_listBoxStyle;
  uiListBoxProps m_listBoxProps;
  int            m_firstVisibleItem;     // the item on the top
};



////////////////////////////////////////////////////////////////////////////////////////////////////
// uiListBox


/**
 * @brief Shows a list of selectable string items
 *
 * @ingroup gui
 */
class uiListBox : public uiCustomListBox {

public:

  /**
   * @brief Creates an instance of the object
   *
   * @param parent The parent window. A listbox must always have a parent window
   * @param pos Top-left coordinates of the listbox relative to the parent
   * @param size The listbox size
   * @param visible If true, the listbox is immediately visible
   * @param styleClassID Optional style class identifier
   */
  uiListBox(uiWindow * parent, const Point & pos, const Size & size, bool visible = true, uint32_t styleClassID = 0);

  /**
   * @brief A list of strings representing the listbox content
   *
   * Other than actual strings, StringList indicates which items are selected.
   * Repainting is required when the string list changes.
   *
   * @return L-value representing listbox items
   */
  StringList & items()                              { return m_items; }

protected:

  virtual int items_getCount()                      { return m_items.count(); }
  virtual void items_deselectAll()                  { m_items.deselectAll(); }
  virtual void items_select(int index, bool select) { m_items.select(index, select); }
  virtual bool items_selected(int index)            { return m_items.selected(index); }
  virtual void items_draw(int index, const Rect & itemRect);


private:

  StringList     m_items;
};


////////////////////////////////////////////////////////////////////////////////////////////////////
// uiFileBrowser

/**
 * @brief Shows and navigates Virtual Filesystem content
 *
 * @ingroup gui
 */
class uiFileBrowser : public uiCustomListBox {

public:

  /**
   * @brief Creates an instance of the object
   *
   * @param parent The parent window. A listbox must always have a parent window
   * @param pos Top-left coordinates of the listbox relative to the parent
   * @param size The listbox size
   * @param visible If true, the listbox is immediately visible
   * @param styleClassID Optional style class identifier
   */
  uiFileBrowser(uiWindow * parent, const Point & pos, const Size & size, bool visible = true, uint32_t styleClassID = 0);

  /**
   * @brief Sets current directory as absolute path
   *
   * Path can include subdirectories (even SPIFFS emulated directories).
   *
   * @param path Absolute path. It must include filesystem path (e.g. "/spiffs")
   */
  void setDirectory(char const * path);

  /**
   * @brief Changes current directory as relative path
   *
   * Path can include subdirectories (even SPIFFS emulated directories).
   *
   * @param path Relative path (e.g. '../subdir')
   */
  void changeDirectory(char const * path);

  /**
   * @brief Determines current directory
   *
   * @return Full path of current directory
   */
  char const * directory() { return m_dir.directory(); }

  /**
   * @brief Determines number of files in current directory
   *
   * @return Number of files, included parent link ("..").
   */
  int count()              { return m_dir.count(); }

  /**
   * @brief Currently selected filename
   *
   * @return Currently selected filename or nullptr.
   */
  char const * filename();

  /**
   * @brief Determines whether currently selected item is a directory
   *
   * @return True if currently selected item is a directory. If False it is an ordinary file.
   */
  bool isDirectory();

  void processEvent(uiEvent * event);

  /**
   * @brief Reloads current directory content and repaints
   */
  void update();

  /**
   * @brief Contains current directory representation
   *
   * @return A FileBrowser object which allows to know directory content and to manipulate it
   */
  FileBrowser & content() { return m_dir; }


protected:

  virtual int items_getCount()                      { return m_dir.count(); }
  virtual void items_deselectAll()                  { m_selected = -1; }
  virtual void items_select(int index, bool select);
  virtual bool items_selected(int index)            { return index == m_selected; }
  virtual void items_draw(int index, const Rect & itemRect);

private:

  void enterSubDir();

  FileBrowser m_dir;
  int         m_selected;  // -1 = no sel

};


////////////////////////////////////////////////////////////////////////////////////////////////////
// uiColorListBox

/**
 * @brief Shows a list of 16 colors, one selectable
 *
 * @ingroup gui
 */
class uiColorListBox : public uiCustomListBox {

public:

  /**
   * @brief Creates an instance of the object
   *
   * @param parent The parent window. A listbox must always have a parent window
   * @param pos Top-left coordinates of the listbox relative to the parent
   * @param size The listbox size
   * @param visible If true, the listbox is immediately visible
   * @param styleClassID Optional style class identifier
   */
  uiColorListBox(uiWindow * parent, const Point & pos, const Size & size, bool visible = true, uint32_t styleClassID = 0);

  /**
   * @brief Currently selected color
   *
   * @return Currently selected color.
   */
  Color color();


protected:

  virtual int items_getCount()                      { return 16; }
  virtual void items_deselectAll()                  { }
  virtual void items_select(int index, bool select) { if (select) m_selectedColor = (Color)index; }
  virtual bool items_selected(int index)            { return index == (int)m_selectedColor; }
  virtual void items_draw(int index, const Rect & itemRect);


private:

  Color m_selectedColor;
};



////////////////////////////////////////////////////////////////////////////////////////////////////
// uiCustomComboBox


/** @brief Contains the listbox style */
struct uiComboBoxStyle {
  RGB888 buttonBackgroundColor = RGB888(64, 64, 64);     /**< Background color of open/close button */
  RGB888 buttonColor           = RGB888(128, 128, 128);  /**< Foreground color of open/close button */

  void adaptToDisplayColors(int displayColors) {
    if (displayColors < 16) {
    }
  }
};


/** @brief Properties of the combobox */
struct uiComboBoxProps {
  uint8_t openOnFocus  : 1;  /**< Open the combobox when it acquires focus */

  uiComboBoxProps()
    : openOnFocus(true)
  {
  }
};


/**
 * @brief This is a combination of a listbox and another component, base of all combobox components
 *
 * @ingroup gui
 */
class uiCustomComboBox : public uiControl
{

public:

  /**
   * @brief Creates an instance of the object
   *
   * @param parent The parent window. A combobox must always have a parent window
   * @param pos Top-left coordinates of the combobox relative to the parent
   * @param size The combobox size
   * @param listHeight Height in pixels of the open listbox
   * @param visible If true, the combobox is immediately visible
   * @param styleClassID Optional style class identifier
   */
  uiCustomComboBox(uiWindow * parent, const Point & pos, const Size & size, int listHeight, bool visible, uint32_t styleClassID);

  ~uiCustomComboBox();

  virtual void processEvent(uiEvent * event);

  /**
   * @brief Sets or gets combobox style
   *
   * @return L-value representing combobox style
   */
  uiComboBoxStyle & comboBoxStyle() { return m_comboBoxStyle; }

  /**
   * @brief Sets or gets listbox style
   *
   * @return L-value representing listbox style
   */
  uiListBoxStyle & listBoxStyle() { return listbox()->listBoxStyle(); }

  /**
   * @brief Sets or gets combobox properties
   *
   * @return L-value representing some combobox properties
   */
  uiComboBoxProps & comboBoxProps() { return m_comboBoxProps; }

  /**
   * @brief Represents currently selected item
   *
   * @return Index of the selected item or -1 if no item is selected
   */
  int selectedItem() { return listbox()->firstSelectedItem(); }

  /**
   * @brief Selects an item
   *
   * @param index Index of the item to select
   */
  void selectItem(int index);


  // Delegates

  /**
   * @brief Change event delegate
   *
   * This delegate is called whenever an item is selected.
   */
  Delegate<> onChange;


protected:

  virtual uiCustomListBox * listbox() = 0;
  virtual uiControl * editcontrol()   = 0;
  virtual void updateEditControl()    = 0;

  Size getEditControlSize();

  virtual void openListBox();
  virtual void closeListBox();
  void switchListBox();

  virtual void paintButton();
  Rect getButtonRect();

  uiWindow * getListBoxParent() { return m_listBoxParent; }

  bool isListBoxOpen()          { return m_listBoxParent->state().visible; }

private:

  int buttonWidth();


  int16_t           m_listHeight;
  int16_t           m_loseFocusBy;
  uiComboBoxStyle   m_comboBoxStyle;
  uiComboBoxProps   m_comboBoxProps;
  uiWindow *        m_listBoxParent;
};




////////////////////////////////////////////////////////////////////////////////////////////////////
// uiComboBox


/**
 * @brief This is a combination of a listbox and a single-line editable textbox
 *
 * @ingroup gui
 */
class uiComboBox : public uiCustomComboBox
{

public:

  /**
   * @brief Creates an instance of the object
   *
   * @param parent The parent window. A combobox must always have a parent window
   * @param pos Top-left coordinates of the combobox relative to the parent
   * @param size The combobox size
   * @param listHeight Height in pixels of the open listbox
   * @param visible If true, the combobox is immediately visible
   * @param styleClassID Optional style class identifier
   */
  uiComboBox(uiWindow * parent, const Point & pos, const Size & size, int listHeight, bool visible = true, uint32_t styleClassID = 0);

  ~uiComboBox();

  /**
   * @brief A list of strings representing items of the combobox
   *
   * Repainting is required when the string list changes.
   *
   * @return L-value representing combobox items
   */
  StringList & items() { return m_listBox->items(); }

  /**
   * @brief Sets or gets text edit style
   *
   * @return L-value representing text edit style (colors, font, etc.)
   */
  uiTextEditStyle & textEditStyle() { return m_textEdit->textEditStyle(); }

  /**
   * @brief Sets or gets text edit properties
   *
   * @return L-value representing some text edit properties
   */
  uiTextEditProps & textEditProps() { return m_textEdit->textEditProps(); }

  /**
   * @brief Replaces current text
   *
   * Text edit must be repainted to display the new text.
   *
   * @param value Text to set
   */
  void setText(char const * value) { m_textEdit->setText(value); }

  /**
   * @brief Gets current content of the text edit
   *
   * @return Text edit content
   */
  char const * text() { return m_textEdit->text(); }


protected:

  uiCustomListBox * listbox()  { return m_listBox; }
  uiControl * editcontrol()    { return m_textEdit; }
  void updateEditControl();

private:
  uiTextEdit * m_textEdit;
  uiListBox *  m_listBox;

};



////////////////////////////////////////////////////////////////////////////////////////////////////
// uiColorComboBox


/**
 * @brief This is a combination of a color listbox and a colorbox
 *
 * @ingroup gui
 */
class uiColorComboBox : public uiCustomComboBox
{

public:

  /**
   * @brief Creates an instance of the object
   *
   * @param parent The parent window. A combobox must always have a parent window
   * @param pos Top-left coordinates of the combobox relative to the parent
   * @param size The combobox size
   * @param listHeight Height in pixels of the open listbox
   * @param visible If true, the combobox is immediately visible
   * @param styleClassID Optional style class identifier
   */
  uiColorComboBox(uiWindow * parent, const Point & pos, const Size & size, int listHeight, bool visible = true, uint32_t styleClassID = 0);

  ~uiColorComboBox();

  /**
   * @brief Sets current selected color
   *
   * @param value Color to set
   */
  void selectColor(Color value) { selectItem((int)value); }

  /**
   * @brief Determines current selected color
   *
   * @return Currently selected color
   */
  Color selectedColor()         { return (Color) selectedItem(); }


protected:

  uiCustomListBox * listbox()  { return m_colorListBox; }
  uiControl * editcontrol()    { return m_colorBox; }
  void updateEditControl();

private:
  uiColorBox *     m_colorBox;
  uiColorListBox * m_colorListBox;

};



////////////////////////////////////////////////////////////////////////////////////////////////////
// uiCheckBox


/** @brief Contains the checkbox style */
struct uiCheckBoxStyle {
  RGB888              backgroundColor          = RGB888(128, 128, 128);  /**< Background color */
  RGB888              checkedBackgroundColor   = RGB888(128, 128, 128);  /**< Background color when checked */
  RGB888              mouseOverBackgroundColor = RGB888(128, 128, 255);  /**< Background color when mouse is over */
  RGB888              mouseOverForegroundColor = RGB888(0, 0, 0);        /**< Foreground color when mouse is over */
  RGB888              foregroundColor          = RGB888(0, 0, 0);        /**< Foreground color */

  void adaptToDisplayColors(int displayColors) {
    if (displayColors < 4) {
    } else if (displayColors < 16) {
      mouseOverForegroundColor = RGB888(255, 255, 255);
    }
  }
};


/** \ingroup Enumerations
 * @brief Specifies the kind of a uiCheckBox (checkbox or radio button)
 */
enum class uiCheckBoxKind : int8_t {
  CheckBox,          /**< Normal checkbox */
  RadioButton,       /**< Radio-button */
};


/**
 * @brief Represents a checkbox or a radiobutton.
 *
 * A checkbox permits the user to make a binary choice.
 * A radio button allows the user to choose only one of a predefined set of mutually exclusive options.
 *
 * @ingroup gui
 */
class uiCheckBox : public uiControl {

public:

  /**
   * @brief Creates an instance of the object
   *
   * @param parent The parent window. A checkbox must always have a parent window
   * @param pos Top-left coordinates of the checkbox relative to the parent
   * @param size The checkbox size
   * @param kind Defines the checkbox behaviour: checkbox or radiobutton
   * @param visible If true, the checkbox is immediately visible
   * @param styleClassID Optional style class identifier
   */
  uiCheckBox(uiWindow * parent, const Point & pos, const Size & size, uiCheckBoxKind kind = uiCheckBoxKind::CheckBox, bool visible = true, uint32_t styleClassID = 0);

  virtual ~uiCheckBox();

  virtual void processEvent(uiEvent * event);

  /**
   * @brief Sets or gets checkbox style
   *
   * @return L-value representing checkbox style
   */
  uiCheckBoxStyle & checkBoxStyle() { return m_checkBoxStyle; }

  /**
   * @brief Determines whether the checkbox or radiobutton is checked
   *
   * @return True when checked
   */
  bool checked()                    { return m_checked; }

  /**
   * @brief Sets current checkbox or radiobutton checked status
   *
   * When in radiobutton mode, setting True all other items with the same uiCheckBox::groupIndex() are deselected.
   *
   * @param value Checkbox or radiobutton checked status
   */
  void setChecked(bool value);

  /**
   * @brief Determines radiobutton group index
   *
   * @return The radiobutton group index (-1 = no group)
   */
  int groupIndex()                  { return m_groupIndex; }

  /**
   * @brief Sets radiobutton group index
   *
   * @param value Radiobutton group index (-1 = no group)
   */
  void setGroupIndex(int value)     { m_groupIndex = value; }


  // Delegates

  /**
   * @brief Change event delegate
   *
   * This delegate is called whenever checkbox or radiobutton is checked or unchecked.
   */
  Delegate<> onChange;

  /**
   * @brief Mouse click event delegate
   *
   * This delegate is called when the mouse button is pressed and released on the same position.
   */
  Delegate<> onClick;


private:

  void paintCheckBox();
  void trigger();
  void unCheckGroup();


  uiCheckBoxStyle m_checkBoxStyle;
  bool            m_checked;
  uiCheckBoxKind  m_kind;
  int16_t         m_groupIndex; // -1 = no group

};



////////////////////////////////////////////////////////////////////////////////////////////////////
// uiSlider


/** @brief Contains the slider style */
struct uiSliderStyle {
  RGB888 backgroundColor    = RGB888(255, 255, 255);    /**< Slider background color */
  RGB888 slideColor         = RGB888(0, 128, 128);      /**< Color of internal slide */
  RGB888 rangeColor         = RGB888(0, 128, 255);      /**< Color of slide before the grip */
  RGB888 gripColor          = RGB888(0, 0, 255);        /**< Color of slider grip */
  RGB888 ticksColor         = RGB888(255, 255, 255);    /**< Ticks color */
  RGB888 mouseOverGripColor = RGB888(255, 255, 255);        /**< Color of slider grip when mouse is over */

  void adaptToDisplayColors(int displayColors) {
    if (displayColors < 4) {
      slideColor         = RGB888(0, 0, 0);
      rangeColor         = RGB888(0, 0, 0);
      gripColor          = RGB888(0, 0, 0);
      mouseOverGripColor = RGB888(255, 255, 255);
    } else if (displayColors < 16) {
      slideColor         = RGB888(0, 0, 0);
      mouseOverGripColor = RGB888(255, 255, 255);
    }
  }
};


/**
 * @brief A slider or track bar is a graphical control element with which a user may set a value by moving an indicator
 *
 * @ingroup gui
 */
class uiSlider : public uiControl {

public:

  /**
   * @brief Creates an instance of the object
   *
   * @param parent The parent window. A slider must always have a parent window
   * @param pos Top-left coordinates of the slider relative to the parent
   * @param size The slider size
   * @param orientation The slider orientation
   * @param visible If true, the slider is immediately visible
   * @param styleClassID Optional style class identifier
   */
  uiSlider(uiWindow * parent, const Point & pos, const Size & size, uiOrientation orientation, bool visible = true, uint32_t styleClassID = 0);

  virtual ~uiSlider();

  virtual void processEvent(uiEvent * event);

  /**
   * @brief Sets or gets slider style
   *
   * @return L-value representing slider style
   */
  uiSliderStyle & sliderStyle() { return m_sliderStyle; }

  /**
   * @brief Determines slider position
   *
   * @return Slider position
   */
  int position() { return m_position; }

  /**
   * @brief Sets the slider position
   *
   * @param value New slider position
   */
  void setPosition(int value);

  /**
   * @brief Gets minimum position
   *
   * @return Minimum position
   */
  int min() { return m_min; }

  /**
   * @brief Gets maximum position
   *
   * @return Maximum position
   */
  int max() { return m_max; }

  /**
   * @brief Sets the minimum and maximum positions and the tick frequency
   *
   * @param min Minimum position (default is 0)
   * @param max Maximum position (default is 99)
   * @param ticksFrequency Distance between tick marks (default is 25, 0 = no ticks)
   */
  void setup(int min, int max, int ticksFrequency);


  /**
   * @brief Slider changed event delegate
   *
   * This delegate is called whenever the slider position changes
   */
  Delegate<> onChange;


private:

  void paintSlider();
  Rect getGripRect();
  void moveGripTo(int x, int y);
  void handleKeyDown(uiKeyEventInfo key);


  uiSliderStyle m_sliderStyle;
  uiOrientation m_orientation;

  int16_t       m_position;
  int16_t       m_min;
  int16_t       m_max;
  int16_t       m_ticksFrequency;
};



////////////////////////////////////////////////////////////////////////////////////////////////////
// uiProgressBar


/** @brief Contains the progress bar style */
struct uiProgressBarStyle {
  RGB888           backgroundColor = RGB888(128, 128, 128);   /**< Progress bar background color */
  RGB888           foregroundColor = RGB888(64, 128, 64);     /**< Progress bar foreground color */
  FontInfo const * textFont        = &FONT_std_14;            /**< Text font */
  RGB888           textColor       = RGB888(255, 255, 255);   /**< Text color */

  void adaptToDisplayColors(int displayColors) {
    if (displayColors < 4) {
      foregroundColor = RGB888(0, 0, 0);
    } else if (displayColors < 8) {
      textColor       = RGB888(0, 0, 0);
    }
  }
};


/** @brief Properties of the progress bar */
struct uiProgressBarProps {
  uint8_t showPercentage : 1;   /**< If true, the percentage value is shown */

  uiProgressBarProps()
    : showPercentage(true)
    {
    }
};


/**
 * @brief A progress bar shows a completion percentage using a colored bar
 *
 * @ingroup gui
 */
class uiProgressBar : public uiControl {

public:

  /**
   * @brief Creates an instance of the object
   *
   * @param parent The parent window. A progress bar must always have a parent window
   * @param pos Top-left coordinates of the progress bar relative to the parent
   * @param size The progress bar size
   * @param visible If true, the progress bar is immediately visible
   * @param styleClassID Optional style class identifier
   */
  uiProgressBar(uiWindow * parent, const Point & pos, const Size & size, bool visible = true, uint32_t styleClassID = 0);

  virtual ~uiProgressBar();

  virtual void processEvent(uiEvent * event);

  /**
   * @brief Sets or gets progress bar style
   *
   * @return L-value representing progress bar style
   */
  uiProgressBarStyle & progressBarStyle() { return m_progressBarStyle; }

  /**
   * @brief Sets or gets progress bar properties
   *
   * @return L-value representing some progress bar properties
   */
  uiProgressBarProps & progressBarProps() { return m_progressBarProps; }

  /**
   * @brief Sets the percentage
   *
   * @param value Percentage to show (0..100)
   */
  void setPercentage(int value);


private:

  void paintProgressBar();


  uiProgressBarStyle   m_progressBarStyle;
  uiProgressBarProps   m_progressBarProps;

  int                  m_percentage;
};



////////////////////////////////////////////////////////////////////////////////////////////////////
// uiKnob


/** @brief Contains the knob style */
struct uiKnobStyle {
  RGB888           backgroundColor     = RGB888(128, 128, 128);  /**< Background color */
  FontInfo const * labelFont           = &FONT_std_12;           /**< Label font (top) */
  RGB888           labelColor          = RGB888(0, 0, 0);        /**< Label color */
  FontInfo const * valueFont           = &FONT_std_12;           /**< Value font (bottom) */
  RGB888           valueColor          = RGB888(0, 0, 255);      /**< Value color */
  RGB888           activeValueColor    = RGB888(255, 255, 255);  /**< Value color while dragging or when focused */
  RGB888           ringColor           = RGB888(0, 0, 255);      /**< Color of the ring dots up to the current position */
  RGB888           ringBackgroundColor = RGB888(64, 64, 64);     /**< Color of the ring dots after the current position */
  RGB888           bodyColor           = RGB888(64, 64, 64);     /**< Color of the knob body */
  RGB888           capColor            = RGB888(0, 0, 0);        /**< Color of the knob cap (inner circle) */
  RGB888           pointerColor        = RGB888(255, 255, 255);  /**< Color of the pointer */

  void adaptToDisplayColors(int displayColors) {
    if (displayColors < 4) {
      backgroundColor     = RGB888(255, 255, 255);
      valueColor          = RGB888(0, 0, 0);
      activeValueColor    = RGB888(0, 0, 0);
      ringColor           = RGB888(0, 0, 0);
      ringBackgroundColor = RGB888(255, 255, 255);
      bodyColor           = RGB888(0, 0, 0);
      capColor            = RGB888(0, 0, 0);
    }
  }
};


/** @brief Properties of the knob */
struct uiKnobProps {
  uint16_t dragPixels = 120;   /**< Vertical mouse movement (pixels) that covers the whole range */
};


/**
 * @brief A rotary knob. The value changes dragging the mouse up and down, with the mouse wheel or with the keyboard
 *
 * The label is drawn above the knob and the value text below it. The value text is the position by default:
 * applications can show a different text calling setValueText() inside onChange. Double clicking the knob
 * restores the default position (see setup()).
 *
 * Example:
 *
 *     auto knob = new uiKnob(frame, "CUTOFF", Point(10, 30), Size(60, 76));
 *     knob->onChange = [=]() {
 *       knob->setValueTextFmt("%d Hz", knob->position() * 100);
 *     };
 *     knob->setup(0, 70, 20);
 *
 * @ingroup gui
 */
class uiKnob : public uiControl {

public:

  /**
   * @brief Creates an instance of the object
   *
   * @param parent The parent window. A knob must always have a parent window
   * @param label Label shown above the knob (copied). Can be nullptr
   * @param pos Top-left coordinates of the knob relative to the parent
   * @param size The knob size
   * @param visible If true, the knob is immediately visible
   * @param styleClassID Optional style class identifier
   */
  uiKnob(uiWindow * parent, char const * label, const Point & pos, const Size & size, bool visible = true, uint32_t styleClassID = 0);

  virtual ~uiKnob();

  virtual void processEvent(uiEvent * event);

  /**
   * @brief Sets or gets knob style
   *
   * @return L-value representing knob style
   */
  uiKnobStyle & knobStyle() { return m_knobStyle; }

  /**
   * @brief Sets or gets knob properties
   *
   * @return L-value representing knob properties
   */
  uiKnobProps & knobProps() { return m_knobProps; }

  /**
   * @brief Sets the range and the default position, moves the knob to the default position and calls onChange
   *
   * @param min Minimum position
   * @param max Maximum position
   * @param defaultPosition Default position (restored by a double click)
   */
  void setup(int min, int max, int defaultPosition);

  /**
   * @brief Determines the knob position
   *
   * @return Knob position
   */
  int position() { return m_position; }

  /**
   * @brief Sets the knob position. onChange is called when the position changes
   *
   * @param value New position (clipped to the range)
   */
  void setPosition(int value);

  /** @brief Gets the minimum position */
  int min() { return m_min; }

  /** @brief Gets the maximum position */
  int max() { return m_max; }

  /**
   * @brief Sets the label
   *
   * @param value Label text (copied)
   */
  void setLabel(char const * value);

  /**
   * @brief Sets the text shown below the knob
   *
   * The text is reset to the position number every time the position changes, before onChange is called.
   *
   * @param value Text to show
   */
  void setValueText(char const * value);

  /**
   * @brief Sets the text shown below the knob, using printf like formatting
   *
   * @param format printf like format string
   */
  void setValueTextFmt(const char * format, ...);

  /**
   * @brief Knob changed event delegate
   *
   * This delegate is called whenever the knob position changes
   */
  Delegate<> onChange;


private:

  void paintKnob();
  void handleKeyDown(uiKeyEventInfo const & key);
  bool isDragging();


  uiKnobStyle m_knobStyle;
  uiKnobProps m_knobProps;
  char *      m_label;
  char        m_valueText[24];
  int         m_position;
  int         m_min;
  int         m_max;
  int         m_default;
  int         m_dragY;
  int         m_dragPosition;
  bool        m_changing;      // inside setPosition(): onChange() may set the value text, repainted once
};



////////////////////////////////////////////////////////////////////////////////////////////////////
// uiToggleSwitch


/** @brief Contains the toggle switch style */
struct uiToggleSwitchStyle {
  RGB888           backgroundColor = RGB888(128, 128, 128);  /**< Background color */
  RGB888           onColor         = RGB888(0, 0, 255);      /**< Track color when on */
  RGB888           offColor        = RGB888(64, 64, 64);     /**< Track color when off */
  RGB888           thumbColor      = RGB888(255, 255, 255);  /**< Thumb color */
  FontInfo const * textFont        = &FONT_std_12;           /**< Font of the "on" and "off" texts */
  RGB888           onTextColor     = RGB888(0, 0, 255);      /**< Text color when on */
  RGB888           offTextColor    = RGB888(64, 64, 64);     /**< Text color when off */
  char const *     onText          = "ON";                   /**< Text shown when on (nullptr = none) */
  char const *     offText         = "OFF";                  /**< Text shown when off (nullptr = none) */

  void adaptToDisplayColors(int displayColors) {
    if (displayColors < 4) {
      backgroundColor = RGB888(255, 255, 255);
      onColor         = RGB888(0, 0, 0);
      offColor        = RGB888(0, 0, 0);
      onTextColor     = RGB888(0, 0, 0);
      offTextColor    = RGB888(0, 0, 0);
    }
  }
};


/**
 * @brief An on/off switch: a track with a sliding thumb, and an optional text on its left
 *
 * The switch is toggled by clicking it or pressing SPACE or ENTER.
 *
 * @ingroup gui
 */
class uiToggleSwitch : public uiControl {

public:

  /**
   * @brief Creates an instance of the object
   *
   * @param parent The parent window. A switch must always have a parent window
   * @param pos Top-left coordinates of the switch relative to the parent
   * @param size The switch size. The track is on the right side, its height is the switch height
   * @param checked Initial state
   * @param visible If true, the switch is immediately visible
   * @param styleClassID Optional style class identifier
   */
  uiToggleSwitch(uiWindow * parent, const Point & pos, const Size & size, bool checked = false, bool visible = true, uint32_t styleClassID = 0);

  virtual ~uiToggleSwitch();

  virtual void processEvent(uiEvent * event);

  /**
   * @brief Sets or gets switch style
   *
   * @return L-value representing switch style
   */
  uiToggleSwitchStyle & toggleSwitchStyle() { return m_toggleSwitchStyle; }

  /**
   * @brief Determines whether the switch is on
   *
   * @return True if the switch is on
   */
  bool checked() { return m_checked; }

  /**
   * @brief Sets the switch state. This method does not call onChange
   *
   * @param value True = on
   */
  void setChecked(bool value);

  /**
   * @brief Switch changed event delegate
   *
   * This delegate is called when the user toggles the switch
   */
  Delegate<> onChange;


private:

  void paintToggleSwitch();
  void toggle();


  uiToggleSwitchStyle m_toggleSwitchStyle;
  bool                m_checked;
};



////////////////////////////////////////////////////////////////////////////////////////////////////
// uiLevelBar


/** @brief Contains the level bar style */
struct uiLevelBarStyle {
  RGB888           backgroundColor = RGB888(128, 128, 128);  /**< Background color */
  RGB888           lowColor        = RGB888(0, 255, 0);      /**< Color of the lit segments */
  RGB888           lowOffColor     = RGB888(0, 64, 0);       /**< Color of the unlit segments */
  RGB888           highColor       = RGB888(255, 128, 0);    /**< Color of the lit segments in the high range */
  RGB888           highOffColor    = RGB888(64, 32, 0);      /**< Color of the unlit segments in the high range */
  FontInfo const * textFont        = &FONT_std_14;           /**< Font of the percentage */
  RGB888           textColor       = RGB888(0, 0, 0);        /**< Color of the percentage */

  void adaptToDisplayColors(int displayColors) {
    if (displayColors < 4) {
      backgroundColor = RGB888(255, 255, 255);
      lowColor        = RGB888(0, 0, 0);
      highColor       = RGB888(0, 0, 0);
      lowOffColor     = RGB888(255, 255, 255);
      highOffColor    = RGB888(255, 255, 255);
    }
  }
};


/** @brief Properties of the level bar */
struct uiLevelBarProps {
  uint8_t segments       = 24;   /**< Number of segments */
  uint8_t highSegments   = 5;    /**< Number of segments (on the right) drawn with the high range colors */
  uint8_t segmentGap     = 2;    /**< Space between segments in pixels */
  uint8_t showPercentage : 1;    /**< If true, the percentage is shown on the right */
  uint8_t allowEdit      : 1;    /**< If true, the level can be changed with the mouse (click, drag, wheel) and the keyboard */

  uiLevelBarProps()
    : showPercentage(true),
      allowEdit(true)
  {
  }
};


/**
 * @brief A horizontal segmented bar, like a led level meter. It can show a level or let the user set it
 *
 * @ingroup gui
 */
class uiLevelBar : public uiControl {

public:

  /**
   * @brief Creates an instance of the object
   *
   * @param parent The parent window. A level bar must always have a parent window
   * @param pos Top-left coordinates of the level bar relative to the parent
   * @param size The level bar size
   * @param visible If true, the level bar is immediately visible
   * @param styleClassID Optional style class identifier
   */
  uiLevelBar(uiWindow * parent, const Point & pos, const Size & size, bool visible = true, uint32_t styleClassID = 0);

  virtual ~uiLevelBar();

  virtual void processEvent(uiEvent * event);

  /**
   * @brief Sets or gets level bar style
   *
   * @return L-value representing level bar style
   */
  uiLevelBarStyle & levelBarStyle() { return m_levelBarStyle; }

  /**
   * @brief Sets or gets level bar properties
   *
   * @return L-value representing level bar properties
   */
  uiLevelBarProps & levelBarProps() { return m_levelBarProps; }

  /**
   * @brief Sets the range of the level
   *
   * @param min Minimum level (default 0)
   * @param max Maximum level (default 100)
   */
  void setRange(int min, int max);

  /**
   * @brief Determines the level
   *
   * @return Current level
   */
  int position() { return m_position; }

  /**
   * @brief Sets the level. onChange is called when the level changes
   *
   * @param value New level (clipped to the range)
   */
  void setPosition(int value);

  /**
   * @brief Level changed event delegate
   *
   * This delegate is called whenever the level changes
   */
  Delegate<> onChange;


private:

  void paintLevelBar();
  int barWidth();
  void setPositionFromX(int x);


  uiLevelBarStyle m_levelBarStyle;
  uiLevelBarProps m_levelBarProps;
  int             m_position;
  int             m_min;
  int             m_max;
};



////////////////////////////////////////////////////////////////////////////////////////////////////
// uiLedBar


/** @brief Contains the led bar style */
struct uiLedBarStyle {
  RGB888 backgroundColor = RGB888(128, 128, 128);   /**< Background color */
  /** Led colors: index 0 is "off", 1..3 are the "on" states */
  RGB888 ledColors[4]    = { RGB888(0, 64, 0),
                             RGB888(0, 255, 0),
                             RGB888(255, 128, 0),
                             RGB888(255, 0, 0) };

  void adaptToDisplayColors(int displayColors) {
    if (displayColors < 4) {
      backgroundColor = RGB888(255, 255, 255);
      ledColors[0]    = RGB888(255, 255, 255);
      ledColors[1] = ledColors[2] = ledColors[3] = RGB888(0, 0, 0);
    }
  }
};


/** @brief Properties of the led bar */
struct uiLedBarProps {
  uint8_t ledGap = 4;   /**< Space between leds in pixels */
};


/**
 * @brief A row of leds. Each led has a state (0 = off, 1..3 = on with different colors)
 *
 * @ingroup gui
 */
class uiLedBar : public uiControl {

public:

  /**
   * @brief Creates an instance of the object
   *
   * @param parent The parent window. A led bar must always have a parent window
   * @param pos Top-left coordinates of the led bar relative to the parent
   * @param size The led bar size. Leds fill the whole height and share the width
   * @param count Number of leds
   * @param visible If true, the led bar is immediately visible
   * @param styleClassID Optional style class identifier
   */
  uiLedBar(uiWindow * parent, const Point & pos, const Size & size, int count, bool visible = true, uint32_t styleClassID = 0);

  virtual ~uiLedBar();

  virtual void processEvent(uiEvent * event);

  /**
   * @brief Sets or gets led bar style
   *
   * @return L-value representing led bar style
   */
  uiLedBarStyle & ledBarStyle() { return m_ledBarStyle; }

  /**
   * @brief Sets or gets led bar properties
   *
   * @return L-value representing led bar properties
   */
  uiLedBarProps & ledBarProps() { return m_ledBarProps; }

  /**
   * @brief Gets the number of leds
   *
   * @return Number of leds
   */
  int count() { return m_count; }

  /**
   * @brief Sets the state of a led. The led bar is repainted only when the state changes
   *
   * @param index Led index (0..count()-1)
   * @param state 0 = off, 1..3 = on (see uiLedBarStyle.ledColors)
   */
  void setLed(int index, int state);

  /**
   * @brief Gets the state of a led
   *
   * @param index Led index (0..count()-1)
   *
   * @return Led state (0..3)
   */
  int led(int index) { return m_states[index]; }


private:

  void paintLedBar();


  uiLedBarStyle m_ledBarStyle;
  uiLedBarProps m_ledBarProps;
  int           m_count;
  uint8_t *     m_states;
};



////////////////////////////////////////////////////////////////////////////////////////////////////
// uiStepper


/** @brief Contains the stepper style */
struct uiStepperStyle {
  RGB888           backgroundColor                = RGB888(128, 128, 128);  /**< Background color */
  RGB888           buttonBackgroundColor          = RGB888(64, 64, 64);     /**< Background color of the buttons */
  RGB888           mouseDownButtonBackgroundColor = RGB888(0, 0, 255);      /**< Background color of a pressed button */
  RGB888           buttonBorderColor              = RGB888(0, 0, 0);        /**< Border color of the buttons */
  RGB888           arrowColor                     = RGB888(255, 255, 255);  /**< Arrow color */
  RGB888           mouseDownArrowColor            = RGB888(255, 255, 255);  /**< Arrow color of a pressed button */
  FontInfo const * textFont                       = &FONT_std_14;           /**< Text font */
  RGB888           textColor                      = RGB888(0, 0, 0);        /**< Text color */

  void adaptToDisplayColors(int displayColors) {
    if (displayColors < 4) {
      backgroundColor                = RGB888(255, 255, 255);
      buttonBackgroundColor          = RGB888(255, 255, 255);
      mouseDownButtonBackgroundColor = RGB888(0, 0, 0);
      arrowColor                     = RGB888(0, 0, 0);
    }
  }
};


/**
 * @brief A text between "previous" and "next" arrow buttons, to step through values (for example an octave)
 *
 * The application handles onStep and updates the text. Keyboard: LEFT/DOWN steps back, RIGHT/UP steps forward.
 *
 * @ingroup gui
 */
class uiStepper : public uiControl {

public:

  /**
   * @brief Creates an instance of the object
   *
   * @param parent The parent window. A stepper must always have a parent window
   * @param pos Top-left coordinates of the stepper relative to the parent
   * @param size The stepper size. The buttons are squares as high as the stepper
   * @param visible If true, the stepper is immediately visible
   * @param styleClassID Optional style class identifier
   */
  uiStepper(uiWindow * parent, const Point & pos, const Size & size, bool visible = true, uint32_t styleClassID = 0);

  virtual ~uiStepper();

  virtual void processEvent(uiEvent * event);

  /**
   * @brief Sets or gets stepper style
   *
   * @return L-value representing stepper style
   */
  uiStepperStyle & stepperStyle() { return m_stepperStyle; }

  /**
   * @brief Sets the text shown between the buttons
   *
   * @param value Text (copied)
   */
  void setText(char const * value);

  /**
   * @brief Gets the text shown between the buttons
   *
   * @return The text
   */
  char const * text() { return m_text; }

  /**
   * @brief Step event delegate
   *
   * This delegate is called with -1 (left button) or +1 (right button)
   */
  Delegate<int> onStep;


private:

  void paintStepper();
  Rect buttonRect(int index);


  uiStepperStyle m_stepperStyle;
  char *         m_text;
  int            m_pressed;   // -1 = none, 0 = left, 1 = right
};



////////////////////////////////////////////////////////////////////////////////////////////////////
// uiTileSelector


/** @brief Contains the tile selector style */
struct uiTileSelectorStyle {
  RGB888           backgroundColor         = RGB888(128, 128, 128);  /**< Background color */
  RGB888           tileBackgroundColor     = RGB888(0, 0, 0);        /**< Tile background color */
  RGB888           tileBorderColor         = RGB888(64, 64, 64);     /**< Tile border color */
  RGB888           selectedTileBorderColor = RGB888(0, 0, 255);      /**< Border color of the selected tile */
  FontInfo const * textFont                = &FONT_std_12;           /**< Font of the captions */
  RGB888           textColor               = RGB888(64, 64, 64);     /**< Caption color */
  RGB888           selectedTextColor       = RGB888(0, 0, 255);      /**< Caption color of the selected tile */

  void adaptToDisplayColors(int displayColors) {
    if (displayColors < 4) {
      backgroundColor         = RGB888(255, 255, 255);
      tileBackgroundColor     = RGB888(255, 255, 255);
      tileBorderColor         = RGB888(0, 0, 0);
      selectedTileBorderColor = RGB888(0, 0, 0);
      textColor               = RGB888(0, 0, 0);
      selectedTextColor       = RGB888(0, 0, 0);
    }
  }
};


/** @brief Properties of the tile selector */
struct uiTileSelectorProps {
  uint8_t tileGap = 4;   /**< Space between tiles in pixels */
};


/**
 * @brief A row of selectable tiles. The content of each tile is painted by the application (onPaintTile)
 *
 * There is one tile for each item (items()). The item text, when not empty, is drawn below its tile as a caption.
 * Keyboard: LEFT and RIGHT select the previous and next tile.
 *
 * @ingroup gui
 */
class uiTileSelector : public uiControl {

public:

  /**
   * @brief Creates an instance of the object
   *
   * @param parent The parent window. A tile selector must always have a parent window
   * @param pos Top-left coordinates of the tile selector relative to the parent
   * @param size The tile selector size (tiles and captions)
   * @param visible If true, the tile selector is immediately visible
   * @param styleClassID Optional style class identifier
   */
  uiTileSelector(uiWindow * parent, const Point & pos, const Size & size, bool visible = true, uint32_t styleClassID = 0);

  virtual ~uiTileSelector();

  virtual void processEvent(uiEvent * event);

  /**
   * @brief Sets or gets tile selector style
   *
   * @return L-value representing tile selector style
   */
  uiTileSelectorStyle & tileSelectorStyle() { return m_tileSelectorStyle; }

  /**
   * @brief Sets or gets tile selector properties
   *
   * @return L-value representing tile selector properties
   */
  uiTileSelectorProps & tileSelectorProps() { return m_tileSelectorProps; }

  /**
   * @brief A list of strings: one tile for each item, the text is the caption. Call repaint() after changing it
   *
   * @return L-value representing the items
   */
  StringList & items() { return m_items; }

  /**
   * @brief Gets the selected tile
   *
   * @return Index of the selected tile, -1 = none
   */
  int selectedItem() { return m_selected; }

  /**
   * @brief Selects a tile. This method does not call onChange
   *
   * @param index Index of the tile to select (-1 = none)
   */
  void selectItem(int index);

  /**
   * @brief Gets the area of a tile (border included), in client coordinates
   *
   * @param index Tile index
   *
   * @return Tile area
   */
  Rect tileRect(int index);

  /**
   * @brief Selection changed event delegate
   *
   * This delegate is called when the user selects a tile
   */
  Delegate<> onChange;

  /**
   * @brief Tile paint delegate
   *
   * Called to paint the content of a tile, after its background and border. Parameters: tile index, tile area
   * (client coordinates) and true if the tile is selected. Paint using canvas().
   */
  Delegate<int, Rect const &, bool> onPaintTile;


private:

  void paintTileSelector();
  int captionHeight();


  uiTileSelectorStyle m_tileSelectorStyle;
  uiTileSelectorProps m_tileSelectorProps;
  StringList          m_items;
  int                 m_selected;
};



////////////////////////////////////////////////////////////////////////////////////////////////////
// uiGraph


/** @brief Contains the graph style */
struct uiGraphStyle {
  RGB888 backgroundColor = RGB888(0, 0, 0);        /**< Background color */
  RGB888 gridColor       = RGB888(0, 64, 64);      /**< Grid lines color */
  RGB888 lineColor       = RGB888(0, 255, 255);    /**< Curve color */
  RGB888 fillColor       = RGB888(0, 0, 128);      /**< Color of the area below the curve (when uiGraphProps.fillArea is set) */

  void adaptToDisplayColors(int displayColors) {
    if (displayColors < 4) {
      backgroundColor = RGB888(255, 255, 255);
      gridColor       = RGB888(255, 255, 255);
      lineColor       = RGB888(0, 0, 0);
      fillColor       = RGB888(255, 255, 255);
    }
  }
};


/** @brief Properties of the graph */
struct uiGraphProps {
  uint8_t hGridLines = 1;   /**< Number of horizontal grid lines (evenly spaced) */
  uint8_t vGridLines = 3;   /**< Number of vertical grid lines (evenly spaced) */
  uint8_t margin     = 3;   /**< Space between the border and the plot area */
  uint8_t fillArea   : 1;   /**< If true, the area below the curve is filled */

  uiGraphProps()
    : fillArea(false)
  {
  }
};


/**
 * @brief Plots a sequence of values as a curve (waveforms, envelopes, frequency responses...)
 *
 * The values are spread over the whole plot width, the vertical scale is set with setRange(). Applications can
 * paint additional elements (markers, labels) with onPaintBackground (before the curve) and onPaint (after it).
 *
 * Example:
 *
 *     int16_t values[100];
 *     for (int i = 0; i < 100; ++i)
 *       values[i] = 100 * sinf(i * 2 * M_PI / 100);
 *     auto graph = new uiGraph(frame, Point(10, 30), Size(200, 64));
 *     graph->setRange(-100, 100);
 *     graph->setValues(values, 100);
 *
 * @ingroup gui
 */
class uiGraph : public uiControl {

public:

  /**
   * @brief Creates an instance of the object
   *
   * @param parent The parent window. A graph must always have a parent window
   * @param pos Top-left coordinates of the graph relative to the parent
   * @param size The graph size
   * @param visible If true, the graph is immediately visible
   * @param styleClassID Optional style class identifier
   */
  uiGraph(uiWindow * parent, const Point & pos, const Size & size, bool visible = true, uint32_t styleClassID = 0);

  virtual ~uiGraph();

  virtual void processEvent(uiEvent * event);

  /**
   * @brief Sets or gets graph style
   *
   * @return L-value representing graph style
   */
  uiGraphStyle & graphStyle() { return m_graphStyle; }

  /**
   * @brief Sets or gets graph properties
   *
   * @return L-value representing graph properties
   */
  uiGraphProps & graphProps() { return m_graphProps; }

  /**
   * @brief Sets the vertical scale
   *
   * @param min Value at the bottom of the plot area
   * @param max Value at the top of the plot area
   */
  void setRange(int min, int max);

  /**
   * @brief Sets the values to plot and repaints the graph
   *
   * @param values Values (copied)
   * @param count Number of values (0 = no curve)
   */
  void setValues(int16_t const * values, int count);

  /**
   * @brief Gets the plot area, in client coordinates
   *
   * @return Plot area
   */
  Rect plotRect();

  /**
   * @brief Converts a value to a vertical coordinate (clipped to the plot area)
   *
   * @param value The value
   *
   * @return Y coordinate in client coordinates
   */
  int valueToY(int value);

  /**
   * @brief Converts a value index to a horizontal coordinate
   *
   * @param index Index of the value (0..count-1)
   *
   * @return X coordinate in client coordinates
   */
  int indexToX(int index);

  /**
   * @brief Background paint delegate
   *
   * Called after the grid and before the curve. The parameter is the plot area. Paint using canvas().
   */
  Delegate<Rect const &> onPaintBackground;

  /**
   * @brief Paint delegate
   *
   * Called after the curve. The parameter is the plot area. Paint using canvas().
   */
  Delegate<Rect const &> onPaint;


private:

  void paintGraph();


  uiGraphStyle m_graphStyle;
  uiGraphProps m_graphProps;
  int16_t *    m_values;
  int          m_count;
  int          m_min;
  int          m_max;
};



////////////////////////////////////////////////////////////////////////////////////////////////////
// uiPianoKeyboard


/** @brief Contains the piano keyboard style */
struct uiPianoKeyboardStyle {
  RGB888           backgroundColor      = RGB888(0, 0, 0);        /**< Background color (gaps between keys) */
  RGB888           topLineColor         = RGB888(128, 0, 0);      /**< Color of the line above the keys */
  RGB888           whiteKeyColor        = RGB888(255, 255, 255);  /**< White keys color */
  RGB888           whiteKeyFrontColor   = RGB888(170, 170, 170);  /**< Color of the front (bottom side) of white keys */
  RGB888           blackKeyColor        = RGB888(0, 0, 0);        /**< Black keys color */
  RGB888           blackKeyBevelColor   = RGB888(85, 85, 85);     /**< Color of the sides and front of black keys */
  RGB888           pressedKeyColor      = RGB888(0, 128, 255);    /**< Color of pressed keys */
  RGB888           pressedKeyFrontColor = RGB888(0, 64, 128);     /**< Color of the front of pressed keys */
  FontInfo const * textFont             = &FONT_std_12;           /**< Font of the key labels and note names */
  RGB888           labelColor           = RGB888(85, 85, 170);    /**< Key labels color on white keys */
  RGB888           blackLabelColor      = RGB888(170, 170, 170);  /**< Key labels color on black keys */
  RGB888           noteNameColor        = RGB888(170, 170, 170);  /**< Note names color */
  RGB888           cNoteNameColor       = RGB888(0, 0, 170);      /**< Color of the "C" note names (drawn in bold) */
  RGB888           pressedTextColor     = RGB888(0, 0, 0);        /**< Text color on pressed keys */

  void adaptToDisplayColors(int displayColors) {
    if (displayColors < 4) {
      topLineColor         = RGB888(0, 0, 0);
      whiteKeyFrontColor   = RGB888(255, 255, 255);
      blackKeyBevelColor   = RGB888(0, 0, 0);
      pressedKeyColor      = RGB888(0, 0, 0);
      pressedKeyFrontColor = RGB888(0, 0, 0);
      labelColor           = RGB888(0, 0, 0);
      blackLabelColor      = RGB888(255, 255, 255);
      noteNameColor        = RGB888(0, 0, 0);
      cNoteNameColor       = RGB888(0, 0, 0);
      pressedTextColor     = RGB888(255, 255, 255);
    }
  }
};


/** @brief Properties of the piano keyboard */
struct uiPianoKeyboardProps {
  uint8_t frontSize     = 6;   /**< Height of the front (bottom side) of the keys */
  uint8_t topLineSize   = 2;   /**< Height of the line above the keys (0 = none) */
  uint8_t showNoteNames : 1;   /**< If true, note names are drawn on white keys */

  uiPianoKeyboardProps()
    : showNoteNames(true)
  {
  }
};


/**
 * @brief An on-screen piano keyboard, played with the mouse (dragging slides between notes) or by the application
 *
 * The application connects the notes to a sound source with onNoteOn and onNoteOff, and can press keys from
 * other inputs (for example the PS/2 keyboard) with press() and release(). A key sounds while at least one
 * source holds it pressed.
 *
 * Example:
 *
 *     auto piano = new uiPianoKeyboard(frame, Point(10, 100), Size(400, 120));
 *     piano->onNoteOn  = [&](int note) { synth.noteOn(note); };
 *     piano->onNoteOff = [&](int note) { synth.noteOff(note); };
 *
 * @ingroup gui
 */
class uiPianoKeyboard : public uiControl {

public:

  /** @brief Source of key presses: the mouse (used internally) */
  static constexpr uint8_t MouseSource    = 1;

  /** @brief Source of key presses: suggested value for the keyboard. Applications can use bits 1..7 */
  static constexpr uint8_t KeyboardSource = 2;

  /**
   * @brief Creates an instance of the object
   *
   * @param parent The parent window. A piano keyboard must always have a parent window
   * @param pos Top-left coordinates of the keyboard relative to the parent
   * @param size The keyboard size. White keys share the width
   * @param keysCount Number of keys (semitones). The default 25 shows two octaves plus the final C
   * @param visible If true, the keyboard is immediately visible
   * @param styleClassID Optional style class identifier
   */
  uiPianoKeyboard(uiWindow * parent, const Point & pos, const Size & size, int keysCount = 25, bool visible = true, uint32_t styleClassID = 0);

  virtual ~uiPianoKeyboard();

  virtual void processEvent(uiEvent * event);

  /**
   * @brief Sets or gets piano keyboard style
   *
   * @return L-value representing piano keyboard style
   */
  uiPianoKeyboardStyle & pianoKeyboardStyle() { return m_pianoKeyboardStyle; }

  /**
   * @brief Sets or gets piano keyboard properties
   *
   * @return L-value representing piano keyboard properties
   */
  uiPianoKeyboardProps & pianoKeyboardProps() { return m_pianoKeyboardProps; }

  /**
   * @brief Sets the note of the leftmost key. All pressed keys are released
   *
   * @param value MIDI note number of a white key (default 60 = C4)
   */
  void setBaseNote(int value);

  /**
   * @brief Gets the note of the leftmost key
   *
   * @return MIDI note number
   */
  int baseNote() { return m_baseNote; }

  /**
   * @brief Gets the number of keys
   *
   * @return Number of keys
   */
  int keysCount() { return m_keysCount; }

  /**
   * @brief Presses a key. onNoteOn is called when the key wasn't already pressed by another source
   *
   * Repeated presses from the same source are ignored (typematic repeats).
   *
   * @param index Key index (0 = leftmost key)
   * @param source Source of the press: a bit flag (MouseSource, KeyboardSource or another bit)
   */
  void press(int index, uint8_t source);

  /**
   * @brief Releases a key. onNoteOff is called when no other source holds the key
   *
   * @param index Key index (0 = leftmost key)
   * @param source Source of the release
   */
  void release(int index, uint8_t source);

  /**
   * @brief Releases all keys (onNoteOff is called for each pressed key)
   */
  void releaseAll();

  /**
   * @brief Determines whether a key is pressed
   *
   * @param index Key index (0 = leftmost key)
   *
   * @return True if the key is pressed by any source
   */
  bool isPressed(int index) { return m_pressed[index] != 0; }

  /**
   * @brief Sets the labels drawn on the keys (for example the computer keys that play them)
   *
   * @param labels Array of keysCount() strings (not copied). An item can be nullptr. nullptr = no labels
   */
  void setKeyLabels(char const * const * labels) { m_keyLabels = labels; repaint(); }

  /**
   * @brief Note on event delegate. The parameter is the MIDI note number
   */
  Delegate<int> onNoteOn;

  /**
   * @brief Note off event delegate. The parameter is the MIDI note number
   */
  Delegate<int> onNoteOff;


private:

  void paintPianoKeyboard();
  void paintKey(int index);
  bool isBlack(int index);
  Rect keyRect(int index, bool forHitTest = false);
  int keyAt(int x, int y);
  void updateLayout();


  uiPianoKeyboardStyle m_pianoKeyboardStyle;
  uiPianoKeyboardProps m_pianoKeyboardProps;
  int                  m_keysCount;
  int                  m_baseNote;
  uint8_t *            m_pressed;       // source flags
  uint8_t *            m_whitesBefore;  // number of white keys before each key
  int                  m_whiteKeys;
  int                  m_mouseKey;
  char const * const * m_keyLabels;
};



////////////////////////////////////////////////////////////////////////////////////////////////////
// uiSimpleMenu


/**
 * @brief Shows a list of selectable string items. An item is selected by clicking it or by pressing ENTER or SPACE.
 *
 * @ingroup gui
 */
class uiSimpleMenu : public uiCustomListBox {

public:

  /**
   * @brief Creates an instance of the object
   *
   * @param parent The parent window. A simplemenu must always have a parent window
   * @param pos Top-left coordinates of the simplemenu relative to the parent
   * @param size The simplemenu size
   * @param visible If true, the simplemenu is immediately visible
   * @param styleClassID Optional style class identifier
   */
  uiSimpleMenu(uiWindow * parent, const Point & pos, const Size & size, bool visible = true, uint32_t styleClassID = 0);

  /**
   * @brief A list of strings representing the simplemenu content
   *
   * Besides the strings themselves, StringList also stores which item is selected.
   * The control must be repainted when the string list changes.
   *
   * @return L-value representing simplemenu items
   */
  StringList & items()                              { return m_items; }

  virtual void processEvent(uiEvent * event);


  // Delegates

  /**
   * @brief Item select event
   *
   * This delegate is called whenever the user clicks an item or presses ENTER or SPACE on the selected item.
   */
  Delegate<int> onSelect;

protected:

  virtual int items_getCount()                      { return m_items.count(); }
  virtual void items_deselectAll()                  { m_items.deselectAll(); }
  virtual void items_select(int index, bool select) { m_items.select(index, select); }
  virtual bool items_selected(int index)            { return m_items.selected(index); }
  virtual void items_draw(int index, const Rect & itemRect);


private:

  StringList     m_items;
};




////////////////////////////////////////////////////////////////////////////////////////////////////
// uiSplitButton


/**
 * @brief A split button combines a button with a drop-down simple menu
 *
 * @ingroup gui
 */
class uiSplitButton : public uiCustomComboBox
{

public:

  /**
   * @brief Creates an instance of the object
   *
   * @param parent The parent window. A splitbutton must always have a parent window
   * @param text The text of the splitbutton
   * @param pos Top-left coordinates of the splitbutton relative to the parent
   * @param size The splitbutton size
   * @param listHeight Height in pixels of the open menu
   * @param itemsText Separator-delimited list of menu items (e.g. "option 1;option 2;option 3")
   * @param separator Character used to separate the items in itemsText. Default is ";"
   * @param visible If true, the splitbutton is immediately visible
   * @param styleClassID Optional style class identifier
   */
  uiSplitButton(uiWindow * parent, char const * text, const Point & pos, const Size & size, int listHeight, char const * itemsText, char separator = ';', bool visible = true, uint32_t styleClassID = 0);

  ~uiSplitButton();

  /**
   * @brief A list of strings representing the menu content
   *
   * @return L-value representing menu items
   */
  StringList & items()         { return m_menu->items(); }

  void processEvent(uiEvent * event);


  // Delegates

  /**
   * @brief Item select event
   *
   * This delegate is called whenever the user clicks an item or presses ENTER or SPACE on the selected item.
   */
  Delegate<int> onSelect;


protected:

  uiCustomListBox * listbox()  { return m_menu; }
  uiControl * editcontrol()    { return m_button; }
  void updateEditControl();
  virtual void openListBox();
  virtual void paintButton();

private:
  uiButton *     m_button;
  uiSimpleMenu * m_menu;
  int            m_selectedItem;

};



////////////////////////////////////////////////////////////////////////////////////////////////////
// uiStyle

/**
 * @brief Base class for application styles
 *
 * Derive from uiStyle and implement setStyle() to configure every UI element that has a given style class ID
 * (the last parameter of the control constructors). Install the style with uiApp::setStyle().
 *
 * @ingroup gui
 */
struct uiStyle {
  /**
   * @brief Called for each UI element to apply the style of its style class
   *
   * @param object The UI element to configure (cast it to the actual control type)
   * @param styleClassID Style class ID specified when the element was created
   */
  virtual void setStyle(uiObject * object, uint32_t styleClassID) = 0;
};



////////////////////////////////////////////////////////////////////////////////////////////////////
// uiApp


/**
 * @brief Properties of the application
 *
 * @ingroup gui
 */
struct uiAppProps {
  uint16_t caretBlinkingTime = 500;   /**< Caret blinking period in milliseconds */
  uint16_t doubleClickTime   = 250;   /**< Maximum delay in milliseconds between two clicks for them to be recognized as a double click */
  bool     realtimeReshaping = false; /**< If true, a window is continuously repainted while being resized; otherwise an inverted outline rectangle is shown */
  bool     realtimeMoving    = false; /**< If true, a window is continuously repainted while being moved; otherwise an inverted outline rectangle is shown */
};


/** \ingroup Enumerations
 * @brief Values returned by uiApp::messageBox()
 */
enum class uiMessageBoxResult {
  Cancel   = 0,   /**< The message box has been canceled */
  Button1  = 1,   /**< Button1 has been pressed */
  ButtonOK = 1,   /**< OK Button (button1) has been pressed */
  Button2  = 2,   /**< Button2 has been pressed */
  Button3  = 3,   /**< Button3 has been pressed */
};


/** \ingroup Enumerations
 * @brief Icon displayed by uiApp::messageBox()
 */
enum class uiMessageBoxIcon {
  None,      /**< No icon is displayed */
  Question,  /**< Question icon */
  Info,      /**< Info icon */
  Warning,   /**< Warning icon */
  Error,     /**< Error icon */
};


struct ModalWindowState {
  uiWindow * window;
  uiWindow * prevFocusedWindow;
  uiWindow * prevActiveWindow;
  uiWindow * prevModal;
  int        modalResult;
};


typedef pair<uiEvtHandler *, TimerHandle_t> uiTimerAssoc;


class Keyboard;
class Mouse;


/**
 * @brief Represents the whole application base class.
 *
 * FabGL UI applications must derive from uiApp, create their UI elements in the uiApp::init() method and finally call uiApp::run() to start the application.
 * uiApp handles the event queue, window management, and the lifecycle and memory management of windows and controls.
 *
 * @see @ref guide_gui
 *
 * @ingroup gui
 */
class uiApp : public uiEvtHandler {

public:

  uiApp();

  virtual ~uiApp();

  /**
   * @brief Initializes the application and runs the main event loop
   *
   * @param displayController Specifies the display controller where to run the UI
   * @param keyboard The keyboard device. The default (nullptr) gets it from the PS2Controller
   * @param mouse The mouse device. The default (nullptr) gets it from the PS2Controller
   *
   * @return The exit code passed to uiApp::quit().
   */
  int run(BitmappedDisplayController * displayController, Keyboard * keyboard = nullptr, Mouse * mouse = nullptr);

  /**
   * @brief Initializes the application and runs the main event loop asynchronously
   *
   * @param displayController Specifies the display controller where to run the UI
   * @param taskStack Specifies the task stack size
   * @param keyboard The keyboard device. The default (nullptr) gets it from the PS2Controller
   * @param mouse The mouse device. The default (nullptr) gets it from the PS2Controller
   *
   * @return The exit code passed to uiApp::quit().
   */
  uiApp & runAsync(BitmappedDisplayController * displayController, int taskStack = 3000, Keyboard * keyboard = nullptr, Mouse * mouse = nullptr);

  /**
   * @brief Waits for an application started with runAsync() to terminate
   */
  void joinAsyncRun();

  /**
   * @brief Terminates the application and frees resources
   *
   * @param exitCode Value returned by uiApp::run().
   */
  void quit(int exitCode);

  /**
   * @brief Places an event in the event queue and returns without waiting for the receiver to process the event
   *
   * @param event Event to send. The event is copied.
   *
   * @return True if the event has been queued, false if the event queue is full
   */
  bool postEvent(uiEvent const * event);

  /**
   * @brief Inserts an event at the head of the event queue and returns without waiting for the receiver to process the event
   *
   * @param event Event to insert. The event is copied.
   *
   * @return True if the event has been queued, false if the event queue is full
   */
  bool insertEvent(uiEvent const * event);

  void postDebugMsg(char const * msg);

  virtual void processEvent(uiEvent * event);

  /**
   * @brief Processes all events in the queue
   *
   * Useful to keep the UI responsive while running long tasks.
   */
  void processEvents();

  /**
   * @brief Gets a pointer to the root window
   *
   * The root window is the first window created and covers the whole screen.
   * All windows and controls must have the root window as parent or ancestor.
   *
   * @return The root window pointer
   */
  uiFrame * rootWindow() { return m_rootWindow; }

  /**
   * @brief Gets a pointer to the currently active window
   *
   * There is only one active window at a time. The active window is the foreground window and receives, together with the focused control, all keyboard events.
   * To set the active window, call uiApp::setActiveWindow().
   *
   * @return The active window, or nullptr if there isn't one
   */
  uiWindow * activeWindow() { return m_activeWindow; }

  /**
   * @brief Sets the active window
   *
   * There is only one active window at a time. The active window is the foreground window and receives, together with the focused control, all keyboard events.
   *
   * @param value Pointer to the new active window, or nullptr to have no active window
   *
   * @return Previous active window
   */
  uiWindow * setActiveWindow(uiWindow * value);

  /**
   * @brief Gets the focused window (control)
   *
   * There is only one focused window or control at a time. The focused window is either the active window or a child of it.
   * The focused window receives, together with the active window, all keyboard events, and may display a blinking caret.
   * Pressing TAB moves the focus between windows according to the tab order (see uiWindow::focusIndex()).
   *
   * To set the focused window, call uiApp::setFocusedWindow().
   *
   * @return Focused window or nullptr if there isn't one
   */
  uiWindow * focusedWindow() { return m_focusedWindow; }

  /**
   * @brief Sets the focused window (control)
   *
   * There is only one focused window or control at a time. The focused window is either the active window or a child of it.
   * The focused window receives, together with the active window, all keyboard events, and may display a blinking caret.
   * Pressing TAB moves the focus between windows according to the tab order (see uiWindow::focusIndex()).
   *
   * @return Previous focused window
   */
  uiWindow * setFocusedWindow(uiWindow * value);

  /**
   * @brief Moves the focus to the control whose focus index is the current one plus a delta
   *
   * Focus moves between windows according to the tab order (see uiWindow::focusIndex()).
   * Calling moveFocus(1) is equivalent to pressing TAB; calling moveFocus(-1) is equivalent to pressing SHIFT-TAB.
   *
   * @param delta Number of controls to skip (1 = next control, -1 = previous control)
   *
   * @return Previous focused window
   */
  uiWindow * moveFocus(int delta);

  void captureMouse(uiWindow * window);

  /**
   * @brief Determines which window is capturing the mouse
   *
   * A window captures the mouse when a drag (mouse moved with the left button held down) starts inside its area.
   *
   * @return The window that captured the mouse, or nullptr
   */
  uiWindow * capturedMouseWindow() { return m_capturedMouseWindow; }

  /**
   * @brief Repaints a window
   *
   * @param window The window to be repainted
   */
  void repaintWindow(uiWindow * window);

  /**
   * @brief Repaints a screen area
   *
   * @param rect Rectangle to be repainted (in screen coordinates)
   */
  void repaintRect(Rect const & rect);

  /**
   * @brief Moves a window
   *
   * @param window Window to be moved
   * @param x New horizontal coordinate, relative to parent window
   * @param y New vertical coordinate, relative to parent window
   */
  void moveWindow(uiWindow * window, int x, int y);

  /**
   * @brief Resizes a window
   *
   * @param window Window to be resized
   * @param width New horizontal size
   * @param height New vertical size
   */
  void resizeWindow(uiWindow * window, int width, int height);

  /**
   * @brief Resizes a window
   *
   * @param window Window to be resized
   * @param size New window size
   */
  void resizeWindow(uiWindow * window, Size size);

  /**
   * @brief Reshapes a window
   *
   * @param window Window to be reshaped
   * @param rect New window bounding box, relative to the parent window
   */
  void reshapeWindow(uiWindow * window, Rect const & rect);

  /**
   * @brief Determines which window a point belongs to
   *
   * @param point A point relative to the screen
   *
   * @return A window
   */
  uiWindow * screenToWindow(Point & point);

  /**
   * @brief Makes a window visible or invisible
   *
   * @param window Window to show or hide
   * @param value True to show the window, false to hide it
   */
  void showWindow(uiWindow * window, bool value);

  /**
   * @brief Makes a window visible and runs it as a modal window
   *
   * A modal window disables the main window while keeping it visible: the user must interact with the modal window before returning to the parent window.
   * A modal window leaves the modal state by calling uiWindow::exitModal().
   * showModalWindow() is equivalent to calling initModalWindow(), processModalWindowEvents() and endModalWindow() in sequence.
   *
   * @param window Window to be made visible and modal
   *
   * @return The value passed to uiWindow::exitModal()
   */
  int showModalWindow(uiWindow * window);

  /**
   * @brief Begins modal window processing
   *
   * Use initModalWindow(), processModalWindowEvents() and endModalWindow() when a long-running operation must be performed while a modal window is displayed.
   *
   * @param window The form to use as modal window
   *
   * @return Modal window state. Pass this value to processModalWindowEvents() and endModalWindow().
   */
  ModalWindowState * initModalWindow(uiWindow * window);

  /**
   * @brief Processes pending events of a modal window
   *
   * Call this method whenever the UI needs to be updated.
   *
   * @param state Value returned by initModalWindow(), used internally to maintain the modal window state.
   * @param timeout Maximum time in milliseconds to wait for events. -1 = wait forever, 0 = do not wait.
   *
   * @return False if EXIT or CLOSE has been received and the modal window should be closed (call endModalWindow()); true if processModalWindowEvents() must be called again.
   */
  bool processModalWindowEvents(ModalWindowState * state, int timeout);

  /**
   * @brief Ends modal window processing
   *
   * Ends the modal state. After this call other windows can receive input again.
   * Always call this method when processModalWindowEvents() returns false, or whenever the modal window must be closed.
   *
   * @param state Value returned by initModalWindow(), used internally to maintain the modal window state.
   *
   * @return The value passed to uiWindow::exitModal()
   */
  int endModalWindow(ModalWindowState * state);

  /**
   * @brief Maximizes or restores a frame
   *
   * @param frame Frame to be maximized or restored
   * @param value True to maximize the frame, false to restore it
   */
  void maximizeFrame(uiFrame * frame, bool value);

  /**
   * @brief Minimizes or restores a frame
   *
   * @param frame Frame to be minimized or restored
   * @param value True to minimize the frame, false to restore it
   */
  void minimizeFrame(uiFrame * frame, bool value);

  void combineMouseMoveEvents(bool value) { m_combineMouseMoveEvents = value; }

  void showCaret(uiWindow * window);

  void setCaret(bool value);

  void setCaret(Point const & pos);

  void setCaret(Rect const & rect);

  /**
   * @brief Sets up a timer
   *
   * When the timer expires, the uiApp::onTimer or uiFrame::onTimer delegate is called.
   * To destroy a timer, call uiApp::killTimer().
   *
   * @param dest Destination window or app
   * @param periodMS Timer period in milliseconds
   *
   * @return Handle identifying the new timer
   */
  uiTimerHandle setTimer(uiEvtHandler * dest, int periodMS);

  /**
   * @brief Kills a timer
   *
   * To create a timer, call uiApp::setTimer().
   *
   * @param handle Handle identifying the timer to destroy
   */
  void killTimer(uiTimerHandle handle);

  void killEvtHandlerTimers(uiEvtHandler * dest);

  /**
   * @brief Sets or gets application properties
   *
   * @return L-value representing some application properties
   */
  uiAppProps & appProps() { return m_appProps; }

  /**
   * @brief Destroys a window
   *
   * @param window Window to destroy
   */
  void destroyWindow(uiWindow * window);

  void cleanWindowReferences(uiWindow * window);

  void purgeEvents(uiEvtHandler * dest);

  /**
   * @brief Enables or disables mouse and keyboard events
   *
   * Mouse and keyboard events are enabled by default.
   *
   * @param value True to enable events, false to disable them
   */
  void enableKeyboardAndMouseEvents(bool value);

  /**
   * @brief Displays a modal dialog box with an icon, a message and up to three buttons
   *
   * @param title The dialog box title. If nullptr, the message box has no title bar
   * @param text The message to be displayed
   * @param button1Text Left button text
   * @param button2Text Middle button text (nullptr if not present)
   * @param button3Text Right button text (nullptr if not present)
   * @param icon Icon to be displayed
   *
   * @return Message box result
   */
  uiMessageBoxResult messageBox(char const * title, char const * text, char const * button1Text, char const * button2Text = nullptr, char const * button3Text = nullptr, uiMessageBoxIcon icon = uiMessageBoxIcon::Question);

  /**
   * @brief Displays a modal dialog box with a message, a text edit and up to two buttons
   *
   * Pressing ENTER (RETURN) is equivalent to pressing the first button.
   * Pressing ESC cancels the dialog box.
   *
   * @param title The dialog box title. If nullptr, the dialog box has no title bar
   * @param text The message to be displayed
   * @param inOutString Initial string of the text edit
   * @param maxLength Maximum length of the inOutString buffer (terminating zero excluded)
   * @param button1Text Left button text
   * @param button2Text Right button text (nullptr if not present)
   *
   * @return Message box result
   */
  uiMessageBoxResult inputBox(char const * title, char const * text, char * inOutString, int maxLength, char const * button1Text, char const * button2Text = nullptr);

  /**
   * @brief Displays a modal open/save dialog box
   *
   * @param title The dialog box title. If nullptr, the dialog box has no title bar
   * @param inOutDirectory Input and output selected directory
   * @param maxDirNameSize Maximum number of characters allowed for the directory (terminating zero excluded)
   * @param inOutFilename Input and output selected filename
   * @param maxFileNameSize Maximum number of characters allowed for the filename (terminating zero excluded)
   * @param buttonOKText OK button text
   * @param buttonCancelText Cancel button text
   * @param frameWidth Dialog box width in pixels
   * @param frameHeight Dialog box height in pixels
   *
   * @return uiMessageBoxResult::ButtonOK when a file has been selected, uiMessageBoxResult::Cancel when the dialog has been canceled.
   */
  uiMessageBoxResult fileDialog(char const * title, char * inOutDirectory, int maxDirNameSize, char * inOutFilename, int maxFileNameSize, char const * buttonOKText, char const * buttonCancelText, int frameWidth = 200, int frameHeight = 250);

  /**
   * @brief Override this method to initialize the application and create its UI
   */
  virtual void init();

  /**
   * @brief Sets the style of the application controls
   *
   * @param value Style class descriptor
   */
  void setStyle(uiStyle * value)                   { m_style = value; }

  /**
   * @brief Gets the current style of the application controls
   *
   * @return Current style (nullptr = default).
   */
  uiStyle * style()                                { return m_style; }

  Keyboard * keyboard()                            { return m_keyboard; }

  Mouse * mouse()                                  { return m_mouse; }

  BitmappedDisplayController * displayController() { return m_displayController; }

  int displayColors()                              { return m_displayColors; }

  Canvas * canvas()                                { return m_canvas; }

  /**
   * @brief Returns the time of the last user action (mouse or keyboard), in milliseconds since boot
   *
   * @return Time in milliseconds
   */
  int lastUserActionTime()                         { return m_lastUserActionTimeMS; }


  // delegates

  /**
   * @brief Timer event delegate
   *
   * This delegate is called when the timer expires.
   * To create a timer, call uiApp::setTimer().
   */
  Delegate<uiTimerHandle> onTimer;


protected:

  bool getEvent(uiEvent * event, int timeOutMS);
  bool peekEvent(uiEvent * event, int timeOutMS);


private:

  void preprocessEvent(uiEvent * event);
  void preprocessMouseEvent(uiEvent * event);
  void preprocessKeyboardEvent(uiEvent * event);
  void filterModalEvent(uiEvent * event);

  static void timerFunc(TimerHandle_t xTimer);

  static void asyncRunTask(void * arg);

  void blinkCaret(bool forceOFF = false);
  void suspendCaret(bool value);


  BitmappedDisplayController * m_displayController;

  int                          m_displayColors;

  Canvas *        m_canvas;

  Keyboard *      m_keyboard;

  Mouse *         m_mouse;

  uiAppProps      m_appProps;

  QueueHandle_t   m_eventsQueue;

  uiFrame *       m_rootWindow;

  uiWindow *      m_activeWindow;        // foreground window. Also gets keyboard events (other than focused window)

  uiWindow *      m_focusedWindow;       // window that captures keyboard events (other than active window)

  uiWindow *      m_lastFocusedWindow;   // previous focused window

  uiWindow *      m_capturedMouseWindow; // window that has captured mouse

  uiWindow *      m_freeMouseWindow;     // window where mouse is over

  uiWindow *      m_modalWindow;         // current modal window

  bool            m_combineMouseMoveEvents;

  uiEvtHandler *  m_keyDownHandler;      // used to produce UIEVT_KEYTYPE

  uiWindow *      m_caretWindow;         // nullptr = caret is not visible
  Rect            m_caretRect;           // caret rect relative to m_caretWindow
  uiTimerHandle   m_caretTimer;
  int             m_caretInvertState;    // -1 = suspended, 1 = rect reversed (cat visible), 0 = rect not reversed (caret invisible)

  int             m_lastMouseUpTimeMS;   // time (MS) at mouse up. Used to measure double clicks
  Point           m_lastMouseUpPos;      // screen position of last mouse up

  uiStyle *       m_style;

  int             m_lastUserActionTimeMS; // time when last user action (mouse/keyboard) has been received, measured in milliseconds since boot

  // associates event handler with FreeRTOS timer
  list<uiTimerAssoc> m_timers;

  // used to wait for asyncRunTask to terminate
  SemaphoreHandle_t m_asyncRunWait;
};




} // end of namespace


// get out of namespace frequently used names
using fabgl::uiObject;
using fabgl::uiButtonKind;
using fabgl::uiTimerHandle;
using fabgl::uiTextEdit;
using fabgl::uiApp;
using fabgl::uiFrame;
using fabgl::uiButton;
using fabgl::uiLabel;
using fabgl::uiStaticLabel;
using fabgl::uiImage;
using fabgl::uiPanel;
using fabgl::uiMessageBoxIcon;
using fabgl::uiPaintBox;
using fabgl::uiOrientation;
using fabgl::uiListBox;
using fabgl::uiComboBox;
using fabgl::uiCheckBox;
using fabgl::uiCheckBoxKind;
using fabgl::uiSlider;
using fabgl::uiStyle;
using fabgl::uiWindowStyle;
using fabgl::uiFrameStyle;
using fabgl::uiScrollableControlStyle;
using fabgl::uiButtonStyle;
using fabgl::uiTextEditStyle;
using fabgl::uiLabelStyle;
using fabgl::uiHAlign;
using fabgl::uiImageStyle;
using fabgl::uiPanelStyle;
using fabgl::uiPaintBoxStyle;
using fabgl::uiListBoxStyle;
using fabgl::uiComboBoxStyle;
using fabgl::uiCheckBoxStyle;
using fabgl::uiSliderStyle;
using fabgl::uiColorListBox;
using fabgl::uiColorBox;
using fabgl::uiColorComboBox;
using fabgl::uiProgressBar;
using fabgl::uiMessageBoxResult;
using fabgl::uiKeyEventInfo;
using fabgl::uiCustomListBox;
using fabgl::uiFileBrowser;
using fabgl::uiSplitButton;
using fabgl::uiSimpleMenu;
using fabgl::uiKnob;
using fabgl::uiKnobStyle;
using fabgl::uiKnobProps;
using fabgl::uiToggleSwitch;
using fabgl::uiToggleSwitchStyle;
using fabgl::uiLevelBar;
using fabgl::uiLevelBarStyle;
using fabgl::uiLevelBarProps;
using fabgl::uiLedBar;
using fabgl::uiLedBarStyle;
using fabgl::uiLedBarProps;
using fabgl::uiStepper;
using fabgl::uiStepperStyle;
using fabgl::uiTileSelector;
using fabgl::uiTileSelectorStyle;
using fabgl::uiTileSelectorProps;
using fabgl::uiGraph;
using fabgl::uiGraphStyle;
using fabgl::uiGraphProps;
using fabgl::uiPianoKeyboard;
using fabgl::uiPianoKeyboardStyle;
using fabgl::uiPianoKeyboardProps;




