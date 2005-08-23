//============================================================================
//
//   SSSS    tt          lll  lll       
//  SS  SS   tt           ll   ll        
//  SS     tttttt  eeee   ll   ll   aaaa 
//   SSSS    tt   ee  ee  ll   ll      aa
//      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
//  SS  SS   tt   ee      ll   ll  aa  aa
//   SSSS     ttt  eeeee llll llll  aaaaa
//
// Copyright (c) 1995-2005 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: ListWidget.hxx,v 1.11 2005-08-23 18:32:51 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef LIST_WIDGET_HXX
#define LIST_WIDGET_HXX

#include "GuiObject.hxx"
#include "Widget.hxx"
#include "Command.hxx"
#include "EditableWidget.hxx"
#include "Rect.hxx"

class StringList;
class ScrollBarWidget;

// Some special commands
enum {
  kListItemDoubleClickedCmd = 'LIdb',  // double click on item - 'data' will be item index
  kListItemActivatedCmd     = 'LIac',  // item activated by return/enter - 'data' will be item index
  kListItemDataChangedCmd   = 'LIch',  // item data changed - 'data' will be item index
  kListSelectionChangedCmd  = 'Lsch',  // selection changed - 'data' will be item index
  kListScrolledCmd          = 'Lscl'   // list scrolled - 'data' will be current position
};

/** ListWidget */
class ListWidget : public EditableWidget
{
  public:
    ListWidget(GuiObject* boss, const GUI::Font& font,
               int x, int y, int w, int h);
    virtual ~ListWidget();

    int getSelected() const     { return _selectedItem; }
    void setSelected(int item);

    const StringList& getList()	const        { return _list; }
    const string& getSelectedString() const  { return _list[_selectedItem]; }

    void scrollTo(int item);

    virtual void handleMouseDown(int x, int y, int button, int clickCount);
    virtual void handleMouseUp(int x, int y, int button, int clickCount);
    virtual void handleMouseWheel(int x, int y, int direction);
    virtual bool handleKeyDown(int ascii, int keycode, int modifiers);
    virtual bool handleKeyUp(int ascii, int keycode, int modifiers);
    virtual void handleCommand(CommandSender* sender, int cmd, int data, int id);

    virtual bool wantsFocus() { return true; }

    void startEditMode();
    void endEditMode();

  protected:
    virtual void drawWidget(bool hilite)  = 0;
    virtual GUI::Rect getEditRect() const = 0;

    int findItem(int x, int y) const;
    void recalc();
    void scrollBarRecalc();

    void abortEditMode();

    void lostFocusWidget();
    void scrollToCurrent();

  protected:
    int  _rows;
    int  _cols;
    int  _rowHeight;
    int  _colWidth;
    int  _currentPos;
    int  _selectedItem;
    int  _currentKeyDown;

    bool _editMode;
    bool _caretInverse;

    ScrollBarWidget* _scrollBar;

    StringList _list;
    string     _backupString;
    string     _quickSelectStr;
    int        _quickSelectTime;
};

#endif
