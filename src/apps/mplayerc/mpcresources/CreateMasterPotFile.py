'''
This script creates the master POT file (English.pot).

@author: Tim Gerundt, Arkadiy Shapkin
@copyright: 2007-2010

Released under the "GNU General Public License"

ID line follows -- this is updated by SVN
$Id: CreateMasterPotFile.vbs 2065 2010-06-20 16:42:14Z kinddragon $
'''

import os, codecs, re
#import win32file

from datetime import datetime
from collections import OrderedDict

NO_BLOCK            = -1
MENU_BLOCK          = 0
DIALOGEX_BLOCK      = 1
STRINGTABLE_BLOCK   = 2
VERSIONINFO_BLOCK   = 3
ACCELERATORS_BLOCK  = 4
BEGIN_BLOCK         = 5
END_BLOCK           = 6

PATH_ENGLISH_POT = "English.pot"
PATH_MERGE_RC = "../mplayerc.rc"
PATH_MERGELANG_RC = "MplayercLang.rc"

class CString:
    Comment = []
    References = []
    Context = None
    Id = ""
    Str = ""

#
def GetStringsFromRcFile(sRcFilePath):    
    oBlacklist = GetStringBlacklist("StringBlacklist.txt")
    
    oStrings = OrderedDict()
    
    if os.path.exists(sRcFilePath): #If the RC file exists...
        sRcFileName = os.path.basename(sRcFilePath)
        iLine = 0
        iBlockType = NO_BLOCK
        sCodePage = ""
        fContinuation = False
        oRcFile = codecs.open(sRcFilePath, 'r', 'utf-16-le')
        oLcFile = codecs.open(PATH_MERGELANG_RC, 'w', 'utf-16-le')
        for sLcLine in oRcFile.readlines(): #For all lines...
            sLine = sLcLine.strip()
            iLine = iLine + 1
            
            sReference = sRcFileName + ":" + str(iLine)
            sString = ""
            sComment = ""
            sContext = ""            
            curType = NO_BLOCK
            
            if fContinuation:
                pass # Nothing to do
            else:
                curType, oMatch = FoundRegExpIndex(sLine, 
                    ["(IDR_.*) MENU", "(IDD_.*) DIALOGEX", 
                    "STRINGTABLE", "(VS_.*) VERSIONINFO",
                    "(IDR_.*) ACCELERATORS", 
                    "\bBEGIN\b", "\bEND\b" ], 0)
                if curType != NO_BLOCK:
                    if curType == END_BLOCK: #If inside stringtable...
                        if iBlockType == STRINGTABLE_BLOCK: 
                            iBlockType = NO_BLOCK
                    elif curType != BEGIN_BLOCK: #IGNORE FOR SPEEDUP!
                        iBlockType = curType

                if sLine.find("//") == 0: #If comment line...
                    sLine = "" #IGNORE FOR SPEEDUP!
                elif curType == NO_BLOCK and len(sLine) > 0: #If NOT empty line...
                    if iBlockType==NO_BLOCK:
                        n, oMatch = FoundRegExpIndex(sLine,
                            ["LANGUAGE (LANG_\w*, SUBLANG_\w*)", "code_page\(([\d]+)\)"])
                        if n == 0: #LANGUAGE
                            sString = oMatch[0]
                            sComment = "LANGUAGE, SUBLANGUAGE"
                        elif n == 1: #code_page...
                            sString = oMatch[0]
                            sComment = "Codepage"
                            sCodePage = oMatch[0]

                    elif iBlockType==MENU_BLOCK or iBlockType==DIALOGEX_BLOCK or iBlockType==STRINGTABLE_BLOCK:
                        if sLine.find("\"") != -1: #If quote found (for speedup)...
                            #--------------------------------------------------------------------------------
                            # Replace 1st string literal only - 2nd string literal specifies control class!
                            #--------------------------------------------------------------------------------
                            oMatch = FoundRegExpMatch(sLine, "\"((?:\"\"|[^\"])*)\"") #String...
                            if oMatch:
                                sTemp = oMatch[0]
                                if len(sTemp) > 0 and sTemp not in oBlacklist: #If NOT blacklisted...
                                    sLcLine = sLcLine.replace("\"" + sTemp + "\"", "\"" + sReference + "\"", 1)
                                    sString = sTemp.replace("\"\"", "\\\"")
                                    oMatch = FoundRegExpMatch(sLine, "//#\. (.*?)$") #If found a comment for the translators...
                                    if oMatch:
                                        sComment = oMatch[0].strip()
                                    else:
                                        oMatch = FoundRegExpMatch(sLine, "//msgctxt (.*?)$") #If found a context for the translation...
                                        if oMatch:
                                            sContext = oMatch[0].strip()
                                            sComment = sContext

                    elif iBlockType==VERSIONINFO_BLOCK:
                        n, oMatch = FoundRegExpIndex(sLine,
                            ["BLOCK \"([0-9A-F]+)\"", "VALUE \"Comments\", \"(.*?)\\?0?\"", "VALUE \"Translation\", (.*?)$"])
                        oMatch = FoundRegExpMatch(sLine, "BLOCK \"([0-9A-F]+)\"")
                        if n == 0: #StringFileInfo.Block...
                            sString = oMatch[0]
                            sComment = "StringFileInfo.Block"
                        elif n == 1: #StringFileInfo.Value...
                            sString = oMatch[0]
                            sComment = "You should use a string like \"Translated by \" followed by the translator names for this string. It is ONLY VISIBLE in the StringFileInfo.Comments property from the final resource file!"
                        elif n == 2: #VarFileInfo.Translation...
                            sString = oMatch[0]
                            sComment = "VarFileInfo.Translation"
            
            if len(sString) > 0:
                sKey = sContext + sString
                if sKey not in oStrings: #If the key is already used...
                    oString = CString()
                else:
                    oString = oStrings[sKey]
                if len(sComment) > 0:
                    oString.Comment = sComment
                if len(oString.References) > 0:
                    oString.References.append(sReference)
                else:
                    oString.References = [sReference]
                oString.Context = sContext
                oString.Id = sString
                oString.Str = ""
                
                if sKey in oStrings: #If the key is already used...
                    oStrings[sKey] = oString
                else:
                    oStrings[sContext + sString] = oString
            if sLine == "#ifndef APSTUDIO_INVOKED":
                break
            oLcFile.write(sLcLine)
            fContinuation = sLine != "" and (",|".find(sLine[-1]) >= 0)

        oLcFile.write("MERGEPOT RCDATA \"English.pot\"\n")
        oRcFile.close();
        oLcFile.close();
    return oStrings, sCodePage

def GetStringBlacklist(sTxtFilePath):  
    oBlacklist = []
    
    if os.path.exists(sTxtFilePath): #If the blacklist file exists...
        oTxtFile = open(sTxtFilePath, "r")
        for line in oTxtFile: #For all lines...
            sLine = line.strip()
          
            if len(sLine) > 0:
                if sLine not in oBlacklist: #If the key is NOT already used...
                    oBlacklist.append(sLine)
        oTxtFile.close
    return oBlacklist

def CreateMasterPotFile(sPotPath, oStrings, sCodePage): 
    oPotFile = codecs.open(sPotPath, 'w', 'utf-8')   
    
    oPotFile.write("# This file is part from MPC-HC <http://mpc-hc.sourceforge.net/>\n")
    oPotFile.write("# Released under the \"GNU General Public License\"\n")
    oPotFile.write("#\n")
    oPotFile.write("# ID line follows -- this is updated by SVN\n")
    oPotFile.write("# $" + "Id: " + "$\n")
    oPotFile.write("#\n")
    oPotFile.write("msgid \"\"\n")
    oPotFile.write("msgstr \"\"\n")
    oPotFile.write("\"Project-Id-Version: MPC-HC\\n\"\n")
    oPotFile.write("\"Report-Msgid-Bugs-To: https://sourceforge.net/apps/trac/mpc-hc/report\\n\"\n")
    oPotFile.write("\"POT-Creation-Date: " + GetPotCreationDate() + "\\n\"\n")
    oPotFile.write("\"PO-Revision-Date: \\n\"\n")
    oPotFile.write("\"Last-Translator: \\n\"\n")
    oPotFile.write("\"Language-Team: English <mpchc-translate@lists.sourceforge.net>\\n\"\n")
    oPotFile.write("\"MIME-Version: 1.0\\n\"\n")
    oPotFile.write("\"Content-Type: text/plain; charset=utf-8\\n\"\n")
    oPotFile.write("\"Content-Transfer-Encoding: 8bit\\n\"\n")
    oPotFile.write("\"X-Poedit-Language: English\\n\"\n")
    oPotFile.write("\"X-Poedit-SourceCharset: utf-8\\n\"\n")
    oPotFile.write("\"X-Poedit-Basepath: .\\n\"\n")
    #oPotFile.write("\"X-Generator: CreateMasterPotFile.py\\n\"\n")
    oPotFile.write("\n")
    for sKey in oStrings.keys(): #For all strings...
        oString = oStrings[sKey]
        if len(oString.Comment) > 0: #If comment exists...
            oPotFile.write("#. " + oString.Comment + "\n")
        aReferences = oString.References
        for ref in aReferences: #For all references...
            oPotFile.write("#: " + ref + "\n")
        oPotFile.write("#, c-format\n")
        if len(oString.Context) > 0: #If context exists...
            oPotFile.write("msgctxt \"" + oString.Context + "\"\n")
        oPotFile.write("msgid \"" + oString.Id + "\"\n")
        oPotFile.write("msgstr \"\"\n")
        oPotFile.write("\n")
    oPotFile.close

#
def FoundRegExpMatch(sString, sPattern, flags = re.IGNORECASE):
    mo = re.search(sPattern, sString, flags)
    if mo:
        g = mo.groups()
        if g:
            return g
        else:
            return []
    return None

def FoundRegExpIndex(sString, exprs, flags = re.IGNORECASE):
    i = 0  
    for expr in exprs:
        oMatch = FoundRegExpMatch(sString, expr, flags)
        if oMatch:
            return i, oMatch
        i = i + 1
    return -1, None

def GetPotCreationDate():
    oNow = datetime.now()
    sYear = str(oNow.year)
    sMonth = str(oNow.month)
    if oNow.month < 10:
        sMonth = "0" + sMonth
    sDay = str(oNow.day)
    if oNow.day < 10:
        sDay = "0" + sDay
    sHour = str(oNow.hour)
    if oNow.hour < 10:
        sHour = "0" + sHour
    sMinute = str(oNow.minute)
    if oNow.minute < 10:
        sMinute = "0" + sMinute    
    return sYear + "-" + sMonth + "-" + sDay + " " + sHour + ":" + sMinute + "+0000"

if __name__ == '__main__':
    StartTime = datetime.now()

    print( "Creating POT file from Merge.rc...")

    oStrings, sCodePage = GetStringsFromRcFile(PATH_MERGE_RC)
    CreateMasterPotFile(PATH_ENGLISH_POT, oStrings, sCodePage)
    EndTime = datetime.now()
    Seconds = str((EndTime-StartTime).seconds)
    
    print("POT file created, after " + Seconds + " second(s).")
