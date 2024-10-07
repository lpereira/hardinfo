#ifndef __UIDEFS_H__
#define __UIDEFS_H__

#include "config.h"

char *uidefs_str =
    "<ui>"
    "	<menubar name=\"MainMenu\">"
    "	<menu name=\"InformationMenu\" action=\"InformationMenuAction\">"
    "		<menuitem name=\"Report\" action=\"ReportAction\" always-show-image=\"true\"/>"
  /*    "		<menuitem name=\"Copy\" action=\"CopyAction\" always-show-image=\"true\"/>"*/
    "           <separator/>"                                                             \
    "           <menuitem name=\"SyncManager\" action=\"SyncManagerAction\" always-show-image=\"true\"/>"
    "           <menuitem name=\"SyncOnStartup\" action=\"SyncOnStartupAction\"/>"
    "		<separator/>"
    "		<menuitem name=\"Quit\" action=\"QuitAction\" always-show-image=\"true\"/>"
    "	</menu>"
    "	<menu name=\"ViewMenu\" action=\"ViewMenuAction\">"
    "		<menuitem name=\"SidePane\" action=\"SidePaneAction\"/>"
    "		<menuitem name=\"Toolbar\" action=\"ToolbarAction\"/>"
#if GTK_CHECK_VERSION(3, 0, 0)
    "	        <menu name=\"ThemeMenu\" action=\"ThemeMenuAction\">"
    "               <menuitem name=\"DisableTheme\" action=\"DisableThemeAction\"/>"
    "               <menuitem name=\"Theme1\" action=\"Theme1Action\"/>"
    "               <menuitem name=\"Theme2\" action=\"Theme2Action\"/>"
    "               <menuitem name=\"Theme3\" action=\"Theme3Action\"/>"
    "               <menuitem name=\"Theme4\" action=\"Theme4Action\"/>"
    "               <menuitem name=\"Theme5\" action=\"Theme5Action\"/>"
    "               <menuitem name=\"Theme6\" action=\"Theme6Action\"/>"
    "	        </menu>"
#endif
    "		<separator/>"
    "		<separator name=\"LastSep\"/>"
    "		<menuitem name=\"Refresh\" action=\"RefreshAction\" always-show-image=\"true\"/>"
    "	</menu>"
    "	<menu name=\"HelpMenu\" action=\"HelpMenuAction\">"
    "		<menuitem name=\"WebPage\" action=\"HomePageAction\" always-show-image=\"true\"/>"
    "		<menuitem name=\"ReportBug\" action=\"ReportBugAction\" always-show-image=\"true\"/>"
    "		<menuitem name=\"About\" action=\"AboutAction\" always-show-image=\"true\"/>"
    "	</menu>"
    "	</menubar>"
    "	<toolbar action=\"MainMenuBar\" action=\"MainMenuBarAction\">"
    "			<toolitem name=\"Refresh\" action=\"RefreshAction\" always-show-image=\"true\"/>"
    "			<separator/>"
    "			<toolitem name=\"Report\" action=\"ReportAction\" always-show-image=\"true\"/>"
  /*    "			<toolitem name=\"Copy\" action=\"CopyAction\"/>"*/
    "			<separator/>"
    "                   <toolitem name=\"SyncManager\" action=\"SyncManagerAction\" always-show-image=\"true\"/>"
    "	</toolbar>"
    "</ui>";


#endif /* __UIDEFS_H__ */
