/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

/*

  Implementation methods for the XUL tree element APIs.

*/

#include "nsCOMPtr.h"
#include "nsRDFCID.h"
#include "nsXULTreeElement.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIPresContext.h"
#include "nsIPresShell.h"
#include "nsINameSpaceManager.h"
#include "nsIFrame.h"
#include "nsITreeFrame.h"
#include "nsString.h"

nsIAtom*             nsXULTreeElement::kSelectedAtom;
nsIAtom*             nsXULTreeElement::kOpenAtom;
nsIAtom*             nsXULTreeElement::kTreeRowAtom;
nsIAtom*             nsXULTreeElement::kTreeItemAtom;
int                  nsXULTreeElement::gRefCnt = 0;

NS_IMPL_ADDREF_INHERITED(nsXULTreeElement, nsXULAggregateElement);
NS_IMPL_RELEASE_INHERITED(nsXULTreeElement, nsXULAggregateElement);

nsresult
nsXULTreeElement::QueryInterface(REFNSIID aIID, void** aResult)
{
    NS_PRECONDITION(aResult != nsnull, "null ptr");
    if (! aResult)
        return NS_ERROR_NULL_POINTER;

    if (aIID.Equals(nsCOMTypeInfo<nsIDOMXULTreeElement>::GetIID())) {
        *aResult = NS_STATIC_CAST(nsIDOMXULTreeElement*, this);
    }
    else if (aIID.Equals(nsCOMTypeInfo<nsIXULTreeContent>::GetIID())) {
        *aResult = NS_STATIC_CAST(nsIXULTreeContent*, this);
    }
    else {
        return nsXULAggregateElement::QueryInterface(aIID, aResult);
    }

    NS_ADDREF(NS_REINTERPRET_CAST(nsISupports*, *aResult));
    return NS_OK;
}


nsXULTreeElement::nsXULTreeElement(nsIDOMXULElement* aOuter)
  : nsXULAggregateElement(aOuter)
{
  if (gRefCnt++ == 0) {
    kSelectedAtom    = NS_NewAtom("selected");
    kOpenAtom        = NS_NewAtom("open");
    kTreeRowAtom     = NS_NewAtom("treerow");
    kTreeItemAtom    = NS_NewAtom("treeitem");
  }

  nsresult rv;

  nsRDFDOMNodeList* children;
  rv = nsRDFDOMNodeList::Create(&children);
  NS_ASSERTION(NS_SUCCEEDED(rv), "unable to create DOM node list");
  if (NS_FAILED(rv)) return;

  mSelectedItems = children;

  children = nsnull;
  rv = nsRDFDOMNodeList::Create(&children);
  NS_ASSERTION(NS_SUCCEEDED(rv), "unable to create DOM node list");
  if (NS_FAILED(rv)) return;

  mSelectedCells = children;
}

nsXULTreeElement::~nsXULTreeElement()
{
#ifdef DEBUG_REFS
    --gInstanceCount;
    fprintf(stdout, "%d - RDF: nsXULTreeElement\n", gInstanceCount);
#endif

  NS_IF_RELEASE(mSelectedItems);
  NS_IF_RELEASE(mSelectedCells);

  if (--gRefCnt == 0) {
    NS_IF_RELEASE(kSelectedAtom);
  }
}

NS_IMETHODIMP
nsXULTreeElement::GetSelectedItems(nsIDOMNodeList** aSelectedItems)
{
  NS_IF_ADDREF(mSelectedItems);
  *aSelectedItems = mSelectedItems;
  return NS_OK;
}

NS_IMETHODIMP
nsXULTreeElement::GetSelectedCells(nsIDOMNodeList** aSelectedCells)
{
  NS_IF_ADDREF(mSelectedCells);
  *aSelectedCells = mSelectedCells;
  return NS_OK;
}

NS_IMETHODIMP
nsXULTreeElement::SelectItem(nsIDOMXULElement* aTreeItem)
{
  // Sanity check. If we're the only item, just bail.
  PRUint32 length;
  mSelectedItems->GetLength(&length);
  if (length == 1) {
    // See if the single item already selected is us.
    nsCOMPtr<nsIDOMNode> domNode;
    mSelectedItems->Item(0, getter_AddRefs(domNode));
    nsCOMPtr<nsIDOMXULElement> treeItem = do_QueryInterface(domNode);
    if (treeItem.get() == aTreeItem)
      return NS_OK;
  }

  // First clear our selection.
  ClearItemSelectionInternal();

  // Now add ourselves to the selection by setting our selected attribute.
  AddItemToSelectionInternal(aTreeItem);

  FireOnSelectHandler();

  return NS_OK;
}

NS_IMETHODIMP
nsXULTreeElement::SelectCell(nsIDOMXULElement* aTreeCell)
{
  // Sanity check. If we're the only item, just bail.
  PRUint32 length;
  mSelectedCells->GetLength(&length);
  if (length == 1) {
    // See if the single item already selected is us.
    nsCOMPtr<nsIDOMNode> domNode;
    mSelectedCells->Item(0, getter_AddRefs(domNode));
    nsCOMPtr<nsIDOMXULElement> treeCell = do_QueryInterface(domNode);
    if (treeCell.get() == aTreeCell)
      return NS_OK;
  }

  // First clear our selection.
  ClearCellSelectionInternal();

  // Now add ourselves to the selection by setting our selected attribute.
  AddCellToSelectionInternal(aTreeCell);

  FireOnSelectHandler();

  return NS_OK;
}

NS_IMETHODIMP    
nsXULTreeElement::ClearItemSelection()
{
  ClearItemSelectionInternal();
  FireOnSelectHandler();
  return NS_OK;
}

void
nsXULTreeElement::ClearItemSelectionInternal()
{
  // Enumerate the elements and remove them from the selection.
  PRUint32 length;
  mSelectedItems->GetLength(&length);
  for (PRUint32 i = 0; i < length; i++) {
    nsCOMPtr<nsIDOMNode> node;
    mSelectedItems->Item(0, getter_AddRefs(node));
    nsCOMPtr<nsIContent> content = do_QueryInterface(node);
    content->UnsetAttribute(kNameSpaceID_None, kSelectedAtom, PR_TRUE);
  }
}

void
nsXULTreeElement::ClearCellSelectionInternal()
{
  // Enumerate the elements and remove them from the selection.
  PRUint32 length;
  mSelectedCells->GetLength(&length);
  for (PRUint32 i = 0; i < length; i++) {
    nsCOMPtr<nsIDOMNode> node;
    mSelectedCells->Item(0, getter_AddRefs(node));
    nsCOMPtr<nsIContent> content = do_QueryInterface(node);
    content->UnsetAttribute(kNameSpaceID_None, kSelectedAtom, PR_TRUE);
  }
}

NS_IMETHODIMP    
nsXULTreeElement::ClearCellSelection()
{
  ClearCellSelectionInternal();
  FireOnSelectHandler();
  return NS_OK;
}

void
nsXULTreeElement::AddItemToSelectionInternal(nsIDOMXULElement* aTreeItem)
{
  // Without clearing the selection, perform the add.
  nsCOMPtr<nsIContent> content = do_QueryInterface(aTreeItem);
  content->SetAttribute(kNameSpaceID_None, kSelectedAtom, nsAutoString("true"), PR_TRUE);
}

NS_IMETHODIMP
nsXULTreeElement::AddItemToSelection(nsIDOMXULElement* aTreeItem)
{
  // Without clearing the selection, perform the add.
  AddItemToSelectionInternal(aTreeItem);
  FireOnSelectHandler();
  return NS_OK;
}


void
nsXULTreeElement::RemoveItemFromSelectionInternal(nsIDOMXULElement* aTreeItem)
{
  nsCOMPtr<nsIContent> content = do_QueryInterface(aTreeItem);
  content->UnsetAttribute(kNameSpaceID_None, kSelectedAtom, PR_TRUE);
}

NS_IMETHODIMP
nsXULTreeElement::RemoveItemFromSelection(nsIDOMXULElement* aTreeItem)
{
  RemoveItemFromSelectionInternal(aTreeItem);
  FireOnSelectHandler();
  return NS_OK;
}

void
nsXULTreeElement::AddCellToSelectionInternal(nsIDOMXULElement* aTreeCell)
{
  // Without clearing the selection, perform the add.
  nsCOMPtr<nsIContent> content = do_QueryInterface(aTreeCell);
  content->SetAttribute(kNameSpaceID_None, kSelectedAtom, nsAutoString("true"), PR_TRUE);
}

NS_IMETHODIMP
nsXULTreeElement::AddCellToSelection(nsIDOMXULElement* aTreeCell)
{
  AddCellToSelectionInternal(aTreeCell);
  FireOnSelectHandler();
  return NS_OK;
}

void
nsXULTreeElement::RemoveCellFromSelectionInternal(nsIDOMXULElement* aTreeCell)
{
  nsCOMPtr<nsIContent> content = do_QueryInterface(aTreeCell);
  content->UnsetAttribute(kNameSpaceID_None, kSelectedAtom, PR_TRUE);
}

NS_IMETHODIMP
nsXULTreeElement::RemoveCellFromSelection(nsIDOMXULElement* aTreeCell)
{
  RemoveCellFromSelectionInternal(aTreeCell);
  FireOnSelectHandler();
  return NS_OK;
}

NS_IMETHODIMP
nsXULTreeElement::ToggleItemSelection(nsIDOMXULElement* aTreeItem)
{
  nsAutoString isSelected;
  aTreeItem->GetAttribute("selected", isSelected);
  if (isSelected == "true")
    RemoveItemFromSelectionInternal(aTreeItem);
  else AddItemToSelectionInternal(aTreeItem);

  FireOnSelectHandler();
  return NS_OK;
}

NS_IMETHODIMP
nsXULTreeElement::ToggleCellSelection(nsIDOMXULElement* aTreeCell)
{
  nsAutoString isSelected;
  aTreeCell->GetAttribute("selected", isSelected);
  if (isSelected == "true")
    RemoveCellFromSelectionInternal(aTreeCell);
  else AddCellToSelectionInternal(aTreeCell);

  FireOnSelectHandler();

  return NS_OK;
}


NS_IMETHODIMP
nsXULTreeElement::SelectItemRange(nsIDOMXULElement* aStartItem, nsIDOMXULElement* aEndItem)
{
  // XXX Fill in.
  return NS_OK;
}

NS_IMETHODIMP
nsXULTreeElement::SelectCellRange(nsIDOMXULElement* aStartItem, nsIDOMXULElement* aEndItem)
{
  // XXX Fill in.
  return NS_OK;
}

NS_IMETHODIMP
nsXULTreeElement::SelectAll()
{
  // XXX Select anything that isn't selected.
  // Write later.
  return NS_OK;
}

NS_IMETHODIMP
nsXULTreeElement::InvertSelection()
{
  // XXX Woo hoo. Write this later.
  // Yikes. Involves an enumeration of the whole tree.
  return NS_OK;
}

nsresult
nsXULTreeElement::FireOnSelectHandler()
{
  nsCOMPtr<nsIContent> content = do_QueryInterface(mOuter);
  nsCOMPtr<nsIDocument> document;
  content->GetDocument(*getter_AddRefs(document));

  // If there's no document (e.g., a selection is occuring in a
  // 'orphaned' node), then there ain't a whole lot to do here!
  if (! document) {
    NS_WARNING("FireOnSelectHandler occurred in orphaned node");
    return NS_OK;
  }

  // The frame code can suppress the firing of this handler by setting an attribute
  // for us.  Look for that and bail if it's present.
  nsCOMPtr<nsIAtom> kSuppressSelectChange = dont_AddRef(NS_NewAtom("suppressonselect"));
  nsAutoString value;
  content->GetAttribute(kNameSpaceID_None, kSuppressSelectChange, value);
  if (value == "true")
    return NS_OK;

  PRInt32 count = document->GetNumberOfShells();
	for (PRInt32 i = 0; i < count; i++) {
		nsIPresShell* shell = document->GetShellAt(i);
		if (nsnull == shell)
				continue;

		// Retrieve the context in which our DOM event will fire.
		nsCOMPtr<nsIPresContext> aPresContext;
		shell->GetPresContext(getter_AddRefs(aPresContext));

		NS_RELEASE(shell);
				
    nsEventStatus status = nsEventStatus_eIgnore;
    nsEvent event;
    event.eventStructType = NS_EVENT;
    event.message = NS_FORM_SELECTED;

    content->HandleDOMEvent(aPresContext, &event, nsnull, NS_EVENT_FLAG_INIT, &status);
  }

  return NS_OK;
}

nsresult
nsXULTreeElement::GetRowIndexOf(nsIDOMXULElement *aElement, PRInt32 *aReturn)
{
  NS_ENSURE_ARG_POINTER(aElement);
  NS_ENSURE_ARG_POINTER(aReturn);

  nsresult rv;

  nsCOMPtr<nsIContent> elementContent = do_QueryInterface(aElement, &rv);
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsIContent> treeContent =
    do_QueryInterface((nsIXULTreeContent*)this,&rv);
  if (NS_FAILED(rv)) return rv;
  
  *aReturn = 0;

  return IndexOfContent(treeContent, elementContent, aReturn);
}


nsresult
nsXULTreeElement::EnsureElementIsVisible(nsIDOMXULElement *aElement)
{
  if (!aElement) return NS_OK;

  nsresult rv;
  
  PRInt32 indexOfContent;
  rv = GetRowIndexOf(aElement, &indexOfContent);
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsIContent> content = do_QueryInterface(mOuter);
  nsCOMPtr<nsIDocument> document;
  content->GetDocument(*getter_AddRefs(document));

  // If there's no document (e.g., a selection is occuring in a
  // 'orphaned' node), then there ain't a whole lot to do here!
  if (! document) {
    NS_WARNING("Trying to EnsureElementIsVisible on orphaned element!");
    return NS_OK;
  }

  // now call EnsureElementIsVisible on all the frames
  PRInt32 count = document->GetNumberOfShells();
	for (PRInt32 i = 0; i < count; i++) {
		nsCOMPtr<nsIPresShell> shell = document->GetShellAt(i);
		if (!shell)
				continue;

    nsIFrame *frame;
    shell->GetPrimaryFrameFor(content, &frame);

    if (frame) {
      nsITreeFrame *treeFrame = nsnull;      
      rv = frame->QueryInterface(NS_GET_IID(nsITreeFrame),
                                 (void **)&treeFrame);
      if (NS_SUCCEEDED(rv) && treeFrame) {
        treeFrame->EnsureRowIsVisible(indexOfContent);
      }
    }

  }

  return NS_OK;
  
  
}


// helper routine for GetRowIndexOf()
// walks the DOM to get the zero-based row index of the current content
// note that aContent can be any element, this will get the index of the
// element's parent
nsresult
nsXULTreeElement::IndexOfContent(nsIContent* aRoot,
                                 nsIContent* aContent,
                                 PRInt32 *aResult)
{
  // number of rows that are direct children of aRoot
  // on success, add this to aResult
  PRInt32 rows=0;
  
  PRInt32 childCount=0;
  aRoot->ChildCount(childCount);
  
  PRInt32 childIndex;
  for (childIndex=0; childIndex<childCount; childIndex++) {
    nsCOMPtr<nsIContent> child;
    aContent->ChildAt(childIndex, *getter_AddRefs(child));
    
    nsCOMPtr<nsIAtom> childTag;
    child->GetTag(*getter_AddRefs(childTag));

    // we hit a treerow, count it
    if (childTag.get() == kTreeRowAtom)
      rows++;
    
    // is this it?
    if (child.get() == aContent) {
      *aResult += rows;
      return NS_OK;
    }

    // check children only if open="true"
    if (childTag.get() == kTreeItemAtom) {
      nsAutoString isOpen;
      child->GetAttribute(kNameSpaceID_None, kOpenAtom, isOpen);

      if (isOpen == "true" &&
          NS_SUCCEEDED(IndexOfContent(child, aContent, aResult))) {
        
        *aResult += rows;
        return NS_OK;
      }
    }
  }

  // not found
  return NS_ERROR_FAILURE;
}

