/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

var Cc = Components.classes;
var Ci = Components.interfaces;
var Cu = Components.utils;
var Cr = Components.results;

Cu.import('resource://gre/modules/accessibility/Utils.jsm');
Cu.import('resource://gre/modules/accessibility/EventManager.jsm');
Cu.import('resource://gre/modules/accessibility/TraversalRules.jsm');
Cu.import('resource://gre/modules/Services.jsm');

function virtualCursorControl(aMessage) {
  try {
    var vc = Utils.getVirtualCursor(content.document);
    var origin = aMessage.json.origin;
    if (origin != 'child') {
      if (moveCursorInFrame(vc.position, aMessage, origin))
        return;
    }

    var details = aMessage.json;
    var rule = TraversalRules[details.rule];
    var moved = 0;
    switch (details.action) {
    case 'moveFirst':
    case 'moveLast':
      moved = vc[details.action](rule);
      break;
    case 'moveNext':
    case 'movePrevious':
      try {
        if (origin == 'parent' && vc.position != null) {
          // TODO: Maybe directly present intead of setting and unsetting.
          var pos = vc.position;
          vc.position = null;
          vc.position = pos;
          moved = true;
        } else {
          moved = vc[details.action](rule);
        }
      } catch (x) {
        moved = vc.moveNext(rule, content.activeElement, true);
      }
      break;
    case 'moveToPoint':
      Logger.info('moveToPoint', details.x, details.y);
      moved = vc.moveToPoint(rule, details.x, details.y, true);
      break;
    case 'activateCurrent':
      activateAccessible(vc.position);
      break;
    default:
      break;
    }

    if (moved == true) {
      moveCursorInFrame(vc.position, aMessage, 'parent');
    } else if (moved == false) {
      Logger.info('Sending it back up', aMessage.name);
      aMessage.json.origin = 'child';
      sendAsyncMessage("AccessFu:VirtualCursor", aMessage.json);
    }
  } catch (x) {
    Logger.error(x);
  }
}

function moveCursorInFrame(aAccessible, aMessage, aOrigin) {
  try {
    if (aAccessible && aAccessible.role == Ci.nsIAccessibleRole.ROLE_INTERNAL_FRAME) {
      var mm = aAccessible.DOMNode.frameLoader.messageManager;
      mm.addMessageListener("AccessFu:VirtualCursor", virtualCursorControl);
      aMessage.json.origin = aOrigin;
      // XXX: OOP content's screen offset is 0, so we remove the real screen offset here.
      aMessage.json.x -= content.mozInnerScreenX;
      aMessage.json.y -= content.mozInnerScreenY;
      mm.sendAsyncMessage("AccessFu:VirtualCursor", aMessage.json);
      return true;
    }
  } catch (x) {
  }
  return false;
}

function activateAccessible(aAccessible) {
  if (aAccessible.actionCount > 0) {
    aAccessible.doAction(0);
  } else {
    // XXX Some mobile widget sets do not expose actions properly
    // (via ARIA roles, etc.), so we need to generate a click.
    // Could possibly be made simpler in the future. Maybe core
    // engine could expose nsCoreUtiles::DispatchMouseEvent()?
    var docAcc = Utils.AccRetrieval.getAccessibleFor(content.document);
    var docX = {}, docY = {}, docW = {}, docH = {};
    docAcc.getBounds(docX, docY, docW, docH);

    var objX = {}, objY = {}, objW = {}, objH = {};
    aAccessible.getBounds(objX, objY, objW, objH);

    var x = Math.round((objX.value - docX.value) + objW.value / 2);
    var y = Math.round((objY.value - docY.value) + objH.value / 2);

    var cwu = content.QueryInterface(Ci.nsIInterfaceRequestor).
      getInterface(Ci.nsIDOMWindowUtils);
    cwu.sendMouseEventToWindow('mousedown', x, y, 0, 1, 0, false);
    cwu.sendMouseEventToWindow('mouseup', x, y, 0, 1, 0, false);
  }
}

function sendPresent(aDetails) {
  Logger.info(content.mozInnerScreenY);
  sendAsyncMessage("AccessFu:Present", aDetails);
}

addMessageListener("AccessFu:VirtualCursor", virtualCursorControl);
addMessageListener("AccessFu:EventManager",
                   function (m) {
                     Logger.info('EventManager', m.json);
                     EventManager[m.json](sendPresent);
                   });

sendAsyncMessage("AccessFu:Ready");
