/* -*- Mode: javascript; tab-width: 20; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Oracle Corporation code.
 *
 * The Initial Developer of the Original Code is
 *  Oracle Corporation
 * Portions created by the Initial Developer are Copyright (C) 2004
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Vladimir Vukicevic <vladimir.vukicevic@oracle.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

//
// calTodo.js
//

//
// constructor
//
function calTodo() {
    this.wrappedJSObject = this;
    this.initItemBase();
    this.initTodo();
}

// var trickery to suppress lib-as-component errors from loader
var calItemBase;

calTodo.prototype = {
    __proto__: calItemBase ? (new calItemBase()) : {},

    QueryInterface: function (aIID) {
        if (!aIID.equals(Components.interfaces.nsISupports) &&
            !aIID.equals(Components.interfaces.calIItemBase) &&
            !aIID.equals(Components.interfaces.calITodo))
        {
            throw Components.results.NS_ERROR_NO_INTERFACE;
        }

        return this;
    },

    initTodo: function () {
        this.mEntryTime = createCalDateTime();
        this.mDueDate = createCalDateTime();
        this.mCompletedDate = createCalDateTime();
        this.mPercentComplete = 0;
    },

    clone: function () {
        var m = new calEvent();
        this.cloneItemBaseInto(m);
        m.mEntryDate = this.mEntryDate.clone();
        m.mDueDate = this.mDueDate.clone();
        m.mCompletedDate = this.mCompletedDate.clone();
        m.mPercentComplete = this.mPercentComplete;

        return m;
    },

    makeImmutable: function () {
        this.mEntryDate.makeImmutable();
        this.mDueDate.makeImmutable();
        this.mCompletedDate.makeImmutable();

        this.makeItemBaseImmutable();
    },

    get recurrenceStartDate() {
        return this.mEntryDate;
    },
};
        
// var decl to prevent spurious error messages when loaded as component

var makeMemberAttr;
if (makeMemberAttr) {
    makeMemberAttr(calTodo, "mEntryDate", null, "entryDate");
    makeMemberAttr(calTodo, "mEndDate", null, "endDate");
    makeMemberAttr(calTodo, "mCompletedDate", null, "completedDate");
    makeMemberAttr(calTodo, "mPercentComplete", 0, "percentComplete");
}
