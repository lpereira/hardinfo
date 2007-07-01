#ifndef __UIDEFS_H__
#define __UIDEFS_H__ 

char *uidefs_str = "<ui>" \
"	<menubar>" \
"		<menu name=\"InformationMenu\" action=\"InformationMenuAction\">" \
"		<menuitem name=\"Report\" action=\"ReportAction\" />" \
/*"		<separator/>" \ */
"		<menuitem name=\"Copy\" action=\"CopyAction\" />" \
/*
 * Save Image is not ready for prime time. Yet.
 * "<menuitem name=\"SaveGraph\" action=\"SaveGraphAction\" />" \
 */
"		<separator/>" \
"		<menuitem name=\"SyncManager\" action=\"SyncManagerAction\" />" \
"		<separator/>" \
"		<menuitem name=\"Quit\" action=\"QuitAction\" />" \
"	</menu>" \
"	<menu name=\"ViewMenu\" action=\"ViewMenuAction\">" \
"		<menuitem name=\"SidePane\" action=\"SidePaneAction\"/>" \
"		<menuitem name=\"Toolbar\" action=\"ToolbarAction\"/>" \
"		<separator/>" \
"		<menuitem name=\"Refresh\" action=\"RefreshAction\"/>" \
"		<separator/>" \
"		<separator name=\"LastSep\"/>" \
"	</menu>" \
"	<menu name=\"HelpMenu\" action=\"HelpMenuAction\">" \
"		<menuitem name=\"OnlineDocs\" action=\"OnlineDocsAction\"/>" \
"		<separator/>" \
"		<menuitem name=\"WebPage\" action=\"HomePageAction\"/>" \
"		<menuitem name=\"ReportBug\" action=\"ReportBugAction\"/>" \
"		<separator/>" \
"		<menuitem name=\"Donate\" action=\"DonateAction\"/>" \
"		<separator/>" \
"		<menu name=\"HelpMenuModules\" action=\"HelpMenuModulesAction\">" \
"			<separator name=\"LastSep\"/>" \
"		</menu>" \
"		<menuitem name=\"About\" action=\"AboutAction\"/>" \
"	</menu>" \
"	</menubar>" \
"	<toolbar action=\"MainMenuBar\" action=\"MainMenuBarAction\">" \
"		<placeholder name=\"ToolItems\">" \
"			<toolitem name=\"Refresh\" action=\"RefreshAction\"/>" \
"			<separator/>" \
"			<toolitem name=\"Copy\" action=\"CopyAction\"/>" \
"			<toolitem name=\"Report\" action=\"ReportAction\"/>" \
"		</placeholder>" \
"	</toolbar>" \
"</ui>";

#endif	/* __UIDEFS_H__ */
