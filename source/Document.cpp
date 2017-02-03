
#include <QMessageBox>
#include <QX11Info>
#include "ui/DialogFind.h"
#include "ui/DialogReplace.h"
#include "ui/DialogWindowTitle.h"
#include "ui/DialogMoveDocument.h"
#include "macro.h"
#include "IndentStyle.h"
#include "WrapStyle.h"
#include "RangesetTable.h"
#include "TextHelper.h"
#include "util/MotifHelper.h"
#include "Document.h"
#include "file.h"
#include "highlight.h"
#include "interpret.h"
#include "menu.h"
#include "res/nedit.bm"
#include "preferences.h"
#include "search.h"
#include "selection.h"
#include "server.h"
#include "smartIndent.h"
#include "TextDisplay.h"
#include "TextBuffer.h"
#include "text.h"
#include "UndoInfo.h"
#include "userCmds.h"

#include "Microline/XmL/Folder.h"
#include "util/clearcase.h"
#include "util/misc.h"
#include "Xlt/BubbleButton.h"
#include "Xlt/BubbleButtonP.h"

#include <cmath>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <Xm/CascadeB.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/MainW.h>
#include <Xm/PanedW.h>
#include <Xm/PanedWP.h>
#include <Xm/ScrollBar.h>
#include <Xm/ScrolledW.h>
#include <Xm/ToggleB.h>

namespace {

// TODO(eteran): use an enum for this
const int FORWARD = 1;
const int REVERSE = 2;

// bitmap data for the close-tab button
const int close_width  = 11;
const int close_height = 11;

/* Thickness of 3D border around statistics and/or incremental search areas
   below the main menu bar */
const int STAT_SHADOW_THICKNESS = 1;

/* Initial minimum height of a pane.  Just a fallback in case setPaneMinHeight
   (which may break in a future release) is not available */
const int PANE_MIN_HEIGHT = 39;

// From Xt, Shell.c, "BIGSIZE"
const Dimension XT_IGNORE_PPOSITION = 32767;

Document *inFocusDocument   = nullptr; // where we are now
Document *lastFocusDocument = nullptr; // where we came from

/*
** perform generic management on the children (toolbars) of toolBarsForm,
** a.k.a. statsForm, by setting the form attachment of the managed child
** widgets per their position/order.
**
** You can optionally create separator after a toolbar widget with it's
** widget name set to "TOOLBAR_SEP", which will appear below the toolbar
** widget. These seperators will then be managed automatically by this
** routine along with the toolbars they 'attached' to.
**
** It also takes care of the attachment offset settings of the child
** widgets to keep the border lines of the parent form displayed, so
** you don't have set them before hand.
**
** Note: XtManage/XtUnmange the target child (toolbar) before calling this
**       function.
**
** Returns the last toolbar widget managed.
**
*/
Widget manageToolBars(Widget toolBarsForm) {
    Q_UNUSED(toolBarsForm);
#if 0 // NOTE(eteran)
	Widget topWidget = nullptr;
	WidgetList children;
	int n, nItems = 0;

	XtVaGetValues(toolBarsForm, XmNchildren, &children, XmNnumChildren, &nItems, nullptr);

	for (n = 0; n < nItems; n++) {
		Widget tbar = children[n];

		if (XtIsManaged(tbar)) {
			if (topWidget) {
				XtVaSetValues(tbar, XmNtopAttachment, XmATTACH_WIDGET, XmNtopWidget, topWidget, XmNbottomAttachment, XmATTACH_NONE, XmNleftOffset, STAT_SHADOW_THICKNESS, XmNrightOffset, STAT_SHADOW_THICKNESS, nullptr);
			} else {
				// the very first toolbar on top
				XtVaSetValues(tbar, XmNtopAttachment, XmATTACH_FORM, XmNbottomAttachment, XmATTACH_NONE, XmNleftOffset, STAT_SHADOW_THICKNESS, XmNtopOffset, STAT_SHADOW_THICKNESS, XmNrightOffset, STAT_SHADOW_THICKNESS, nullptr);
			}

			topWidget = tbar;

			// if the next widget is a separator, turn it on
			if (n + 1 < nItems && !strcmp(XtName(children[n + 1]), "TOOLBAR_SEP")) {
				XtManageChild(children[n + 1]);
			}
		} else {
			/* Remove top attachment to widget to avoid circular dependency.
			   Attach bottom to form so that when the widget is redisplayed
			   later, it will trigger the parent form to resize properly as
			   if the widget is being inserted */
			XtVaSetValues(tbar, XmNtopAttachment, XmATTACH_NONE, XmNbottomAttachment, XmATTACH_FORM, nullptr);

			// if the next widget is a separator, turn it off
			if (n + 1 < nItems && !strcmp(XtName(children[n + 1]), "TOOLBAR_SEP")) {
				XtUnmanageChild(children[n + 1]);
			}
		}
	}

	if (topWidget) {
		if (strcmp(XtName(topWidget), "TOOLBAR_SEP")) {
			XtVaSetValues(topWidget, XmNbottomAttachment, XmATTACH_FORM, XmNbottomOffset, STAT_SHADOW_THICKNESS, nullptr);
		} else {
			// is a separator
			Widget wgt;
			XtVaGetValues(topWidget, XmNtopWidget, &wgt, nullptr);

			// don't need sep below bottom-most toolbar
			XtUnmanageChild(topWidget);
			XtVaSetValues(wgt, XmNbottomAttachment, XmATTACH_FORM, XmNbottomOffset, STAT_SHADOW_THICKNESS, nullptr);
		}
	}

	return topWidget;
#endif
    return nullptr;
}

/*
** Paned windows are impossible to adjust after they are created, which makes
** them nearly useless for NEdit (or any application which needs to dynamically
** adjust the panes) unless you tweek some private data to overwrite the
** desired and minimum pane heights which were set at creation time.  These
** will probably break in a future release of Motif because of dependence on
** private data.
*/
void setPaneDesiredHeight(Widget w, int height) {
    Q_UNUSED(w);
    Q_UNUSED(height);
#if 0 // NOTE(eteran)
	reinterpret_cast<XmPanedWindowConstraintPtr>(w->core.constraints)->panedw.dheight = height;
#endif
}

/*
**
*/
void setPaneMinHeight(Widget w, int min) {
    Q_UNUSED(w);
    Q_UNUSED(min);
#if 0 // NOTE(eteran)
	reinterpret_cast<XmPanedWindowConstraintPtr>(w->core.constraints)->panedw.min = min;
#endif
}

/*
**
*/
Widget containingPane(Widget w) {
	/* The containing pane used to simply be the first parent, but with
	   the introduction of an XmFrame, it's the grandparent. */
	return XtParent(XtParent(w));
}

/*
** Xt timer procedure for updating size hints.  The new sizes of objects in
** the window are not ready immediately after adding or removing panes.  This
** is a timer routine to be invoked with a timeout of 0 to give the event
** loop a chance to finish processing the size changes before reading them
** out for setting the window manager size hints.
*/
void wmSizeUpdateProc(XtPointer clientData, XtIntervalId *id) {

	(void)id;
    Q_UNUSED(clientData);
#if 0 // NOTE(eteran)
	static_cast<Document *>(clientData)->UpdateWMSizeHints();
#endif
}

/*
** hide all the tearoffs spawned from this menu.
** It works recursively to close the tearoffs of the submenus
*/
void hideTearOffs(Widget menuPane) {
	WidgetList itemList;
	Widget subMenuID;
	Cardinal nItems;
	int n;

	// hide all submenu tearoffs
	XtVaGetValues(menuPane, XmNchildren, &itemList, XmNnumChildren, &nItems, nullptr);
	for (n = 0; n < (int)nItems; n++) {
		if (XtClass(itemList[n]) == xmCascadeButtonWidgetClass) {
			XtVaGetValues(itemList[n], XmNsubMenuId, &subMenuID, nullptr);
			hideTearOffs(subMenuID);
		}
	}

	// hide tearoff for this menu
	if (!XmIsMenuShell(XtParent(menuPane)))
		XtUnmapWidget(XtParent(menuPane));
}

/*
** Redisplay menu tearoffs previously hid by hideTearOffs()
*/
void redisplayTearOffs(Widget menuPane) {
    Q_UNUSED(menuPane)
#if 0 // NOTE(eteran)
	WidgetList itemList;
	Widget subMenuID;
	Cardinal nItems;
	int n;

	// redisplay all submenu tearoffs
	XtVaGetValues(menuPane, XmNchildren, &itemList, XmNnumChildren, &nItems, nullptr);
	for (n = 0; n < (int)nItems; n++) {
		if (XtClass(itemList[n]) == xmCascadeButtonWidgetClass) {
			XtVaGetValues(itemList[n], XmNsubMenuId, &subMenuID, nullptr);
			redisplayTearOffs(subMenuID);
		}
	}

	// redisplay tearoff for this menu
	if (!XmIsMenuShell(XtParent(menuPane)))
		ShowHiddenTearOff(menuPane);
#endif
}

/*
** return the integer position of a tab in the tabbar it
** belongs to, or -1 if there's an error, somehow.
*/
int getTabPosition(Widget tab) {
	WidgetList tabList;
	int tabCount;
	Widget tabBar = XtParent(tab);

	XtVaGetValues(tabBar, XmNtabWidgetList, &tabList, XmNtabCount, &tabCount, nullptr);

	for (int i = 0; i < tabCount; i++) {
		if (tab == tabList[i]) {
			return i;
		}
	}

	return -1; // something is wrong!
}

std::list<UndoInfo *> cloneUndoItems(const std::list<UndoInfo *> &orgList) {

	std::list<UndoInfo *> list;
	for(UndoInfo *undo : orgList) {
		list.push_back(new UndoInfo(*undo));
	}
	return list;
}

void cloneTextPanes(Document *window, Document *orgWin) {

	short paneHeights[MAX_PANES + 1];
	int insertPositions[MAX_PANES + 1];
	int topLines[MAX_PANES + 1];
	int horizOffsets[MAX_PANES + 1];
	int i;
	int focusPane;
	int lineNumCols;
	int totalHeight = 0;

	// transfer the primary selection
	TextSelection sel = orgWin->buffer_->primary_;

	if (sel.selected) {
		if (sel.rectangular)
			window->buffer_->BufRectSelect(sel.start, sel.end, sel.rectStart, sel.rectEnd);
		else
			window->buffer_->BufSelect(sel.start, sel.end);
	} else
		window->buffer_->BufUnselect();

	/* Record the current heights, scroll positions, and insert positions
	   of the existing panes, keyboard focus */
	focusPane = 0;

	for (i = 0; i <= orgWin->textPanes_.size(); i++) {
		Widget text = orgWin->GetPaneByIndex(i);

		auto textD = textD_of(text);

		insertPositions[i] = textD->TextGetCursorPos();
		XtVaGetValues(containingPane(text), XmNheight, &paneHeights[i], nullptr);
		totalHeight += paneHeights[i];
		textD->TextDGetScroll(&topLines[i], &horizOffsets[i]);

		if (text == orgWin->lastFocus_) {
			focusPane = i;
		}
	}

	int paneCount = orgWin->textPanes_.size();

	// Copy some parameters
	int emTabDist      = textD_of(orgWin->textArea_)->getEmulateTabs();
	QString delimiters = textD_of(orgWin->textArea_)->getWordDelimiters();
	int wrapMargin     = textD_of(orgWin->textArea_)->getWrapMargin();


	lineNumCols = orgWin->showLineNumbers_ ? MIN_LINE_NUM_COLS : 0;

	auto textD = textD_of(window->textArea_);
	textD->setEmulateTabs(emTabDist);
	textD->setWordDelimiters(delimiters);
	textD->setWrapMargin(wrapMargin);
	textD->setLineNumCols(lineNumCols);

	// clone split panes, if any
	if (paneCount) {
		// Unmanage & remanage the panedWindow so it recalculates pane heights
		XtUnmanageChild(window->splitPane_);

		/* Create a text widget to add to the pane and set its buffer and
		   highlight data to be the same as the other panes in the orgWin */

		for (i = 0; i < orgWin->textPanes_.size(); i++) {

			Widget text;

            text = Document::createTextArea(window->splitPane_, window, 1, 1, emTabDist, delimiters.toLatin1().data(), wrapMargin, lineNumCols);
			auto newTextD = textD_of(text);
			newTextD->TextSetBuffer(window->buffer_);

			if (window->highlightData_) {
				AttachHighlightToWidget(text, window);
			}

			XtManageChild(text);
			window->textPanes_.push_back(text);

			// Fix up the colors


			newTextD->setForegroundPixel(textD->foregroundPixel());
			newTextD->setBackgroundPixel(textD->backgroundPixel());

			newTextD->TextDSetColors(
				textD->foregroundPixel(),
				textD->backgroundPixel(),
				textD->getSelectFGPixel(),
				textD->getSelectBGPixel(),
				textD->getHighlightFGPixel(),
				textD->getHighlightBGPixel(),
				textD->getLineNumFGPixel(),
				textD->getCursorFGPixel());
		}

		// Set the minimum pane height in the new pane
		window->UpdateMinPaneHeights();

		for (i = 0; i <= window->textPanes_.size(); i++) {
			Widget text = window->GetPaneByIndex(i);
			setPaneDesiredHeight(containingPane(text), paneHeights[i]);
		}

		// Re-manage panedWindow to recalculate pane heights & reset selection
		XtManageChild(window->splitPane_);
	}

	// Reset all of the heights, scroll positions, etc.
	for (i = 0; i <= window->textPanes_.size(); i++) {
		Widget text = window->GetPaneByIndex(i);
		auto textD = textD_of(text);
		textD->TextSetCursorPos(insertPositions[i]);
		textD->TextDSetScroll(topLines[i], horizOffsets[i]);

		// dim the cursor
		textD->TextDSetCursorStyle(DIM_CURSOR);
		textD->TextDUnblankCursor();
	}

	// set the focus pane
	for (i = 0; i <= window->textPanes_.size(); i++) {
		Widget text = window->GetPaneByIndex(i);
		if (i == focusPane) {
			window->lastFocus_ = text;
			XmProcessTraversal(text, XmTRAVERSE_CURRENT);
			break;
		}
	}

	/* Update the window manager size hints after the sizes of the panes have
	   been set (the widget heights are not yet readable here, but they will
	   be by the time the event loop gets around to running this timer proc) */
	XtAppAddTimeOut(XtWidgetToApplicationContext(window->shell_), 0, wmSizeUpdateProc, window);
}

/* Add an icon to an applicaction shell widget.  addWindowIcon adds a large
** (primary window) icon, AddSmallIcon adds a small (secondary window) icon.
**
** Note: I would prefer that these were not hardwired, but yhere is something
** weird about the  XmNiconPixmap resource that prevents it from being set
** from the defaults in the application resource database.
*/
void addWindowIcon(Widget shell) {
	static Pixmap iconPixmap = 0, maskPixmap = 0;

	if (iconPixmap == 0) {
		iconPixmap = XCreateBitmapFromData(TheDisplay, RootWindowOfScreen(XtScreen(shell)), (char *)iconBits, iconBitmapWidth, iconBitmapHeight);
		maskPixmap = XCreateBitmapFromData(TheDisplay, RootWindowOfScreen(XtScreen(shell)), (char *)maskBits, iconBitmapWidth, iconBitmapHeight);
	}
	XtVaSetValues(shell, XmNiconPixmap, iconPixmap, XmNiconMask, maskPixmap, nullptr);
}

/*
**
*/
UndoTypes determineUndoType(int nInserted, int nDeleted) {
	int textDeleted, textInserted;

	textDeleted = (nDeleted > 0);
	textInserted = (nInserted > 0);

	if (textInserted && !textDeleted) {
		// Insert
		if (nInserted == 1)
			return ONE_CHAR_INSERT;
		else
			return BLOCK_INSERT;
	} else if (textInserted && textDeleted) {
		// Replace
		if (nInserted == 1)
			return ONE_CHAR_REPLACE;
		else
			return BLOCK_REPLACE;
	} else if (!textInserted && textDeleted) {
		// Delete
		if (nDeleted == 1)
			return ONE_CHAR_DELETE;
		else
			return BLOCK_DELETE;
	} else {
		// Nothing deleted or inserted
		return UNDO_NOOP;
	}
}

}

/*
** set/clear menu sensitivity if the calling document is on top.
*/
void Document::SetSensitive(Widget w, Boolean sensitive) const {
	if (IsTopDocument()) {
		XtSetSensitive(w, sensitive);
	}
}

/*
** set/clear toggle menu state if the calling document is on top.
*/
void Document::SetToggleButtonState(Widget w, Boolean state, Boolean notify) const {
	if (IsTopDocument()) {
		XmToggleButtonSetState(w, state, notify);
	}
}

/*
** remember the last document.
*/
Document *Document::MarkLastDocument() {
	Document *prev = lastFocusDocument;

	Q_ASSERT(this);

	if (this)
		lastFocusDocument = this;

	return prev;
}

/*
** remember the active (top) document.
*/
Document *Document::MarkActiveDocument() {
	Document *prev = inFocusDocument;

	Q_ASSERT(this);

	if (this)
		inFocusDocument = this;

	return prev;
}

/*
** Bring up the last active window
*/
void Document::LastDocument() const {

	auto it = std::find_if(WindowList.begin(), WindowList.end(), [](Document *win) {
		return lastFocusDocument == win;
	});

	if(it == WindowList.end()) {
		return;
	}

	Document *const win = *it;

	if (shell_ == win->shell_) {
		win->RaiseDocument();
	} else {
		win->RaiseFocusDocumentWindow(true);
	}
}

/*
** raise the document and its shell window and optionally focus.
*/
void Document::RaiseFocusDocumentWindow(Boolean focus) {

	Q_ASSERT(this);

	if (!this)
		return;

	RaiseDocument();
	RaiseShellWindow(shell_, focus);
}

/*
**
*/
bool Document::IsTopDocument() const {
	return this == GetTopDocument(shell_);
}

/*
**
*/
Widget Document::GetPaneByIndex(int index) const {
	Widget text = nullptr;
	if (index >= 0 && index <= textPanes_.size()) {
		text = (index == 0) ? textArea_ : textPanes_[index - 1];
	}
	return text;
}

/*
** make sure window is alive is kicking
*/
bool Document::IsValidWindow() {

	auto it = std::find_if(WindowList.begin(), WindowList.end(), [this](Document *win) {
		return this == win;
	});

	return it != WindowList.end();
}

/*
** check if tab bar is to be shown on this window
*/
bool Document::GetShowTabBar() {
	if (!GetPrefTabBar())
		return false;
	else if (TabCount() == 1)
		return !GetPrefTabBarHideOne();
	else
		return true;
}

/*
** Returns true if window is iconic (as determined by the WM_STATE property
** on the shell window.  I think this is the most reliable way to tell,
** but if someone has a better idea please send me a note).
*/
bool Document::IsIconic() {
	unsigned long *property = nullptr;
	unsigned long nItems;
	unsigned long leftover;
	static Atom wmStateAtom = 0;
	Atom actualType;
	int actualFormat;

	if (wmStateAtom == 0) {
		wmStateAtom = XInternAtom(XtDisplay(shell_), "WM_STATE", false);
	}

	if (XGetWindowProperty(XtDisplay(shell_), XtWindow(shell_), wmStateAtom, 0L, 1L, false, wmStateAtom, &actualType, &actualFormat, &nItems, &leftover, (uint8_t **)&property) != Success || nItems != 1 || !property) {
		return false;
	}

	int result = (*property == IconicState);
	XtFree((char *)property);
	return result;
}

/*
** return the next/previous docment on the tab list.
**
** If <wrap> is true then the next tab of the rightmost tab will be the
** second tab from the right, and the the previous tab of the leftmost
** tab will be the second from the left.  This is useful for getting
** the next tab after a tab detaches/closes and you don't want to wrap around.
*/
Document *Document::getNextTabWindow(int direction, int crossWin, int wrap) {
	WidgetList tabList;
	Document *win;
	int tabCount, tabTotalCount;
	int tabPos, nextPos;
	int i, n;
	int nBuf = crossWin ? WindowCount() : TabCount();

	if (nBuf <= 1) {
		return nullptr;
	}

	// get the list of tabs
	auto tabs = new Widget[nBuf];
	tabTotalCount = 0;
	if (crossWin) {
		int n, nItems;
		WidgetList children;

		XtVaGetValues(TheAppShell, XmNchildren, &children, XmNnumChildren, &nItems, nullptr);

		// get list of tabs in all windows
		for (n = 0; n < nItems; n++) {
			if (strcmp(XtName(children[n]), "textShell") || ((win = WidgetToWindow(children[n])) == nullptr))
				continue; // skip non-text-editor windows

			XtVaGetValues(win->tabBar_, XmNtabWidgetList, &tabList, XmNtabCount, &tabCount, nullptr);

			for (i = 0; i < tabCount; i++) {
				tabs[tabTotalCount++] = tabList[i];
			}
		}
	} else {
		// get list of tabs in this window
		XtVaGetValues(tabBar_, XmNtabWidgetList, &tabList, XmNtabCount, &tabCount, nullptr);

		for (i = 0; i < tabCount; i++) {
			if (TabToWindow(tabList[i])) // make sure tab is valid
				tabs[tabTotalCount++] = tabList[i];
		}
	}

	// find the position of the tab in the tablist
	tabPos = 0;
	for (n = 0; n < tabTotalCount; n++) {
		if (tabs[n] == tab_) {
			tabPos = n;
			break;
		}
	}

	// calculate index position of next tab
	nextPos = tabPos + direction;
	if (nextPos >= nBuf) {
		if (wrap)
			nextPos = 0;
		else
			nextPos = nBuf - 2;
	} else if (nextPos < 0) {
		if (wrap)
			nextPos = nBuf - 1;
		else
			nextPos = 1;
	}

	// return the document where the next tab belongs to
	win = TabToWindow(tabs[nextPos]);
	delete [] tabs;
	return win;
}

/*
** Displays and undisplays the statistics line (regardless of settings of
** showStats_ or modeMessageDisplayed_)
*/
void Document::showStatistics(int state) {
	if (state) {
		XtManageChild(statsLineForm_);
		showStatsForm();
	} else {
		XtUnmanageChild(statsLineForm_);
		showStatsForm();
	}

	// Tell WM that the non-expandable part of the this has changed size
	// Already done in showStatsForm
	// UpdateWMSizeHints();
}

/*
** Show or hide the extra display area under the main menu bar which
** optionally contains the status line and the incremental search bar
*/
void Document::showStatsForm() {
	Widget statsAreaForm = XtParent(statsLineForm_);
	Widget mainW = XtParent(statsAreaForm);

	/* The very silly use of XmNcommandWindowLocation and XmNshowSeparator
	   below are to kick the main this widget to position and remove the
	   status line when it is managed and unmanaged.  At some Motif version
	   level, the showSeparator trick backfires and leaves the separator
	   shown, but fortunately the dynamic behavior is fixed, too so the
	   workaround is no longer necessary, either.  (... the version where
	   this occurs may be earlier than 2.1. */
	if (manageToolBars(statsAreaForm)) {
		XtUnmanageChild(statsAreaForm); /*... will this fix Solaris 7??? */
		XtVaSetValues(mainW, XmNcommandWindowLocation, XmCOMMAND_ABOVE_WORKSPACE, nullptr);
		XtManageChild(statsAreaForm);
		XtVaSetValues(mainW, XmNshowSeparator, false, nullptr);
		UpdateStatsLine();
	} else {
		XtUnmanageChild(statsAreaForm);
		XtVaSetValues(mainW, XmNcommandWindowLocation, XmCOMMAND_BELOW_WORKSPACE, nullptr);
	}

	// Tell WM that the non-expandable part of the this has changed size
	UpdateWMSizeHints();
}

/*
** Add a this to the the this list.
*/
void Document::addToWindowList() {
	WindowList.push_front(this);
}

/*
** Remove a this from the list of windows
*/
void Document::removeFromWindowList() {

	auto it = std::find_if(WindowList.begin(), WindowList.end(), [this](Document *win) {
		return win == this;
	});

	if(it != WindowList.end()) {
		WindowList.erase(it);
	}
}

/*
** Remove redundant expose events on tab bar.
*/
void Document::CleanUpTabBarExposeQueue() {
	XEvent event;
	XExposeEvent ev;
	int count;

	Q_ASSERT(this);

	if(!this)
		return;

	// remove redundant expose events on tab bar
	count = 0;
	while (XCheckTypedWindowEvent(TheDisplay, XtWindow(tabBar_), Expose, &event))
		count++;

	// now we can update tabbar
	if (count) {
		ev.type = Expose;
		ev.display = TheDisplay;
		ev.window = XtWindow(tabBar_);
		ev.x = 0;
		ev.y = 0;
		ev.width = XtWidth(tabBar_);
		ev.height = XtHeight(tabBar_);
		ev.count = 0;
		XSendEvent(TheDisplay, XtWindow(tabBar_), false, ExposureMask, (XEvent *)&ev);
	}
}

/*
** refresh window state for this document
*/
void Document::RefreshWindowStates() {
	if (!IsTopDocument())
		return;

	if (modeMessageDisplayed_)
		XmTextSetStringEx(statsLine_, modeMessage_);
	else
		UpdateStatsLine();

	UpdateWindowReadOnly();
	UpdateWindowTitle();

	// show/hide statsline as needed
	if (modeMessageDisplayed_ && !XtIsManaged(statsLineForm_)) {
		// turn on statline to display mode message
		showStatistics(true);
	} else if (showStats_ && !XtIsManaged(statsLineForm_)) {
		// turn on statsline since it is enabled
		showStatistics(true);
	} else if (!showStats_ && !modeMessageDisplayed_ && XtIsManaged(statsLineForm_)) {
		// turn off statsline since there's nothing to show
		showStatistics(false);
	}

	// signal if macro/shell is running
	if (shellCmdData_ || macroCmdData_)
		BeginWait(shell_);
	else
		EndWait(shell_);

	// we need to force the statsline to reveal itself
	if (XtIsManaged(statsLineForm_)) {
		XmTextSetCursorPosition(statsLine_, 0);    // start of line
		XmTextSetCursorPosition(statsLine_, 9000); // end of line
	}

	XmUpdateDisplay(statsLine_);
	refreshMenuBar();

	updateLineNumDisp();
}

/*
** Refresh the various settings/state of the shell window per the
** settings of the top document.
*/
void Document::refreshMenuBar() {
#if 0 // NOTE(eteran):
	RefreshMenuToggleStates();

	// Add/remove language specific menu items
	UpdateUserMenus(this);

	// refresh selection-sensitive menus
	DimSelectionDepUserMenuItems(this, wasSelected_);
#endif
}

/*
**  If necessary, enlarges the window and line number display area to make
**  room for numbers.
*/
int Document::updateLineNumDisp() {
	if (!showLineNumbers_) {
		return 0;
	}

	/* Decide how wide the line number field has to be to display all
	   possible line numbers */
	return updateGutterWidth();
}

/*
**  Set the new gutter width in the window. Sadly, the only way to do this is
**  to set it on every single document, so we have to iterate over them.
**
**  (Iteration taken from TabCount(); is there a better way to do it?)
*/
int Document::updateGutterWidth() {

	int reqCols = MIN_LINE_NUM_COLS;
	int newColsDiff = 0;
	int maxCols = 0;


	for(Document *document: WindowList) {
		if (document->shell_ == shell_) {
			//  We found ourselves a document from this window.
			int lineNumCols, tmpReqCols;
			TextDisplay *textD = textD_of(document->textArea_);

			lineNumCols = textD_of(document->textArea_)->getLineNumCols();

			/* Is the width of the line number area sufficient to display all the
			   line numbers in the file?  If not, expand line number field, and the
			   this width. */

			if (lineNumCols > maxCols) {
				maxCols = lineNumCols;
			}

			tmpReqCols = textD->getBufferLinesCount() < 1 ? 1 : static_cast<int>(log10(static_cast<double>(textD->getBufferLinesCount()) + 1)) + 1;

			if (tmpReqCols > reqCols) {
				reqCols = tmpReqCols;
			}
		}
	}

	if (reqCols != maxCols) {
		XFontStruct *fs;
		Dimension windowWidth;
		short fontWidth;

		newColsDiff = reqCols - maxCols;

		fs = textD_of(textArea_)->getFont();
		fontWidth = fs->max_bounds.width;

		XtVaGetValues(shell_, XmNwidth, &windowWidth, nullptr);
		XtVaSetValues(shell_, XmNwidth, (Dimension)windowWidth + (newColsDiff * fontWidth), nullptr);

		UpdateWMSizeHints();
	}

	for (Document *document: WindowList) {
		if (document->shell_ == shell_) {
			int i;
			int lineNumCols;

			lineNumCols = textD_of(document->textArea_)->getLineNumCols();

			if (lineNumCols == reqCols) {
				continue;
			}

			//  Update all panes of this document.
			for (i = 0; i <= document->textPanes_.size(); i++) {
				Widget text = document->GetPaneByIndex(i);
				textD_of(text)->setLineNumCols(reqCols);
			}
		}
	}

	return reqCols;
}

/*
** Refresh the menu entries per the settings of the
** top document.
*/
void Document::RefreshMenuToggleStates() {

	if (!IsTopDocument())
		return;
#if 0 // NOTE(eteran): transitioned
	// File menu
	XtSetSensitive(printSelItem_, wasSelected_);

	// Edit menu

	XtSetSensitive(undoItem_, !undo_.empty());
	XtSetSensitive(redoItem_, !redo_.empty());
	XtSetSensitive(printSelItem_, wasSelected_);
	XtSetSensitive(cutItem_, wasSelected_);
	XtSetSensitive(copyItem_, wasSelected_);
	XtSetSensitive(delItem_, wasSelected_);

	// Preferences menu
	XmToggleButtonSetState(statsLineItem_, showStats_, false);
	XmToggleButtonSetState(iSearchLineItem_, showISearchLine_, false);
	XmToggleButtonSetState(lineNumsItem_, showLineNumbers_, false);
	XmToggleButtonSetState(highlightItem_, highlightSyntax_, false);
	XtSetSensitive(highlightItem_, languageMode_ != PLAIN_LANGUAGE_MODE);
	XmToggleButtonSetState(backlightCharsItem_, backlightChars_, false);
	XmToggleButtonSetState(saveLastItem_, saveOldVersion_, false);
	XmToggleButtonSetState(autoSaveItem_, autoSave_, false);
	XmToggleButtonSetState(overtypeModeItem_, overstrike_, false);
	XmToggleButtonSetState(matchSyntaxBasedItem_, matchSyntaxBased_, false);
	XmToggleButtonSetState(readOnlyItem_, lockReasons_.isUserLocked(), false);

	XtSetSensitive(smartIndentItem_, SmartIndentMacrosAvailable(LanguageModeName(languageMode_).toLatin1().data()));
#endif
	SetAutoIndent(indentStyle_);
	SetAutoWrap(wrapMode_);
	SetShowMatching(showMatchingStyle_);
	SetLanguageMode(this, languageMode_, false);

	// Windows Menu
#if 0 // NOTE(eteran): transitioned
	XtSetSensitive(splitPaneItem_, textPanes_.size() < MAX_PANES);
	XtSetSensitive(closePaneItem_, textPanes_.size() > 0);
	XtSetSensitive(detachDocumentItem_, TabCount() > 1);
	XtSetSensitive(contextDetachDocumentItem_, TabCount() > 1);

	auto it = std::find_if(WindowList.begin(), WindowList.end(), [this](Document *win) {
		return win->shell_ != shell_;
	});

	XtSetSensitive(moveDocumentItem_, it != WindowList.end());
#endif
}

/*
** update the tab label, etc. of a tab, per the states of it's document.
*/
void Document::RefreshTabState() {

	const char *tag = XmFONTLIST_DEFAULT_TAG;
	uint8_t alignment;

	QString labelString;

	/* Set tab label to document's filename. Position of
	   "*" (modified) will change per label alignment setting */
	XtVaGetValues(tab_, XmNalignment, &alignment, nullptr);
	if (alignment != XmALIGNMENT_END) {
		labelString = QString(QLatin1String("%1%2")).arg(fileChanged_ ? QLatin1String("*") : QLatin1String("")).arg(filename_);
	} else {
		labelString = QString(QLatin1String("%1%2")).arg(filename_).arg(fileChanged_ ? QLatin1String("*") : QLatin1String(""));
	}

	// Make the top document stand out a little more
	if (IsTopDocument()) {
		tag = (String) "BOLD";
	}

	XmString s1 = XmStringCreateLtoREx(labelString, tag);

	if (GetPrefShowPathInWindowsMenu() && filenameSet_) {
		labelString.append(QLatin1String(" - "));
		labelString.append(path_);
	}

	XmString tipString = XmStringCreateSimpleEx(labelString);

	XtVaSetValues(tab_, XltNbubbleString, tipString, XmNlabelString, s1, nullptr);
	XmStringFree(s1);
	XmStringFree(tipString);
}

/*
** return the number of documents owned by this shell window
*/
int Document::TabCount() const {

	int nDocument = 0;

	for(Document *win: WindowList) {
		if (win->shell_ == shell_) {
			nDocument++;
		}
	}

	return nDocument;
}

/*
** raise the document and its shell window and focus depending on pref.
*/
void Document::RaiseDocumentWindow() {
	RaiseDocument();
	RaiseShellWindow(shell_, GetPrefFocusOnRaise());
}

/*
** Update the optional statistics line.
*/
void Document::UpdateStatsLine() {
	int line;
	int colNum;
	Widget statW = statsLine_;
	XmString xmslinecol;

	if (!IsTopDocument()) {
		return;
	}

	/* This routine is called for each character typed, so its performance
	   affects overall editor perfomance.  Only update if the line is on. */
	if (!showStats_) {
		return;
	}

	auto textD = lastFocus();

	// Compose the string to display. If line # isn't available, leave it off
	int pos            = textD->TextGetCursorPos();
	size_t string_size = filename_.size() + path_.size() + 45;
	auto string        = new char[string_size];
	const char *format = (fileFormat_ == DOS_FILE_FORMAT) ? " DOS" : (fileFormat_ == MAC_FILE_FORMAT ? " Mac" : "");
	char slinecol[32];

	if (!textD->TextDPosToLineAndCol(pos, &line, &colNum)) {
		snprintf(string, string_size, "%s%s%s %d bytes", path_.toLatin1().data(), filename_.toLatin1().data(), format, buffer_->BufGetLength());
		snprintf(slinecol, sizeof(slinecol), "L: ---  C: ---");
	} else {
		snprintf(slinecol, sizeof(slinecol), "L: %d  C: %d", line, colNum);
		if (showLineNumbers_) {
			snprintf(string, string_size, "%s%s%s byte %d of %d", path_.toLatin1().data(), filename_.toLatin1().data(), format, pos, buffer_->BufGetLength());
		} else {
			snprintf(string, string_size, "%s%s%s %d bytes", path_.toLatin1().data(), filename_.toLatin1().data(), format, buffer_->BufGetLength());
		}
	}

	// Update the line/column number
	xmslinecol = XmStringCreateSimpleEx(slinecol);
	XtVaSetValues(statsLineColNo_, XmNlabelString, xmslinecol, nullptr);
	XmStringFree(xmslinecol);

	// Don't clobber the line if there's a special message being displayed
	if (!modeMessageDisplayed_) {
		// Change the text in the stats line
		XmTextReplace(statW, 0, XmTextGetLastPosition(statW), string);
	}
	delete [] string;

	// Update the line/col display
	xmslinecol = XmStringCreateSimpleEx(slinecol);
	XtVaSetValues(statsLineColNo_, XmNlabelString, xmslinecol, nullptr);
	XmStringFree(xmslinecol);
}

/*
** Update the window manager's size hints.  These tell it the increments in
** which it is allowed to resize the window.  While this isn't particularly
** important for NEdit (since it can tolerate any window size), setting these
** hints also makes the resize indicator show the window size in characters
** rather than pixels, which is very helpful to users.
*/
void Document::UpdateWMSizeHints() {

	Dimension shellWidth;
	Dimension shellHeight;
	Dimension textHeight;
	Dimension hScrollBarHeight;
	int fontHeight;
	int fontWidth;
	Widget hScrollBar;
	auto textD = textD_of(textArea_);

	// Find the dimensions of a single character of the text font
	XFontStruct *fs = textD->getFont();
	fontHeight      = textD->fontAscent() + textD->fontDescent();
	fontWidth       = fs->max_bounds.width;

	/* Find the base (non-expandable) width and height of the editor this.

	   FIXME:
	   To workaround the shrinking-this bug on some WM such as Metacity,
	   which caused the this to shrink as we switch between documents
	   using different font sizes on the documents in the same this, the
	   base width, and similarly the base height, is ajusted such that:
	        shellWidth = baseWidth + cols * textWidth
	   There are two issues with this workaround:
	   1. the right most characters may appear partially obsure
	   2. the Col x Row info reported by the WM will be based on the fully
	      display text.
	*/


	int marginHeight = textD_of(textArea_)->getMarginHeight();
#if 0
	int marginWidth  = textD_of(textArea_)->getMarginWidth();
#endif

	XtVaGetValues(textArea_, XmNheight, &textHeight, nullptr);
	int totalHeight = textHeight - 2 * marginHeight;

	for (int i = 0; i < textPanes_.size(); i++) {
		XtVaGetValues(textPanes_[i], XmNheight, &textHeight, textNhScrollBar, &hScrollBar, nullptr);
		totalHeight += textHeight - 2 * marginHeight;
		if (!XtIsManaged(hScrollBar)) {
			XtVaGetValues(hScrollBar, XmNheight, &hScrollBarHeight, nullptr);
			totalHeight -= hScrollBarHeight;
		}
	}

	XtVaGetValues(shell_, XmNwidth, &shellWidth, XmNheight, &shellHeight, nullptr);

	const int nCols      = textD->getRect().width / fontWidth;
	const int nRows      = totalHeight / fontHeight;
	const int baseWidth  = shellWidth - nCols * fontWidth;
	const int baseHeight = shellHeight - nRows * fontHeight;

	// Set the size hints in the shell widget
	XtVaSetValues(
		shell_,
		XmNwidthInc,   fs->max_bounds.width,
		XmNheightInc,  fontHeight,
		XmNbaseWidth,  baseWidth,
		XmNbaseHeight, baseHeight,
		XmNminWidth,   baseWidth + fontWidth,
		XmNminHeight,  baseHeight + (1 + textPanes_.size()) * fontHeight,
		nullptr);

	/* Motif will keep placing this on the shell every time we change it,
	   so it needs to be undone every single time.  This only seems to
	   happen on mult-head dispalys on screens 1 and higher. */

	RemovePPositionHint(shell_);
}

/*
**
*/
int Document::WidgetToPaneIndex(Widget w) const {

	int paneIndex = 0;

	for (int i = 0; i <= textPanes_.size(); ++i) {
		Widget text = GetPaneByIndex(i);
		if (text == w) {
			paneIndex = i;
		}
	}
	return (paneIndex);
}

/*
**
*/
void Document::SetTabDist(int tabDist) {
	if (buffer_->tabDist_ != tabDist) {
		int saveCursorPositions[MAX_PANES + 1];
		int saveVScrollPositions[MAX_PANES + 1];
		int saveHScrollPositions[MAX_PANES + 1];
		int paneIndex;

		ignoreModify_ = true;

		for (paneIndex = 0; paneIndex <= textPanes_.size(); ++paneIndex) {
			Widget w = GetPaneByIndex(paneIndex);
			auto textD = textD_of(w);

			textD->TextDGetScroll(&saveVScrollPositions[paneIndex], &saveHScrollPositions[paneIndex]);
			saveCursorPositions[paneIndex] = textD->TextGetCursorPos();
			textD->setModifyingTabDist(1);
		}

		buffer_->BufSetTabDistance(tabDist);

		for (paneIndex = 0; paneIndex <= textPanes_.size(); ++paneIndex) {
			Widget w = GetPaneByIndex(paneIndex);
			auto textD = textD_of(w);

			textD->setModifyingTabDist(0);
			textD->TextSetCursorPos(saveCursorPositions[paneIndex]);
			textD->TextDSetScroll(saveVScrollPositions[paneIndex], saveHScrollPositions[paneIndex]);
		}

		ignoreModify_ = false;
	}
}

/*
**
*/
void Document::SetEmTabDist(int emTabDist) {

	textD_of(textArea_)->setEmulateTabs(emTabDist);
	for (int i = 0; i < textPanes_.size(); ++i) {
		textD_of(textPanes_[i])->setEmulateTabs(emTabDist);
	}
}

/*
**
*/
void Document::showTabBar(int state) {
	if (state) {
		XtManageChild(XtParent(tabBar_));
		showStatsForm();
	} else {
		XtUnmanageChild(XtParent(tabBar_));
		showStatsForm();
	}
}

/*
**
*/
void Document::ShowTabBar(int state) {
	if (XtIsManaged(XtParent(tabBar_)) == state)
		return;
	showTabBar(state);
}

/*
** Turn on and off the continuing display of the incremental search line
** (when off, it is popped up and down as needed via TempShowISearch)
*/
void Document::ShowISearchLine(bool state) {

	if (showISearchLine_ == state)
		return;
	showISearchLine_ = state;

#if 0
	showISearch(state);
#endif
	/* i-search line is shell-level, hence other tabbed
	   documents in the this should synch */
	for (Document *win: WindowList) {
		if (win->shell_ != shell_ || win == this)
			continue;
		win->showISearchLine_ = state;
	}
}

/*
** Display a special message in the stats line (show the stats line if it
** is not currently shown).
*/
void Document::SetModeMessage(const char *message) {
	/* this document may be hidden (not on top) or later made hidden,
	   so we save a copy of the mode message, so we can restore the
	   statsline when the document is raised to top again */
	modeMessageDisplayed_ = true;
	modeMessage_ = QLatin1String(message);

	if (!IsTopDocument()) {
		return;
	}

	XmTextSetStringEx(statsLine_, message);

	/*
	 * Don't invoke the stats line again, if stats line is already displayed.
	 */
	if (!showStats_) {
		showStatistics(true);
	}
}

/*
** Clear special statistics line message set in SetModeMessage, returns
** the statistics line to its original state as set in window->showStats_
*/
void Document::ClearModeMessage() {

	if (!modeMessageDisplayed_) {
		return;
	}

	modeMessageDisplayed_ = false;
	modeMessage_ = QString();

	if (!IsTopDocument()) {
		return;
	}

	/*
	 * Remove the stats line only if indicated by it's window state.
	 */
	if (!showStats_) {
		showStatistics(false);
	}

	UpdateStatsLine();
}

/*
** Set the backlight character class string
*/
void Document::SetBacklightChars(char *applyBacklightTypes) {

	const bool is_applied = XmToggleButtonGetState(backlightCharsItem_) ? true : false;
	const bool do_apply   = applyBacklightTypes ? true : false;

	backlightChars_ = do_apply;

	if(backlightChars_) {
		backlightCharTypes_ = QLatin1String(applyBacklightTypes);
	} else {
		backlightCharTypes_ = QString();
	}

	textD_of(textArea_)->setBacklightCharTypes(backlightCharTypes_);
	for (int i = 0; i < textPanes_.size(); i++) {
		textD_of(textPanes_[i])->setBacklightCharTypes(backlightCharTypes_);
	}

	if (is_applied != do_apply) {
		SetToggleButtonState(backlightCharsItem_, do_apply, false);
	}
}

/*
** Update the read-only state of the text area(s) in the window, and
** the ReadOnly toggle button in the File menu to agree with the state in
** the window data structure.
*/
void Document::UpdateWindowReadOnly() {

	if (!IsTopDocument()) {
		return;
	}

	bool state = lockReasons_.isAnyLocked();

	textD_of(textArea_)->setReadOnly(state);

	for (int i = 0; i < textPanes_.size(); i++) {
		textD_of(textPanes_[i])->setReadOnly(state);
	}

#if 0 // NOTE(eteran): transitioned
	XmToggleButtonSetState(readOnlyItem_, state, false);
	XtSetSensitive(readOnlyItem_, !lockReasons_.isAnyLockedIgnoringUser());
#endif
}

/*
** Bring up the previous window by tab order
*/
void Document::PreviousDocument() {

	if (WindowList.size() == 1) {
		return;
	}

	Document *win = getNextTabWindow(-1, GetPrefGlobalTabNavigate(), 1);
	if(!win) {
		return;
	}

	if (shell_ == win->shell_) {
		win->RaiseDocument();
	} else {
		win->RaiseFocusDocumentWindow(true);
	}
}

/*
** Bring up the next window by tab order
*/
void Document::NextDocument() {


	if (WindowList.size() == 1) {
		return;
	}

	Document *win = getNextTabWindow(1, GetPrefGlobalTabNavigate(), 1);
	if(!win) {
		return;
	}

	if (shell_ == win->shell_) {
		win->RaiseDocument();
	} else {
		win->RaiseFocusDocumentWindow(true);
	}
}

/*
** Change the window appearance and the window data structure to show
** that the file it contains has been modified
*/
void Document::SetWindowModified(bool modified) {
	if (fileChanged_ == false && modified == true) {
		SetSensitive(closeItem_, true);
		fileChanged_ = true;
		UpdateWindowTitle();
		RefreshTabState();
	} else if (fileChanged_ == true && modified == false) {
		fileChanged_ = false;
		UpdateWindowTitle();
		RefreshTabState();
	}
}

/*
** Update the window title to reflect the filename, read-only, and modified
** status of the window data structure
*/
void Document::UpdateWindowTitle() {
#if 0 // NOTE(eteran)
	if (!IsTopDocument()) {
		return;
	}

	QString clearCaseTag = GetClearCaseViewTag();

	QString title = DialogWindowTitle::FormatWindowTitle(
		filename_,
		path_,
		clearCaseTag,
		QLatin1String(GetPrefServerName()),
		IsServer,
		filenameSet_,
		lockReasons_,
		fileChanged_,
		QLatin1String(GetPrefTitleFormat()));


	QString iconTitle = filename_;
	if (fileChanged_) {
		iconTitle.append(QLatin1String("*"));
	}

	XtVaSetValues(shell_, XmNtitle, title.toLatin1().data(), XmNiconName, iconTitle.toLatin1().data(), nullptr);

	/* If there's a find or replace dialog up in "Keep Up" mode, with a
	   file name in the title, update it too */
	if (auto dialog = qobject_cast<DialogFind *>(dialogFind_)) {
		if(dialog->keepDialog()) {
			title = QString(QLatin1String("Find (in %1)")).arg(filename_);
			dialog->setWindowTitle(title);
		}
	}

	if(auto dialog = getDialogReplace()) {
		if(dialog->keepDialog()) {
			title = QString(QLatin1String("Replace (in %1)")).arg(filename_);
			dialog->setWindowTitle(title);
		}
	}

	// Update the Windows menus with the new name
	InvalidateWindowMenus();
#endif
}

/*
** Sort tabs in the tab bar alphabetically, if demanded so.
*/
void Document::SortTabBar() {

	if (!GetPrefSortTabs())
		return;

	// need more than one tab to sort
	const int nDoc = TabCount();
	if (nDoc < 2) {
		return;
	}

	// first sort the documents
	std::vector<Document *> windows;
	windows.reserve(nDoc);

	for(Document *w: WindowList) {
		if (shell_ == w->shell_)
			windows.push_back(w);
	}

	std::sort(windows.begin(), windows.end(), [](const Document *a, const Document *b) {
		if(a->filename_ < b->filename_) {
			return true;
		}
		return a->path_ < b->path_;
	});

	// assign tabs to documents in sorted order
	WidgetList tabList;
	int tabCount;
	XtVaGetValues(tabBar_, XmNtabWidgetList, &tabList, XmNtabCount, &tabCount, nullptr);

	for (int i = 0, j = 0; i < tabCount && j < nDoc; i++) {
		if (tabList[i]->core.being_destroyed)
			continue;

		// set tab as active
		if (windows[j]->IsTopDocument())
			XmLFolderSetActiveTab(tabBar_, i, false);

		windows[j]->tab_ = tabList[i];
		windows[j]->RefreshTabState();
		j++;
	}
}

/*
** Update the minimum allowable height for a split pane after a change
** to font or margin height.
*/
void Document::UpdateMinPaneHeights() {

	auto textD = textD_of(textArea_);
	Dimension hsbHeight;
	Dimension swMarginHeight;
	Dimension frameShadowHeight;
	int minPaneHeight;
	Widget hScrollBar;

	// find the minimum allowable size for a pane
	XtVaGetValues(textArea_, textNhScrollBar, &hScrollBar, nullptr);
	XtVaGetValues(containingPane(textArea_), XmNscrolledWindowMarginHeight, &swMarginHeight, nullptr);
	XtVaGetValues(XtParent(textArea_), XmNshadowThickness, &frameShadowHeight, nullptr);

	int marginHeight = textD->getMarginHeight();

	XtVaGetValues(hScrollBar, XmNheight, &hsbHeight, nullptr);
	minPaneHeight = textD->fontAscent() + textD->fontDescent() + marginHeight * 2 + swMarginHeight * 2 + hsbHeight + 2 * frameShadowHeight;

	// Set it in all of the widgets in the this
	setPaneMinHeight(containingPane(textArea_), minPaneHeight);

	for (int i = 0; i < textPanes_.size(); i++) {
		setPaneMinHeight(containingPane(textPanes_[i]), minPaneHeight);
	}
}

/*
** Update the "New (in X)" menu item to reflect the preferences
*/
void Document::UpdateNewOppositeMenu(int openInTab) {
    Q_UNUSED(openInTab);
#if 0 // NOTE(eteran): transitioned
	XmString lbl;
	if (openInTab)
		XtVaSetValues(newOppositeItem_, XmNlabelString, lbl = XmStringCreateSimpleEx("New Window"), XmNmnemonic, 'W', nullptr);
	else
		XtVaSetValues(newOppositeItem_, XmNlabelString, lbl = XmStringCreateSimpleEx("New Tab"), XmNmnemonic, 'T', nullptr);
	XmStringFree(lbl);
#endif
}

/*
** Set insert/overstrike mode
*/
void Document::SetOverstrike(bool overstrike) {

	textD_of(textArea_)->setOverstrike(overstrike);

	for (int i = 0; i < textPanes_.size(); i++) {
		textD_of(textPanes_[i])->setOverstrike(overstrike);
	}

	overstrike_ = overstrike;
}

/*
** Select auto-wrap mode, one of NO_WRAP, NEWLINE_WRAP, or CONTINUOUS_WRAP
*/
void Document::SetAutoWrap(int state) {
	bool autoWrap = (state == NEWLINE_WRAP);
	bool contWrap = (state == CONTINUOUS_WRAP);


	textD_of(textArea_)->setAutoWrap(autoWrap);
	textD_of(textArea_)->setContinuousWrap(contWrap);

	for (int i = 0; i < textPanes_.size(); i++) {
		textD_of(textPanes_[i])->setAutoWrap(autoWrap);
		textD_of(textPanes_[i])->setContinuousWrap(contWrap);
	}

	wrapMode_ = state;

	if (IsTopDocument()) {
#if 0 // NOTE(eteran): transitioned
		XmToggleButtonSetState(newlineWrapItem_, autoWrap, false);
		XmToggleButtonSetState(continuousWrapItem_, contWrap, false);
		XmToggleButtonSetState(noWrapItem_, state == NO_WRAP, false);
#endif
	}
}

/*
** Set the auto-scroll margin
*/
void Document::SetAutoScroll(int margin) {

	textD_of(textArea_)->setCursorVPadding(margin);
	for(Widget tw : textPanes_) {
		textD_of(tw)->setCursorVPadding(margin);
	}
}

/*
**
*/
void Document::SetColors(const char *textFg, const char *textBg, const char *selectFg, const char *selectBg, const char *hiliteFg, const char *hiliteBg, const char *lineNoFg, const char *cursorFg) {

	Pixel textFgPix   = AllocColor(textFg);
	Pixel textBgPix   = AllocColor(textBg);
	Pixel selectFgPix = AllocColor(selectFg);
	Pixel selectBgPix = AllocColor(selectBg);
	Pixel hiliteFgPix = AllocColor(hiliteFg);
	Pixel hiliteBgPix = AllocColor(hiliteBg);
	Pixel lineNoFgPix = AllocColor(lineNoFg);
	Pixel cursorFgPix = AllocColor(cursorFg);


	// Update the main pane
	TextDisplay *textD = textD_of(textArea_);
	textD->setForegroundPixel(textFgPix);
	textD->setBackgroundPixel(textBgPix);

	textD->TextDSetColors(textFgPix, textBgPix, selectFgPix, selectBgPix, hiliteFgPix, hiliteBgPix, lineNoFgPix, cursorFgPix);

	// Update any additional panes
	for(Widget tw : textPanes_) {
		if(TextDisplay *textD = textD_of(tw)) {
			textD->setForegroundPixel(textFgPix);
			textD->setBackgroundPixel(textBgPix);
			textD->TextDSetColors(textFgPix, textBgPix, selectFgPix, selectBgPix, hiliteFgPix, hiliteBgPix, lineNoFgPix, cursorFgPix);
		}
	}

	// Redo any syntax highlighting
	if (highlightData_) {
		UpdateHighlightStyles(this);
	}
}

/*
** close all the documents in a window
*/
bool Document::CloseAllDocumentInWindow() {
#if 0
	if (TabCount() == 1) {
		// only one document in the window
		return CloseFileAndWindow(this, PROMPT_SBC_DIALOG_RESPONSE);
	} else {
		Widget winShell = shell_;
		Document *topDocument;

		// close all _modified_ documents belong to this window
		for (auto it = WindowList.begin(); it != WindowList.end();) {
			Document *const win = *it;
			if (win->shell_ == winShell && win->fileChanged_) {
				auto next = std::next(it);
				if (!CloseFileAndWindow(win, PROMPT_SBC_DIALOG_RESPONSE)) {
					return false;
				}
				it = next;
			} else {
				++it;
			}
		}

		// see there's still documents left in the window
		auto it = std::find_if(WindowList.begin(), WindowList.end(), [winShell](Document *win) {
			return win->shell_ == winShell;
		});

		if (it != WindowList.end()) {

			topDocument = GetTopDocument(winShell);

			// close all non-top documents belong to this window
			for (auto it = WindowList.begin(); it != WindowList.end(); ) {
				Document *const win = *it;
				if (win->shell_ == winShell && win != topDocument) {
					auto next = std::next(it);
					if (!CloseFileAndWindow(win, PROMPT_SBC_DIALOG_RESPONSE)) {
						return false;
					}
					it = next;
				} else {
					++it;
				}
			}

			// close the last document and its window
			if (!CloseFileAndWindow(topDocument, PROMPT_SBC_DIALOG_RESPONSE)) {
				return false;
			}
		}
	}
#endif
	return true;
}

/*
** spin off the document to a new window
*/
Document *Document::DetachDocument() {
#if 0
	Document *win = nullptr;

	if (TabCount() < 2) {
		return nullptr;
	}

	/* raise another document in the same shell this if the this
	   being detached is the top document */
	if (IsTopDocument()) {
		win = getNextTabWindow(1, 0, 0);
		if(win) {
			win->RaiseDocument();
		}
	}

	// Create a new this
	auto cloneWin = new Document(filename_, nullptr, false);

	/* CreateWindow() simply adds the new this's pointer to the
	   head of WindowList. We need to adjust the detached this's
	   pointer, so that macro functions such as focus_window("last")
	   will travel across the documents per the sequence they're
	   opened. The new doc will appear to replace it's former self
	   as the old doc is closed. */
	WindowList.pop_front();
	auto curr = std::find_if(WindowList.begin(), WindowList.end(), [this](Document *doc) {
		return doc == this;
	});

	WindowList.insert(std::next(curr), cloneWin);

	/* these settings should follow the detached document.
	   must be done before cloning this, else the height
	   of split panes may not come out correctly */
	cloneWin->ShowISearchLine(showISearchLine_);
	cloneWin->ShowStatsLine(showStats_);

	// clone the document & its pref settings
	cloneDocument(cloneWin);

	// remove the document from the old this
	fileChanged_ = false;
	CloseFileAndWindow(this, NO_SBC_DIALOG_RESPONSE);

	// refresh former host this
	if (win) {
		win->RefreshWindowStates();
	}

	// this should keep the new document this fresh
	cloneWin->RefreshWindowStates();
	cloneWin->RefreshTabState();
	cloneWin->SortTabBar();

	return cloneWin;
#endif
    return nullptr;
}

/*
** If the selection (or cursor position if there's no selection) is not
** fully shown, scroll to bring it in to view.  Note that as written,
** this won't work well with multi-line selections.  Modest re-write
** of the horizontal scrolling part would be quite easy to make it work
** well with rectangular selections.
*/
void Document::MakeSelectionVisible(Widget textPane) {

	Dimension width;
	bool isRect;
	int horizOffset;
	int lastLineNum;
	int left;
	int leftLineNum;
	int leftX;
	int linesToScroll;
	int margin;
	int rectEnd;
	int rectStart;
	int right;
	int rightLineNum;
	int rightX;
	int rows;
	int scrollOffset;
	int targetLineNum;
	int topLineNum;
	int y;

	auto textD   = textD_of(textPane);
	int topChar  = textD->TextFirstVisiblePos();
	int lastChar = textD->TextLastVisiblePos();

	// find out where the selection is
	if (!buffer_->BufGetSelectionPos(&left, &right, &isRect, &rectStart, &rectEnd)) {
		left = right = textD->TextGetCursorPos();
		isRect = false;
	}

	/* Check vertical positioning unless the selection is already shown or
	   already covers the display.  If the end of the selection is below
	   bottom, scroll it in to view until the end selection is scrollOffset
	   lines from the bottom of the display or the start of the selection
	   scrollOffset lines from the top.  Calculate a pleasing distance from the
	   top or bottom of the window, to scroll the selection to (if scrolling is
	   necessary), around 1/3 of the height of the window */
	if (!((left >= topChar && right <= lastChar) || (left <= topChar && right >= lastChar))) {

		rows = textD->getRows();

		scrollOffset = rows / 3;
		textD->TextDGetScroll(&topLineNum, &horizOffset);
		if (right > lastChar) {
			// End of sel. is below bottom of screen
			leftLineNum = topLineNum + textD->TextDCountLines(topChar, left, false);
			targetLineNum = topLineNum + scrollOffset;
			if (leftLineNum >= targetLineNum) {
				// Start of sel. is not between top & target
				linesToScroll = textD->TextDCountLines(lastChar, right, false) + scrollOffset;
				if (leftLineNum - linesToScroll < targetLineNum)
					linesToScroll = leftLineNum - targetLineNum;
				// Scroll start of selection to the target line
				textD->TextDSetScroll(topLineNum + linesToScroll, horizOffset);
			}
		} else if (left < topChar) {
			// Start of sel. is above top of screen
			lastLineNum = topLineNum + rows;
			rightLineNum = lastLineNum - textD->TextDCountLines(right, lastChar, false);
			targetLineNum = lastLineNum - scrollOffset;
			if (rightLineNum <= targetLineNum) {
				// End of sel. is not between bottom & target
				linesToScroll = textD->TextDCountLines(left, topChar, false) + scrollOffset;
				if (rightLineNum + linesToScroll > targetLineNum)
					linesToScroll = targetLineNum - rightLineNum;
				// Scroll end of selection to the target line
				textD->TextDSetScroll(topLineNum - linesToScroll, horizOffset);
			}
		}
	}

	/* If either end of the selection off screen horizontally, try to bring it
	   in view, by making sure both end-points are visible.  Using only end
	   points of a multi-line selection is not a great idea, and disaster for
	   rectangular selections, so this part of the routine should be re-written
	   if it is to be used much with either.  Note also that this is a second
	   scrolling operation, causing the display to jump twice.  It's done after
	   vertical scrolling to take advantage of TextDPosToXY which requires it's
	   reqested position to be vertically on screen) */
	if (textD->TextDPositionToXY(left, &leftX, &y) && textD->TextDPositionToXY(right, &rightX, &y) && leftX <= rightX) {
		textD->TextDGetScroll(&topLineNum, &horizOffset);

		margin = textD->getMarginWidth();
		width  = textD->getWidth();

		if (leftX < margin + textD->getLineNumLeft() + textD->getLineNumWidth())
			horizOffset -= margin + textD->getLineNumLeft() + textD->getLineNumWidth() - leftX;
		else if (rightX > width - margin)
			horizOffset += rightX - (width - margin);

		textD->TextDSetScroll(topLineNum, horizOffset);
	}

	// make sure that the statistics line is up to date
	UpdateStatsLine();
}

/*
** Move document to an other window.
**
** the moving document will receive certain window settings from
** its new host, i.e. the window size, stats and isearch lines.
*/
Document *Document::MoveDocument(Document *toWindow) {
    Q_UNUSED(toWindow);
#if 0
	Document *win = nullptr, *cloneWin;

	// prepare to move document
	if (TabCount() < 2) {
		// hide the this to make it look like we are moving
		XtUnmapWidget(shell_);
	} else if (IsTopDocument()) {
		// raise another document to replace the document being moved
		win = getNextTabWindow(1, 0, 0);
		if(win) {
			win->RaiseDocument();
		}
	}

	// relocate the document to target this
	cloneWin = toWindow->CreateDocument(filename_);
	cloneWin->ShowTabBar(cloneWin->GetShowTabBar());
	cloneDocument(cloneWin);

	/* CreateDocument() simply adds the new this's pointer to the
	   head of WindowList. We need to adjust the detached this's
	   pointer, so that macro functions such as focus_window("last")
	   will travel across the documents per the sequence they're
	   opened. The new doc will appear to replace it's former self
	   as the old doc is closed. */
	WindowList.pop_front();
	auto curr = std::find_if(WindowList.begin(), WindowList.end(), [this](Document *doc) {
		return doc == this;
	});

	WindowList.insert(std::next(curr), cloneWin);

	// remove the document from the old this
	fileChanged_ = false;
	CloseFileAndWindow(this, NO_SBC_DIALOG_RESPONSE);

	// some menu states might have changed when deleting document
	if (win) {
		win->RefreshWindowStates();
	}

	// this should keep the new document this fresh
	cloneWin->RaiseDocumentWindow();
	cloneWin->RefreshTabState();
	cloneWin->SortTabBar();

	return cloneWin;
#endif
    return nullptr;
}

void Document::ShowWindowTabBar() {
	if (GetPrefTabBar()) {
		if (GetPrefTabBarHideOne()) {
			ShowTabBar(TabCount() > 1);
		} else {
			ShowTabBar(true);
		}
	} else {
		ShowTabBar(false);
	}
}

/*
** Turn on and off the display of line numbers
*/
void Document::ShowLineNumbers(bool state) {
	Widget text;
	int i;
	unsigned reqCols = 0;
	Dimension windowWidth;
	auto textD = textD_of(textArea_);

	if (showLineNumbers_ == state)
		return;
	showLineNumbers_ = state;

	/* Just setting showLineNumbers_ is sufficient to tell
	   updateLineNumDisp() to expand the line number areas and the this
	   size for the number of lines required.  To hide the line number
	   display, set the width to zero, and contract the this width. */
	if (state) {
		reqCols = updateLineNumDisp();
	} else {
		XtVaGetValues(shell_, XmNwidth, &windowWidth, nullptr);

		int marginWidth = textD_of(textArea_)->getMarginWidth();

		XtVaSetValues(shell_, XmNwidth, windowWidth - textD->getRect().left + marginWidth, nullptr);

		for (i = 0; i <= textPanes_.size(); i++) {
			text = GetPaneByIndex(i);
			textD_of(text)->setLineNumCols(0);
		}
	}

	/* line numbers panel is shell-level, hence other
	   tabbed documents in the this should synch */
	for (auto it = WindowList.begin(); it != WindowList.end(); ++it) {
		Document *const win = *it;

		if (win->shell_ != shell_ || win == this) {
			continue;
		}

		win->showLineNumbers_ = state;

		for (i = 0; i <= win->textPanes_.size(); i++) {
			text = win->GetPaneByIndex(i);
			//  reqCols should really be cast here, but into what? XmRInt?
			textD_of(text)->setLineNumCols(reqCols);
		}
	}

	// Tell WM that the non-expandable part of the this has changed size
	UpdateWMSizeHints();
}

/*
** Turn on and off the display of the statistics line
*/
void Document::ShowStatsLine(int state) {

	int i;

	/* In continuous wrap mode, text widgets must be told to keep track of
	   the top line number in absolute (non-wrapped) lines, because it can
	   be a costly calculation, and is only needed for displaying line
	   numbers, either in the stats line, or along the left margin */
	for (i = 0; i <= textPanes_.size(); i++) {
		Widget text = GetPaneByIndex(i);
		textD_of(text)->TextDMaintainAbsLineNum(state);
	}
	showStats_ = state;
	showStatistics(state);

	/* i-search line is shell-level, hence other tabbed
	   documents in the this should synch */
	for (Document *win: WindowList) {
		if (win->shell_ != shell_ || win == this)
			continue;
		win->showStats_ = state;
	}
}

/*
** Set autoindent state to one of  NO_AUTO_INDENT, AUTO_INDENT, or SMART_INDENT.
*/
void Document::SetAutoIndent(int state) {
    Q_UNUSED(state);
#if 0 // NOTE(eteran): transitioned
	bool autoIndent  = (state == AUTO_INDENT);
	bool smartIndent = (state == SMART_INDENT);

	if (indentStyle_ == SMART_INDENT && !smartIndent) {
		EndSmartIndent(this);
	} else if (smartIndent && indentStyle_ != SMART_INDENT) {
		BeginSmartIndent(this, true);
	}

	indentStyle_ = state;


	textD_of(textArea_)->setAutoIndent(smartIndent);
	textD_of(textArea_)->setSmartIndent(autoIndent);

	for(Widget tw : textPanes_) {
		textD_of(tw)->setAutoIndent(smartIndent);
		textD_of(tw)->setSmartIndent(autoIndent);
	}

	if (IsTopDocument()) {
		XmToggleButtonSetState(smartIndentItem_, smartIndent, false);
		XmToggleButtonSetState(autoIndentItem_, autoIndent, false);
		XmToggleButtonSetState(autoIndentOffItem_, state == NO_AUTO_INDENT, false);
	}
#endif
}

/*
** Set showMatching state to one of NO_FLASH, FLASH_DELIMIT or FLASH_RANGE.
** Update the menu to reflect the change of state.
*/
void Document::SetShowMatching(int state) {
	showMatchingStyle_ = state;
	if (IsTopDocument()) {
#if 0 // NOTE(eteran): transitioned
		XmToggleButtonSetState(showMatchingOffItem_, state == NO_FLASH, false);
		XmToggleButtonSetState(showMatchingDelimitItem_, state == FLASH_DELIMIT, false);
		XmToggleButtonSetState(showMatchingRangeItem_, state == FLASH_RANGE, false);
#endif
	}
}

/*
** Set the fonts for "window" from a font name, and updates the display.
** Also updates window->fontList_ which is used for statistics line.
**
** Note that this leaks memory and server resources.  In previous NEdit
** versions, fontLists were carefully tracked and freed, but X and Motif
** have some kind of timing problem when widgets are distroyed, such that
** fonts may not be freed immediately after widget destruction with 100%
** safety.  Rather than kludge around this with timerProcs, I have chosen
** to create new fontLists only when the user explicitly changes the font
** (which shouldn't happen much in normal NEdit operation), and skip the
** futile effort of freeing them.
*/
void Document::SetFonts(const char *fontName, const char *italicName, const char *boldName, const char *boldItalicName) {

	XFontStruct *font;
	int oldFontWidth;
	int oldFontHeight;
	int fontWidth;
	int fontHeight;
	int borderWidth;
	int borderHeight;
	bool highlightChanged = false;
	Dimension oldWindowWidth;
	Dimension oldWindowHeight;
	Dimension oldTextWidth;
	Dimension oldTextHeight;
	Dimension textHeight;
	Dimension newWindowWidth;
	Dimension newWindowHeight;

	auto textD = textD_of(textArea_);

	// Check which fonts have changed
	bool primaryChanged = QLatin1String(fontName) != fontName_;

	if (QLatin1String(italicName) != italicFontName_) {
		highlightChanged = true;
	}

	if (QLatin1String(boldName) != boldFontName_) {
		highlightChanged = true;
	}

	if (QLatin1String(boldItalicName) != boldItalicFontName_) {
		highlightChanged = true;
	}

	if (!primaryChanged && !highlightChanged)
		return;

	/* Get information about the current this sizing, to be used to
	   determine the correct this size after the font is changed */
	XtVaGetValues(shell_, XmNwidth, &oldWindowWidth, XmNheight, &oldWindowHeight, nullptr);


	int marginHeight     = textD->getMarginHeight();
#if 0
	int marginWidth      = textD->getMarginWidth();
#endif
	XFontStruct *oldFont = textD->getFont();
	textHeight           = textD->getHeight();

	oldTextWidth = textD->getRect().width + textD->getLineNumWidth();
	oldTextHeight = textHeight - 2 * marginHeight;

	for(Widget tw : textPanes_) {
		textHeight = textD_of(tw)->getHeight();
		oldTextHeight += textHeight - 2 * marginHeight;
	}

	borderWidth   = oldWindowWidth - oldTextWidth;
	borderHeight  = oldWindowHeight - oldTextHeight;
	oldFontWidth  = oldFont->max_bounds.width;
	oldFontHeight = textD->fontAscent() + textD->fontDescent();

	/* Change the fonts in the this data structure.  If the primary font
	   didn't work, use Motif's fallback mechanism by stealing it from the
	   statistics line.  Highlight fonts are allowed to be nullptr, which
	   is interpreted as "use the primary font" */
	if (primaryChanged) {
		fontName_ = QLatin1String(fontName);
		font = XLoadQueryFont(TheDisplay, fontName);
		if(!font)
			XtVaGetValues(statsLine_, XmNfontList, &fontList_, nullptr);
		else
			fontList_ = XmFontListCreate(font, XmSTRING_DEFAULT_CHARSET);
	}

	if (highlightChanged) {
		italicFontName_   = QLatin1String(italicName);
        italicFontStruct_ = XLoadQueryFont(TheDisplay, italicName);

		boldFontName_   = QLatin1String(boldName);
		boldFontStruct_ = XLoadQueryFont(TheDisplay, boldName);

		boldItalicFontName_   = QLatin1String(boldItalicName);
		boldItalicFontStruct_ = XLoadQueryFont(TheDisplay, boldItalicName);
	}

	// Change the primary font in all the widgets
	if (primaryChanged) {
		font = GetDefaultFontStruct(fontList_);

		textD_of(textArea_)->setFont(font);

		for(Widget tw : textPanes_) {
			textD_of(tw)->setFont(font);
		}
	}

	/* Change the highlight fonts, even if they didn't change, because
	   primary font is read through the style table for syntax highlighting */
	if (highlightData_)
		UpdateHighlightStyles(this);

	/* Change the this manager size hints.
	   Note: this has to be done _before_ we set the new sizes. ICCCM2
	   compliant this managers (such as fvwm2) would otherwise resize
	   the this twice: once because of the new sizes requested, and once
	   because of the new size increments, resulting in an overshoot. */
	UpdateWMSizeHints();

	/* Use the information from the old this to re-size the this to a
	   size appropriate for the new font, but only do so if there's only
	   _one_ document in the this, in order to avoid growing-this bug */
	if (TabCount() == 1) {
		fontWidth = GetDefaultFontStruct(fontList_)->max_bounds.width;
		fontHeight = textD->fontAscent() + textD->fontDescent();
		newWindowWidth = (oldTextWidth * fontWidth) / oldFontWidth + borderWidth;
		newWindowHeight = (oldTextHeight * fontHeight) / oldFontHeight + borderHeight;
		XtVaSetValues(shell_, XmNwidth, newWindowWidth, XmNheight, newWindowHeight, nullptr);
	}

	// Change the minimum pane height
	UpdateMinPaneHeights();
}


/*
** Close a document, or an editor window
*/
void Document::CloseWindow() {
#if 0 // NOTE(eteran): transitioned
	int keepWindow, state;
	Document *win;
	Document *topBuf = nullptr;
	Document *nextBuf = nullptr;

	// Free smart indent macro programs
	EndSmartIndent(this);

	/* Clean up macro references to the doomed window.  If a macro is
	   executing, stop it.  If macro is calling this (closing its own
	   window), leave the window alive until the macro completes */
	keepWindow = !MacroWindowCloseActions(this);

	// Kill shell sub-process and free related memory
	AbortShellCommand(this);

	// Unload the default tips files for this language mode if necessary
	UnloadLanguageModeTipsFile(this);

	/* If a window is closed while it is on the multi-file replace dialog
	   list of any other window (or even the same one), we must update those
	   lists or we end up with dangling references. Normally, there can
	   be only one of those dialogs at the same time (application modal),
	   but LessTif doesn't even (always) honor application modalness, so
	   there can be more than one dialog. */
	RemoveFromMultiReplaceDialog(this);

	// Destroy the file closed property for this file
	DeleteFileClosedProperty(this);

	/* Remove any possibly pending callback which might fire after the
	   widget is gone. */
	cancelTimeOut(&flashTimeoutID_);
	cancelTimeOut(&markTimeoutID_);

	/* if this is the last window, or must be kept alive temporarily because
	   it's running the macro calling us, don't close it, make it Untitled */
	if (keepWindow || (WindowList.size() == 1 && this == WindowList.front())) {
		filename_ = QLatin1String("");

		QString name = UniqueUntitledName();
		lockReasons_.clear();

		fileMode_     = 0;
		fileUid_      = 0;
		fileGid_      = 0;
		filename_     = name;
		path_         = QLatin1String("");
		ignoreModify_ = true;

		buffer_->BufSetAllEx("");

		ignoreModify_ = false;
		nMarks_       = 0;
		filenameSet_  = false;
		fileMissing_  = true;
		fileChanged_  = false;
		fileFormat_   = UNIX_FILE_FORMAT;
		lastModTime_  = 0;
		device_       = 0;
		inode_        = 0;

		StopHighlighting(this);
		EndSmartIndent(this);
		UpdateWindowTitle();
		UpdateWindowReadOnly();
#if 0 // NOTE(eteran): transitioned
		XtSetSensitive(closeItem_, false);
		XtSetSensitive(readOnlyItem_, true);
		XmToggleButtonSetState(readOnlyItem_, false, false);
#endif
		ClearUndoList();
		ClearRedoList();
		XmTextSetStringEx(statsLine_, ""); // resets scroll pos of stats line from long file names
		UpdateStatsLine();
		DetermineLanguageMode(this, true);
		RefreshTabState();
		updateLineNumDisp();
		return;
	}

	// Free syntax highlighting patterns, if any. w/o redisplaying
	FreeHighlightingData(this);

	/* remove the buffer modification callbacks so the buffer will be
	   deallocated when the last text widget is destroyed */
	buffer_->BufRemoveModifyCB(modifiedCB, this);
	buffer_->BufRemoveModifyCB(SyntaxHighlightModifyCB, this);


	// free the undo and redo lists
	ClearUndoList();
	ClearRedoList();

	// close the document/window
	if (TabCount() > 1) {
		if (MacroRunWindow() && MacroRunWindow() != this && MacroRunWindow()->shell_ == shell_) {
			nextBuf = MacroRunWindow();
			if(nextBuf) {
				nextBuf->RaiseDocument();
			}
		} else if (IsTopDocument()) {
			// need to find a successor before closing a top document
			nextBuf = getNextTabWindow(1, 0, 0);
			if(nextBuf) {
				nextBuf->RaiseDocument();
			}
		} else {
			topBuf = GetTopDocument(shell_);
		}
	}

	// remove the window from the global window list, update window menus
	removeFromWindowList();
	InvalidateWindowMenus();
	CheckCloseDim(); // Close of window running a macro may have been disabled.

	// remove the tab of the closing document from tab bar
	XtDestroyWidget(tab_);

	// refresh tab bar after closing a document
	if (nextBuf) {
		nextBuf->ShowWindowTabBar();
		nextBuf->updateLineNumDisp();
	} else if (topBuf) {
		topBuf->ShowWindowTabBar();
		topBuf->updateLineNumDisp();
	}

	// dim/undim Detach_Tab menu items
	win = nextBuf ? nextBuf : topBuf;
	if (win) {
		state = win->TabCount() > 1;
		XtSetSensitive(win->detachDocumentItem_, state);
		XtSetSensitive(win->contextDetachDocumentItem_, state);
	}

	// dim/undim Attach_Tab menu items
	state = WindowList.front()->TabCount() < WindowCount();

#if 0 // NOTE(eteran): transitioned
	for(Document *win: WindowList) {
		if (win->IsTopDocument()) {
			XtSetSensitive(win->moveDocumentItem_, state);
			XtSetSensitive(win->contextMoveDocumentItem_, state);
		}
	}
#endif

	// free background menu cache for document
	FreeUserBGMenuCache(&userBGMenuCache_);

	// destroy the document's pane, or the window
	if (nextBuf || topBuf) {
		deleteDocument();
	} else {
		// free user menu cache for window
		FreeUserMenuCache(userMenuCache_);

		// remove and deallocate all of the widgets associated with window
		CloseAllPopupsFor(shell_);
		XtDestroyWidget(shell_);
	}


#if 1
	// TODO(eteran): why did I need to add this?!?
	//               looking above, RaiseDocument (which triggers the update)
	//               is called before removeFromWindowList, so I'm not 100% sure
	//               it ever worked without this...
	if(auto dialog = getDialogReplace()) {
		dialog->UpdateReplaceActionButtons();
	}
#endif

	// deallocate the window data structure
	delete this;
#endif
}

/*
**
*/
void Document::deleteDocument() {

	Q_ASSERT(this);

	if (!this) {
		return;
	}

	XtDestroyWidget(splitPane_);
}

/*
** Calculate the dimension of the text area, in terms of rows & cols,
** as if there's only one single text pane in the window.
*/
void Document::getTextPaneDimension(int *nRows, int *nCols) {

	Widget hScrollBar;
	Dimension hScrollBarHeight;
	Dimension paneHeight;
	int totalHeight;
	int fontHeight;

	auto textD = textD_of(textArea_);

	// width is the same for panes
	*nCols = textD->getColumns();

	// we have to work out the height, as the text area may have been split
	int marginHeight = textD->getMarginHeight();
#if 0
	int marginWidth  = textD->getMarginWidth();
#endif
	XtVaGetValues(textArea_, textNhScrollBar, &hScrollBar, nullptr);
	XtVaGetValues(hScrollBar, XmNheight, &hScrollBarHeight, nullptr);
	XtVaGetValues(splitPane_, XmNheight, &paneHeight, nullptr);

	totalHeight = paneHeight - 2 * marginHeight - hScrollBarHeight;
	fontHeight = textD->fontAscent() + textD->fontDescent();
	*nRows = totalHeight / fontHeight;
}

/*
** Add another independently scrollable pane to the current document,
** splitting the pane which currently has keyboard focus.
*/
void Document::SplitPane() {
	short paneHeights[MAX_PANES + 1];
	int insertPositions[MAX_PANES + 1];
	int topLines[MAX_PANES + 1];
	int horizOffsets[MAX_PANES + 1];
	int i;
	int focusPane;
	int totalHeight = 0;
	Widget text = nullptr;
	TextDisplay *newTextD;

	// Don't create new panes if we're already at the limit
	if (textPanes_.size() >= MAX_PANES)
		return;

	/* Record the current heights, scroll positions, and insert positions
	   of the existing panes, keyboard focus */
	focusPane = 0;
	for (i = 0; i <= textPanes_.size(); i++) {
		Widget text = GetPaneByIndex(i);

		auto textD = textD_of(text);
		insertPositions[i] = textD->TextGetCursorPos();
		XtVaGetValues(containingPane(text), XmNheight, &paneHeights[i], nullptr);
		totalHeight += paneHeights[i];
		textD->TextDGetScroll(&topLines[i], &horizOffsets[i]);
		if (text == lastFocus_)
			focusPane = i;
	}

	// Unmanage & remanage the panedWindow so it recalculates pane heights
	XtUnmanageChild(splitPane_);

	/* Create a text widget to add to the pane and set its buffer and
	   highlight data to be the same as the other panes in the document */

	int emTabDist      = textD_of(textArea_)->getEmulateTabs();
	QString delimiters = textD_of(textArea_)->getWordDelimiters();
	int wrapMargin     = textD_of(textArea_)->getWrapMargin();
	int lineNumCols    = textD_of(textArea_)->getLineNumCols();

    text = createTextArea(splitPane_, this, 1, 1, emTabDist, delimiters.toLatin1().data(), wrapMargin, lineNumCols);
	auto textD = textD_of(text);

	textD->TextSetBuffer(buffer_);
	if (highlightData_)
		AttachHighlightToWidget(text, this);

	if (backlightChars_) {
		textD->setBacklightCharTypes(backlightCharTypes_);
	}

	XtManageChild(text);
	textPanes_.push_back(text);

	// Fix up the colors
	textD = textD_of(textArea_);
	newTextD = textD_of(text);
	newTextD->setForegroundPixel(textD->foregroundPixel());
	newTextD->setBackgroundPixel(textD->backgroundPixel());

	newTextD->TextDSetColors(
		textD->foregroundPixel(),
		textD->backgroundPixel(),
		textD->getSelectFGPixel(),
		textD->getSelectBGPixel(),
		textD->getHighlightFGPixel(),
		textD->getHighlightBGPixel(),
		textD->getLineNumFGPixel(),
		textD->getCursorFGPixel());

	// Set the minimum pane height in the new pane
	UpdateMinPaneHeights();

	// adjust the heights, scroll positions, etc., to split the focus pane
	for (i = textPanes_.size(); i > focusPane; i--) {
		insertPositions[i] = insertPositions[i - 1];
		paneHeights[i]     = paneHeights[i - 1];
		topLines[i]        = topLines[i - 1];
		horizOffsets[i]    = horizOffsets[i - 1];
	}
	paneHeights[focusPane]     = paneHeights[focusPane] / 2;
	paneHeights[focusPane + 1] = paneHeights[focusPane];

	for (i = 0; i <= textPanes_.size(); i++) {
		Widget text = GetPaneByIndex(i);
		setPaneDesiredHeight(containingPane(text), paneHeights[i]);
	}

	// Re-manage panedWindow to recalculate pane heights & reset selection
	if (IsTopDocument())
		XtManageChild(splitPane_);

	// Reset all of the heights, scroll positions, etc.
	for (i = 0; i <= textPanes_.size(); i++) {
		Widget text = GetPaneByIndex(i);
		auto textD = textD_of(text);

		textD->TextSetCursorPos(insertPositions[i]);
		textD->TextDSetScroll(topLines[i], horizOffsets[i]);
		setPaneDesiredHeight(containingPane(text), totalHeight / (textPanes_.size() + 1));
	}
	XmProcessTraversal(lastFocus_, XmTRAVERSE_CURRENT);

	/* Update the this manager size hints after the sizes of the panes have
	   been set (the widget heights are not yet readable here, but they will
	   be by the time the event loop gets around to running this timer proc) */
	XtAppAddTimeOut(XtWidgetToApplicationContext(shell_), 0, wmSizeUpdateProc, this);
}

/*
** Save the position and size of a window as an X standard geometry string.
** A string of at least MAX_GEOMETRY_STRING_LEN characters should be
** provided in the argument "geomString" to receive the result.
*/
void Document::getGeometryString(char *geomString) {
	int x, y, fontWidth, fontHeight, baseWidth, baseHeight;
	unsigned int width, height, dummyW, dummyH, bw, depth, nChild;
	Window parent, root, *child, w = XtWindow(shell_);
	Display *dpy = XtDisplay(shell_);

	// Find the width and height from the this of the shell
	XGetGeometry(dpy, w, &root, &x, &y, &width, &height, &bw, &depth);

	/* Find the top left corner (x and y) of the this decorations.  (This
	   is what's required in the geometry string to restore the this to it's
	   original position, since the this manager re-parents the this to
	   add it's title bar and menus, and moves the requested this down and
	   to the left.)  The position is found by traversing the this hier-
	   archy back to the this to the last parent before the root this */
	for (;;) {
		XQueryTree(dpy, w, &root, &parent, &child, &nChild);
		XFree((char *)child);
		if (parent == root)
			break;
		w = parent;
	}
	XGetGeometry(dpy, w, &root, &x, &y, &dummyW, &dummyH, &bw, &depth);

	/* Use this manager size hints (set by UpdateWMSizeHints) to
	   translate the width and height into characters, as opposed to pixels */
	XtVaGetValues(shell_, XmNwidthInc, &fontWidth, XmNheightInc, &fontHeight, XmNbaseWidth, &baseWidth, XmNbaseHeight, &baseHeight, nullptr);
	width = (width - baseWidth) / fontWidth;
	height = (height - baseHeight) / fontHeight;

	// Write the string
	CreateGeometryString(geomString, x, y, width, height, XValue | YValue | WidthValue | HeightValue);
}

/*
** Raise a tabbed document within its shell window.
**
** NB: use RaiseDocumentWindow() to raise the doc and
**     its shell window.
*/
void Document::RaiseDocument() {
	Document *win;

	if (WindowList.empty()) {
		return;
	}

	Document *lastwin = MarkActiveDocument();
	if (lastwin != this && lastwin->IsValidWindow()) {
		lastwin->MarkLastDocument();
	}

	// document already on top?
	XtVaGetValues(mainWin_, XmNuserData, &win, nullptr);
	if (win == this) {
		return;
	}

	// set the document as top document
	XtVaSetValues(mainWin_, XmNuserData, this, nullptr);

	// show the new top document
	XtVaSetValues(mainWin_, XmNworkWindow, splitPane_, nullptr);
	XtManageChild(splitPane_);
	XRaiseWindow(TheDisplay, XtWindow(splitPane_));

	/* Turn on syntax highlight that might have been deferred.
	   NB: this must be done after setting the document as
	       XmNworkWindow and managed, else the parent shell
	   this may shrink on some this-managers such as
	   metacity, due to changes made in UpdateWMSizeHints().*/
	if (highlightSyntax_ && highlightData_ == nullptr)
		StartHighlighting(this, false);

	// put away the bg menu tearoffs of last active document
	hideTearOffs(win->bgMenuPane_);

	// restore the bg menu tearoffs of active document
	redisplayTearOffs(bgMenuPane_);

	// set tab as active
	XmLFolderSetActiveTab(tabBar_, getTabPosition(tab_), false);

	/* set keyboard focus. Must be done before unmanaging previous
	   top document, else lastFocus will be reset to textArea */
	XmProcessTraversal(lastFocus_, XmTRAVERSE_CURRENT);

	/* we only manage the top document, else the next time a document
	   is raised again, it's textpane might not resize properly.
	   Also, somehow (bug?) XtUnmanageChild() doesn't hide the
	   splitPane, which obscure lower part of the statsform when
	   we toggle its components, so we need to put the document at
	   the back */
	XLowerWindow(TheDisplay, XtWindow(win->splitPane_));
	XtUnmanageChild(win->splitPane_);
	win->RefreshTabState();

	/* now refresh this state/info. RefreshWindowStates()
	   has a lot of work to do, so we update the screen first so
	   the document appears to switch swiftly. */
	XmUpdateDisplay(splitPane_);
	RefreshWindowStates();
	RefreshTabState();

	// put away the bg menu tearoffs of last active document
	hideTearOffs(win->bgMenuPane_);

	// restore the bg menu tearoffs of active document
	redisplayTearOffs(bgMenuPane_);

	/* Make sure that the "In Selection" button tracks the presence of a
	   selection and that the this inherits the proper search scope. */
	if(auto dialog = getDialogReplace()) {
		dialog->UpdateReplaceActionButtons();
	}

	UpdateWMSizeHints();
}

/*
** clone a document's states and settings into the other.
*/
void Document::cloneDocument(Document *window) {

	window->path_     = path_;
	window->filename_ = filename_;

	window->ShowLineNumbers(showLineNumbers_);

	window->ignoreModify_ = true;

	// copy the text buffer
	auto orgDocument = buffer_->BufAsStringEx();
	window->buffer_->BufSetAllEx(orgDocument);

	// copy the tab preferences (here!)
	window->buffer_->BufSetTabDistance(buffer_->tabDist_);
	window->buffer_->useTabs_ = buffer_->useTabs_;

	int emTabDist = textD_of(textArea_)->getEmulateTabs();

	window->SetEmTabDist(emTabDist);

	window->ignoreModify_ = false;


	QByteArray fontStr   = fontName_.toLatin1();
	QByteArray iFontStr  = italicFontName_.toLatin1();
	QByteArray bFontStr  = boldFontName_.toLatin1();
	QByteArray biFontStr = boldItalicFontName_.toLatin1();

	// transfer text fonts
	char *params[4];
	params[0] = fontStr.data();
	params[1] = iFontStr.data();
	params[2] = bFontStr.data();
	params[3] = biFontStr.data();
	XtCallActionProc(window->textArea_, "set_fonts", nullptr, params, 4);

	window->SetBacklightChars(backlightCharTypes_.toLatin1().data());

	/* Clone rangeset info.

	   FIXME:
	   Cloning of rangesets must be done before syntax highlighting,
	   else the rangesets do not be highlighted (colored) properly
	   if syntax highlighting is on.
	*/
	if(buffer_->rangesetTable_) {
		window->buffer_->rangesetTable_ = new RangesetTable(window->buffer_, *buffer_->rangesetTable_);
	} else {
		window->buffer_->rangesetTable_ = nullptr;
	}

	// Syntax highlighting
	window->languageMode_ = languageMode_;
	window->highlightSyntax_ = highlightSyntax_;
	if (window->highlightSyntax_) {
		StartHighlighting(window, false);
	}

	// TODO(eteran): clone the find dialogs as well

	// copy states of original document
	window->filenameSet_            = filenameSet_;
	window->fileFormat_             = fileFormat_;
	window->lastModTime_            = lastModTime_;
	window->fileChanged_            = fileChanged_;
	window->fileMissing_            = fileMissing_;
	window->lockReasons_            = lockReasons_;
	window->autoSaveCharCount_      = autoSaveCharCount_;
	window->autoSaveOpCount_        = autoSaveOpCount_;
	window->undoMemUsed_            = undoMemUsed_;
	window->lockReasons_            = lockReasons_;
	window->autoSave_               = autoSave_;
	window->saveOldVersion_         = saveOldVersion_;
	window->wrapMode_               = wrapMode_;
	window->SetOverstrike(overstrike_);
	window->showMatchingStyle_      = showMatchingStyle_;
	window->matchSyntaxBased_       = matchSyntaxBased_;
#if 0
	window->showStats_              = showStats_;
	window->showISearchLine_        = showISearchLine_;
	window->showLineNumbers_        = showLineNumbers_;
	window->modeMessageDisplayed_   = modeMessageDisplayed_;
	window->ignoreModify_           = ignoreModify_;
	window->windowMenuValid_        = windowMenuValid_;
	window->flashTimeoutID_         = flashTimeoutID_;
	window->wasSelected_            = wasSelected_;
	window->fontName_               = fontName_;
	window->italicFontName_         = italicFontName_;
	window->boldFontName_           = boldFontName_;
	window->boldItalicFontName_     = boldItalicFontName_;
	window->fontList_               = fontList_;
	window->italicFontStruct_       = italicFontStruct_;
	window->boldFontStruct_         = boldFontStruct_;
	window->boldItalicFontStruct_   = boldItalicFontStruct_;
	window->markTimeoutID_          = markTimeoutID_;
	window->highlightData_          = highlightData_;
	window->shellCmdData_           = shellCmdData_;
	window->macroCmdData_           = macroCmdData_;
	window->smartIndentData_        = smartIndentData_;
#endif
	window->iSearchHistIndex_       = iSearchHistIndex_;
	window->iSearchStartPos_        = iSearchStartPos_;
	window->iSearchLastRegexCase_   = iSearchLastRegexCase_;
	window->iSearchLastLiteralCase_ = iSearchLastLiteralCase_;

	window->device_ = device_;
	window->inode_ = inode_;
	window->fileClosedAtom_ = fileClosedAtom_;
	fileClosedAtom_ = None;

	// copy the text/split panes settings, cursor pos & selection
	cloneTextPanes(window, this);

	// copy undo & redo list
	window->undo_ = cloneUndoItems(undo_);
	window->redo_ = cloneUndoItems(redo_);

	// copy bookmarks
	window->nMarks_ = nMarks_;
	memcpy(&window->markTable_, &markTable_, sizeof(Bookmark) * window->nMarks_);

	// kick start the auto-indent engine
	window->indentStyle_ = NO_AUTO_INDENT;
	window->SetAutoIndent(indentStyle_);

	// synchronize window state to this document
	window->RefreshWindowStates();
}

/*
** Create a new editor window
*/
Document::Document(const QString &name, char *geometry, bool iconic) {

#if 0
	XmString s1;
	XmFontList statsFontList;

	static Pixmap closeTabPixmap = 0;
#endif
	dialogFind_            = nullptr;
	dialogReplace_         = nullptr;
	dialogColors_          = nullptr;
	dialogFonts_           = nullptr;

	multiFileReplSelected_ = false;
	multiFileBusy_         = false;
	writableWindows_       = nullptr;
	nWritableWindows_      = 0;
	fileChanged_           = false;
	fileMode_              = 0;
	fileUid_               = 0;
	fileGid_               = 0;
	filenameSet_           = false;
	fileFormat_            = UNIX_FILE_FORMAT;
	lastModTime_           = 0;
	fileMissing_           = true;
	filename_              = name;
	undo_                  = std::list<UndoInfo *>();
	redo_                  = std::list<UndoInfo *>();
	textPanes_.clear();
	autoSaveCharCount_     = 0;
	autoSaveOpCount_       = 0;
	undoMemUsed_           = 0;

	lockReasons_.clear();

	indentStyle_        = GetPrefAutoIndent(PLAIN_LANGUAGE_MODE);
	autoSave_           = GetPrefAutoSave();
	saveOldVersion_     = GetPrefSaveOldVersion();
	wrapMode_           = GetPrefWrap(PLAIN_LANGUAGE_MODE);
	overstrike_         = false;
	showMatchingStyle_  = GetPrefShowMatching();
	matchSyntaxBased_   = GetPrefMatchSyntaxBased();
	showStats_          = GetPrefStatsLine();
	showISearchLine_    = GetPrefISearchLine();
	showLineNumbers_    = GetPrefLineNums();
	highlightSyntax_    = GetPrefHighlightSyntax();
	backlightCharTypes_ = QString();
	backlightChars_     = GetPrefBacklightChars();

	if (backlightChars_) {
		const char *cTypes = GetPrefBacklightCharTypes();
		if (cTypes && backlightChars_) {
			backlightCharTypes_ = QLatin1String(cTypes);
		}
	}

	modeMessageDisplayed_   = false;
	modeMessage_            = QString();
	ignoreModify_           = false;
	windowMenuValid_        = false;
	flashTimeoutID_         = 0;
	fileClosedAtom_         = None;
	wasSelected_            = false;

	fontName_               = GetPrefFontName();
	italicFontName_         = GetPrefItalicFontName();
	boldFontName_           = GetPrefBoldFontName();
	boldItalicFontName_     = GetPrefBoldItalicFontName();

	fontList_               = GetPrefFontList();
	italicFontStruct_       = GetPrefItalicFont();
	boldFontStruct_         = GetPrefBoldFont();
	boldItalicFontStruct_   = GetPrefBoldItalicFont();
	nMarks_                 = 0;
	markTimeoutID_          = 0;
	highlightData_          = nullptr;
	shellCmdData_           = nullptr;
	macroCmdData_           = nullptr;
	smartIndentData_        = nullptr;
	languageMode_           = PLAIN_LANGUAGE_MODE;
	iSearchHistIndex_       = 0;
	iSearchStartPos_        = -1;
	iSearchLastRegexCase_   = true;
	iSearchLastLiteralCase_ = false;
	tab_                    = nullptr;
	device_                 = 0;
	inode_                  = 0;


	char newGeometry[MAX_GEOM_STRING_LEN];
	unsigned int rows;
	unsigned int cols;
	int bitmask;
	int x = 0;
	int y = 0;
	Arg al[20];
	int ac;

	/* If window geometry was specified, split it apart into a window position
	   component and a window size component.  Create a new geometry string
	   containing the position component only.  Rows and cols are stripped off
	   because we can't easily calculate the size in pixels from them until the
	   whole window is put together.  Note that the preference resource is only
	   for clueless users who decide to specify the standard X geometry
	   application resource, which is pretty useless because width and height
	   are the same as the rows and cols preferences, and specifying a window
	   location will force all the windows to pile on top of one another */
	if (geometry == nullptr || geometry[0] == '\0') {
		geometry = GetPrefGeometry();
	}

	if (geometry == nullptr || geometry[0] == '\0') {
		rows = GetPrefRows();
		cols = GetPrefCols();
		newGeometry[0] = '\0';
	} else {
		bitmask = XParseGeometry(geometry, &x, &y, &cols, &rows);
		if (bitmask == 0)
			fprintf(stderr, "Bad window geometry specified: %s\n", geometry);
		else {
			if (!(bitmask & WidthValue))
				cols = GetPrefCols();
			if (!(bitmask & HeightValue))
				rows = GetPrefRows();
		}
		CreateGeometryString(newGeometry, x, y, 0, 0, bitmask & ~(WidthValue | HeightValue));
	}

	// Create a new toplevel shell to hold the window
	QByteArray nameStr = name.toLatin1();
	ac = 0;
	XtSetArg(al[ac], XmNtitle, nameStr.data());
	ac++;
	XtSetArg(al[ac], XmNdeleteResponse, XmDO_NOTHING);
	ac++;
	XtSetArg(al[ac], XmNiconName, nameStr.data());
	ac++;
	XtSetArg(al[ac], XmNgeometry, newGeometry[0] == '\0' ? nullptr : newGeometry);
	ac++;
	XtSetArg(al[ac], XmNinitialState, iconic ? IconicState : NormalState);
	ac++;

	if (newGeometry[0] == '\0') {
		XtSetArg(al[ac], XtNx, XT_IGNORE_PPOSITION);
		ac++;
		XtSetArg(al[ac], XtNy, XT_IGNORE_PPOSITION);
		ac++;
	}

	shell_ = CreateWidget(TheAppShell, "textShell", topLevelShellWidgetClass, al, ac);

	addWindowIcon(shell_);

	/* Create a MainWindow to manage the menubar and text area, set the
	   userData resource to be used by WidgetToWindow to recover the
	   window pointer from the widget id of any of the window's widgets */
	XtSetArg(al[ac], XmNuserData, this);
	ac++;
	mainWin_ = XmCreateMainWindow(shell_, (String) "main", al, ac);
	XtManageChild(mainWin_);

	// The statsAreaForm holds the stats line and the I-Search line.
	Widget statsAreaForm = XtVaCreateWidget("statsAreaForm", xmFormWidgetClass, mainWin_, XmNmarginWidth, STAT_SHADOW_THICKNESS, XmNmarginHeight, STAT_SHADOW_THICKNESS,
	                                 // XmNautoUnmanage, false,
	                                 nullptr);

	/* NOTE: due to a bug in openmotif 2.1.30, NEdit used to crash when
	   the i-search bar was active, and the i-search text widget was focussed,
	   and the window's width was resized to nearly zero.
	   In theory, it is possible to avoid this by imposing a minimum
	   width constraint on the nedit windows, but that width would have to
	   be at least 30 characters, which is probably unacceptable.
	   Amazingly, adding a top offset of 1 pixel to the toggle buttons of
	   the i-search bar, while keeping the the top offset of the text widget
	   to 0 seems to avoid avoid the crash. */

	iSearchForm_ = XtVaCreateWidget("iSearchForm", xmFormWidgetClass, statsAreaForm, XmNshadowThickness, 0, XmNleftAttachment, XmATTACH_FORM, XmNleftOffset, STAT_SHADOW_THICKNESS, XmNtopAttachment, XmATTACH_FORM, XmNtopOffset,
	                                       STAT_SHADOW_THICKNESS, XmNrightAttachment, XmATTACH_FORM, XmNrightOffset, STAT_SHADOW_THICKNESS, XmNbottomOffset, STAT_SHADOW_THICKNESS, nullptr);
	if (showISearchLine_) {
		XtManageChild(iSearchForm_);
	}


#if 0 // NOTE(eteran): transitioned
	/* Disable keyboard traversal of the find, clear and toggle buttons.  We
	   were doing this previously by forcing the keyboard focus back to the
	   text widget whenever a toggle changed.  That causes an ugly focus flash
	   on screen.  It's better just not to go there in the first place.
	   Plus, if the user really wants traversal, it's an X resource so it
	   can be enabled without too much pain and suffering. */

	if (isrcFind == 0) {
		isrcFind = createBitmapWithDepth(iSearchForm_, (char *)isrcFind_bits, isrcFind_width, isrcFind_height);
	}

	s1 = XmStringCreateSimpleEx("Find");
	iSearchFindButton_ = XtVaCreateManagedWidget("iSearchFindButton", xmPushButtonWidgetClass, iSearchForm_, XmNlabelString, s1, XmNlabelType, XmPIXMAP, XmNlabelPixmap, isrcFind,
	                                                    XmNtraversalOn, false, XmNmarginHeight, 1, XmNmarginWidth, 1, XmNleftAttachment, XmATTACH_FORM,
	                                                    // XmNleftOffset, 3,
	                                                    XmNleftOffset, 0, XmNtopAttachment, XmATTACH_FORM, XmNtopOffset, 1, XmNbottomAttachment, XmATTACH_FORM, XmNbottomOffset, 1, nullptr);
	XmStringFree(s1);

	s1 = XmStringCreateSimpleEx("Case");
	iSearchCaseToggle_ = XtVaCreateManagedWidget("iSearchCaseToggle", xmToggleButtonWidgetClass, iSearchForm_, XmNlabelString, s1, XmNset,
	                                                    GetPrefSearch() == SEARCH_CASE_SENSE || GetPrefSearch() == SEARCH_REGEX || GetPrefSearch() == SEARCH_CASE_SENSE_WORD, XmNtopAttachment, XmATTACH_FORM, XmNbottomAttachment,
	                                                    XmATTACH_FORM, XmNtopOffset, 1, // see openmotif note above
	                                                    XmNrightAttachment, XmATTACH_FORM, XmNmarginHeight, 0, XmNtraversalOn, false, nullptr);
	XmStringFree(s1);

	s1 = XmStringCreateSimpleEx("RegExp");
	iSearchRegexToggle_ = XtVaCreateManagedWidget("iSearchREToggle", xmToggleButtonWidgetClass, iSearchForm_, XmNlabelString, s1, XmNset, GetPrefSearch() == SEARCH_REGEX_NOCASE || GetPrefSearch() == SEARCH_REGEX,
	                            XmNtopAttachment, XmATTACH_FORM, XmNbottomAttachment, XmATTACH_FORM, XmNtopOffset, 1, // see openmotif note above
	                            XmNrightAttachment, XmATTACH_WIDGET, XmNrightWidget, iSearchCaseToggle_, XmNmarginHeight, 0, XmNtraversalOn, false, nullptr);
	XmStringFree(s1);

	s1 = XmStringCreateSimpleEx("Rev");
	iSearchRevToggle_ = XtVaCreateManagedWidget("iSearchRevToggle", xmToggleButtonWidgetClass, iSearchForm_, XmNlabelString, s1, XmNset, false, XmNtopAttachment, XmATTACH_FORM,
	                                                   XmNbottomAttachment, XmATTACH_FORM, XmNtopOffset, 1, // see openmotif note above
	                                                   XmNrightAttachment, XmATTACH_WIDGET, XmNrightWidget, iSearchRegexToggle_, XmNmarginHeight, 0, XmNtraversalOn, false, nullptr);
	XmStringFree(s1);

	if (isrcClear == 0) {
		isrcClear = createBitmapWithDepth(iSearchForm_, (char *)isrcClear_bits, isrcClear_width, isrcClear_height);
	}

	s1 = XmStringCreateSimpleEx("<x");
	iSearchClearButton_ = XtVaCreateManagedWidget("iSearchClearButton", xmPushButtonWidgetClass, iSearchForm_, XmNlabelString, s1, XmNlabelType, XmPIXMAP, XmNlabelPixmap, isrcClear,
	                                                     XmNtraversalOn, false, XmNmarginHeight, 1, XmNmarginWidth, 1, XmNrightAttachment, XmATTACH_WIDGET, XmNrightWidget, iSearchRevToggle_, XmNrightOffset, 2, XmNtopAttachment,
	                                                     XmATTACH_FORM, XmNtopOffset, 1, XmNbottomAttachment, XmATTACH_FORM, XmNbottomOffset, 1, nullptr);
	XmStringFree(s1);

	iSearchText_ = XtVaCreateManagedWidget("iSearchText", xmTextWidgetClass, iSearchForm_, XmNmarginHeight, 1, XmNnavigationType, XmEXCLUSIVE_TAB_GROUP, XmNleftAttachment, XmATTACH_WIDGET, XmNleftWidget,
	                                              iSearchFindButton_, XmNrightAttachment, XmATTACH_WIDGET, XmNrightWidget, iSearchClearButton_,
	                                              // XmNrightOffset, 5,
	                                              XmNtopAttachment, XmATTACH_FORM, XmNtopOffset, 0, // see openmotif note above
	                                              XmNbottomAttachment, XmATTACH_FORM, XmNbottomOffset, 0, nullptr);
	RemapDeleteKey(iSearchText_);

	SetISearchTextCallbacks(this);

	// create the a form to house the tab bar and close-tab button
	Widget tabForm = XtVaCreateWidget("tabForm", xmFormWidgetClass, statsAreaForm, XmNmarginHeight, 0, XmNmarginWidth, 0, XmNspacing, 0, XmNresizable, false, XmNleftAttachment, XmATTACH_FORM, XmNrightAttachment, XmATTACH_FORM, XmNshadowThickness, 0, nullptr);

	// button to close top document
	if (closeTabPixmap == 0) {
		closeTabPixmap = createBitmapWithDepth(tabForm, (char *)close_bits, close_width, close_height);
	}
	Widget closeTabBtn = XtVaCreateManagedWidget("closeTabBtn", xmPushButtonWidgetClass, tabForm, XmNmarginHeight, 0, XmNmarginWidth, 0, XmNhighlightThickness, 0, XmNlabelType, XmPIXMAP, XmNlabelPixmap, closeTabPixmap, XmNshadowThickness, 1, XmNtraversalOn, false, XmNrightAttachment, XmATTACH_FORM, XmNrightOffset, 3, XmNbottomAttachment, XmATTACH_FORM, XmNbottomOffset, 3, nullptr);
	XtAddCallback(closeTabBtn, XmNactivateCallback, closeTabCB, mainWin_);

	// create the tab bar
	tabBar_ = XtVaCreateManagedWidget("tabBar", xmlFolderWidgetClass, tabForm, XmNresizePolicy, XmRESIZE_PACK, XmNleftAttachment, XmATTACH_FORM, XmNleftOffset, 0, XmNrightAttachment, XmATTACH_WIDGET, XmNrightWidget, closeTabBtn, XmNrightOffset, 5, XmNbottomAttachment, XmATTACH_FORM, XmNbottomOffset, 0, XmNtopAttachment, XmATTACH_FORM, nullptr);

	tabMenuPane_ = CreateTabContextMenu(tabBar_, this);
	AddTabContextMenuAction(tabBar_);

	/* create an unmanaged composite widget to get the folder
	   widget to hide the 3D shadow for the manager area.
	   Note: this works only on the patched XmLFolder widget */
	Widget form = XtVaCreateWidget("form", xmFormWidgetClass, tabBar_, XmNheight, 1, XmNresizable, false, nullptr);

	(void)form;

	XtAddCallback(tabBar_, XmNactivateCallback, raiseTabCB, nullptr);

	tab_ = addTab(tabBar_, name);

	// A form to hold the stats line text and line/col widgets
	statsLineForm_ = XtVaCreateWidget("statsLineForm", xmFormWidgetClass, statsAreaForm, XmNshadowThickness, 0, XmNtopAttachment, showISearchLine_ ? XmATTACH_WIDGET : XmATTACH_FORM, XmNtopWidget, iSearchForm_,
	                                         XmNrightAttachment, XmATTACH_FORM, XmNleftAttachment, XmATTACH_FORM, XmNbottomAttachment, XmATTACH_FORM, XmNresizable, false, //
	                                         nullptr);

	// A separate display of the line/column number
	s1 = XmStringCreateSimpleEx("L: ---  C: ---");
	statsLineColNo_ = XtVaCreateManagedWidget("statsLineColNo", xmLabelWidgetClass, statsLineForm_, XmNlabelString, s1, XmNshadowThickness, 0, XmNmarginHeight, 2, XmNtraversalOn,
	                                                 false, XmNtopAttachment, XmATTACH_FORM, XmNrightAttachment, XmATTACH_FORM, XmNbottomAttachment, XmATTACH_FORM, //
	                                                 nullptr);
	XmStringFree(s1);

	/* Create file statistics display area.  Using a text widget rather than
	   a label solves a layout problem with the main window, which messes up
	   if the label is too long (we would need a resize callback to control
	   the length when the window changed size), and allows users to select
	   file names and line numbers.  Colors are copied from parent
	   widget, because many users and some system defaults color text
	   backgrounds differently from other widgets. */
	Pixel bgpix;
	Pixel fgpix;
	XtVaGetValues(statsLineForm_, XmNbackground, &bgpix, nullptr);
	XtVaGetValues(statsLineForm_, XmNforeground, &fgpix, nullptr);

	statsLine_ = XtVaCreateManagedWidget("statsLine", xmTextWidgetClass, statsLineForm_, XmNbackground, bgpix, XmNforeground, fgpix, XmNshadowThickness, 0, XmNhighlightColor, bgpix, XmNhighlightThickness,
	                                0,                  /* must be zero, for OM (2.1.30) to aligns tatsLineColNo & statsLine */
	                                XmNmarginHeight, 1, /* == statsLineColNo.marginHeight - 1, to align with statsLineColNo */
	                                XmNscrollHorizontal, false, XmNeditMode, XmSINGLE_LINE_EDIT, XmNeditable, false, XmNtraversalOn, false, XmNcursorPositionVisible, false, XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,                //
	                                XmNtopWidget, statsLineColNo_, XmNleftAttachment, XmATTACH_FORM, XmNrightAttachment, XmATTACH_WIDGET, XmNrightWidget, statsLineColNo_, XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET, //
	                                XmNbottomWidget, statsLineColNo_, XmNrightOffset, 3, nullptr);

	// Give the statsLine the same font as the statsLineColNo
	XtVaGetValues(statsLineColNo_, XmNfontList, &statsFontList, nullptr);
	XtVaSetValues(statsLine_, XmNfontList, statsFontList, nullptr);

	// Manage the statsLineForm
	if (showStats_) {
		XtManageChild(statsLineForm_);
	}

	/* If the fontList was nullptr, use the magical default provided by Motif,
	   since it must have worked if we've gotten this far */
	if (!fontList_) {
		XtVaGetValues(statsLine_, XmNfontList, &fontList_, nullptr);
	}

	// Create the menu bar
	menuBar_ = CreateMenuBar(mainWin_, this);
	XtManageChild(menuBar_);

	// Create paned window to manage split pane behavior
	splitPane_ = XtVaCreateManagedWidget("pane", xmPanedWindowWidgetClass, mainWin_, XmNseparatorOn, false, XmNspacing, 3, XmNsashIndent, -2, nullptr);
	XmMainWindowSetAreas(mainWin_, menuBar_, statsAreaForm, nullptr, nullptr, splitPane_);

	/* Store a copy of document/window pointer in text pane to support
	   action procedures. See also WidgetToWindow() for info. */
	XtVaSetValues(splitPane_, XmNuserData, this, nullptr);

	/* Patch around Motif's most idiotic "feature", that its menu accelerators
	   recognize Caps Lock and Num Lock as modifiers, and don't trigger if
	   they are engaged */
	AccelLockBugPatch(splitPane_, menuBar_);

	/* Create the first, and most permanent text area (other panes may
	   be added & removed, but this one will never be removed */
    textArea_ = createTextArea(splitPane_, this, rows, cols, GetPrefEmTabDist(PLAIN_LANGUAGE_MODE), GetPrefDelimiters().toLatin1().data(), GetPrefWrapMargin(), showLineNumbers_ ? MIN_LINE_NUM_COLS : 0);
	XtManageChild(textArea_);
	lastFocus_ = textArea_;

	// Set the initial colors from the globals.
	SetColors(
		GetPrefColorName(TEXT_FG_COLOR),
		GetPrefColorName(TEXT_BG_COLOR),
		GetPrefColorName(SELECT_FG_COLOR),
		GetPrefColorName(SELECT_BG_COLOR),
		GetPrefColorName(HILITE_FG_COLOR),
		GetPrefColorName(HILITE_BG_COLOR),
	    GetPrefColorName(LINENO_FG_COLOR),
		GetPrefColorName(CURSOR_FG_COLOR));

	/* Create the right button popup menu (note: order is important here,
	   since the translation for popping up this menu was probably already
	   added in createTextArea, but CreateBGMenu requires textArea_
	   to be set so it can attach the menu to it (because menu shells are
	   finicky about the kinds of widgets they are attached to)) */
	bgMenuPane_ = CreateBGMenu(this);

	// cache user menus: init. user background menu cache
	InitUserBGMenuCache(&userBGMenuCache_);


	// ------------------------------------------------------------------------------
	// NOTE(eteran): looks like at this point the UI is setup, now start creating the
	//               main text buffer view
	// ------------------------------------------------------------------------------

	/* Create the text buffer rather than using the one created automatically
	   with the text area widget.  This is done so the syntax highlighting
	   modify callback can be called to synchronize the style buffer BEFORE
	   the text display's callback is called upon to display a modification */
	buffer_ = new TextBuffer;
	buffer_->BufAddModifyCB(SyntaxHighlightModifyCB, this);

	// Attach the buffer to the text widget, and add callbacks for modify
	auto textD = textD_of(textArea_);
	textD->TextSetBuffer(buffer_);

	buffer_->BufAddModifyCB(modifiedCB, this);

	// Designate the permanent text area as the owner for selections
	textD->HandleXSelections();

	// Set the requested hardware tab distance and useTabs in the text buffer
	buffer_->BufSetTabDistance(GetPrefTabDist(PLAIN_LANGUAGE_MODE));
	buffer_->useTabs_ = GetPrefInsertTabs();

	// add the window to the global window list, update the Windows menus
	addToWindowList();
	InvalidateWindowMenus();

	bool showTabBar = GetShowTabBar();
	if (showTabBar)
		XtManageChild(tabForm);

	manageToolBars(statsAreaForm);

	if (showTabBar || showISearchLine_ || showStats_)
		XtManageChild(statsAreaForm);

	// realize all of the widgets in the new window
	RealizeWithoutForcingPosition(shell_);
	XmProcessTraversal(textArea_, XmTRAVERSE_CURRENT);

	// Make close command in window menu gracefully prompt for close
	AddMotifCloseCallback(shell_, closeCB, this);

	// Make window resizing work in nice character heights
	UpdateWMSizeHints();

	// Set the minimum pane height for the initial text pane
	UpdateMinPaneHeights();

	// dim/undim Attach_Tab menu items
#if 0 // NOTE(eteran): transitioned
	int state = TabCount() < WindowCount();
#endif

	for(Document *win: WindowList) {
		if (win->IsTopDocument()) {
#if 0 // NOTE(eteran): transitioned
			XtSetSensitive(win->moveDocumentItem_, state);
			XtSetSensitive(win->contextMoveDocumentItem_, state);
#endif
		}
	}
#endif
}

Document::~Document() {
}


/*
** Create a new document in the shell window.
** Document are created in 'background' so that the user
** menus, ie. the Macro/Shell/BG menus, will not be updated
** unnecessarily; hence speeding up the process of opening
** multiple files.
*/
Document *Document::CreateDocument(const QString &name) {
    Q_UNUSED(name);
#if 0
	int nCols;
	int nRows;

	// Allocate some memory for the new window data structure
	// start with a copy of the this document
	auto window = new Document(*this);


    // share these dialog items with parent shell
	window->dialogFind_    = nullptr;
	window->dialogReplace_ = nullptr;

    window->showLineNumbers_ = GetPrefLineNums();
    window->showStats_       = GetPrefStatsLine();
    window->showISearchLine_ = GetPrefISearchLine();


	window->multiFileReplSelected_ = false;
	window->multiFileBusy_         = false;
	window->writableWindows_       = nullptr;
	window->nWritableWindows_      = 0;
	window->fileChanged_           = false;
	window->fileMissing_           = true;
	window->fileMode_              = 0;
	window->fileUid_               = 0;
	window->fileGid_               = 0;
	window->filenameSet_           = false;
	window->fileFormat_            = UNIX_FILE_FORMAT;
	window->lastModTime_           = 0;
	window->filename_              = name;
	window->undo_                  = std::list<UndoInfo *>();
	window->redo_                  = std::list<UndoInfo *>();
	window->textPanes_.clear();
	window->autoSaveCharCount_     = 0;
	window->autoSaveOpCount_       = 0;
	window->undoMemUsed_           = 0;

	window->lockReasons_.clear();

	window->indentStyle_           = GetPrefAutoIndent(PLAIN_LANGUAGE_MODE);
	window->autoSave_              = GetPrefAutoSave();
	window->saveOldVersion_        = GetPrefSaveOldVersion();
	window->wrapMode_              = GetPrefWrap(PLAIN_LANGUAGE_MODE);
	window->overstrike_            = false;
	window->showMatchingStyle_     = GetPrefShowMatching();
	window->matchSyntaxBased_      = GetPrefMatchSyntaxBased();
	window->highlightSyntax_       = GetPrefHighlightSyntax();
	window->backlightCharTypes_    = QString();
	window->backlightChars_        = GetPrefBacklightChars();

	if (window->backlightChars_) {
		const char *cTypes = GetPrefBacklightCharTypes();
		if (cTypes && window->backlightChars_) {
			window->backlightCharTypes_ = QLatin1String(cTypes);
		}
	}

	window->modeMessageDisplayed_  = false;
	window->modeMessage_           = QString();
	window->ignoreModify_          = false;
	window->windowMenuValid_       = false;
	window->flashTimeoutID_        = 0;
	window->fileClosedAtom_        = None;
	window->wasSelected_           = false;

	window->fontName_              = GetPrefFontName();
	window->italicFontName_        = GetPrefItalicFontName();
	window->boldFontName_          = GetPrefBoldFontName();
	window->boldItalicFontName_    = GetPrefBoldItalicFontName();

	window->dialogColors_          = nullptr;
	window->fontList_              = GetPrefFontList();
	window->italicFontStruct_      = GetPrefItalicFont();
	window->boldFontStruct_        = GetPrefBoldFont();
	window->boldItalicFontStruct_  = GetPrefBoldItalicFont();
	window->dialogFonts_           = nullptr;
	window->nMarks_                = 0;
	window->markTimeoutID_         = 0;
	window->highlightData_         = nullptr;
	window->shellCmdData_          = nullptr;
	window->macroCmdData_          = nullptr;
	window->smartIndentData_       = nullptr;
	window->languageMode_          = PLAIN_LANGUAGE_MODE;
	window->iSearchHistIndex_      = 0;
	window->iSearchStartPos_       = -1;
	window->iSearchLastRegexCase_  = true;
	window->iSearchLastLiteralCase_= false;
	window->tab_                   = nullptr;
	window->bgMenuUndoItem_        = nullptr;
	window->bgMenuRedoItem_        = nullptr;
	window->device_                = 0;
	window->inode_                 = 0;

	if (!window->fontList_) {
		XtVaGetValues(statsLine_, XmNfontList, &window->fontList_, nullptr);
	}

	getTextPaneDimension(&nRows, &nCols);

	/* Create pane that actaully holds the new document. As
	   document is created in 'background', we need to hide
	   it. If we leave it unmanaged without setting it to
	   the XmNworkWindow of the mainWin, due to a unknown
	   bug in Motif where splitpane's scrollWindow child
	   somehow came up with a height taller than the splitpane,
	   the bottom part of the text editing widget is obstructed
	   when later brought up by  RaiseDocument(). So we first
	   manage it hidden, then unmanage it and reset XmNworkWindow,
	   then let RaiseDocument() show it later. */
	Widget pane = XtVaCreateWidget("pane", xmPanedWindowWidgetClass, window->mainWin_, XmNmarginWidth, 0, XmNmarginHeight, 0, XmNseparatorOn, false, XmNspacing, 3, XmNsashIndent, -2, XmNmappedWhenManaged, false, nullptr);
	XtVaSetValues(window->mainWin_, XmNworkWindow, pane, nullptr);
	XtManageChild(pane);
	window->splitPane_ = pane;

	/* Store a copy of document/window pointer in text pane to support
	   action procedures. See also WidgetToWindow() for info. */
	XtVaSetValues(pane, XmNuserData, window, nullptr);

	/* Patch around Motif's most idiotic "feature", that its menu accelerators
	   recognize Caps Lock and Num Lock as modifiers, and don't trigger if
	   they are engaged */
	AccelLockBugPatch(pane, window->menuBar_);

	/* Create the first, and most permanent text area (other panes may
	   be added & removed, but this one will never be removed */
	Widget text;
    text = createTextArea(pane, window, nRows, nCols, GetPrefEmTabDist(PLAIN_LANGUAGE_MODE), GetPrefDelimiters().toLatin1().data(), GetPrefWrapMargin(), window->showLineNumbers_ ? MIN_LINE_NUM_COLS : 0);
	XtManageChild(text);

	window->textArea_  = text;
	window->lastFocus_ = text;

	// Set the initial colors from the globals.
	window->SetColors(
		GetPrefColorName(TEXT_FG_COLOR),
		GetPrefColorName(TEXT_BG_COLOR),
		GetPrefColorName(SELECT_FG_COLOR),
		GetPrefColorName(SELECT_BG_COLOR),
		GetPrefColorName(HILITE_FG_COLOR),
		GetPrefColorName(HILITE_BG_COLOR),
		GetPrefColorName(LINENO_FG_COLOR),
		GetPrefColorName(CURSOR_FG_COLOR));

#if 0 // NOTE(eteran)
	/* Create the right button popup menu (note: order is important here,
	   since the translation for popping up this menu was probably already
	   added in createTextArea, but CreateBGMenu requires window->textArea_
	   to be set so it can attach the menu to it (because menu shells are
	   finicky about the kinds of widgets they are attached to)) */
	window->bgMenuPane_ = CreateBGMenu(window);

	// cache user menus: init. user background menu cache
	InitUserBGMenuCache(&window->userBGMenuCache_);

	/* Create the text buffer rather than using the one created automatically
	   with the text area widget.  This is done so the syntax highlighting
	   modify callback can be called to synchronize the style buffer BEFORE
	   the text display's callback is called upon to display a modification */
	window->buffer_ = new TextBuffer;
	window->buffer_->BufAddModifyCB(SyntaxHighlightModifyCB, window);

	// Attach the buffer to the text widget, and add callbacks for modify
	auto textD = textD_of(text);
	textD->TextSetBuffer(window->buffer_);
	window->buffer_->BufAddModifyCB(modifiedCB, window);

	// Designate the permanent text area as the owner for selections
	textD->HandleXSelections();

	// Set the requested hardware tab distance and useTabs in the text buffer
	window->buffer_->BufSetTabDistance(GetPrefTabDist(PLAIN_LANGUAGE_MODE));
	window->buffer_->useTabs_ = GetPrefInsertTabs();
	window->tab_ = addTab(window->tabBar_, name);

	// add the window to the global window list, update the Windows menus
	InvalidateWindowMenus();
	window->addToWindowList();

	// return the shell ownership to previous tabbed doc
	XtVaSetValues(window->mainWin_, XmNworkWindow, splitPane_, nullptr);
	XLowerWindow(TheDisplay, XtWindow(window->splitPane_));
	XtUnmanageChild(window->splitPane_);
	XtVaSetValues(window->splitPane_, XmNmappedWhenManaged, true, nullptr);
#endif
	return window;
    #endif
return nullptr;
}

/*
**
*/
Document *Document::GetTopDocument(Widget w) {
	Document *window = WidgetToWindow(w);
	return WidgetToWindow(window->shell_);
}

/*
** Recover the window pointer from any widget in the window, by searching
** up the widget hierarcy for the top level container widget where the
** window pointer is stored in the userData field. In a tabbed window,
** this is the window pointer of the top (active) document, which is
** returned if w is 'shell-level' widget - menus, find/replace dialogs, etc.
**
** To support action routine in tabbed windows, a copy of the window
** pointer is also store in the splitPane widget.
*/
Document *Document::WidgetToWindow(Widget w) {
	Document *window = nullptr;
	Widget parent;

	while (true) {
		// return window pointer of document
		if (XtClass(w) == xmPanedWindowWidgetClass)
			break;

		if (XtClass(w) == topLevelShellWidgetClass) {
			WidgetList items;

			/* there should be only 1 child for the shell -
			   the main window widget */
			XtVaGetValues(w, XmNchildren, &items, nullptr);
			w = items[0];
			break;
		}

		parent = XtParent(w);
		if(!parent)
			return nullptr;

		// make sure it is not a dialog shell
		if (XtClass(parent) == topLevelShellWidgetClass && XmIsMainWindow(w))
			break;

		w = parent;
	}

	XtVaGetValues(w, XmNuserData, &window, nullptr);

	return window;
}

void Document::Undo() {
#if 0 // NOTE(eteran): transitioned
	int restoredTextLength;

	// return if nothing to undo
	if (undo_.empty()) {
		return;
	}

	UndoInfo *undo = undo_.front();

	/* BufReplaceEx will eventually call SaveUndoInformation.  This is mostly
	   good because it makes accumulating redo operations easier, however
	   SaveUndoInformation needs to know that it is being called in the context
	   of an undo.  The inUndo field in the undo record indicates that this
	   record is in the process of being undone. */
	undo->inUndo = true;

	// use the saved undo information to reverse changes
	buffer_->BufReplaceEx(undo->startPos, undo->endPos, undo->oldText);

	restoredTextLength = undo->oldText.size();
	if (!buffer_->primary_.selected || GetPrefUndoModifiesSelection()) {
		/* position the cursor in the focus pane after the changed text
		   to show the user where the undo was done */
		auto textD = lastFocus();
		textD->TextSetCursorPos(undo->startPos + restoredTextLength);
	}

	if (GetPrefUndoModifiesSelection()) {
		if (restoredTextLength > 0) {
			buffer_->BufSelect(undo->startPos, undo->startPos + restoredTextLength);
		} else {
			buffer_->BufUnselect();
		}
	}
	MakeSelectionVisible(lastFocus_);

	/* restore the file's unmodified status if the file was unmodified
	   when the change being undone was originally made.  Also, remove
	   the backup file, since the text in the buffer is now identical to
	   the original file */
	if (undo->restoresToSaved) {
		SetWindowModified(false);
		RemoveBackupFile(this);
	}

	// free the undo record and remove it from the chain
	removeUndoItem();
#endif
}

void Document::Redo() {
#if 0 // NOTE(eteran): transitioned
	int restoredTextLength;

	// return if nothing to redo
	if (redo_.empty()) {
		return;
	}

	UndoInfo *redo = redo_.front();

	/* BufReplaceEx will eventually call SaveUndoInformation.  To indicate
	   to SaveUndoInformation that this is the context of a redo operation,
	   we set the inUndo indicator in the redo record */
	redo->inUndo = true;

	// use the saved redo information to reverse changes
	buffer_->BufReplaceEx(redo->startPos, redo->endPos, redo->oldText);

	restoredTextLength = redo->oldText.size();
	if (!buffer_->primary_.selected || GetPrefUndoModifiesSelection()) {
		/* position the cursor in the focus pane after the changed text
		   to show the user where the undo was done */
		auto textD = lastFocus();
		textD->TextSetCursorPos(redo->startPos + restoredTextLength);
	}
	if (GetPrefUndoModifiesSelection()) {

		if (restoredTextLength > 0) {
			buffer_->BufSelect(redo->startPos, redo->startPos + restoredTextLength);
		} else {
			buffer_->BufUnselect();
		}
	}
	MakeSelectionVisible(lastFocus_);

	/* restore the file's unmodified status if the file was unmodified
	   when the change being redone was originally made. Also, remove
	   the backup file, since the text in the buffer is now identical to
	   the original file */
	if (redo->restoresToSaved) {
		SetWindowModified(false);
		RemoveBackupFile(this);
	}

	// remove the redo record from the chain and free it
	removeRedoItem();
#endif
}


/*
** Add an undo record (already allocated by the caller) to the this's undo
** list if the item pushes the undo operation or character counts past the
** limits, trim the undo list to an acceptable length.
*/
void Document::addUndoItem(UndoInfo *undo) {

	// Make the undo menu item sensitive now that there's something to undo
	if (undo_.empty()) {
#if 0 // NOTE(eteran): transitioned
		SetSensitive(undoItem_, true);
		SetBGMenuUndoSensitivity(this, true);
#endif
	}

	// Add the item to the beginning of the list
	undo_.push_front(undo);

	// Increment the operation and memory counts
	undoMemUsed_ += undo->oldLen;

	// Trim the list if it exceeds any of the limits
	if (undo_.size() > UNDO_OP_LIMIT) {
		trimUndoList(UNDO_OP_TRIMTO);
	}

	if (undoMemUsed_ > UNDO_WORRY_LIMIT) {
		trimUndoList(UNDO_WORRY_TRIMTO);
	}

	if (undoMemUsed_ > UNDO_PURGE_LIMIT) {
		trimUndoList(UNDO_PURGE_TRIMTO);
	}
}

/*
** Add an item (already allocated by the caller) to the this's redo list.
*/
void Document::addRedoItem(UndoInfo *redo) {
	// Make the redo menu item sensitive now that there's something to redo
	if (redo_.empty()) {
#if 0 // NOTE(eteran): transitioned
		SetSensitive(redoItem_, true);
		SetBGMenuRedoSensitivity(this, true);
#endif
	}

	// Add the item to the beginning of the list
	redo_.push_front(redo);
}

/*
** Pop (remove and free) the current (front) undo record from the undo list
*/
void Document::removeUndoItem() {

	if (undo_.empty()) {
		return;
	}

	UndoInfo *undo = undo_.front();


	// Decrement the operation and memory counts
	undoMemUsed_ -= undo->oldLen;

	// Remove and free the item
	undo_.pop_front();
	delete undo;

	// if there are no more undo records left, dim the Undo menu item
	if (undo_.empty()) {
#if 0 // NOTE(eteran): transitioned
		SetSensitive(undoItem_, false);
		SetBGMenuUndoSensitivity(this, false);
#endif
	}
}

/*
** Pop (remove and free) the current (front) redo record from the redo list
*/
void Document::removeRedoItem() {
	UndoInfo *redo = redo_.front();

	// Remove and free the item
	redo_.pop_front();
	delete redo;

	// if there are no more redo records left, dim the Redo menu item
	if (redo_.empty()) {
#if 0 // NOTE(eteran): transitioned
		SetSensitive(redoItem_, false);
		SetBGMenuRedoSensitivity(this, false);
#endif
	}
}

/*
** Add deleted text to the beginning or end
** of the text saved for undoing the last operation.  This routine is intended
** for continuing of a string of one character deletes or replaces, but will
** work with more than one character.
*/
void Document::appendDeletedText(view::string_view deletedText, int deletedLen, int direction) {
	UndoInfo *undo = undo_.front();

	// re-allocate, adding space for the new character(s)
	std::string comboText;
	comboText.reserve(undo->oldLen + deletedLen);

	// copy the new character and the already deleted text to the new memory
	if (direction == FORWARD) {
		comboText.append(undo->oldText);
		comboText.append(deletedText.begin(), deletedText.end());
	} else {
		comboText.append(deletedText.begin(), deletedText.end());
		comboText.append(undo->oldText);
	}

	// keep track of the additional memory now used by the undo list
	undoMemUsed_++;

	// free the old saved text and attach the new
	undo->oldText = comboText;
	undo->oldLen += deletedLen;
}

/*
** Trim records off of the END of the undo list to reduce it to length
** maxLength
*/
void Document::trimUndoList(int maxLength) {

	if (undo_.empty()) {
		return;
	}

	auto it = undo_.begin();
	int i   = 1;

	// Find last item on the list to leave intact
	while(it != undo_.end() && i < maxLength) {
		++it;
		++i;
	}

	// Trim off all subsequent entries
	while(it != undo_.end()) {
		UndoInfo *u = *it;

		undoMemUsed_ -= u->oldLen;
		delete u;

		it = undo_.erase(it);
	}
}

/*
** ClearUndoList, ClearRedoList
**
** Functions for clearing all of the information off of the undo or redo
** lists and adjusting the edit menu accordingly
*/
void Document::ClearUndoList() {
	while (!undo_.empty()) {
		removeUndoItem();
	}
}
void Document::ClearRedoList() {
	while (!redo_.empty()) {
		removeRedoItem();
	}
}

/*
** SaveUndoInformation stores away the changes made to the text buffer.  As a
** side effect, it also increments the autoSave operation and character counts
** since it needs to do the classification anyhow.
**
** Note: This routine must be kept efficient.  It is called for every
**       character typed.
*/
void Document::SaveUndoInformation(int pos, int nInserted, int nDeleted, view::string_view deletedText) {

	const int isUndo = (!undo_.empty() && undo_.front()->inUndo);
	const int isRedo = (!redo_.empty() && redo_.front()->inUndo);

	/* redo operations become invalid once the user begins typing or does
	   other editing.  If this is not a redo or undo operation and a redo
	   list still exists, clear it and dim the redo menu item */
	if (!(isUndo || isRedo) && !redo_.empty()) {
		ClearRedoList();
	}

	/* figure out what kind of editing operation this is, and recall
	   what the last one was */
	const UndoTypes newType = determineUndoType(nInserted, nDeleted);
	if (newType == UNDO_NOOP) {
		return;
	}

	UndoInfo *const currentUndo = undo_.empty() ? nullptr : undo_.front();

	const UndoTypes oldType = (!currentUndo || isUndo) ? UNDO_NOOP : currentUndo->type;

	/*
	** Check for continuations of single character operations.  These are
	** accumulated so a whole insertion or deletion can be undone, rather
	** than just the last character that the user typed.  If the this
	** is currently in an unmodified state, don't accumulate operations
	** across the save, so the user can undo back to the unmodified state.
	*/
	if (fileChanged_) {

		// normal sequential character insertion
		if (((oldType == ONE_CHAR_INSERT || oldType == ONE_CHAR_REPLACE) && newType == ONE_CHAR_INSERT) && (pos == currentUndo->endPos)) {
			currentUndo->endPos++;
			autoSaveCharCount_++;
			return;
		}

		// overstrike mode replacement
		if ((oldType == ONE_CHAR_REPLACE && newType == ONE_CHAR_REPLACE) && (pos == currentUndo->endPos)) {
			appendDeletedText(deletedText, nDeleted, FORWARD);
			currentUndo->endPos++;
			autoSaveCharCount_++;
			return;
		}

		// forward delete
		if ((oldType == ONE_CHAR_DELETE && newType == ONE_CHAR_DELETE) && (pos == currentUndo->startPos)) {
			appendDeletedText(deletedText, nDeleted, FORWARD);
			return;
		}

		// reverse delete
		if ((oldType == ONE_CHAR_DELETE && newType == ONE_CHAR_DELETE) && (pos == currentUndo->startPos - 1)) {
			appendDeletedText(deletedText, nDeleted, REVERSE);
			currentUndo->startPos--;
			currentUndo->endPos--;
			return;
		}
	}

	/*
	** The user has started a new operation, create a new undo record
	** and save the new undo data.
	*/
	auto undo = new UndoInfo(newType, pos, pos + nInserted);

	// if text was deleted, save it
	if (nDeleted > 0) {
		undo->oldLen = nDeleted + 1; // +1 is for null at end
		undo->oldText = deletedText.to_string();
	}

	// increment the operation count for the autosave feature
	autoSaveOpCount_++;

	/* if the this is currently unmodified, remove the previous
	   restoresToSaved marker, and set it on this record */
	if (!fileChanged_) {
		undo->restoresToSaved = true;

		for(UndoInfo *u : undo_) {
			u->restoresToSaved = false;
		}

		for(UndoInfo *u : redo_) {
			u->restoresToSaved = false;
		}
	}

	/* Add the new record to the undo list  unless SaveUndoInfo is
	   saving information generated by an Undo operation itself, in
	   which case, add the new record to the redo list. */
	if (isUndo) {
		addRedoItem(undo);
	} else {
		addUndoItem(undo);
	}
}

QString Document::FullPath() const {
	return QString(QLatin1String("%1%2")).arg(path_, filename_);
}


DialogReplace *Document::getDialogReplace() const {
	return qobject_cast<DialogReplace *>(dialogReplace_);
}


void Document::EditCustomTitleFormat() {
}

/*
** find which document a tab belongs to
*/
Document *Document::TabToWindow(Widget tab) {

	auto it = std::find_if(WindowList.begin(), WindowList.end(), [tab](Document *win) {
		return win->tab_ == tab;
	});

	if(it != WindowList.end()) {
		return *it;
	}

	return nullptr;
}

/*
** Check if there is already a window open for a given file
*/
Document *Document::FindWindowWithFile(const QString &name, const QString &path) {
    Q_UNUSED(name);
    Q_UNUSED(path);
#if 0 // NOTE(eteran)
	/* I don't think this algorithm will work on vms so I am
	   disabling it for now */
	if (!GetPrefHonorSymlinks()) {
		struct stat attribute;

		QString fullname = QString(QLatin1String("%1%2")).arg(path, name);

		if (stat(fullname.toLatin1().data(), &attribute) == 0) {

			auto it = std::find_if(WindowList.begin(), WindowList.end(), [attribute](Document *window) {
				return attribute.st_dev == window->device_ && attribute.st_ino == window->inode_;
			});


			if(it != WindowList.end()) {
				return *it;
			}
		} /*  else:  Not an error condition, just a new file. Continue to check
		      whether the filename is already in use for an unsaved document.  */
	}


	auto it = std::find_if(WindowList.begin(), WindowList.end(), [name, path](Document *window) {
		return (window->filename_ == name) && (window->path_ == path);
	});

	if(it != WindowList.end()) {
		return *it;
	}
#endif
	return nullptr;
}

int Document::WindowCount() {
#if 0 // NOTE(eteran)
	int n = 0;
	for(Document *win: WindowList) {
		(void)win;
		++n;
	}
	return n;
#endif
    return 0;
}

Widget  Document::createTextArea(Widget parent, Document *window, int rows, int cols, int emTabDist, char *delimiters, int wrapMargin, int lineNumCols) {

	// Create a text widget inside of a scrolled window widget
	Widget sw         = XtVaCreateManagedWidget("scrolledW",         xmScrolledWindowWidgetClass, parent, XmNpaneMaximum, SHRT_MAX,     XmNpaneMinimum, PANE_MIN_HEIGHT, XmNhighlightThickness, 0, nullptr);
	Widget hScrollBar = XtVaCreateManagedWidget("textHorScrollBar",  xmScrollBarWidgetClass,      sw,     XmNorientation, XmHORIZONTAL, XmNrepeatDelay, 10, nullptr);
	Widget vScrollBar = XtVaCreateManagedWidget("textVertScrollBar", xmScrollBarWidgetClass,      sw,     XmNorientation, XmVERTICAL,   XmNrepeatDelay, 10, nullptr);
	Widget frame      = XtVaCreateManagedWidget("textFrame",         xmFrameWidgetClass,          sw,     XmNshadowType,  XmSHADOW_IN,  nullptr);

	Widget text = XtVaCreateManagedWidget(
		"text",
		textWidgetClass,
		frame,
		textNrows,               rows,
		textNcolumns,            cols,
		textNlineNumCols,        lineNumCols,
		textNemulateTabs,        emTabDist,
		textNfont,               GetDefaultFontStruct(window->fontList_),
		textNhScrollBar,         hScrollBar,
		textNvScrollBar,         vScrollBar,
		textNreadOnly,           window->lockReasons_.isAnyLocked(),
		textNwordDelimiters,     delimiters,
		textNwrapMargin,         wrapMargin,
		textNautoIndent,         (window->indentStyle_ == AUTO_INDENT),
		textNsmartIndent,        (window->indentStyle_ == SMART_INDENT),
		textNautoWrap,           (window->wrapMode_    == NEWLINE_WRAP),
		textNcontinuousWrap,     (window->wrapMode_    == CONTINUOUS_WRAP),
		textNoverstrike,         window->overstrike_,
		textNhidePointer,        (Boolean)GetPrefTypingHidesPointer(),
		textNcursorVPadding,     GetVerticalAutoScroll(),
		nullptr);
#if 0 // NOTE(eteran): transitioned
	XtVaSetValues(sw, XmNworkWindow, frame, XmNhorizontalScrollBar, hScrollBar, XmNverticalScrollBar, vScrollBar, nullptr);

	TextDisplay *textD = textD_of(text);
	textD->setBacklightCharTypes(window->backlightCharTypes_);

	// add focus, drag, cursor tracking, and smart indent callbacks	
	textD->addFocusCallback(focusCB, window);
	textD->addCursorMovementCallback(movedCB, window);
	textD->addDragStartCallback(dragStartCB, window);
	textD->addDragEndCallback(dragEndCB, window);
	textD->addsmartIndentCallback(SmartIndentCB, window);

	/* This makes sure the text area initially has a the insert point shown
	   ... (check if still true with the nedit text widget, probably not) */
	XmAddTabGroup(containingPane(text));

	// compensate for Motif delete/backspace problem
	RemapDeleteKey(text);

	// Augment translation table for right button popup menu
	AddBGMenuAction(text);

	/* If absolute line numbers will be needed for display in the statistics
	   line, tell the widget to maintain them (otherwise, it's a costly
	   operation and performance will be better without it) */
	textD->TextDMaintainAbsLineNum(window->showStats_);
#endif
    return text;
}


TextDisplay *Document::lastFocus() const {
	return textD_of(lastFocus_);
}

/*
** Is a calltip displayed?  Returns the calltip ID of the currently displayed
** calltip, or 0 if there is no calltip displayed.  If called with
** calltipID != 0, returns 0 unless there is a calltip being
** displayed with that calltipID.
*/
int Document::GetCalltipID(int calltipID) {
    Q_UNUSED(calltipID);
#if 0 // NOTE(eteran)
	return lastFocus()->TextDGetCalltipID(calltipID);
#endif
    return 0;
}

/*
** Pop-up a calltip.
** If a calltip is already being displayed it is destroyed and replaced with
** the new calltip.  Returns the ID of the calltip or 0 on failure.
*/
int Document::ShowCalltip(view::string_view text, bool anchored, int pos, int hAlign, int vAlign, int alignMode) {
    Q_UNUSED(text);
    Q_UNUSED(anchored);
    Q_UNUSED(pos);
    Q_UNUSED(hAlign);
    Q_UNUSED(vAlign);
    Q_UNUSED(alignMode);
#if 0 // NOTE(eteran)
	return lastFocus()->TextDShowCalltip(text, anchored, pos, hAlign, vAlign, alignMode);
#endif
    return 0;
}

/*
** Pop-down a calltip if one exists, else do nothing
*/
void Document::KillCalltip(int calltipID) {
    Q_UNUSED(calltipID);
#if 0 // NOTE(eteran)
	lastFocus()->TextDKillCalltip(calltipID);
#endif
}
