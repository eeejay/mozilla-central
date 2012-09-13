/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

'use strict';

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

var EXPORTED_SYMBOLS = ['Adapters'];

Cu.import('resource://gre/modules/accessibility/Utils.jsm');
Cu.import('resource://gre/modules/tts.jsm');
Cu.import('resource://gre/modules/Geometry.jsm');

var Adapters = {
  attach: function attach(aWindow) {
    this.chromeWin = aWindow;
  },

  Speech: function Speech(aDetails, aBrowser) {
    function doSpeech() {
      for each (var action in aDetails.actions)
        tts[action.method](action.data, action.options);
    }

    if (!this.audioElement) {
      let earcons = [
        ['tick', 'chrome://global/content/accessibility/tick.wav']
      ];
      this.audioElement = new this.chromeWin.Audio();
      tts.init(
        this.audioElement,
        function () {
          let earconsToAdd = earcons.length;
          for (var i in earcons) {
            let [name, uri] = earcons[i];
            tts.addEarcon(
              name, uri,
              function () {
                if (--earconsToAdd == 0)
                  doSpeech();
              });
          }
        });
    } else {
      doSpeech();
    }
  },

  Visual: function Visual(aDetails, aBrowser) {
    if (!this.highlightBox) {
      // Add highlight box
      this.highlightBox = this.chromeWin.document.
        createElementNS('http://www.w3.org/1999/xhtml', 'div');
      this.chromeWin.document.documentElement.appendChild(this.highlightBox);
      this.highlightBox.id = 'virtual-cursor-box';

      // Add highlight inset for inner shadow
      let inset = this.chromeWin.document.
        createElementNS('http://www.w3.org/1999/xhtml', 'div');
      inset.id = 'virtual-cursor-inset';

      this.highlightBox.appendChild(inset);
    }

    if (aDetails.method == 'show') {
      let padding = aDetails.padding;
      let bounds = new Rect(aDetails.bounds.left, aDetails.bounds.top,
                            aDetails.bounds.right - aDetails.bounds.left,
                            aDetails.bounds.bottom - aDetails.bounds.top);
      let vp = Utils.getViewport(this.chromeWin) || { zoom: 1.0, offsetY: 0 };

      let browserOffset = aBrowser.getBoundingClientRect();
      let r = bounds.translate(browserOffset.left, browserOffset.top).
        scale(vp.zoom, vp.zoom).expandToIntegers();

      // First hide it to avoid flickering when changing the style.
      this.highlightBox.style.display = 'none';
      this.highlightBox.style.top = (r.top - padding) + 'px';
      this.highlightBox.style.left = (r.left - padding) + 'px';
      this.highlightBox.style.width = (r.width + padding*2) + 'px';
      this.highlightBox.style.height = (r.height + padding*2) + 'px';
      this.highlightBox.style.display = 'block';
    } else if (aDetails.method == 'hide') {
      this.highlightBox.style.display = 'none';
    }
  }
};