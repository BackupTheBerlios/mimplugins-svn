#ifndef M_GENMENU_H
#define M_GENMENU_H
/*
  Main features:
  1) Independet from clist,may be used in any module.
  2) Module defined Exec and Check services.
  3) Menu with any level of popups,icons for root of popup.
  4) You may use measure/draw/processcommand even if menuobject is unknown.
	  
  Idea of GenMenu module consists of that,
  it must be independet and offers only general menu purpose services:
  MO_CREATENEWMENUOBJECT
  MO_REMOVEMENUOBJECT
  MO_ADDNEWMENUITEM
  MO_REMOVEMENUITEM
  ...etc

  And then each module that want use and offer to others menu handling
  must create own services.For example i rewrited mainmenu and
  contactmenu code in clistmenus.c.If you look at code all functions
  are very identical, and vary only in check/exec services.

  So template set of function will like this:
  Remove<NameMenu>Item
  Add<NameMenu>Item
  Build<NameMenu>
  <NameMenu>ExecService
  <NameMenu>CheckService

  ExecService and CheckService used as callbacks when GenMenu must
  processcommand for menu item or decide to show or not item.This make
  GenMenu independet of which params must passed to service when user 
  click on menu,this decide each module.
						28-04-2003 Bethoven

*/



/*
Analog to CLISTMENUITEM,but invented two params root and ownerdata.
root is used for creating any level popup menus,set to -1 to build 
at first level and root=MenuItemHandle to place items in submenu 
of this item.Must be used two new flags CMIF_ROOTPOPUP and CMIF_CHILDPOPUP
(defined in m_clist.h)

ownerdata is passed to callback services(ExecService and CheckService) 
when building menu or processed command.
*/
typedef struct{
int cbSize;
char *pszName;
int position;
int root;
int flags;
HICON hIcon;
DWORD hotKey;
void *ownerdata;
}TMO_MenuItem,*PMO_MenuItem;

/*
This structure passed to CheckService. 
*/
typedef struct{
void *MenuItemOwnerData;
int MenuItemHandle;
WPARAM wParam;//from  ListParam.wParam when building menu
LPARAM lParam;//from  ListParam.lParam when building menu
}TCheckProcParam,*PCheckProcParam;

typedef struct{
int cbSize;
char *name;

/*
This service called when module build menu(MO_BUILDMENU).
Service called with params 

wparam=PCheckProcParam
lparam=0
if return==FALSE item is skiped.
*/
char *CheckService;

/*
This service called when user select menu item.
Service called with params 
wparam=ownerdata
lparam=lParam from MO_PROCESSCOMMAND
*/
char *ExecService;//called when processmenuitem called
}TMenuParam,*PMenuParam;

//used in MO_BUILDMENU
typedef struct tagListParam{
	int rootlevel;
	int MenuObjectHandle;
	int wParam,lParam;
}ListParam,*lpListParam;

typedef struct{
HMENU menu;
int ident;
LPARAM lParam;
}ProcessCommandParam,*lpProcessCommandParam;

//wparam started hMenu
//lparam ListParam*
//result hMenu
#define MO_BUILDMENU						"MO/BuildMenu"

//wparam=MenuItemHandle
//lparam userdefined
//returns TRUE if it processed the command, FALSE otherwise
#define MO_PROCESSCOMMAND					"MO/ProcessCommand"

//if menu not known call this
//LOWORD(wparam) menuident (from WM_COMMAND message)
//returns TRUE if it processed the command, FALSE otherwise
//Service automatically find right menuobject and menuitem 
//and call MO_PROCESSCOMMAND
#define MO_PROCESSCOMMANDBYMENUIDENT		"MO/ProcessCommandByMenuIdent"


//wparam=0;
//lparam=PMenuParam;
//returns=MenuObjectHandle on success,-1 on failure
#define MO_CREATENEWMENUOBJECT				"MO/CreateNewMenuObject"

//wparam=MenuObjectHandle
//lparam=0
//returns 0 on success,-1 on failure
//Note: you must free all ownerdata structures, before you 
//call this service.MO_REMOVEMENUOBJECT NOT free it.
#define MO_REMOVEMENUOBJECT					"MO/RemoveMenuObject"


//wparam=MenuItemHandle
//lparam=0
//returns 0 on success,-1 on failure.
//You must free ownerdata before this call.
//If MenuItemHandle is root all child will be removed too.
#define MO_REMOVEMENUITEM					"MO/RemoveMenuItem"

//wparam=MenuObjectHandle
//lparam=PMO_MenuItem
//return MenuItemHandle on success,-1 on failure
//Service supports old menu items (without CMIF_ROOTPOPUP or 
//CMIF_CHILDPOPUP flag).For old menu items needed root will be created
//automatically.
#define MO_ADDNEWMENUITEM					"MO/AddNewMenuItem"

//wparam MenuItemHandle
//returns ownerdata on success,NULL on failure
//Useful to get and free ownerdata before delete menu item.
#define MO_MENUITEMGETOWNERDATA				"MO/MenuItemGetOwnerData"

//wparam MenuItemHandle
//lparam PMO_MenuItem
//returns 0 on success,-1 on failure
#define MO_MODIFYMENUITEM					"MO/ModifyMenuItem"

//wparam=MenuItemHandle
//lparam=PMO_MenuItem
//returns 0 and filled PMO_MenuItem structure on success and 
//-1 on failure
#define MO_GETMENUITEM						"MO/GetMenuItem"

//wparam=MenuObjectHandle
//lparam=vKey
//returns TRUE if it processed the command, FALSE otherwise
//this should be called in WM_KEYDOWN
#define	MO_PROCESSHOTKEYS					"MO/ProcessHotKeys"

//process a WM_DRAWITEM message
//wparam=0
//lparam=LPDRAWITEMSTRUCT
//returns TRUE if it processed the command, FALSE otherwise
#define MO_DRAWMENUITEM						"MO/DrawMenuItem"

//process a WM_MEASUREITEM message
//wparam=0
//lparam=LPMEASUREITEMSTRUCT
//returns TRUE if it processed the command, FALSE otherwise
#define MO_MEASUREMENUITEM					"MO/MeasureMenuItem"

//set uniq name to menuitem(used to store it in database when enabled OPT_USERDEFINEDITEMS)
#define OPT_MENUITEMSETUNIQNAME								1

//Set FreeService for menuobject. When freeing menuitem it will be called with
//wParam=MenuItemHandle
//lParam=mi.ownerdata
#define OPT_MENUOBJECT_SET_FREE_SERVICE						2

//Set onAddService for menuobject. 
#define OPT_MENUOBJECT_SET_ONADD_SERVICE					3

//enable ability user to edit menuitems via options page.
#define OPT_USERDEFINEDITEMS 1


typedef struct tagOptParam{
	int Handle;
	int Setting;
	int Value;
}OptParam,*lpOptParam;

//wparam=0
//lparam=*lpOptParam
//returns TRUE if it processed the command, FALSE otherwise
#define MO_SETOPTIONSMENUOBJECT					"MO/SetOptionsMenuObject"


//wparam=0
//lparam=*lpOptParam
//returns TRUE if it processed the command, FALSE otherwise
#define MO_SETOPTIONSMENUITEM					"MO/SetOptionsMenuItem"

#endif