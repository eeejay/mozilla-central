/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License.  You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are Copyright (C) 1998
 * Netscape Communications Corporation.  All Rights Reserved.
 */

#include "nsBrowserAppCore.h"

#include "nsIBrowserWindow.h"
#include "nsIWebShell.h"
#include "pratom.h"
#include "prprf.h"
#include "nsIComponentManager.h"
#include "nsAppCores.h"
#include "nsAppCoresCIDs.h"
#include "nsIDOMAppCoresManager.h"

#include "nsIFileSpecWithUI.h"

#include "nsIScriptContext.h"
#include "nsIScriptContextOwner.h"
#include "nsIScriptGlobalObject.h"
#include "nsIDOMDocument.h"
#include "nsIDOMXULDocument.h"
#include "nsIDocument.h"
#include "nsIDOMWindow.h"

#include "nsIScriptGlobalObject.h"
#include "nsIWebShell.h"
#include "nsIWebShellWindow.h"
#include "nsCOMPtr.h"

#include "nsIServiceManager.h"
#include "nsIURL.h"
#ifndef NECKO
#include "nsINetService.h"
#else
#include "nsIIOService.h"
#include "nsIURL.h"
static NS_DEFINE_CID(kIOServiceCID, NS_IOSERVICE_CID);
#endif // NECKO
#include "nsIWidget.h"
#include "plevent.h"

#include "nsIAppShell.h"
#include "nsIAppShellService.h"
#include "nsAppShellCIDs.h"

#include "nsIDocumentViewer.h"
#include "nsIDOMHTMLImageElement.h"
#include "nsICmdLineService.h"
#include "nsIGlobalHistory.h"

#include "nsIDOMXULDocument.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMNode.h"
#include "nsIDOMElement.h"


#include "nsIPresContext.h"
#include "nsIPresShell.h"
#include "nsFileSpec.h"  // needed for nsAutoCString

#include "nsIDocumentLoader.h"
#include "nsIObserverService.h"

#ifdef ClientWallet
#include "nsIFileLocator.h"
#include "nsIFileSpec.h"
#include "nsFileLocations.h"
#include "nsIWalletService.h"
static NS_DEFINE_IID(kIWalletServiceIID, NS_IWALLETSERVICE_IID);
static NS_DEFINE_IID(kWalletServiceCID, NS_WALLETSERVICE_CID);
#endif

// Interface for "unknown content type handler" component/service.
#include "nsIUnkContentTypeHandler.h"

// Stuff to implement file download dialog.
#include "nsIXULWindowCallbacks.h"
#include "nsIDocumentObserver.h"
#include "nsIContent.h"
#include "nsINameSpaceManager.h"
#include "nsFileStream.h"

// Stuff to implement find/findnext
#include "nsIFindComponent.h"
#ifdef DEBUG_warren
#include "prlog.h"
#if defined(DEBUG) || defined(FORCE_PR_LOG)
static PRLogModuleInfo* gTimerLog = nsnull;
#endif /* DEBUG || FORCE_PR_LOG */
#endif

static NS_DEFINE_IID(kIDocumentViewerIID, NS_IDOCUMENT_VIEWER_IID);


/* Define Class IDs */
static NS_DEFINE_IID(kAppShellServiceCID,        NS_APPSHELL_SERVICE_CID);
static NS_DEFINE_IID(kBrowserAppCoreCID,         NS_BROWSERAPPCORE_CID);
static NS_DEFINE_IID(kCmdLineServiceCID,    NS_COMMANDLINE_SERVICE_CID);
static NS_DEFINE_IID(kCGlobalHistoryCID,       NS_GLOBALHISTORY_CID);
static NS_DEFINE_IID(kCSessionHistoryCID,       NS_SESSION_HISTORY_CID);

/* Define Interface IDs */
static NS_DEFINE_IID(kICmdLineServiceIID,   NS_ICOMMANDLINE_SERVICE_IID);
static NS_DEFINE_IID(kIAppShellServiceIID,       NS_IAPPSHELL_SERVICE_IID);
static NS_DEFINE_IID(kISupportsIID,              NS_ISUPPORTS_IID);
static NS_DEFINE_IID(kIBrowserAppCoreIID,        NS_IDOMBROWSERAPPCORE_IID);
static NS_DEFINE_IID(kIDOMDocumentIID,           nsIDOMDocument::GetIID());
static NS_DEFINE_IID(kIDocumentIID,              nsIDocument::GetIID());
#ifdef NECKO
#else
static NS_DEFINE_IID(kINetSupportIID,            NS_INETSUPPORT_IID);
#endif
static NS_DEFINE_IID(kIStreamObserverIID,        NS_ISTREAMOBSERVER_IID);
static NS_DEFINE_IID(kIWebShellWindowIID,        NS_IWEBSHELL_WINDOW_IID);
static NS_DEFINE_IID(kIGlobalHistoryIID,       NS_IGLOBALHISTORY_IID);
static NS_DEFINE_IID(kISessionHistoryIID,       NS_ISESSION_HISTORY_IID);
static NS_DEFINE_IID(kIWebShellIID,              NS_IWEB_SHELL_IID);

#define APP_DEBUG 0

#define FILE_PROTOCOL "file://"

static nsresult
FindNamedXULElement(nsIWebShell * aShell, const char *aId, nsCOMPtr<nsIDOMElement> * aResult );


static nsresult setAttribute( nsIWebShell *shell,
                              const char *id,
                              const char *name,
                              const nsString &value );
/////////////////////////////////////////////////////////////////////////
// nsBrowserAppCore
/////////////////////////////////////////////////////////////////////////

nsBrowserAppCore::nsBrowserAppCore()
{
  if (APP_DEBUG) printf("Created nsBrowserAppCore\n");

  mScriptObject         = nsnull;
  mToolbarWindow        = nsnull;
  mToolbarScriptContext = nsnull;
  mContentWindow        = nsnull;
  mContentScriptContext = nsnull;
  mWebShellWin          = nsnull;
  mWebShell             = nsnull;
  mContentAreaWebShell  = nsnull;
  mSHistory             = nsnull;
  IncInstanceCount();
  NS_INIT_REFCNT();

}

nsBrowserAppCore::~nsBrowserAppCore()
{
  // We own none of these things, so no need to release
  //NS_IF_RELEASE(mToolbarWindow)
  //NS_IF_RELEASE(mToolbarScriptContext);
  //NS_IF_RELEASE(mContentWindow);
  //NS_IF_RELEASE(mContentScriptContext);
  //NS_IF_RELEASE(mWebShellWin);
  //NS_IF_RELEASE(mWebShell);
  //NS_IF_RELEASE(mContentAreaWebShell);
  NS_IF_RELEASE(mSHistory);

  DecInstanceCount();  
}


NS_IMPL_ADDREF_INHERITED(nsBrowserAppCore, nsBaseAppCore)
NS_IMPL_RELEASE_INHERITED(nsBrowserAppCore, nsBaseAppCore)


NS_IMETHODIMP 
nsBrowserAppCore::QueryInterface(REFNSIID aIID,void** aInstancePtr)
{
  if (aInstancePtr == NULL) {
    return NS_ERROR_NULL_POINTER;
  }

  // Always NULL result, in case of failure
  *aInstancePtr = NULL;

  if ( aIID.Equals(kIBrowserAppCoreIID) ) {
    *aInstancePtr = (void*) ((nsIDOMBrowserAppCore*)this);
    AddRef();
    return NS_OK;
  }

  /* This isn't supported any more
  if (aIID.Equals(kIStreamObserverIID)) {
    *aInstancePtr = (void*) ((nsIStreamObserver*)this);
    NS_ADDREF_THIS();
    return NS_OK;
  }
*/
  if (aIID.Equals(nsIDocumentLoaderObserver::GetIID())) {
    *aInstancePtr = (void*) ((nsIDocumentLoaderObserver*)this);
    NS_ADDREF_THIS();
    return NS_OK;
  }

  if (aIID.Equals( nsIObserver::GetIID())) {
    *aInstancePtr = (void*) ((nsIObserver*)this);
    NS_ADDREF_THIS();
    return NS_OK;
  }

  return nsBaseAppCore::QueryInterface(aIID, aInstancePtr);
}


NS_IMETHODIMP 
nsBrowserAppCore::GetScriptObject(nsIScriptContext *aContext, void** aScriptObject)
{
  NS_PRECONDITION(nsnull != aScriptObject, "null arg");
  nsresult res = NS_OK;
  if (nsnull == mScriptObject) 
  {
      res = NS_NewScriptBrowserAppCore(aContext, 
                                (nsISupports *)(nsIDOMBrowserAppCore*)this, 
                                nsnull, 
                                &mScriptObject);
  }

  *aScriptObject = mScriptObject;
  return res;
}

static
nsIPresShell*
GetPresShellFor(nsIWebShell* aWebShell)
{
  nsIPresShell* shell = nsnull;
  if (nsnull != aWebShell) {
    nsIContentViewer* cv = nsnull;
    aWebShell->GetContentViewer(&cv);
    if (nsnull != cv) {
      nsIDocumentViewer* docv = nsnull;
      cv->QueryInterface(kIDocumentViewerIID, (void**) &docv);
      if (nsnull != docv) {
        nsIPresContext* cx;
        docv->GetPresContext(cx);
	      if (nsnull != cx) {
	        cx->GetShell(&shell);
	        NS_RELEASE(cx);
	      }
        NS_RELEASE(docv);
      }
      NS_RELEASE(cv);
    }
  }
  return shell;
}

NS_IMETHODIMP    
nsBrowserAppCore::Init(const nsString& aId)
{
   
  nsBaseAppCore::Init(aId);



  // register object into Service Manager
  static NS_DEFINE_IID(kIDOMAppCoresManagerIID, NS_IDOMAPPCORESMANAGER_IID);
  static NS_DEFINE_IID(kAppCoresManagerCID,  NS_APPCORESMANAGER_CID);

  nsIDOMAppCoresManager * appCoreManager;
  nsresult rv = nsServiceManager::GetService(kAppCoresManagerCID,
                                             kIDOMAppCoresManagerIID,
                                             (nsISupports**)&appCoreManager);
  if (NS_SUCCEEDED(rv)) {
	  appCoreManager->Add((nsIDOMBaseAppCore *)(nsBaseAppCore *)this);
    nsServiceManager::ReleaseService(kAppCoresManagerCID, appCoreManager);
  }

  rv = nsComponentManager::CreateInstance(kCSessionHistoryCID,
                                          nsnull,
                                          kISessionHistoryIID,
                                          (void **) &mSHistory);
  if (NS_FAILED(rv)) return rv;

  BeginObserving();

  return rv;

}

NS_IMETHODIMP    
nsBrowserAppCore::SetDocumentCharset(const nsString& aCharset)
{
  nsCOMPtr<nsIScriptGlobalObject> globalObj( do_QueryInterface(mContentWindow) );
  if (!globalObj) {
    return NS_ERROR_FAILURE;
  }

  nsIWebShell * webShell;
  globalObj->GetWebShell(&webShell);
  if (nsnull != webShell) {
    webShell->SetDefaultCharacterSet( aCharset.GetUnicode());

    NS_RELEASE(webShell);
  }

  return NS_OK;
}

NS_IMETHODIMP    
nsBrowserAppCore::Back()
{
  GoBack(mContentAreaWebShell);
	return NS_OK;
}

NS_IMETHODIMP
#ifdef NECKO
nsBrowserAppCore::Reload(nsLoadFlags flags)
#else
nsBrowserAppCore::Reload(PRInt32  aType)
#endif
{
#ifdef  NECKO
	if (mContentAreaWebShell)
	   Reload(mContentAreaWebShell, flags);
#else
	if (mContentAreaWebShell)
	   Reload(mContentAreaWebShell, (nsURLReloadType) aType);
#endif /* NECKO */
	return NS_OK;
}   

NS_IMETHODIMP
nsBrowserAppCore::Forward()
{
  GoForward(mContentAreaWebShell);
	return NS_OK;
}



NS_IMETHODIMP    
nsBrowserAppCore::Stop()
{
  mContentAreaWebShell->Stop();
  setAttribute( mWebShell, "Browser:Throbber", "busy", "false" );
	return NS_OK;
}

#ifdef ClientWallet
#define WALLET_EDITOR_NAME "walleted.html"

#define WALLET_SAMPLES_URL "http://people.netscape.com/morse/wallet/samples/"
//#define WALLET_SAMPLES_URL "http://peoplestage/morse/wallet/samples/"

nsresult ProfileDirectory(nsFileSpec& dirSpec) {
  nsIFileSpec* spec = NS_LocateFileOrDirectory(
  						nsSpecialFileSpec::App_UserProfileDirectory50);
  if (!spec)
  	return NS_ERROR_FAILURE;
  return spec->GetFileSpec(&dirSpec);
}



NS_IMETHODIMP
nsBrowserAppCore::GotoHistoryIndex(PRInt32 aIndex)
{
    printf("In nsBrowserAppCOre::gotoHistoryIndex\n");
    Goto(aIndex, mContentAreaWebShell, PR_FALSE);
	return NS_OK;

}

NS_IMETHODIMP
nsBrowserAppCore::BackButtonPopup()
{


 // Get handle to the "backbuttonpopup" element
  nsCOMPtr<nsIDOMElement>   backPopupElement;
  nsresult rv = FindNamedXULElement(mWebShell, "backbuttonpopup", &backPopupElement);

  if (!NS_SUCCEEDED(rv) ||  !backPopupElement)
  {
     return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIDOMNode> backPopupNode(do_QueryInterface(backPopupElement)); 
  if (!backPopupNode) {
    return NS_ERROR_FAILURE;
  }

  nsString  name;
  backPopupNode->GetNodeName(name);
  if (APP_DEBUG) printf("Popup Node name = %s\n", name.ToNewCString());

  
  //get handle to the Menu item under popup
  nsCOMPtr<nsIDOMNode>   menu;
  backPopupNode->GetFirstChild(getter_AddRefs(menu));
  if (!menu) {
	   printf("Call to GetFirstChild failed\n");
     return NS_ERROR_FAILURE;
  }
  

  menu->GetNodeName(name);
  if (APP_DEBUG) printf("First child name = %s\n", name.ToNewCString());
  PRBool hasChildren=PR_FALSE;

  // Check if menu has children. If so, remove them.
  menu->HasChildNodes(&hasChildren);
  if (hasChildren) {
	  ClearHistoryPopup(menu);
  }    // hasChildren
  else {
	 if (APP_DEBUG) printf("Menu has no children\n");
  }
	 
             
      /* Now build the popup list */
  if (!mSHistory) {
	  printf("mSHistory is null\n");
     return NS_ERROR_FAILURE;
  }

  PRInt32 indix=0, i=0;
  //Get current index in Session History
  mSHistory->getCurrentIndex(indix);

  //Decide on the # of items in the popup list 
  if (indix > SHISTORY_POPUP_LIST)
     i  = indix-SHISTORY_POPUP_LIST;

  for (PRInt32 j=indix-1;j>=i;j--) {
      const PRUnichar * url=nsnull, *title=nsnull;

      mSHistory->GetURLForIndex(j, &url);
      nsAutoString  histURL(url);
      mSHistory->GetTitleForIndex(j, &title);
      nsAutoString  histTitle(title);
      if (APP_DEBUG) printf("URL = %s, TITLE = %s\n", histURL.ToNewCString(), histTitle.ToNewCString());
      CreateMenuItem(menu, j, url);
     } 

  return NS_OK;

}



NS_IMETHODIMP nsBrowserAppCore::CreateMenuItem(
  nsIDOMNode *    aParentMenu,
  PRInt32      aIndex,
  const PRUnichar *  aName)
{
   if (APP_DEBUG) printf("In CreateMenuItem\n");
  nsresult rv=NS_OK;  
  nsCOMPtr<nsIDOMDocument>  doc;

  aParentMenu->GetOwnerDocument(getter_AddRefs(doc));
  if (!NS_SUCCEEDED(rv)) {
	  printf("&&&& Error Getting handle to the document &&&&\n");
	  return NS_ERROR_FAILURE;
  }
  nsString menuitemName(aName);

  
  // Create nsMenuItem
  nsCOMPtr<nsIDOMElement>  menuItemElement;
  nsString  tagName("menuitem");
  rv = doc->CreateElement(tagName, getter_AddRefs(menuItemElement));

  if (!NS_SUCCEEDED(rv)) {
	  printf("***** Error creating the menu item element *****\n");
	  return NS_ERROR_FAILURE;
  }

  // Make a DOMNode out of it
  nsCOMPtr<nsIDOMNode>  menuItemNode = do_QueryInterface(menuItemElement);

  if (!menuItemNode) {
	  printf("***** Error converting DOMElement to DOMNode *****\n");
	  return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIDOMNode> resultNode;
  // Make nsMenuItem a child of nsMenu
  rv = aParentMenu->AppendChild(menuItemNode, getter_AddRefs(resultNode));

  if (!NS_SUCCEEDED(rv)) 
  {
       printf(" **** Error appending menuitem to menu *****\n");
	   return NS_ERROR_FAILURE;
  }

  //Set the label for the menu item
  nsString menuitemlabel(aName);
  if (APP_DEBUG) printf("Setting menu name to %s\n", menuitemlabel.ToNewCString());
  rv = menuItemElement->SetAttribute(nsString("value"), menuitemlabel);
  if (!NS_SUCCEEDED(rv))
	  printf("**** Error Setting node value for menu item ****\n");

  //Set the onaction attribute
  nsString menuitemCmd("gotoHistoryIndex(");
  menuitemCmd.Append(aIndex);
  menuitemCmd += ")";  
  const char * actionhandler = menuitemCmd.ToNewCString();

  if (APP_DEBUG) printf("Setting action handler to %s\n", menuitemCmd.ToNewCString());
  nsString attrName("onaction");
  rv = menuItemElement->SetAttribute(attrName, menuitemCmd);
  if (!NS_SUCCEEDED(rv))
	  printf("ERROR setting onaction handler\n");
  
  
  // Set the hist attribute to true
  rv = menuItemElement->SetAttribute(nsString("ishist"), nsString("true"));
  if (!NS_SUCCEEDED(rv))
	  printf("ERROR setting ishist handler\n");
  

  return NS_OK;
}



NS_IMETHODIMP
nsBrowserAppCore::ForwardButtonPopup()
{


     printf("Inside ForwardButtonPopup\n");
	 
  if (APP_DEBUG) printf("In BrowserAppCore::Forwardbuttonpopup\n");

 // Get handle to the "forwardbuttonpopup" element
  nsCOMPtr<nsIDOMElement>   forwardPopupElement;
  nsresult rv = FindNamedXULElement(mWebShell, "forwardbuttonpopup", &forwardPopupElement);

  if (!NS_SUCCEEDED(rv) ||  !forwardPopupElement)
  {
	 printf("Couldn't get handle to forwardPopupElement\n");
     return NS_ERROR_FAILURE;
  }

  // Make a nsIDOMNode out of it
  nsCOMPtr<nsIDOMNode> forwardPopupNode(do_QueryInterface(forwardPopupElement)); 
  if (!forwardPopupNode) {
	  printf("Couldn't make a node out of forwardpopupelement\n");
    return NS_ERROR_FAILURE;
  }

  nsString  name;
  forwardPopupNode->GetNodeName(name);
  if (APP_DEBUG) printf("Popup Node name = %s\n", name.ToNewCString());

  
  //get handle to the Menu item under popup
  nsCOMPtr<nsIDOMNode>   menu;
  forwardPopupNode->GetFirstChild(getter_AddRefs(menu));
  if (!menu) {
	  printf("Call to GetFirstChild failed\n");
     return NS_ERROR_FAILURE;
  }
  
  menu->GetNodeName(name);
  if (APP_DEBUG) printf("First child name = %s\n", name.ToNewCString());
  PRBool hasChildren=PR_FALSE;

  // Check if menu has children. If so, remove them.
  menu->HasChildNodes(&hasChildren);
  if (hasChildren) {
	  // Remove all old entries 
	  ClearHistoryPopup(menu);
  }    // hasChildren
  else {
	  if (APP_DEBUG) printf("Menu has no children\n");
  }	 
             
      /* Now build the popup list */
  if (!mSHistory) {
	  printf("mSHistory is null\n");
     return NS_ERROR_FAILURE;
  }
  PRInt32 indix=0, i=0, length=0;
  //Get current index in Session History
  mSHistory->getCurrentIndex(indix);
 //Get total length of Session History
  mSHistory->getHistoryLength(length);

  //Decide on the # of items in the popup list 
  if ((length-indix) > SHISTORY_POPUP_LIST)
     i  = indix+SHISTORY_POPUP_LIST;
  else
	 i = length;

  for (PRInt32 j=indix+1;j<i;j++) {
      const PRUnichar * url=nsnull, *title=nsnull;

      mSHistory->GetURLForIndex(j, &url);
      nsAutoString  histURL(url);
      mSHistory->GetTitleForIndex(j, &title);
      nsAutoString  histTitle(title);
      if (APP_DEBUG) printf("URL = %s, TITLE = %s\n", histURL.ToNewCString(), histTitle.ToNewCString());
      CreateMenuItem(menu, j, url);
  } 
	 return NS_OK;

}


NS_IMETHODIMP
nsBrowserAppCore::UpdateGoMenu()
{

  printf("inside Updategomenu\n");
 // Get handle to the "backbuttonpopup" element
  nsCOMPtr<nsIDOMElement>   mainToolboxElement;
  nsresult rv = FindNamedXULElement(mWebShell, "main-toolbox", &mainToolboxElement);

  if (!NS_SUCCEEDED(rv) ||  !mainToolboxElement)
  {
	 printf("Couldn't get handle to Go menu\n");
     return NS_ERROR_FAILURE;
  }
  else {
	  if (APP_DEBUG) printf("Got handle to the main-toolbox element\n");
	
  }
  nsCOMPtr<nsIDOMNode> mainToolboxNode(do_QueryInterface(mainToolboxElement)); 
  if (!mainToolboxNode) {
    return NS_ERROR_FAILURE;
  }
  nsString  name;
  mainToolboxElement->GetAttribute(nsAutoString("value"), name);
  if (APP_DEBUG) printf("Main Toolbox Node lable = %s\n", name.ToNewCString());




   //get handle to the menubar item under popup
  nsCOMPtr<nsIDOMNode>   menubar;
  mainToolboxNode->GetFirstChild(getter_AddRefs(menubar));
  if (!menubar) {
     printf("Call to get menubar failed\n");
     return NS_ERROR_FAILURE;
  }
  nsCOMPtr<nsIDOMElement> menubarElement(do_QueryInterface(menubar));
  if (!menubarElement) {
	printf("Could n't get DOMElement out of DOMNode for menubar\n");
	return NS_ERROR_FAILURE;
  }
  nsString fcName;
  menubarElement->GetAttribute(nsAutoString("value"), fcName);
  if (APP_DEBUG) printf("Main Toolbox's first child name  = %s\n", fcName.ToNewCString());

  nsCOMPtr<nsIDOMNode> goMenuNode;
    // get handle to the go menu under toolbar
   PRBool hasChildren=PR_FALSE;
  // Check if toolbar has children.
  menubar->HasChildNodes(&hasChildren);
  if (hasChildren) {	
     nsIDOMNodeList *   childList=nsnull;

     //Get handle to the children list
     rv = menubar->GetChildNodes(&childList);
     if (NS_SUCCEEDED(rv) && childList) {
        PRInt32 ccount=0;
        childList->GetLength((unsigned int *)&ccount);
        
		// Get the 'Go' menu
        for (PRInt32 i=0; i<ccount; i++) {
            nsIDOMNode * child=nsnull;
            rv = childList->Item(i, &child);
			// Get element out of the node
			nsCOMPtr<nsIDOMElement> childElement(do_QueryInterface(child));
			if (!childElement) {
				printf("Could n't get DOMElement out of DOMNode for child\n");
				return NS_ERROR_FAILURE;
			}
			nsString nodelabel;
            rv = childElement->GetAttribute(nsAutoString("value"), nodelabel);
			if (APP_DEBUG) printf("Node Name for menu = %s\n", nodelabel.ToNewCString());
			if (!NS_SUCCEEDED(rv)) {
				printf("Couldn't get node name\n");
				return NS_ERROR_FAILURE;
			}
			nsString nodeid;
            rv = childElement->GetAttribute(nsAutoString("id"), nodeid);
			if (nodeid == "gomenu") {
				goMenuNode = child;
				break;
			}
        } //(for)	
     }   // if (childList)
	 NS_RELEASE(childList);
  }    // hasChildren
  else {
	  if (APP_DEBUG) printf("Menubar has no children\n");
  }

  if (!goMenuNode) {
     printf("Couldn't find Go Menu. returning\n");
	 return NS_ERROR_FAILURE;
 
  }

    //get handle to the menupopup under gomenu
  nsCOMPtr<nsIDOMNode>   menuPopup;
  goMenuNode->GetFirstChild(getter_AddRefs(menuPopup));
  if (!menuPopup) {
	  printf("Call to get menupopup under go menu failed\n");
     return NS_ERROR_FAILURE;
  }
    nsCOMPtr<nsIDOMElement> menuPopupElement(do_QueryInterface(menuPopup));
  if (!menuPopupElement) {
	printf("Could n't get DOMElement out of DOMNode for menuPopup\n");
	return NS_ERROR_FAILURE;
  }
			nsString nodelabel;
            rv = menuPopupElement->GetAttribute(nsAutoString("value"), nodelabel);
	if (APP_DEBUG) printf("Node Name for menu popup = %s\n", nodelabel.ToNewCString());
  // Clear all history children under Go menu
  ClearHistoryPopup(menuPopup);

  /* Now build the history list */
  if (!mSHistory) {
	  printf("mSHistory is null\n");
     return NS_ERROR_FAILURE;
  }
  
  PRInt32 length=0,i=0;
  //Get total length of the  Session History
  mSHistory->getHistoryLength(length);

  //Decide on the # of items in the popup list 
  if (length > SHISTORY_POPUP_LIST)
     i  = length-SHISTORY_POPUP_LIST;

  for (PRInt32 j=length-1;j>=i;j--) {
      const PRUnichar * url=nsnull, *title=nsnull;

      mSHistory->GetURLForIndex(j, &url);
      nsAutoString  histURL(url);
      mSHistory->GetTitleForIndex(j, &title);
      nsAutoString  histTitle(title);
      if (APP_DEBUG) printf("URL = %s, TITLE = %s\n", histURL.ToNewCString(), histTitle.ToNewCString());
      CreateMenuItem(goMenuNode, j, url);
     }
  return NS_OK;

}



NS_IMETHODIMP
nsBrowserAppCore::ClearHistoryPopup(nsIDOMNode * aParent)
{

	 nsresult rv;
	 nsIDOMNode * menu = aParent;

     nsIDOMNodeList *   childList=nsnull;

     //Get handle to the children list
     rv = menu->GetChildNodes(&childList);
     if (NS_SUCCEEDED(rv) && childList) {
        PRInt32 ccount=0;
        childList->GetLength((unsigned int *)&ccount);

        // Remove the children that has the 'hist' attribute set to true.
        for (PRInt32 i=0; i<ccount; i++) {
            nsIDOMNode * child=nsnull;
            rv = childList->Item(i, &child);
			// Get element out of the node
			nsCOMPtr<nsIDOMElement> childElement(do_QueryInterface(child));
			if (!childElement) {
				printf("Could n't get DOMElement out of DOMNode for child\n");
				return NS_ERROR_FAILURE;
			}
			nsString  attrname("ishist");
			nsString  attrvalue;
			childElement->GetAttribute(attrname, attrvalue);
			if (attrvalue == "true") {
				// It is a history menu item. Remove it
                nsIDOMNode * ret=nsnull;
			    if (NS_SUCCEEDED(rv) && child) {
             rv = menu->RemoveChild(child, &ret);
			       if (NS_SUCCEEDED(rv)) {
				       if (ret) {
				           if (APP_DEBUG) printf("Child %x removed from the popuplist \n", child);
			             //Child was removed. release it.
                   //  NS_IF_RELEASE(child);
					     }
				       else {
					       printf("Child %x was not removed from popuplist\n", child);
					     }
				     }
			       else
				     {
				        printf("Child %x was not removed from popuplist\n", child);
                return NS_ERROR_FAILURE;
				     }
				  } 
				  NS_IF_RELEASE(ret);
			}  // atrrvalue == true	
			NS_RELEASE(child);
    } //(for)	
  }   // if (childList)
	 NS_RELEASE(childList);
	 return NS_OK;
}


PRInt32
newWind(char* urlName)
{
  nsresult rv;
	  char *  urlstr=urlName;

  /*
   * Create the Application Shell instance...
   */
  NS_WITH_SERVICE(nsIAppShellService, appShell, kAppShellServiceCID, &rv);
  if (NS_FAILED(rv)) return rv;

  /*
   * Post an event to the shell instance to load the AppShell 
   * initialization routines...  
   * 
   * This allows the application to enter its event loop before having to 
   * deal with GUI initialization...
   */
  ///write me...
  nsIURI* url;
  
#ifndef NECKO
  rv = NS_NewURL(&url, urlstr);
#else
  nsIURI *uri = nsnull;
  NS_WITH_SERVICE(nsIIOService, service, kIOServiceCID, &rv);
  if (NS_FAILED(rv)) return rv;

  rv = service->NewURI(urlstr, nsnull, &uri);
  if (NS_FAILED(rv)) return rv;

  rv = uri->QueryInterface(nsIURI::GetIID(), (void**)&url);
  NS_RELEASE(uri);
#endif // NECKO
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsIWebShellWindow> newWindow;
  appShell->CreateTopLevelWindow(nsnull, url, PR_TRUE, PR_TRUE,
              NS_CHROME_ALL_CHROME, nsnull, NS_SIZETOCONTENT, NS_SIZETOCONTENT,
              getter_AddRefs(newWindow));

  NS_RELEASE(url);
  
  return NS_OK;
}

static void DOMWindowToWebShellWindow(
              nsIDOMWindow *DOMWindow,
              nsCOMPtr<nsIWebShellWindow> *webWindow)
{
  if (!DOMWindow)
    return; // with webWindow unchanged -- its constructor gives it a null ptr

  nsCOMPtr<nsIScriptGlobalObject> globalScript(do_QueryInterface(DOMWindow));
  nsCOMPtr<nsIWebShell> webshell, rootWebshell;
  if (globalScript)
    globalScript->GetWebShell(getter_AddRefs(webshell));
  if (webshell)
    webshell->GetRootWebShellEvenIfChrome(*getter_AddRefs(rootWebshell));
  if (rootWebshell) {
    nsCOMPtr<nsIWebShellContainer> webshellContainer;
    rootWebshell->GetContainer(*getter_AddRefs(webshellContainer));
    *webWindow = do_QueryInterface(webshellContainer);
  }
}

NS_IMETHODIMP    
nsBrowserAppCore::WalletEditor(nsIDOMWindow* aWin)
{
    // (code adapted from nsToolkitCore::ShowModal. yeesh.)
    nsresult           rv;
    nsIAppShellService *appShell;
    nsIWebShellWindow  *window;

    window = nsnull;

    nsCOMPtr<nsIURI> urlObj;
    char *urlstr = "resource:/res/samples/WalletEditor.html";
#ifndef NECKO
    rv = NS_NewURL(getter_AddRefs(urlObj), urlstr);
#else
    NS_WITH_SERVICE(nsIIOService, service, kIOServiceCID, &rv);
    if (NS_FAILED(rv)) return rv;

    nsIURI *uri = nsnull;
    rv = service->NewURI(urlstr, nsnull, &uri);
    if (NS_FAILED(rv)) return rv;

    rv = uri->QueryInterface(nsIURI::GetIID(), (void**)&urlObj);
    NS_RELEASE(uri);
#endif // NECKO
    if (NS_FAILED(rv))
        return rv;

    rv = nsServiceManager::GetService(kAppShellServiceCID, kIAppShellServiceIID,
                                    (nsISupports**) &appShell);
    if (NS_FAILED(rv))
        return rv;

    // Create "save to disk" nsIXULCallbacks...
    //nsIXULWindowCallbacks *cb = new nsFindDialogCallbacks( aURL, aContentType );
    nsIXULWindowCallbacks *cb = nsnull;

    nsCOMPtr<nsIWebShellWindow> parent;
    DOMWindowToWebShellWindow(aWin, &parent);
    window = nsnull;
    appShell->CreateTopLevelWindow(parent, urlObj, PR_TRUE, PR_TRUE,
                              NS_CHROME_ALL_CHROME | NS_CHROME_OPEN_AS_DIALOG,
                              cb, 504, 436, &window);
    if (window != nsnull) {
      appShell->RunModalDialog(&window, parent, nsnull, NS_CHROME_ALL_CHROME,
                               cb, 504, 436);
      NS_RELEASE(window);
    }
    nsServiceManager::ReleaseService(kAppShellServiceCID, appShell);

    return rv;
}

NS_IMETHODIMP    
nsBrowserAppCore::SignonViewer(nsIDOMWindow* aWin)
{
    // (code adapted from nsToolkitCore::ShowModal. yeesh.)
    nsresult           rv;
    nsIAppShellService *appShell;
    nsIWebShellWindow  *window;

    window = nsnull;

    nsCOMPtr<nsIURI> urlObj;
    char * urlstr = "resource:/res/samples/SignonViewer.html";
#ifndef NECKO
    rv = NS_NewURL(getter_AddRefs(urlObj), urlstr);
#else
    NS_WITH_SERVICE(nsIIOService, service, kIOServiceCID, &rv);
    if (NS_FAILED(rv)) return rv;

    nsIURI *uri = nsnull;
    rv = service->NewURI(urlstr, nsnull, &uri);
    if (NS_FAILED(rv)) return rv;

    rv = uri->QueryInterface(nsIURI::GetIID(), (void**)&urlObj);
    NS_RELEASE(uri);
#endif // NECKO
    if (NS_FAILED(rv))
        return rv;

    rv = nsServiceManager::GetService(kAppShellServiceCID, kIAppShellServiceIID,
                                    (nsISupports**) &appShell);
    if (NS_FAILED(rv))
        return rv;

    // Create "save to disk" nsIXULCallbacks...
    //nsIXULWindowCallbacks *cb = new nsFindDialogCallbacks( aURL, aContentType );
    nsIXULWindowCallbacks *cb = nsnull;

    nsCOMPtr<nsIWebShellWindow> parent;
    DOMWindowToWebShellWindow(aWin, &parent);
    window = nsnull;
    appShell->CreateTopLevelWindow(parent, urlObj, PR_TRUE, PR_TRUE,
                                 NS_CHROME_ALL_CHROME | NS_CHROME_OPEN_AS_DIALOG,
                                 cb, 504, 436, &window);
    if (window != nsnull) {
      appShell->RunModalDialog(&window, parent, nsnull, NS_CHROME_ALL_CHROME,
                               cb, 504, 436);
      NS_RELEASE(window);
    }
    nsServiceManager::ReleaseService(kAppShellServiceCID, appShell);
    return rv;
}

NS_IMETHODIMP    
nsBrowserAppCore::CookieViewer(nsIDOMWindow* aWin)
{
    // (code adapted from nsToolkitCore::ShowModal. yeesh.)
    nsresult           rv;
    nsIAppShellService *appShell;
    nsIWebShellWindow  *window;

    window = nsnull;

    nsCOMPtr<nsIURI> urlObj;
    char *urlstr = "resource:/res/samples/CookieViewer.html";
#ifndef NECKO
    rv = NS_NewURL(getter_AddRefs(urlObj), urlstr);
#else
    NS_WITH_SERVICE(nsIIOService, service, kIOServiceCID, &rv);
    if (NS_FAILED(rv)) return rv;

    nsIURI *uri = nsnull;
    rv = service->NewURI(urlstr, nsnull, &uri);
    if (NS_FAILED(rv)) return rv;

    rv = uri->QueryInterface(nsIURI::GetIID(), (void**)&urlObj);
    NS_RELEASE(uri);
#endif // NECKO
    if (NS_FAILED(rv))
        return rv;

    rv = nsServiceManager::GetService(kAppShellServiceCID, kIAppShellServiceIID,
                                    (nsISupports**) &appShell);
    if (NS_FAILED(rv))
        return rv;

    // Create "save to disk" nsIXULCallbacks...
    //nsIXULWindowCallbacks *cb = new nsFindDialogCallbacks( aURL, aContentType );
    nsIXULWindowCallbacks *cb = nsnull;

    nsCOMPtr<nsIWebShellWindow> parent;
    DOMWindowToWebShellWindow(aWin, &parent);
    window = nsnull;
    appShell->CreateTopLevelWindow(parent, urlObj, PR_TRUE, PR_TRUE,
                              NS_CHROME_ALL_CHROME | NS_CHROME_OPEN_AS_DIALOG,
                              cb, 504, 436, &window);

    if (window != nsnull) {
      appShell->RunModalDialog(&window, parent, nsnull, NS_CHROME_ALL_CHROME,
                               cb, 504, 436);
      NS_RELEASE(window);
    }

    nsServiceManager::ReleaseService(kAppShellServiceCID, appShell);
    return rv;
}

NS_IMETHODIMP    
nsBrowserAppCore::WalletPreview(nsIDOMWindow* aWin, nsIDOMWindow* aForm)
{
  NS_PRECONDITION(aForm != nsnull, "null ptr");
  if (! aForm)
    return NS_ERROR_NULL_POINTER;

  nsIPresShell* shell;
  shell = nsnull;
  nsCOMPtr<nsIWebShell> webcontent; 

  nsCOMPtr<nsIScriptGlobalObject> scriptGlobalObject; 
  scriptGlobalObject = do_QueryInterface(aForm); 
  scriptGlobalObject->GetWebShell(getter_AddRefs(webcontent)); 

  shell = GetPresShellFor(webcontent);
  nsIWalletService *walletservice;
  nsresult res = nsServiceManager::GetService(kWalletServiceCID,
                                     kIWalletServiceIID,
                                     (nsISupports **)&walletservice);
  if (NS_SUCCEEDED(res) && (nsnull != walletservice)) {
    res = walletservice->WALLET_Prefill(shell, PR_FALSE);
    nsServiceManager::ReleaseService(kWalletServiceCID, walletservice);
    if (NS_FAILED(res)) { /* this just means that there was nothing to prefill */
      return NS_OK;
    }
  } else {
    return res;
  }


    // (code adapted from nsToolkitCore::ShowModal. yeesh.)
    nsresult           rv;
    nsIAppShellService *appShell;
    nsIWebShellWindow  *window;

    window = nsnull;

    nsCOMPtr<nsIURI> urlObj;
    char * urlstr = "resource:/res/samples/WalletPreview.html";
#ifndef NECKO
    rv = NS_NewURL(getter_AddRefs(urlObj), urlstr);
#else
    NS_WITH_SERVICE(nsIIOService, service, kIOServiceCID, &rv);
    if (NS_FAILED(rv)) return rv;

    nsIURI *uri = nsnull;
    rv = service->NewURI(urlstr, nsnull, &uri);
    if (NS_FAILED(rv)) return rv;

    rv = uri->QueryInterface(nsIURI::GetIID(), (void**)&urlObj);
    NS_RELEASE(uri);
#endif // NECKO
    if (NS_FAILED(rv))
        return rv;

    rv = nsServiceManager::GetService(kAppShellServiceCID, kIAppShellServiceIID,
                                    (nsISupports**) &appShell);
    if (NS_FAILED(rv))
        return rv;

    // Create "save to disk" nsIXULCallbacks...
    //nsIXULWindowCallbacks *cb = new nsFindDialogCallbacks( aURL, aContentType );
    nsIXULWindowCallbacks *cb = nsnull;

    nsCOMPtr<nsIWebShellWindow> parent;
    DOMWindowToWebShellWindow(aWin, &parent);
    window = nsnull;
    appShell->CreateTopLevelWindow(parent, urlObj, PR_TRUE, PR_TRUE,
                              NS_CHROME_ALL_CHROME | NS_CHROME_OPEN_AS_DIALOG,
                              cb, 504, 436, &window);
    if (window != nsnull) {
      appShell->RunModalDialog(&window, parent, nsnull, NS_CHROME_ALL_CHROME,
                               cb, 504, 436);
      NS_RELEASE(window);
    }
    nsServiceManager::ReleaseService(kAppShellServiceCID, appShell);

    return rv;
}

NS_IMETHODIMP    
nsBrowserAppCore::WalletChangePassword()
{
  nsIWalletService *walletservice;
  nsresult res;
  res = nsServiceManager::GetService(kWalletServiceCID,
                                     kIWalletServiceIID,
                                     (nsISupports **)&walletservice);
  if ((NS_OK == res) && (nsnull != walletservice)) {
    res = walletservice->WALLET_ChangePassword();
    nsServiceManager::ReleaseService(kWalletServiceCID, walletservice);
  }
  return NS_OK;
}

#include "nsIDOMHTMLDocument.h"
static NS_DEFINE_IID(kIDOMHTMLDocumentIID, NS_IDOMHTMLDOCUMENT_IID);
NS_IMETHODIMP    
nsBrowserAppCore::WalletQuickFillin(nsIDOMWindow* aWin)
{
  NS_PRECONDITION(aWin != nsnull, "null ptr");
  if (! aWin)
    return NS_ERROR_NULL_POINTER;

  nsIPresShell* shell;
  shell = nsnull;
  nsCOMPtr<nsIWebShell> webcontent; 

  nsCOMPtr<nsIScriptGlobalObject> scriptGlobalObject; 
  scriptGlobalObject = do_QueryInterface(aWin); 
  scriptGlobalObject->GetWebShell(getter_AddRefs(webcontent)); 

  shell = GetPresShellFor(webcontent);
  nsIWalletService *walletservice;
  nsresult res;
  res = nsServiceManager::GetService(kWalletServiceCID,
                                     kIWalletServiceIID,
                                     (nsISupports **)&walletservice);
  if ((NS_OK == res) && (nsnull != walletservice)) {
    res = walletservice->WALLET_Prefill(shell, PR_TRUE);
    nsServiceManager::ReleaseService(kWalletServiceCID, walletservice);
    return NS_OK;
  } else {
    return res;
  }
}

NS_IMETHODIMP    
nsBrowserAppCore::WalletRequestToCapture(nsIDOMWindow* aWin)
{
  NS_PRECONDITION(aWin != nsnull, "null ptr");
  if (! aWin)
    return NS_ERROR_NULL_POINTER;

  nsIPresShell* shell;
  shell = nsnull;
  nsCOMPtr<nsIWebShell> webcontent; 

  nsCOMPtr<nsIScriptGlobalObject> scriptGlobalObject; 
  scriptGlobalObject = do_QueryInterface(aWin); 
  scriptGlobalObject->GetWebShell(getter_AddRefs(webcontent)); 

  shell = GetPresShellFor(webcontent);
  nsIWalletService *walletservice;
  nsresult res;
  res = nsServiceManager::GetService(kWalletServiceCID,
                                     kIWalletServiceIID,
                                     (nsISupports **)&walletservice);
  if ((NS_OK == res) && (nsnull != walletservice)) {
    res = walletservice->WALLET_RequestToCapture(shell);
    nsServiceManager::ReleaseService(kWalletServiceCID, walletservice);
    return NS_OK;
  } else {
    return res;
  }
}

NS_IMETHODIMP    
nsBrowserAppCore::WalletSamples()
{
  /* bring up the samples in a new window */
  mContentAreaWebShell->LoadURL(nsString(WALLET_SAMPLES_URL).GetUnicode(), nsnull, nsnull);
  return NS_OK;
}

#else
NS_IMETHODIMP
nsBrowserAppCore::WalletEditor() {
  return NS_OK;
}
NS_IMETHODIMP    
nsBrowserAppCore::WalletSamples() {
  return NS_OK;
}
NS_IMETHODIMP
nsBrowserAppCore::WalletChangePassword() {
  return NS_OK;
}
NS_IMETHODIMP
nsBrowserAppCore::WalletRequestToCapture(nsIDOMWindow*) {
  return NS_OK;
}
NS_IMETHODIMP
nsBrowserAppCore::WalletQuickFillin(nsIDOMWindow*) {
  return NS_OK;
}
NS_IMETHODIMP
nsBrowserAppCore::WalletSafeFillin(nsIDOMWindow*, nsIDOMWindow*) {
  return NS_OK;
}
#endif

NS_IMETHODIMP    
nsBrowserAppCore::LoadUrl(const nsString& aUrl)
{
  nsresult rv = NS_OK;
  char * urlstr = nsnull;
  urlstr = aUrl.ToNewCString();

  if (!urlstr)
    return NS_OK;

  /* Ask nsWebShell to load the URl */
  nsString id;
  GetId(id);
  if ( id.Find("ViewSource") == 0 ) {
    // Viewing source, load with "view-source" command.
    rv = mContentAreaWebShell->LoadURL(nsString(urlstr).GetUnicode(), "view-source", nsnull, PR_FALSE );
  } else {
    // Normal browser.
    rv = mContentAreaWebShell->LoadURL(nsString(urlstr).GetUnicode());
  }

  delete[] urlstr;

  return rv;
}

NS_IMETHODIMP    
nsBrowserAppCore::LoadInitialPage(void)
{
  static PRBool cmdLineURLUsed = PR_FALSE;
  char * urlstr = nsnull;
  nsresult rv;
  nsICmdLineService * cmdLineArgs;

  
  // Examine content URL.
  if ( mContentAreaWebShell ) {
      const PRUnichar *url = 0;
      rv = mContentAreaWebShell->GetURL( 0, &url );
	  /* Check whether url is valid. Otherwise we compare 0x00 with 
	   * "about:blank" and there by return from here with out 
	   * loading the command line url or default home page.
	   */
      if ( NS_SUCCEEDED( rv ) && url ) {
          if ( nsString(url) != "about:blank" ) {
              // Something has already been loaded (probably via window.open),
              // leave it be.
              return NS_OK;
          }
      }
  }

  rv = nsServiceManager::GetService(kCmdLineServiceCID,
                                    kICmdLineServiceIID,
                                    (nsISupports **)&cmdLineArgs);
  if (NS_FAILED(rv)) {
    if (APP_DEBUG) fprintf(stderr, "Could not obtain CmdLine processing service\n");
    return NS_ERROR_FAILURE;
  }

  // Get the URL to load
  rv = cmdLineArgs->GetURLToLoad(&urlstr);
  nsServiceManager::ReleaseService(kCmdLineServiceCID, cmdLineArgs);

  if ( !cmdLineURLUsed && urlstr != nsnull) {
  // A url was provided. Load it
     if (APP_DEBUG) printf("Got Command line URL to load %s\n", urlstr);
     rv = LoadUrl(nsString(urlstr));
     cmdLineURLUsed = PR_TRUE;
     return rv;
  }

  // No URL was provided in the command line. Load the default provided
  // in the navigator.xul;

  // but first, abort if the window doesn't want a default page loaded
  if (mWebShellWin) {
    PRBool loadDefault;
    mWebShellWin->ShouldLoadDefaultPage(&loadDefault);
    if (!loadDefault)
      return NS_OK;
  }

  nsCOMPtr<nsIDOMElement>    argsElement;

  rv = FindNamedXULElement(mWebShell, "args", &argsElement);
  if (!argsElement) {
  // Couldn't get the "args" element from the xul file. Load a blank page
     if (APP_DEBUG) printf("Couldn't find args element\n");
     nsString * url = new nsString("about:blank"); 
     rv = LoadUrl(nsString(urlstr));
     return rv;
  }

  // Load the default page mentioned in the xul file.
    nsString value;
    argsElement->GetAttribute(nsString("value"), value);
    if ((value.ToNewCString()) != "") {
        if (APP_DEBUG) printf("Got args value from xul file to be %s\n", value.ToNewCString());
    rv = LoadUrl(value);
    return rv;
    }

    if (APP_DEBUG) printf("Quitting LoadInitialPage\n");
    return NS_OK;
}

NS_IMETHODIMP    
nsBrowserAppCore::SetToolbarWindow(nsIDOMWindow* aWin)
{
  NS_PRECONDITION(aWin != nsnull, "null ptr");
  if (! aWin)
    return NS_ERROR_NULL_POINTER;

  mToolbarWindow = aWin;
  // NS_ADDREF(aWin);			 WE DO NOT OWN THIS
  
  // we do not own the script context, so don't addref it
  nsCOMPtr<nsIScriptContext>	scriptContext = getter_AddRefs(GetScriptContext(aWin));
  mToolbarScriptContext = scriptContext;

	return NS_OK;
}

NS_IMETHODIMP    
nsBrowserAppCore::SetContentWindow(nsIDOMWindow* aWin)
{
  NS_PRECONDITION(aWin != nsnull, "null ptr");
  if (! aWin)
    return NS_ERROR_NULL_POINTER;

  mContentWindow = aWin;
  // NS_ADDREF(aWin); WE DO NOT OWN THIS


  // we do not own the script context, so don't addref it
  nsCOMPtr<nsIScriptContext>	scriptContext = getter_AddRefs(GetScriptContext(aWin));
  mContentScriptContext = scriptContext;

  nsCOMPtr<nsIScriptGlobalObject> globalObj( do_QueryInterface(mContentWindow) );
  if (!globalObj) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIWebShell> webShell;
  globalObj->GetWebShell(getter_AddRefs(webShell));
  if (webShell) {
    mContentAreaWebShell = webShell;
    // NS_ADDREF(mContentAreaWebShell); WE DO NOT OWN THIS
    webShell->SetDocLoaderObserver((nsIDocumentLoaderObserver *)this);
    webShell->SetSessionHistory((nsISessionHistory *)this);

    const PRUnichar * name;
    webShell->GetName( &name);
    nsAutoString str(name);

    if (APP_DEBUG) {
      printf("Attaching to Content WebShell [%s]\n", (const char *)nsAutoCString(str));
    }
  }

  return NS_OK;

}



NS_IMETHODIMP    
nsBrowserAppCore::SetWebShellWindow(nsIDOMWindow* aWin)
{
  NS_PRECONDITION(aWin != nsnull, "null ptr");
  if (! aWin)
    return NS_ERROR_NULL_POINTER;

  nsCOMPtr<nsIScriptGlobalObject> globalObj( do_QueryInterface(aWin) );
  if (!globalObj) {
    return NS_ERROR_FAILURE;
  }

  nsIWebShell * webShell;
  globalObj->GetWebShell(&webShell);
  if (nsnull != webShell) {
    mWebShell = webShell;
    //NS_ADDREF(mWebShell); WE DO NOT OWN THIS
    const PRUnichar * name;
    webShell->GetName( &name);
    nsAutoString str(name);

    if (APP_DEBUG) {
      printf("Attaching to WebShellWindow[%s]\n", (const char *)nsAutoCString(str));
    }

    nsIWebShellContainer * webShellContainer;
    webShell->GetContainer(webShellContainer);
    if (nsnull != webShellContainer)
    {
    	nsCOMPtr<nsIWebShellWindow> webShellWin;
      if (NS_OK == webShellContainer->QueryInterface(kIWebShellWindowIID, getter_AddRefs(webShellWin)))
      {
        mWebShellWin = webShellWin;		// WE DO NOT OWN THIS
      }
      NS_RELEASE(webShellContainer);
    }
    NS_RELEASE(webShell);
  }
  return NS_OK;
}



// Utility to extract document from a webshell object.
static nsCOMPtr<nsIDocument> getDocument( nsIWebShell *aWebShell ) {
    nsCOMPtr<nsIDocument> result;

    // Get content viewer from the web shell.
    nsCOMPtr<nsIContentViewer> contentViewer;
    nsresult rv = aWebShell ? aWebShell->GetContentViewer(getter_AddRefs(contentViewer))
                            : NS_ERROR_NULL_POINTER;

    if ( contentViewer ) {
        // Up-cast to a document viewer.
        nsCOMPtr<nsIDocumentViewer> docViewer(do_QueryInterface(contentViewer));
        if ( docViewer ) {
            // Get the document from the doc viewer.
            docViewer->GetDocument(*getter_AddRefs(result));
        } else {
            if (APP_DEBUG) printf( "%s %d: Upcast to nsIDocumentViewer failed\n",
                                   __FILE__, (int)__LINE__ );
        }
    } else {
        if (APP_DEBUG) printf( "%s %d: GetContentViewer failed, rv=0x%X\n",
                               __FILE__, (int)__LINE__, (int)rv );
    }
    return result;
}

// Utility to set element attribute.
static nsresult setAttribute( nsIWebShell *shell,
                              const char *id,
                              const char *name,
                              const nsString &value ) {
    nsresult rv = NS_OK;

    nsCOMPtr<nsIDocument> doc = getDocument( shell );

    if ( doc ) {
        // Up-cast.
        nsCOMPtr<nsIDOMXULDocument> xulDoc( do_QueryInterface(doc) );
        if ( xulDoc ) {
            // Find specified element.
            nsCOMPtr<nsIDOMElement> elem;
            rv = xulDoc->GetElementById( id, getter_AddRefs(elem) );
            if ( elem ) {
                // Set the text attribute.
                rv = elem->SetAttribute( name, value );
                if ( rv != NS_OK ) {
                     if (APP_DEBUG) printf("SetAttribute failed, rv=0x%X\n",(int)rv);
                }
            } else {
                if (APP_DEBUG) printf("GetElementByID failed, rv=0x%X\n",(int)rv);
            }
        } else {
          if (APP_DEBUG)   printf("Upcast to nsIDOMXULDocument failed\n");
        }
    } else {
        if (APP_DEBUG) printf("getDocument failed, rv=0x%X\n",(int)rv);
    }
    return rv;
}

// nsIDocumentLoaderObserver methods

NS_IMETHODIMP
nsBrowserAppCore::OnStartDocumentLoad(nsIDocumentLoader* aLoader, nsIURI* aURL, const char* aCommand)
{
  NS_PRECONDITION(aLoader != nsnull, "null ptr");
  if (! aLoader)
    return NS_ERROR_NULL_POINTER;

  NS_PRECONDITION(aURL != nsnull, "null ptr");
  if (! aURL)
    return NS_ERROR_NULL_POINTER;

  nsresult rv;

  // Notify observers that a document load has started in the
  // content window.
  NS_WITH_SERVICE(nsIObserverService, observer, NS_OBSERVERSERVICE_PROGID, &rv);
  if (NS_FAILED(rv)) return rv;

#ifdef NECKO
  char* url;
#else
  const char* url;
#endif
  rv = aURL->GetSpec(&url);
  if (NS_FAILED(rv)) return rv;

  nsAutoString urlStr(url);
#ifdef NECKO
  nsCRT::free(url);
#endif

  nsAutoString kStartDocumentLoad("StartDocumentLoad");
  rv = observer->Notify(mContentWindow,
                        kStartDocumentLoad.GetUnicode(),
                        urlStr.GetUnicode());

  // XXX Ignore rv for now. They are using nsIEnumerator instead of
  // nsISimpleEnumerator.

#ifdef DEBUG_warren
  char* urls;
  aURL->GetSpec(&urls);
  if (gTimerLog == nsnull)
    gTimerLog = PR_NewLogModule("Timer");
  mLoadStartTime = PR_IntervalNow();
  PR_LOG(gTimerLog, PR_LOG_DEBUG, 
         (">>>>> Starting timer for %s\n", urls));
  printf(">>>>> Starting timer for %s\n", urls);
  nsCRT::free(urls);
#endif

  // Kick start the throbber
  setAttribute( mWebShell, "Browser:Throbber", "busy", "true" );

  // Enable the Stop buton
  setAttribute( mWebShell, "canStop", "disabled", "" );

  //Disable the reload button
  setAttribute(mWebShell, "canReload", "disabled", "true");

  return NS_OK;
}


NS_IMETHODIMP
#ifdef NECKO
nsBrowserAppCore::OnEndDocumentLoad(nsIDocumentLoader* aLoader, nsIChannel* channel, nsresult aStatus,
									nsIDocumentLoaderObserver * aObserver)
#else
nsBrowserAppCore::OnEndDocumentLoad(nsIDocumentLoader* aLoader, nsIURI *aUrl, PRInt32 aStatus,
									nsIDocumentLoaderObserver * aObserver)
#endif
{
  NS_PRECONDITION(aLoader != nsnull, "null ptr");
  if (! aLoader)
    return NS_ERROR_NULL_POINTER;

#ifdef NECKO
  NS_PRECONDITION(channel != nsnull, "null ptr");
  if (! channel)
    return NS_ERROR_NULL_POINTER;
#else
  NS_PRECONDITION(aUrl != nsnull, "null ptr");
  if (! aUrl)
    return NS_ERROR_NULL_POINTER;
#endif

  nsresult rv;

  nsIWebShell * aWebShell= nsnull, * parent = nsnull;

  // Notify observers that a document load has started in the
  // content window.
  NS_WITH_SERVICE(nsIObserverService, observer, NS_OBSERVERSERVICE_PROGID, &rv);
  if (NS_FAILED(rv)) return rv;

#ifdef NECKO
  nsCOMPtr<nsIURI> aUrl;
  rv = channel->GetURI(getter_AddRefs(aUrl));
  if (NS_FAILED(rv)) return rv;
  char* url;
#else
  const char* url;
#endif
  rv = aUrl->GetSpec(&url);
  if (NS_FAILED(rv)) return rv;

  nsAutoString urlStr(url);

  nsAutoString kEndDocumentLoad("EndDocumentLoad");
  nsAutoString kFailDocumentLoad("FailDocumentLoad");

  rv = aObserver->QueryInterface(kIWebShellIID, (void **)&aWebShell);

  if (aStatus != NS_OK) {
		  goto done;
  }

  /* If this is a frame, don't do any of the Global History
   * & observer thingy 
   */
  if (aWebShell)
      aWebShell->GetParent(parent);

  if (parent) {
      /* This is a frame */
          goto end;
  }
  rv = observer->Notify(mContentWindow,
                        NS_SUCCEEDED(aStatus) ? kEndDocumentLoad.GetUnicode() : kFailDocumentLoad.GetUnicode(),
                        urlStr.GetUnicode());

  // XXX Ignore rv for now. They are using nsIEnumerator instead of
  // nsISimpleEnumerator.

done:
  // Stop the throbber and set the urlbar string
	if (aStatus == NS_OK)
      setAttribute( mWebShell, "urlbar", "value", url);  

	    /* To satisfy a request from the QA group */
	if (aStatus == NS_OK) {
      fprintf(stdout, "Document %s loaded successfully\n", url);
      fflush(stdout);
	}
	else {
      fprintf(stdout, "Error loading URL %s \n", url);
      fflush(stdout);
	}

end:

#ifdef DEBUG_warren
  nsCOMPtr<nsIURI> aURL;
  channel->GetURI(getter_AddRefs(aURL));
  char* urls;
  aURL->GetSpec(&urls);
  if (gTimerLog == nsnull)
    gTimerLog = PR_NewLogModule("Timer");
  PRIntervalTime end = PR_IntervalNow();
  PRIntervalTime diff = end - mLoadStartTime;
  PR_LOG(gTimerLog, PR_LOG_DEBUG, 
         (">>>>> Stopping timer for %s. Elapsed: %.3f\n", 
          urls, PR_IntervalToMilliseconds(diff) / 1000.0));
  printf(">>>>> Stopping timer for %s. Elapsed: %.3f\n", 
         urls, PR_IntervalToMilliseconds(diff) / 1000.0);
  nsCRT::free(urls);
#endif

  setAttribute( mWebShell, "Browser:Throbber", "busy", "false" );
  PRBool result=PR_TRUE;
  // Check with sessionHistory if you can go forward
  canForward(result);
  setAttribute(mWebShell, "canGoForward", "disabled", (result == PR_TRUE) ? "" : "true");

    // Check with sessionHistory if you can go back
  canBack(result);
  setAttribute(mWebShell, "canGoBack", "disabled", (result == PR_TRUE) ? "" : "true");

    //Disable the Stop button
  setAttribute( mWebShell, "canStop", "disabled", "true" );

	//Enable the reload button
	setAttribute(mWebShell, "canReload", "disabled", "");

	//Update the go menu with history entries. Not ready yet
	//UpdateGoMenu();
#ifdef NECKO
  nsCRT::free(url);
#endif
  return NS_OK;
}

NS_IMETHODIMP
#ifdef NECKO
nsBrowserAppCore::HandleUnknownContentType(nsIDocumentLoader* loader, 
                                           nsIChannel* channel,
                                           const char *aContentType,
                                           const char *aCommand )
#else
nsBrowserAppCore::HandleUnknownContentType(nsIDocumentLoader* loader, 
                                           nsIURI *aURL,
                                           const char *aContentType,
                                           const char *aCommand )
#endif
{
    nsresult rv = NS_OK;

    // Turn off the indicators in the chrome.
    setAttribute( mWebShell, "Browser:Throbber", "busy", "false" );

    // Get "unknown content type handler" and have it handle this.
    nsIUnknownContentTypeHandler *handler;
    rv = nsServiceManager::GetService( NS_IUNKNOWNCONTENTTYPEHANDLER_PROGID,
                                       nsIUnknownContentTypeHandler::GetIID(),
                                       (nsISupports**)&handler );

    if ( NS_SUCCEEDED( rv ) ) {
        /* Have handler take care of this. */
        // Get DOM window.
        nsCOMPtr<nsIDOMWindow> domWindow;
        rv = mWebShellWin->ConvertWebShellToDOMWindow( mWebShell,
                                                       getter_AddRefs( domWindow ) );
        if ( NS_SUCCEEDED( rv ) && domWindow ) {
#ifdef NECKO
            rv = handler->HandleUnknownContentType( channel, aContentType, domWindow );
#else
            rv = handler->HandleUnknownContentType( aURL, aContentType, domWindow );
#endif
        } else {
            #ifdef NS_DEBUG
            printf( "%s %d: ConvertWebShellToDOMWindow failed, rv=0x%08X\n",
                    __FILE__, (int)__LINE__, (int)rv );
            #endif
        }

        // Release the unknown content type handler service object.
        nsServiceManager::ReleaseService( NS_IUNKNOWNCONTENTTYPEHANDLER_PROGID, handler );
    } else {
        #ifdef NS_DEBUG
        printf( "%s %d: GetService failed for unknown content type handler, rv=0x%08X\n",
                __FILE__, (int)__LINE__, (int)rv );
        #endif
    }

    return rv;
}

NS_IMETHODIMP
#ifdef NECKO
nsBrowserAppCore::OnStartURLLoad(nsIDocumentLoader* loader, 
                                 nsIChannel* channel,
                                 nsIContentViewer* aViewer)
#else
nsBrowserAppCore::OnStartURLLoad(nsIDocumentLoader* loader, 
                                 nsIURI* aURL, const char* aContentType,
                                 nsIContentViewer* aViewer)
#endif
{

   return NS_OK;
}

NS_IMETHODIMP
#ifdef NECKO
nsBrowserAppCore::OnProgressURLLoad(nsIDocumentLoader* loader, 
                                    nsIChannel* channel, PRUint32 aProgress, 
                                    PRUint32 aProgressMax)
#else
nsBrowserAppCore::OnProgressURLLoad(nsIDocumentLoader* loader, 
                                    nsIURI* aURL, PRUint32 aProgress, 
                                    PRUint32 aProgressMax)
#endif
{
  nsresult rv = NS_OK;
  PRUint32 progress = aProgressMax ? ( aProgress * 100 ) / aProgressMax : 0;
#ifdef NECKO
  nsCOMPtr<nsIURI> aURL;
  rv = channel->GetURI(getter_AddRefs(aURL));
  if (NS_FAILED(rv)) return rv;
  char *urlString = 0;
#else
  const char *urlString = 0;
#endif
  aURL->GetSpec( &urlString );
#ifdef NECKO
  nsCRT::free(urlString);
#endif
  return rv;
}


NS_IMETHODIMP
#ifdef NECKO
nsBrowserAppCore::OnStatusURLLoad(nsIDocumentLoader* loader, 
                                  nsIChannel* channel, nsString& aMsg)
#else
nsBrowserAppCore::OnStatusURLLoad(nsIDocumentLoader* loader, 
                                  nsIURI* aURL, nsString& aMsg)
#endif
{
  nsresult rv = setAttribute( mWebShell, "Browser:Status", "value", aMsg );
   return rv;
}


NS_IMETHODIMP
#ifdef NECKO
nsBrowserAppCore::OnEndURLLoad(nsIDocumentLoader* loader, 
                               nsIChannel* channel, nsresult aStatus)
#else
nsBrowserAppCore::OnEndURLLoad(nsIDocumentLoader* loader, 
                               nsIURI* aURL, PRInt32 aStatus)
#endif
{

   return NS_OK;
}

/////////////////////////////////////////////////////////
//             nsISessionHistory methods              //
////////////////////////////////////////////////////////


NS_IMETHODIMP    
nsBrowserAppCore::GoBack(nsIWebShell * aPrev)
{
  if (mSHistory) {
    mSHistory->GoBack(aPrev);
  }
	return NS_OK;
}

NS_IMETHODIMP    
nsBrowserAppCore::GoForward(nsIWebShell * aPrev)
{
  if (mSHistory) {
    mSHistory->GoForward(aPrev);
  }
	return NS_OK;
}

NS_IMETHODIMP
#ifndef NECKO
nsBrowserAppCore::Reload(nsIWebShell * aPrev, nsURLReloadType aType)
#else
nsBrowserAppCore::Reload(nsIWebShell * aPrev, nsLoadFlags aType)
#endif // NECKO
{

	if (mSHistory)
		mSHistory->Reload(aPrev, aType);
	return NS_OK;
}

NS_IMETHODIMP
nsBrowserAppCore::add(nsIWebShell * aWebShell)
{
   nsresult rv;
   if (mSHistory) 
     rv = mSHistory->add(aWebShell);
   return rv;
}

NS_IMETHODIMP
nsBrowserAppCore::Goto(PRInt32 aGotoIndex, nsIWebShell * aPrev, PRBool aIsReloading)
{
   nsresult rv;
   if (mSHistory) 
     rv = mSHistory->Goto(aGotoIndex, aPrev, PR_FALSE);
   return rv;
}


NS_IMETHODIMP
nsBrowserAppCore::SetLoadingFlag(PRBool aFlag)
{
  return NS_OK;
}


NS_IMETHODIMP
nsBrowserAppCore::GetLoadingFlag(PRBool &aFlag)
{
  return NS_OK;
}

NS_IMETHODIMP
nsBrowserAppCore::SetLoadingHistoryEntry(nsHistoryEntry *  aHistoryEntry)
{
  return NS_OK;
}


NS_IMETHODIMP
nsBrowserAppCore::canForward(PRBool & aResult)
{
   PRBool result;

   if (mSHistory) {
     mSHistory->canForward(result);
     aResult = result;
   }
   return NS_OK;
}

NS_IMETHODIMP
nsBrowserAppCore::canBack(PRBool & aResult)
{
   PRBool result;

   if (mSHistory) {
     mSHistory->canBack(result);
     aResult = result;
   }
   return NS_OK;
}

NS_IMETHODIMP
nsBrowserAppCore::getHistoryLength(PRInt32 & aResult)
{
   PRInt32 result;
   if (mSHistory)
     mSHistory->getHistoryLength(result);
   
   aResult = result;
   return NS_OK;
}


NS_IMETHODIMP
nsBrowserAppCore::getCurrentIndex(PRInt32 & aResult)
{
   PRInt32 result;

   if (mSHistory)
     mSHistory->getCurrentIndex(result);
   
   aResult = result;
   return NS_OK;

}

NS_IMETHODIMP
nsBrowserAppCore::GetURLForIndex(PRInt32 aIndex, const PRUnichar** aURL)
{

   if (mSHistory)
     return  mSHistory->GetURLForIndex(aIndex, aURL);
   return NS_OK;
}

NS_IMETHODIMP
nsBrowserAppCore::SetURLForIndex(PRInt32 aIndex, const PRUnichar* aURL)
{
   if (mSHistory)
      mSHistory->SetURLForIndex(aIndex, aURL);
   return NS_OK;
}

NS_IMETHODIMP
nsBrowserAppCore::GetTitleForIndex(PRInt32 aIndex, const PRUnichar** aTitle)
{

   if (mSHistory)
      mSHistory->GetTitleForIndex(aIndex, aTitle);
   return NS_OK;
}

NS_IMETHODIMP
nsBrowserAppCore::SetTitleForIndex(PRInt32 aIndex, const PRUnichar* aTitle)
{
   if (mSHistory)
      mSHistory->SetTitleForIndex(aIndex, aTitle);
   return NS_OK;
}
/*
NS_IMETHODIMP
cloneHistory(nsISessionHistory * aSessionHistory) {
  return NS_OK;

}
*/

////////////////////////////////////////////////////////////////////////

NS_IMETHODIMP    
nsBrowserAppCore::NewWindow()
{  
  nsresult rv;
  
  char * urlstr = nsnull;

  // Default URL if one was not provided in the cmdline
  if (nsnull == urlstr)
      urlstr = "chrome://navigator/content/";
  else
      fprintf(stderr, "URL to load is %s\n", urlstr);

  /*
   * Create the Application Shell instance...
   */
  NS_WITH_SERVICE(nsIAppShellService, appShell, kAppShellServiceCID, &rv);

  if (!NS_SUCCEEDED(rv)) return rv;

  /*
   * Post an event to the shell instance to load the AppShell 
   * initialization routines...  
   * 
   * This allows the application to enter its event loop before having to 
   * deal with GUI initialization...
   */
  ///write me...
  nsIURI* url;

#ifndef NECKO  
  rv = NS_NewURL(&url, urlstr);
#else
  NS_WITH_SERVICE(nsIIOService, service, kIOServiceCID, &rv);
  if (NS_FAILED(rv)) return rv;

  nsIURI *uri = nsnull;
  rv = service->NewURI(urlstr, nsnull, &uri);
  if (NS_FAILED(rv)) return rv;

  rv = uri->QueryInterface(nsIURI::GetIID(), (void**)&url);
  NS_RELEASE(uri);
#endif // NECKO
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsIWebShellWindow> newWindow;
  appShell->CreateTopLevelWindow(nsnull, url, PR_TRUE, PR_TRUE,
              NS_CHROME_ALL_CHROME, nsnull, NS_SIZETOCONTENT, NS_SIZETOCONTENT,
              getter_AddRefs(newWindow));
  NS_RELEASE(url);
  
  return NS_OK;
}

//----------------------------------------------------------
static void BuildFileURL(const char * aFileName, nsString & aFileURL) 
{
  nsAutoString fileName(aFileName);
  char * str = fileName.ToNewCString();

  PRInt32 len = strlen(str);
  PRInt32 sum = len + sizeof(FILE_PROTOCOL);
  char* lpszFileURL = new char[sum];

  // Translate '\' to '/'
  for (PRInt32 i = 0; i < len; i++) {
    if (str[i] == '\\') {
	    str[i] = '/';
    }
  }

  // Build the file URL
  PR_snprintf(lpszFileURL, sum, "%s%s", FILE_PROTOCOL, str);

  // create string
  aFileURL = lpszFileURL;
  delete[] lpszFileURL;
  delete[] str;
}

//----------------------------------------------------------------------------------------
NS_IMETHODIMP nsBrowserAppCore::OpenWindow()
//----------------------------------------------------------------------------------------
{  
  nsCOMPtr<nsIFileSpecWithUI> fileSpec(getter_AddRefs(NS_CreateFileSpecWithUI()));
  if (!fileSpec)
  	return NS_ERROR_FAILURE;

  nsresult rv = fileSpec->ChooseInputFile(
  	"Open File", nsIFileSpecWithUI::eAllStandardFilters, nsnull, nsnull);
  if (NS_FAILED(rv))
    return rv;
  
  char buffer[1024];
  char* urlString;
  rv = fileSpec->GetURLString(&urlString);
  if (NS_FAILED(rv))
    return rv;
  PR_snprintf( buffer, sizeof buffer, "OpenFile(\"%s\")", urlString);
  nsCRT::free(urlString);
  ExecuteScript( mToolbarScriptContext, buffer );
  return rv;
} // nsBrowserAppCore::OpenWindow

//----------------------------------------
void nsBrowserAppCore::SetButtonImage(nsIDOMNode * aParentNode, PRInt32 aBtnNum, const nsString &aResName)
{
  PRInt32 count = 0;
  nsCOMPtr<nsIDOMNode> button(FindNamedDOMNode(nsAutoString("button"), aParentNode, count, aBtnNum)); 
  count = 0;
  nsCOMPtr<nsIDOMNode> img(FindNamedDOMNode(nsAutoString("img"), button, count, 1)); 
  nsCOMPtr<nsIDOMHTMLImageElement> imgElement(do_QueryInterface(img));
  if (imgElement) {
    imgElement->SetSrc(aResName);
  }

}


NS_IMETHODIMP    
nsBrowserAppCore::PrintPreview()
{ 

  return NS_OK;
}

NS_IMETHODIMP    
nsBrowserAppCore::Copy()
{ 
  nsIPresShell * presShell = GetPresShellFor(mContentAreaWebShell);
  if (nsnull != presShell) {
    presShell->DoCopy();
  }

  return NS_OK;
}

NS_IMETHODIMP    
nsBrowserAppCore::Print()
{  
  if (nsnull != mContentAreaWebShell) {
    nsIContentViewer *viewer = nsnull;

    mContentAreaWebShell->GetContentViewer(&viewer);

    if (nsnull != viewer) {
      viewer->Print();
      NS_RELEASE(viewer);
    }
  }
  return NS_OK;
}

NS_IMETHODIMP    
nsBrowserAppCore::Close()
{ 
  EndObserving();

  // Undo other stuff we did in SetContentWindow.
  if ( mContentAreaWebShell ) {
      mContentAreaWebShell->SetDocLoaderObserver( 0 );
      mContentAreaWebShell->SetSessionHistory( 0 );
  }

  // session history is an instance, not a service
  NS_IF_RELEASE(mSHistory);

  // Release search context.
  mSearchContext = 0;

  return NS_OK;
}

NS_IMETHODIMP    
nsBrowserAppCore::Exit()
{  
  nsIAppShellService* appShell = nsnull;

  /*
   * Create the Application Shell instance...
   */
  nsresult rv = nsServiceManager::GetService(kAppShellServiceCID,
                                             kIAppShellServiceIID,
                                             (nsISupports**)&appShell);
  if (NS_SUCCEEDED(rv)) {
    appShell->Shutdown();
    nsServiceManager::ReleaseService(kAppShellServiceCID, appShell);
  } 
  return NS_OK;
}

void
nsBrowserAppCore::InitializeSearch( nsIFindComponent *finder )
{
    nsresult rv = NS_OK;

    if ( finder && !mSearchContext ) {
        // Create the search context for this browser window.
        rv = finder->CreateContext( mContentAreaWebShell, nsnull, getter_AddRefs(mSearchContext));
        if ( NS_FAILED( rv ) ) {
            #ifdef NS_DEBUG
            printf( "%s %d CreateContext failed, rv=0x%X\n",
                    __FILE__, (int)__LINE__, (int)rv );
            #endif
        }
    }
}

NS_IMETHODIMP    
nsBrowserAppCore::Find()
{
    nsresult rv = NS_OK;
    PRBool   found = PR_FALSE;
    
    // Get find component.
    nsIFindComponent *finder;
    rv = nsServiceManager::GetService( NS_IFINDCOMPONENT_PROGID,
                                       nsIFindComponent::GetIID(),
                                       (nsISupports**)&finder );
    if ( NS_SUCCEEDED( rv ) ) {
        // Make sure we've initialized searching for this document.
        InitializeSearch( finder );

        // Perform find via find component.
        if ( finder && mSearchContext ) {
            rv = finder->Find( mSearchContext, &found );
        }

        // Release the service.
        nsServiceManager::ReleaseService( NS_IFINDCOMPONENT_PROGID, finder );
    } else {
        #ifdef NS_DEBUG
            printf( "%s %d: GetService failed for find component, rv=0x08%X\n",
                    __FILE__, (int)__LINE__, (int)rv );
        #endif
    }

    return rv;
}

NS_IMETHODIMP    
nsBrowserAppCore::FindNext()
{
    nsresult rv = NS_OK;
    PRBool   found = PR_FALSE;

    // Get find component.
    nsIFindComponent *finder;
    rv = nsServiceManager::GetService( NS_IFINDCOMPONENT_PROGID,
                                       nsIFindComponent::GetIID(),
                                       (nsISupports**)&finder );
    if ( NS_SUCCEEDED( rv ) ) {
        // Make sure we've initialized searching for this document.
        InitializeSearch( finder );

        // Perform find via find component.
        if ( finder && mSearchContext ) {
            rv = finder->FindNext( mSearchContext, &found );
        }

        // Release the service.
        nsServiceManager::ReleaseService( NS_IFINDCOMPONENT_PROGID, finder );
    } else {
        #ifdef NS_DEBUG
            printf( "%s %d: GetService failed for find component, rv=0x08%X\n",
                    __FILE__, (int)__LINE__, (int)rv );
        #endif
    }

    return rv;
}

NS_IMETHODIMP    
nsBrowserAppCore::ExecuteScript(nsIScriptContext * aContext, const nsString& aScript)
{
  if (nsnull != aContext) {
    const char* url = "";
    PRBool isUndefined = PR_FALSE;
    nsString rVal;
    if (APP_DEBUG) {
      printf("Executing [%s]\n", (const char *)nsAutoCString(aScript));
    }
    aContext->EvaluateString(aScript, url, 0, rVal, &isUndefined);
  } 
  return NS_OK;
}



NS_IMETHODIMP    
nsBrowserAppCore::DoDialog()
{
  // (adapted from nsToolkitCore)
  nsresult           rv;

  nsCOMPtr<nsIURI> urlObj;
  char * urlstr = "resource:/res/samples/Password.html";
#ifndef NECKO
  rv = NS_NewURL(getter_AddRefs(urlObj), urlstr);
#else
  NS_WITH_SERVICE(nsIIOService, service, kIOServiceCID, &rv);
  if (NS_FAILED(rv)) return rv;

  nsIURI *uri = nsnull;
  rv = service->NewURI(urlstr, nsnull, &uri);
  if (NS_FAILED(rv)) return rv;

  rv = uri->QueryInterface(nsIURI::GetIID(), (void**)&urlObj);
  NS_RELEASE(uri);
#endif // NECKO
  if (NS_FAILED(rv))
    return rv;

  NS_WITH_SERVICE(nsIAppShellService, appShell, kAppShellServiceCID, &rv);
  if (NS_FAILED(rv))
    return rv;

  rv = appShell->RunModalDialog(nsnull, mWebShellWin, urlObj,
                               NS_CHROME_ALL_CHROME | NS_CHROME_OPEN_AS_DIALOG,
                               nsnull, 300, 200);
  return rv;
}

//----------------------------------------------------------------------



static nsresult
FindNamedXULElement(nsIWebShell * aShell,
                              const char *aId,
                              nsCOMPtr<nsIDOMElement> * aResult ) {
    nsresult rv = NS_OK;

    nsCOMPtr<nsIContentViewer> cv;
    rv = aShell ? aShell->GetContentViewer(getter_AddRefs(cv))
               : NS_ERROR_NULL_POINTER;
    if ( cv ) {
        // Up-cast.
        nsCOMPtr<nsIDocumentViewer> docv(do_QueryInterface(cv));
        if ( docv ) {
            // Get the document from the doc viewer.
            nsCOMPtr<nsIDocument> doc;
            rv = docv->GetDocument(*getter_AddRefs(doc));
            if ( doc ) {
                // Up-cast.

                nsCOMPtr<nsIDOMXULDocument> xulDoc( do_QueryInterface(doc) );
                  if ( xulDoc ) {
                    // Find specified element.
                    nsCOMPtr<nsIDOMElement> elem;

                    rv = xulDoc->GetElementById( aId, getter_AddRefs(elem) );
                    if ( elem ) {
			*aResult =  elem;
                    } else {
                       if (APP_DEBUG) printf("GetElementByID failed, rv=0x%X\n",(int)rv);
                    }
                } else {
                  if (APP_DEBUG)   printf("Upcast to nsIDOMXULDocument failed\n");
                }

            } else {
               if (APP_DEBUG)  printf("GetDocument failed, rv=0x%X\n",(int)rv);
            }
        } else {
             if (APP_DEBUG)  printf("Upcast to nsIDocumentViewer failed\n");
        }
    } else {
       if (APP_DEBUG) printf("GetContentViewer failed, rv=0x%X\n",(int)rv);
    }
    return rv;
}

static const char *prefix = "component://netscape/appshell/component/browser/window";

void
nsBrowserAppCore::BeginObserving() {
    // Get observer service.
    nsIObserverService *svc = 0;
    nsresult rv = nsServiceManager::GetService( NS_OBSERVERSERVICE_PROGID,
                                                nsIObserverService::GetIID(),
                                                (nsISupports**)&svc );
    if ( NS_SUCCEEDED( rv ) && svc ) {
        // Add/Remove object as observer of web shell window topics.
        nsString topic1 = prefix;
        topic1 += ";status";
        nsString topic2 = prefix;
        topic2 += ";progress";
        rv = svc->AddObserver( this, topic1.GetUnicode() );
        rv = svc->AddObserver( this, topic2.GetUnicode() );
        // Release the service.
        nsServiceManager::ReleaseService( NS_OBSERVERSERVICE_PROGID, svc );
    }

    return;
}

void
nsBrowserAppCore::EndObserving() {
    // Get observer service.
    nsIObserverService *svc = 0;
    nsresult rv = nsServiceManager::GetService( NS_OBSERVERSERVICE_PROGID,
                                                nsIObserverService::GetIID(),
                                                (nsISupports**)&svc );
    if ( NS_SUCCEEDED( rv ) && svc ) {
        // Add/Remove object as observer of web shell window topics.
        nsString topic1 = prefix;
        topic1 += ";status";
        nsString topic2 = prefix;
        topic2 += ";progress";
        rv = svc->RemoveObserver( this, topic1.GetUnicode() );
        rv = svc->RemoveObserver( this, topic2.GetUnicode() );
        // Release the service.
        nsServiceManager::ReleaseService( NS_OBSERVERSERVICE_PROGID, svc );
    }

    return;
}

NS_IMETHODIMP
nsBrowserAppCore::Observe( nsISupports *aSubject,
                           const PRUnichar *aTopic,
                           const PRUnichar *someData ) {
    nsresult rv = NS_OK;

    // We only are interested if aSubject is our web shell window.
    if ( aSubject && mWebShellWin ) {
        nsIWebShellWindow *window = 0;
        rv = aSubject->QueryInterface( nsIWebShellWindow::GetIID(), (void**)&window );
        if ( NS_SUCCEEDED( rv ) && window ) {
            nsString topic1 = prefix;
            topic1 += ";status";
            nsString topic2 = prefix;
            topic2 += ";progress";
            // Compare to our window.
            if ( window == mWebShellWin ) {
                // Get topic substring.
                if ( topic1 == aTopic ) {
                    // Update status text.
                    rv = setAttribute( mWebShell, "Browser:Status", "value", someData );
                } else if ( topic2 == aTopic ) {
                    // We don't process this, yet.
                }
            } else {
                // Not for this app core.
            }
            // Release the window.
            window->Release();
        }
    }

    return rv;
}
