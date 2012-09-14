/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

var Cc = Components.classes;
var Ci = Components.interfaces;
var Cu = Components.utils;
var Cr = Components.results;

Cu.import('resource://gre/modules/accessibility/Utils.jsm');
Cu.import('resource://gre/modules/accessibility/Presenters.jsm');
Cu.import('resource://gre/modules/Services.jsm');

var EXPORTED_SYMBOLS = ['EventManager'];

var EventManager = {
  start: function start(aPresentFunc) {
    Logger.info('EventManager start!');
    this.presentFunc = aPresentFunc || function() {};
    this.presenters = [new VisualPresenter()];

    if (Utils.MozBuildApp == 'b2g') {
      this.presenters.push(new SpeechPresenter());
    } else if (Utils.MozBuildApp == 'mobile/android') {
      this.presenters.push(new AndroidPresenter());
    }

    Logger.info('EventManager start!', Utils.MozBuildApp, [p.type for each (p in this.presenters)].join(', '));

    if (this._started)
      return;

    this._started = true;
    Services.obs.addObserver(this, 'accessible-event', false);
  },

  stop: function stop() {
    Services.obs.removeObserver(this, 'accessible-event');
    this._started = false;
  },

  observe: function observe(aSubject, aTopic, aData) {
    switch (aTopic) {
      case 'accessible-event':
        var event;
        try {
          event = aSubject.QueryInterface(Ci.nsIAccessibleEvent);
          this.handleAccEvent(event);
        } catch (ex) {
          Logger.error(ex);
          return;
        }
    }
  },

  handleAccEvent: function handleAccEvent(aEvent) {
    switch (aEvent.eventType) {
      case Ci.nsIAccessibleEvent.EVENT_VIRTUALCURSOR_CHANGED:
        {
          let pivot = aEvent.accessible.
            QueryInterface(Ci.nsIAccessibleCursorable).virtualCursor;
          let position = pivot.position;
          if (position.role == Ci.nsIAccessibleRole.ROLE_INTERNAL_FRAME)
            break;
          let event = aEvent.
            QueryInterface(Ci.nsIAccessibleVirtualCursorChangeEvent);
          let presenterContext =
            new PresenterContext(position, event.oldAccessible);
          let reason = event.reason;
          let details = [];
          this.presenters.forEach(
            function(p) {
              let toPresent = p.pivotChanged(presenterContext, reason);
              if (toPresent)
                details.push(toPresent);
            });

          this.presentFunc(details);
        }
    }
  }
};
