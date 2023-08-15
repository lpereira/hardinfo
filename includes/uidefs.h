#ifndef __UIDEFS_H__
#define __UIDEFS_H__

#include "config.h"

#if RELEASE
#define DEBUG_TOOLBAR_ITEMS
#else /* !RELEASE */
#define DEBUG_TOOLBAR_ITEMS                                                    \
    "<separator/>"                                                             \
    "<toolitem name=\"ReportBug\" action=\"ReportBugAction\" />"
#endif /* !RELEASE */

#ifdef HAS_LIBSOUP
#define SYNC_MANAGER_MENU_ITEMS                                                \
    "<separator/>"                                                             \
    "<menuitem name=\"SyncManager\" action=\"SyncManagerAction\" "             \
    "always-show-image=\"true\"/>"                                             \
    "<menuitem name=\"SyncOnStartup\" action=\"SyncOnStartupAction\"/>"
#define SYNC_MANAGER_TOOL_ITEMS                                                \
    "<toolitem name=\"SyncManager\" action=\"SyncManagerAction\"/>"

#else /* !HAS_LIBSOUP */
#define SYNC_MANAGER_MENU_ITEMS
#define SYNC_MANAGER_TOOL_ITEMS
#endif /* !HAS_LIBSOUP */

char *uidefs_str =
    "<ui>"
    "	<menubar>"
    "	<menu name=\"InformationMenu\" action=\"InformationMenuAction\">"
    "		<menuitem name=\"Report\" action=\"ReportAction\" />"
    "		<menuitem name=\"Copy\" action=\"CopyAction\" />"
    SYNC_MANAGER_MENU_ITEMS \
    "		<separator/>"
    "		<menuitem name=\"Quit\" action=\"QuitAction\" />"
    "	</menu>"
    "	<menu name=\"ViewMenu\" action=\"ViewMenuAction\">"
    "		<menuitem name=\"SidePane\" action=\"SidePaneAction\"/>"
    "		<menuitem name=\"Toolbar\" action=\"ToolbarAction\"/>"
    "		<separator/>"
    "		<separator name=\"LastSep\"/>"
    "		<menuitem name=\"Refresh\" action=\"RefreshAction\"/>"
    "	</menu>"
    "	<menu name=\"HelpMenu\" action=\"HelpMenuAction\">"
    "		<menuitem name=\"WebPage\" action=\"HomePageAction\"/>"
    "		<menuitem name=\"ReportBug\" action=\"ReportBugAction\"/>"
    "		<separator/>"
    "		<menu name=\"HelpMenuModules\" "
    "action=\"HelpMenuModulesAction\">"
    "			<separator name=\"LastSep\"/>"
    "		</menu>"
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
    DEBUG_TOOLBAR_ITEMS \
    SYNC_MANAGER_TOOL_ITEMS \
    "		</placeholder>"
    "	</toolbar>"
    "</ui>";

#endif /* __UIDEFS_H__ */
