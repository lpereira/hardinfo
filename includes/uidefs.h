#ifndef __UIDEFS_H__
#define __UIDEFS_H__

#include "config.h"

#define SYNC_MANAGER_MENU_ITEMS                                                \
    "<separator/>"                                                             \
    "<menuitem name=\"SyncManager\" action=\"SyncManagerAction\" always-show-image=\"true\"/>"           \
    "<menuitem name=\"SyncOnStartup\" action=\"SyncOnStartupAction\"/>"
#define SYNC_MANAGER_TOOL_ITEMS                                                \
    "<toolitem name=\"SyncManager\" action=\"SyncManagerAction\" always-show-image=\"true\"/>"

char *uidefs_str =
    "<ui>"
    "	<menubar>"
    "	<menu name=\"InformationMenu\" action=\"InformationMenuAction\">"
    "		<menuitem name=\"Report\" action=\"ReportAction\" always-show-image=\"true\"/>"
    "		<menuitem name=\"Copy\" action=\"CopyAction\" always-show-image=\"true\"/>"
    SYNC_MANAGER_MENU_ITEMS \
    "		<separator/>"
    "		<menuitem name=\"Quit\" action=\"QuitAction\" />"
    "	</menu>"
    "	<menu name=\"ViewMenu\" action=\"ViewMenuAction\">"
    "		<menuitem name=\"SidePane\" action=\"SidePaneAction\"/>"
    "		<menuitem name=\"Toolbar\" action=\"ToolbarAction\"/>"
    "	   <menu name=\"ThemeMenu\" action=\"ThemeMenuAction\">"
    "            <menuitem name=\"DisableTheme\" action=\"DisableThemeAction\"/>"
    "            <menuitem name=\"Theme1\" action=\"Theme1Action\"/>"
    "            <menuitem name=\"Theme2\" action=\"Theme2Action\"/>"
    "            <menuitem name=\"Theme3\" action=\"Theme3Action\"/>"
    "            <menuitem name=\"Theme4\" action=\"Theme4Action\"/>"
    "            <menuitem name=\"Theme5\" action=\"Theme5Action\"/>"
    "            <menuitem name=\"Theme6\" action=\"Theme6Action\"/>"
    "	   </menu>"
    "		<separator/>"
    "		<separator name=\"LastSep\"/>"
    "		<menuitem name=\"Refresh\" action=\"RefreshAction\"/>"
    "	</menu>"
    "	<menu name=\"HelpMenu\" action=\"HelpMenuAction\">"
    "		<menuitem name=\"WebPage\" action=\"HomePageAction\" always-show-image=\"true\"/>"
    "		<menuitem name=\"ReportBug\" action=\"ReportBugAction\" always-show-image=\"true\"/>"
    "		<menuitem name=\"About\" action=\"AboutAction\"/>"
    "	</menu>"
    "	</menubar>"
    "	<toolbar action=\"MainMenuBar\" action=\"MainMenuBarAction\">"
    "		<placeholder name=\"ToolItems\">"
    "			<toolitem name=\"Refresh\" action=\"RefreshAction\"/>"
    "			<separator/>"
    "			<toolitem name=\"Report\" action=\"ReportAction\"/>"
    "			<toolitem name=\"Copy\" action=\"CopyAction\"/>"
    "			<separator/>"
    SYNC_MANAGER_TOOL_ITEMS \
    "		</placeholder>"
    "	</toolbar>"
    "</ui>";

/*DISABLED
    "		<separator/>"
    "		<menu name=\"HelpMenuModules\" "
    "action=\"HelpMenuModulesAction\">"
    "			<separator name=\"LastSep\"/>"
    "		</menu>"
*/

#endif /* __UIDEFS_H__ */
