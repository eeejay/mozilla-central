/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

'use strict';

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

var EXPORTED_SYMBOLS = ['VirtualCursorController'];

Cu.import('resource://gre/modules/accessibility/TraversalRules.jsm');
Cu.import('resource://gre/modules/accessibility/Utils.jsm');

var VirtualCursorController = {
  exploreByTouch: false,
  editableState: 0,

  attach: function attach(aWindow) {
    this.chromeWin = aWindow;
    this.chromeWin.document.addEventListener('keypress', this, true);
    this.chromeWin.addEventListener('mozAccessFuGesture', this, true);
  },

  detach: function detach() {
    this.chromeWin.document.removeEventListener('keypress', this, true);
    this.chromeWin.removeEventListener('mozAccessFuGesture', this, true);
  },

  handleEvent: function VirtualCursorController_handleEvent(aEvent) {
    switch (aEvent.type) {
      case 'keypress':
        this._handleKeypress(aEvent);
        break;
      case 'mozAccessFuGesture':
        this._handleGesture(aEvent);
        break;
    }
  },

  _handleGesture: function _handleGesture(aEvent) {
    let detail = aEvent.detail;
    Logger.info('Gesture', detail.type,
                '(fingers: ' + detail.touches.length + ')');

    if (detail.touches.length == 1) {
      switch (detail.type) {
        case 'swiperight':
          this.moveCursor('moveNext', 'Simple');
          break;
        case 'swipeleft':
          this.moveCursor('movePrevious', 'Simple');
          break;
        case 'doubletap':
          this.activateCurrent();
          break;
        case 'explore':
          this.moveCursor('moveToPoint', 'Simple', detail.x, detail.y);
          break;
      }
    }

    if (detail.touches.length == 3) {
      switch (detail.type) {
        case 'swiperight':
          this.scroll(-1, true);
          break;
        case 'swipedown':
          this.scroll(-1);
          break;
        case 'swipeleft':
          this.scroll(1, true);
          break;
        case 'swipeup':
          this.scroll(1);
          break;
      }
    }
  },

  _handleKeypress: function _handleKeypress(aEvent) {
    let target = aEvent.target;

    // Ignore keys with modifiers so the content could take advantage of them.
    if (aEvent.ctrlKey || aEvent.altKey || aEvent.metaKey)
      return;

    switch (aEvent.keyCode) {
      case 0:
        // an alphanumeric key was pressed, handle it separately.
        // If it was pressed with either alt or ctrl, just pass through.
        // If it was pressed with meta, pass the key on without the meta.
        if (this.editableState)
          return;

        let key = String.fromCharCode(aEvent.charCode);
        try {
          let [methodName, rule] = this.keyMap[key];
          this.moveCursor(methodName, rule);
        } catch (x) {
          return;
        }
        break;
      case aEvent.DOM_VK_RIGHT:
        if (this.editableState) {
          if (target.selectionEnd != target.textLength)
            // Don't move forward if caret is not at end of entry.
            // XXX: Fix for rtl
            return;
          else
            target.blur();
        }
        this.moveCursor(aEvent.shiftKey ? 'moveLast' : 'moveNext', 'Simple');
        break;
      case aEvent.DOM_VK_LEFT:
        if (this.editableState) {
          if (target.selectionEnd != 0)
            // Don't move backward if caret is not at start of entry.
            // XXX: Fix for rtl
            return;
          else
            target.blur();
        }
        this.moveCursor(aEvent.shiftKey ? 'moveFirst' : 'movePrevious', 'Simple');
        break;
      case aEvent.DOM_VK_UP:
        if (this.editableState & Ci.nsIAccessibleStates.EXT_STATE_MULTI_LINE) {
          if (target.selectionEnd != 0)
            // Don't blur content if caret is not at start of text area.
            return;
          else
            target.blur();
        }

        if (Utils.MozBuildApp == 'mobile/android')
          // Return focus to native Android browser chrome.
          Cc['@mozilla.org/android/bridge;1'].
            getService(Ci.nsIAndroidBridge).handleGeckoMessage(
              JSON.stringify({ gecko: { type: 'ToggleChrome:Focus' } }));
        break;
      case aEvent.DOM_VK_RETURN:
      case aEvent.DOM_VK_ENTER:
        if (this.editableState)
          return;
        this.activateCurrent();
        break;
      case aEvent.DOM_VK_PAUSE:
        try {
          Logger.dumpTree(
            Logger.INFO,
            Utils.AccRetrieval.getAccessibleFor(
              Utils.getCurrentContentDoc(this.chromeWin)));
        } catch (x) {
          Logger.error(x);
        }
        break;
      default:
        return;
    }

    aEvent.preventDefault();
    aEvent.stopPropagation();
  },

  moveCursor: function moveCursor(aAction, aRule, aX, aY) {
    let mm = Utils.getCurrentBrowser(this.chromeWin).frameLoader.messageManager;
    mm.sendAsyncMessage('AccessFu:VirtualCursor',
                        {action: aAction, rule: aRule,
                         x: aX, y: aY, origin: 'top'});
  },

  activateCurrent: function activateCurrent() {
    let mm = Utils.getCurrentBrowser(this.chromeWin).frameLoader.messageManager;
    mm.sendAsyncMessage('AccessFu:Activate', {});
  },

  scroll: function scroll(aPage, aHorizontal) {
    let mm = Utils.getCurrentBrowser(this.chromeWin).frameLoader.messageManager;
    mm.sendAsyncMessage('AccessFu:Scroll', {page: aPage, horizontal: aHorizontal, origin: 'top'});
  },

  moveCursorToObject: function moveCursorToObject(aVirtualCursor,
                                                  aAccessible, aRule) {
    aVirtualCursor.moveNext(aRule || TraversalRules.Simple, aAccessible, true);
  },

  keyMap: {
    a: ['moveNext', 'Anchor'],
    A: ['movePrevious', 'Anchor'],
    b: ['moveNext', 'Button'],
    B: ['movePrevious', 'Button'],
    c: ['moveNext', 'Combobox'],
    C: ['movePrevious', 'Combobox'],
    e: ['moveNext', 'Entry'],
    E: ['movePrevious', 'Entry'],
    f: ['moveNext', 'FormElement'],
    F: ['movePrevious', 'FormElement'],
    g: ['moveNext', 'Graphic'],
    G: ['movePrevious', 'Graphic'],
    h: ['moveNext', 'Heading'],
    H: ['movePrevious', 'Heading'],
    i: ['moveNext', 'ListItem'],
    I: ['movePrevious', 'ListItem'],
    k: ['moveNext', 'Link'],
    K: ['movePrevious', 'Link'],
    l: ['moveNext', 'List'],
    L: ['movePrevious', 'List'],
    p: ['moveNext', 'PageTab'],
    P: ['movePrevious', 'PageTab'],
    r: ['moveNext', 'RadioButton'],
    R: ['movePrevious', 'RadioButton'],
    s: ['moveNext', 'Separator'],
    S: ['movePrevious', 'Separator'],
    t: ['moveNext', 'Table'],
    T: ['movePrevious', 'Table'],
    x: ['moveNext', 'Checkbox'],
    X: ['movePrevious', 'Checkbox']
  }
};
