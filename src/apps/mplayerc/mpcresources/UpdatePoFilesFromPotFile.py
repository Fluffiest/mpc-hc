#
# This script updates the language PO files from the master POT file.
#
# Copyright (C) 2007-2010 by Tim Gerundt, Arkadiy Shapkin
# Released under the "GNU General License"
#
# ID line follows -- this is updated by SVN
# $Id: UpdatePoFilesFromPotFile.vbs 1990 2010-05-30 02:34:55Z kinddragon $

import os, sys, shutil, codecs, re

from datetime import datetime
from collections import OrderedDict

class CSubContent:
    sMsgCtxt2 = ""
    sMsgId2 = ""
    sMsgStr2 = ""
    sTranslatorComments = ""
    sExtractedComments = ""
    sReferences = ""
    sFlags = ""

def GetContentFromPoFile(sPoPath):
    reMsgCtxt = re.compile("^msgctxt \"(.*)\"$", re.IGNORECASE)
    reMsgId = re.compile("^msgid \"(.*)\"$", re.IGNORECASE)
    reMsgContinued = re.compile("^\"(.*)\"$", re.IGNORECASE)

    oContent = OrderedDict()

    iMsgStarted = 0
    sMsgId = ""
    sMsgCtxt = ""
    oMatch = None
    oSubContent = CSubContent()
    oTextFile = codecs.open(sPoPath, 'r', 'utf-8')
    for line in oTextFile: #For all lines...
        sLine = line.strip()
        if sLine != "": #if NOT empty line...
            if sLine[0] != "#": #if NOT comment line...
                if reMsgCtxt.search(sLine): #if "msgctxt"...
                    iMsgStarted = 1
                    oMatch = reMsgCtxt.search(sLine).groups()
                    sMsgCtxt = oMatch[0]
                    oSubContent.sMsgCtxt2 = sLine + "\n"
                elif reMsgId.search(sLine): #if "msgid"...
                    iMsgStarted = 2
                    oMatch = reMsgId.search(sLine).groups()
                    sMsgId = oMatch[0]
                    oSubContent.sMsgId2 = sLine + "\n"
                elif sLine[:8] == "msgstr \"": #if "msgstr"...
                    iMsgStarted = 3
                    oSubContent.sMsgStr2 = sLine + "\n"
                else:
                    m = reMsgContinued.search(sLine) #if "msgctxt", "msgid" or "msgstr" continued...
                    if (m):
                        if iMsgStarted == 1:
                            sMsgCtxt = sMsgCtxt + oMatch[0]
                            oSubContent.sMsgCtxt2 = oSubContent.sMsgCtxt2 + sLine + "\n"
                        elif iMsgStarted == 2:
                            oMatch = reMsgContinued.search(sLine).groups()
                            sMsgId = sMsgId + oMatch[0]
                            oSubContent.sMsgId2 = oSubContent.sMsgId2 + sLine + "\n"
                        elif iMsgStarted == 3:
                            oSubContent.sMsgStr2 = oSubContent.sMsgStr2 + sLine + "\n"   
            else: #if comment line...
                iMsgStarted = -1
                s = sLine[:2]
                if s == "#~": #Obsolete message...
                    iMsgStarted = 0
                elif s == "#.": #Extracted comment...
                    oSubContent.sExtractedComments = oSubContent.sExtractedComments + sLine + "\n"
                elif s == "#:": #Reference...
                    oSubContent.sReferences = oSubContent.sReferences + sLine + "\n"
                elif s == "#,": #Flag...
                    oSubContent.sFlags = oSubContent.sFlags + sLine + "\n"
                else: #Translator comment...
                    oSubContent.sTranslatorComments = oSubContent.sTranslatorComments + sLine + "\n"
        elif iMsgStarted != 0: #if empty line AND there is pending translation...
            iMsgStarted = 0 #Don't process same translation twice
            if sMsgId == "": 
                sMsgId = "__head__"
            if (sMsgCtxt + sMsgId) not in oContent: #if the key is NOT already used...
                oContent[sMsgCtxt + sMsgId] = oSubContent            
            sMsgCtxt = ""
            oSubContent = CSubContent()
    oTextFile.close()
    return oContent

def CreateUpdatedPoFile(sPoPath, oEnglishPotContent, oLanguagePoContent):
    #--------------------------------------------------------------------------------
    # Backup the old PO file...
    #--------------------------------------------------------------------------------
    sBakPath = sPoPath + ".bak"
    if os.path.exists(sBakPath):
        os.remove(sBakPath)
    
    shutil.move(sPoPath, sBakPath)
    #--------------------------------------------------------------------------------

    oPoFile = codecs.open(sPoPath, 'w', 'utf-8')

    oLanguage = oLanguagePoContent["__head__"]
    oPoFile.write(oLanguage.sTranslatorComments)
    oPoFile.write(oLanguage.sMsgId2)
    oPoFile.write(oLanguage.sMsgStr2)
    oPoFile.write("\n")
    for sKey in oEnglishPotContent.keys(): #For all English content...
        if sKey != "__head__":
            oEnglish = oEnglishPotContent[sKey]
            if sKey in oLanguagePoContent: #if translation exists...
                oLanguage = oLanguagePoContent[sKey]
            else: #if translation NOT exists...
                oLanguage = oEnglish
            
            oPoFile.write(oLanguage.sTranslatorComments)
            oPoFile.write(oEnglish.sExtractedComments)
            oPoFile.write(oEnglish.sReferences)
            oPoFile.write(oLanguage.sFlags)
            oPoFile.write(oLanguage.sMsgCtxt2)
            oPoFile.write(oLanguage.sMsgId2)
            oPoFile.write(oLanguage.sMsgStr2)
            oPoFile.write("\n")
    oPoFile.close()

if __name__ == '__main__':
    StartTime = datetime.now()

    print("Updating PO files from POT file...")

    oEnglishPotContent = GetContentFromPoFile("English.pot")
    if len(oEnglishPotContent) == 0:
        raise "Error reading content from English.pot"
    oLanguages = sys.argv
    del oLanguages[0]
    if len(oLanguages) == 0:
        oLanguages = os.listdir(".")
    for oLanguage in oLanguages: #For all languages...
        sLanguage = oLanguage
        ext = os.path.splitext(sLanguage)[1].lower()
        if ext == ".po":
            print(os.path.basename(sLanguage))
                
            oLanguagePoContent = GetContentFromPoFile(sLanguage)
            if len(oLanguagePoContent) > 0: #if content exists...
                CreateUpdatedPoFile(sLanguage, oEnglishPotContent, oLanguagePoContent)

    EndTime = datetime.now()
    Seconds = str((EndTime-StartTime).seconds)

    print("All PO files updated, after " + Seconds + " second(s).")
