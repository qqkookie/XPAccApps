/*
 * PROJECT:    ReactOS Notepad
 * LICENSE:    LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:    Providing a Windows-compatible simple text editor for ReactOS
 * COPYRIGHT:  Copyright 1998,99 Marcel Baur <mbaur@g26.ethz.ch>
 */

#pragma once

VOID UpdateWindowLayout(VOID);

VOID DoShowHideStatusBar(VOID);
VOID UpdateStatusBar(VOID);
VOID StatusBarUpdateCaretPos(VOID);
VOID ToggleStatusBar(VOID);

VOID UpdateWindowCaption(BOOL clearModifyAlert);

VOID DoCreateEditWindow(VOID);

BOOL CreateStatusTabControl(VOID);
BOOL AddNewEditTab(VOID);
VOID OnTabChange(VOID);
int  CloseTab(VOID);
VOID SetTabHeader(VOID);
int  FindDupPathTab(LPCTSTR filePath);

VOID DIALOG_FileNew(VOID);
VOID DIALOG_FileNewWindow(VOID);
VOID DIALOG_FileOpen(VOID);
VOID DIALOG_MenuRecent(int menu_id);
VOID DIALOG_EditWrap(VOID);

BOOL DIALOG_FileSave(VOID);
BOOL DIALOG_FileSaveAs(VOID);
VOID DIALOG_FileClose(VOID);
VOID DIALOG_FileExit(VOID);

VOID DIALOG_EditUndo(VOID);
VOID DIALOG_EditCut(VOID);
VOID DIALOG_EditCopy(VOID);
VOID DIALOG_EditPaste(VOID);
VOID DIALOG_EditDelete(VOID);
VOID DIALOG_EditSelectAll(VOID);
VOID DIALOG_EditTimeDate(BOOL isotime);

VOID EnableSearchMenu(VOID);
VOID DIALOG_Search(VOID);
VOID DIALOG_SearchNext(BOOL bDown);
VOID DIALOG_Replace(VOID);

VOID DIALOG_GoTo(VOID);
VOID DIALOG_SelectFont(VOID);

VOID DIALOG_HelpContents(VOID);
VOID DIALOG_HelpAboutNotepad(VOID);

VOID ShowLastError(VOID);
int  StringMsgBox( int formatId, LPCTSTR szString, DWORD dwFlags);

VOID AlertFileNotFound(LPCTSTR szFileName);
int  AlertFileNotExist(LPCTSTR szFileName);
int  AlertFileNotSaved(LPCTSTR szFileName);
int  AlertUnicodeCharactersLost(LPCWSTR szFileName);

// ----- printing.c ------------

VOID DIALOG_FilePrint(VOID);
VOID DIALOG_FilePageSetup(VOID);
