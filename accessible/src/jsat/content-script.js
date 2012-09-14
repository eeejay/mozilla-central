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
      if (forwardMessage(vc, aMessage))
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
        if (origin == 'parent' && vc.position == null) {
          if (details.action == 'moveNext')
            moved = vc.moveFirst(rule);
          else
            moved = vc.moveLast(rule);
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
    default:
      break;
    }

    if (moved == true) {
      forwardMessage(vc, aMessage);
    } else if (moved == false && details.action != 'moveToPoint') {
      if (origin == 'parent') {
        Logger.info('SETTING TO NULL');
        vc.position = null;
      }
      aMessage.json.origin = 'child';
      sendAsyncMessage("AccessFu:VirtualCursor", aMessage.json);
    }
  } catch (x) {
    Logger.error(x);
  }
}

function forwardMessage(aVirtualCursor, aMessage) {
  try {
    var acc = aVirtualCursor.position;
    if (acc && acc.role == Ci.nsIAccessibleRole.ROLE_INTERNAL_FRAME) {
      var mm = acc.DOMNode.frameLoader.messageManager;
      mm.addMessageListener(aMessage.name, virtualCursorControl);
      aMessage.json.origin = 'parent';
      // XXX: OOP content's screen offset is 0, so we remove the real screen offset here.
      aMessage.json.x -= content.mozInnerScreenX;
      aMessage.json.y -= content.mozInnerScreenY;
      mm.sendAsyncMessage(aMessage.name, aMessage.json);
      return true;
    }
  } catch (x) {
  }
  return false;
}

function activateCurrent(aMessage) {
  Logger.info('activateCurrent');
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

  var vc = Utils.getVirtualCursor(content.document);
  if (!forwardMessage(vc, aMessage))
    activateAccessible(vc.position);
}

function scroll(aMessage) {
  var vc = Utils.getVirtualCursor(content.document);

  function tryToScroll() {
    var horiz = aMessage.json.horizontal;
    var page = aMessage.json.page;

    // Search up heirarchy for scrollable element.
    var acc = vc.position;
    while (acc) {
      Logger.info(Logger.accessibleToString(acc), horiz);
      var elem = acc.DOMNode;

      // We will do window scrolling next.
      if (elem == content.document)
        break;

      if (!horiz && elem.clientHeight < elem.scrollHeight) {
        var s = content.getComputedStyle(elem);
        if (s.overflowY == 'scroll' || s.overflowY == 'auto') {
          elem.scrollTop += page * elem.clientHeight;
          return true;
        }
      }

      if (horiz) {
        if (elem.clientWidth < elem.scrollWidth) {
          var s = content.getComputedStyle(elem);
          if (s.overflowX == 'scroll' || s.overflowX == 'auto') {
            elem.scrollLeft += page * elem.clientWidth;
            return true;
          }
        }

        var controllers = acc.
          getRelationByType(
            Ci.nsIAccessibleRelation.RELATION_CONTROLLED_BY);
        Logger.info(controllers.targetsCount);
        for (var i=0; controllers.targetsCount > i; i++) {
          var controller = controllers.getTarget(i);
          // If the section has a controlling slider, it should be considered
          // the page-turner.
          if (controller.role == Ci.nsIAccessibleRole.ROLE_SLIDER) {
            Logger.info('paging', Logger.accessibleToString(acc));
            // Sliders are controlled with ctrl+right/left. I just decided :)
            var evt = content.document.createEvent("KeyboardEvent");
            evt.initKeyEvent('keypress', true, true, null,
                             true, false, false, false,
                             (page > 0) ? evt.DOM_VK_RIGHT : evt.DOM_VK_LEFT, 0);
            controller.DOMNode.dispatchEvent(evt);
            return true;
          }
        }
      }
      acc = acc.parent;
    }

    // Scroll window.
    if (!horiz && content.scrollMaxY &&
        ((page > 0 && content.scrollY < content.scrollMaxY) ||
         (page < 0 && content.scrollY > 0))) {
      content.scroll(0, content.innerHeight);
      return true;
    } else if (horiz && content.scrollMaxX &&
               ((page > 0 && content.scrollX < content.scrollMaxX) ||
                (page < 0 && content.scrollX > 0))) {
      content.scroll(content.innerWidth, 0);
      return true;
    }

    return false;
  }

  if (aMessage.json.origin != 'child') {
    if (forwardMessage(vc, aMessage))
      return;
  }

  if (!tryToScroll()) {
    // Failed to scroll anything in this document. Try in parent document.
    aMessage.json.origin = 'child';
    sendAsyncMessage("AccessFu:Scroll", aMessage.json);
  }
}

function sendPresent(aDetails) {
  sendAsyncMessage("AccessFu:Present", aDetails);
}

addMessageListener("AccessFu:VirtualCursor", virtualCursorControl);
addMessageListener("AccessFu:Activate", activateCurrent);
addMessageListener("AccessFu:Scroll", scroll);
addMessageListener("AccessFu:EventManager",
                   function (m) {
                     Logger.info('EventManager', m.json);
                     EventManager[m.json](sendPresent);
                   });

sendAsyncMessage("AccessFu:Ready");
