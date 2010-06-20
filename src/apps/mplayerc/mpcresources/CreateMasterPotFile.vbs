Option Explicit
''
' This script creates the master POT file (English.pot).
'
' Copyright (C) 2007 by Tim Gerundt
' Released under the "GNU General Public License"
'
' ID line follows -- this is updated by SVN
' $Id$

Const ForReading = 1

Const NO_BLOCK = 0
Const MENU_BLOCK = 1
Const DIALOGEX_BLOCK = 2
Const STRINGTABLE_BLOCK = 3
Const VERSIONINFO_BLOCK = 4
Const ACCELERATORS_BLOCK = 5

Dim oFSO

Set oFSO = CreateObject("Scripting.FileSystemObject")

Call Main

''
' ...
Sub Main
  Dim oStrings, oComments, sCodePage
  Dim StartTime, EndTime, Seconds
  
  StartTime = Time
  
  Wscript.Echo "Warning: " & Wscript.ScriptName & " can take several seconds to finish!"
  
  Set oStrings = GetStringsFromRcFile("../mplayerc.rc", oComments, sCodePage)
  CreateMasterPotFile "English.pot", oStrings, oComments, sCodePage
  
  EndTime = Time
  Seconds = DateDiff("s", StartTime, EndTime)
  
  Wscript.Echo Wscript.ScriptName & " finished after " & Seconds & " seconds!"
End Sub

''
' ...
Function GetStringsFromRcFile(ByVal sRcFilePath, ByRef oComments, ByRef sCodePage)
  Dim oBlacklist, oStrings, oRcFile, sLine, iLine
  Dim sRcFileName, iBlockType, sReference, sString, sComment, oMatches, oMatch, sTemp
  
  '--------------------------------------------------------------------------------
  ' Blacklist...
  '--------------------------------------------------------------------------------
  Set oBlacklist = CreateObject("Scripting.Dictionary")
  oBlacklist.Add "_HDR_POPUP_", True
  oBlacklist.Add "_ITEM_POPUP_", True
  oBlacklist.Add "_POPUP_", True
  oBlacklist.Add "Btn", True
  oBlacklist.Add "Button", True
  oBlacklist.Add "Button1", True
  oBlacklist.Add "Dif", True
  oBlacklist.Add "IDS_SAVEVSS_FMT", True
  oBlacklist.Add "List1", True
  oBlacklist.Add "msctls_progress32", True
  oBlacklist.Add "Static", True
  oBlacklist.Add "SysListView32", True
  oBlacklist.Add "SysTreeView32", True
  oBlacklist.Add "Tree1", True
  '--------------------------------------------------------------------------------
  
  Set oStrings = CreateObject("Scripting.Dictionary")
  Set oComments = CreateObject("Scripting.Dictionary")
  
  If (oFSO.FileExists(sRcFilePath) = True) Then 'If the RC file exists...
    sRcFileName = oFSO.GetFileName(sRcFilePath)
    iLine = 0
    iBlockType = NO_BLOCK
    sCodePage = ""
    Set oRcFile = oFSO.OpenTextFile(sRcFilePath, ForReading, False, -1)
    Do Until oRcFile.AtEndOfStream = True 'For all lines...
      sLine = Trim(oRcFile.ReadLine)
      iLine = iLine + 1
      
      sReference = sRcFileName & ":" & iLine
      sString = ""
      sComment = ""
      
      If (FoundRegExpMatch(sLine, "IDR_.* MENU", oMatch) = True) Then 'MENU...
        iBlockType = MENU_BLOCK
      ElseIf (FoundRegExpMatch(sLine, "IDD_.* DIALOGEX", oMatch) = True) Then 'DIALOGEX...
        iBlockType = DIALOGEX_BLOCK
      ElseIf (sLine = "STRINGTABLE") Then 'STRINGTABLE...
        iBlockType = STRINGTABLE_BLOCK
      ElseIf (FoundRegExpMatch(sLine, "VS_.* VERSIONINFO", oMatch) = True) Then 'VERSIONINFO...
        iBlockType = VERSIONINFO_BLOCK
      ElseIf (FoundRegExpMatch(sLine, "IDR_.* ACCELERATORS", oMatch) = True) Then 'ACCELERATORS...
        iBlockType = ACCELERATORS_BLOCK
      ElseIf (sLine = "END") Then 'END...
        If (iBlockType = STRINGTABLE_BLOCK) Then 'If inside stringtable...
          iBlockType = NO_BLOCK
        End If
      ElseIf (sLine <> "") Then 'If NOT empty line...
        Select Case iBlockType
          Case NO_BLOCK:
            If (FoundRegExpMatch(sLine, "LANGUAGE (LANG_\w*, SUBLANG_\w*)", oMatch) = True) Then 'LANGUAGE...
              sString = oMatch.SubMatches(0)
              sComment = "LANGUAGE, SUBLANGUAGE"
            ElseIf (FoundRegExpMatch(sLine, "code_page\(([\d]+)\)", oMatch) = True) Then 'code_page...
              sString = oMatch.SubMatches(0)
              sComment = "Codepage"
              sCodePage = oMatch.SubMatches(0)
            End If
            
          Case MENU_BLOCK, DIALOGEX_BLOCK, STRINGTABLE_BLOCK:
            If (FoundRegExpMatches(sLine, """(.*?)""", oMatches) = True) Then 'String...
              For Each oMatch In oMatches 'For all strings...
                sTemp = oMatch.SubMatches(0)
                If (sTemp <> "") And (oBlacklist.Exists(sTemp) = False) Then 'If NOT blacklisted...
                  If (oStrings.Exists(sTemp) = True) Then 'If the key is already used...
                    oStrings(sTemp) = oStrings(sTemp) & vbTab & sReference
                  Else 'If the key is NOT already used...
                    oStrings.Add sTemp, sReference
                  End If
                End If
              Next
            End If
            
          Case VERSIONINFO_BLOCK:
            If (FoundRegExpMatch(sLine, "BLOCK ""([0-9A-F]+)""", oMatch) = True) Then 'StringFileInfo.Block...
              sString = oMatch.SubMatches(0)
              sComment = "StringFileInfo.Block"
            ElseIf (FoundRegExpMatch(sLine, "VALUE ""(.*?)"", ""(.*?)\\?0?""", oMatch) = True) Then 'StringFileInfo.Value...
              If (oMatch.SubMatches(0) <> "FileVersion") And (oMatch.SubMatches(0) <> "ProductVersion") Then 'If NOT file or product version...
                sString = oMatch.SubMatches(1)
                sComment = "StringFileInfo." & oMatch.SubMatches(0)
              End If
            ElseIf (FoundRegExpMatch(sLine, "VALUE ""Translation"", (.*?)$", oMatch) = True) Then 'VarFileInfo.Translation...
              sString = oMatch.SubMatches(0)
              sComment = "VarFileInfo.Translation"
            End If
            
        End Select
      End If
      
      If (sString <> "") Then
        If (oStrings.Exists(sString) = True) Then 'If the key is already used...
          oStrings(sString) = oStrings(sString) & vbTab & sReference
        Else 'If the key is NOT already used...
          oStrings.Add sString, sReference
        End If
        
        If (sComment <> "") Then
          If (oComments.Exists(sString) = True) Then 'If the comment key is already used...
            If (oComments(sString) <> sComment) Then 'If new comment...
              oComments(sString) = oComments(sString) & vbTab & sComment
            End If
          Else 'If the comment key is NOT already used...
            oComments.Add sString, sComment
          End If
        End If
      End If
    Loop
    oRcFile.Close
  End If
  Set GetStringsFromRcFile = oStrings
End Function

''
' ...
Sub CreateMasterPotFile(ByVal sPotPath, ByVal oStrings, ByVal oComments, ByVal sCodePage)
  Dim oPotFile, sMsgId, aComments, aReferences, i
  
  Set oPotFile = oFSO.CreateTextFile(sPotPath, True, True)
  
  oPotFile.WriteLine "# This file is part from MPC-HC <http://mpc-hc.sourceforge.net/>"
  oPotFile.WriteLine "# Released under the ""GNU General Public License"""
  oPotFile.WriteLine "#"
  oPotFile.WriteLine "msgid """""
  oPotFile.WriteLine "msgstr """""
  oPotFile.WriteLine """Project-Id-Version: MPC-HC\n"""
  oPotFile.WriteLine """Report-Msgid-Bugs-To: https://sourceforge.net/apps/trac/mpc-hc/report\n"""
  oPotFile.WriteLine """POT-Creation-Date: " & GetPotCreationDate() & "\n"""
  oPotFile.WriteLine """PO-Revision-Date: \n"""
  oPotFile.WriteLine """Last-Translator: \n"""
  oPotFile.WriteLine """Language-Team: English <mpchc-translate@lists.sourceforge.net>\n"""
  oPotFile.WriteLine """MIME-Version: 1.0\n"""
  oPotFile.WriteLine """Content-Type: text/plain; charset=CP" & sCodePage & "\n"""
  oPotFile.WriteLine """Content-Transfer-Encoding: 8bit\n"""
  oPotFile.WriteLine """X-Poedit-Language: English\n"""
  oPotFile.WriteLine """X-Poedit-SourceCharset: CP" & sCodePage & "\n"""
  oPotFile.WriteLine """X-Generator: CreateMasterPotFile.vbs\n"""
  oPotFile.WriteLine
  For Each sMsgId In oStrings.Keys 'For all strings...
    aComments = SplitByTab(oComments(sMsgId))
    For i = LBound(aComments) To UBound(aComments) 'For all comments...
      oPotFile.WriteLine "#. " & aComments(i)
    Next
    aReferences = SplitByTab(oStrings(sMsgId))
    For i = LBound(aReferences) To UBound(aReferences) 'For all references...
      oPotFile.WriteLine "#: " & aReferences(i)
    Next
    oPotFile.WriteLine "#, c-format"
    oPotFile.WriteLine "msgid """ & sMsgId & """"
    oPotFile.WriteLine "msgstr """""
    oPotFile.WriteLine
  Next
  oPotFile.Close
End Sub

''
' ...
Function FoundRegExpMatch(ByVal sString, ByVal sPattern, ByRef oMatchReturn)
  Dim oRegExp, oMatches
  
  Set oRegExp = New RegExp
  oRegExp.Pattern = sPattern
  oRegExp.IgnoreCase = True
  
  oMatchReturn = Null
  FoundRegExpMatch = False
  If (oRegExp.Test(sString) = True) Then
    Set oMatches = oRegExp.Execute(sString)
    Set oMatchReturn = oMatches(0)
    FoundRegExpMatch = True
  End If
End Function

''
' ...
Function FoundRegExpMatches(ByVal sString, ByVal sPattern, ByRef oMatchesReturn)
  Dim oRegExp
  
  Set oRegExp = New RegExp
  oRegExp.Pattern = sPattern
  oRegExp.IgnoreCase = True
  oRegExp.Global = True
  
  oMatchesReturn = Null
  FoundRegExpMatches = False
  If (oRegExp.Test(sString) = True) Then
    Set oMatchesReturn = oRegExp.Execute(sString)
    FoundRegExpMatches = True
  End If
End Function

''
' ...
Function SplitByTab(ByVal sString)
  SplitByTab = Array()
  If (InStr(sString, vbTab) > 0) Then
    SplitByTab = Split(sString, vbTab, -1)
  Else
    SplitByTab = Array(sString)
  End If
End Function

''
' ...
Function GetPotCreationDate()
  Dim oNow, sYear, sMonth, sDay, sHour, sMinute
  
  oNow = Now()
  sYear = Year(oNow)
  sMonth = Month(oNow)
  If (sMonth < 10) Then sMonth = "0" & sMonth
  sDay = Day(oNow)
  If (sDay < 10) Then sDay = "0" & sDay
  sHour = Hour(oNow)
  If (sHour < 10) Then sHour = "0" & sHour
  sMinute = Minute(oNow)
  If (sMinute < 10) Then sMinute = "0" & sMinute
  
  GetPotCreationDate = sYear & "-" & sMonth & "-" & sDay & " " & sHour & ":" & sMinute & "+0000"
End Function
