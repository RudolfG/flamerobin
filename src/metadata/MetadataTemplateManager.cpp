/*
  Copyright (c) 2004-2011 The FlameRobin Development Team

  Permission is hereby granted, free of charge, to any person obtaining
  a copy of this software and associated documentation files (the
  "Software"), to deal in the Software without restriction, including
  without limitation the rights to use, copy, modify, merge, publish,
  distribute, sublicense, and/or sell copies of the Software, and to
  permit persons to whom the Software is furnished to do so, subject to
  the following conditions:

  The above copyright notice and this permission notice shall be included
  in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
  CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
  TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


  $Id$

*/

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include <wx/arrstr.h>
#include <wx/dir.h>
#include <wx/regex.h>

#include "config/Config.h"
#include "core/CodeTemplateProcessor.h"
#include "core/FRError.h"
#include "core/StringUtils.h"
#include "metadata/metadataitem.h"
#include "metadata/MetadataTemplateManager.h"

//-----------------------------------------------------------------------------
TemplateDescriptor::TemplateDescriptor(const wxFileName& templateFileName,
    MetadataItem* metadataItem)
    : templateFileNameM(templateFileName), metadataItemM(metadataItem),
    menuPositionM(0)
{
    loadDescriptionFromConfigFile();
}
//-----------------------------------------------------------------------------
void TemplateDescriptor::loadDescriptionFromConfigFile()
{
    wxFileName confFileName(templateFileNameM);
    confFileName.SetExt(wxT("info"));
    if (confFileName.FileExists())
    {
        configM.setConfigFileName(confFileName);
        menuCaptionM = expandTemplateCommands(
            configM.get(wxT("templateInfo/menuCaption"),
            templateFileNameM.GetName()));
        menuPositionM = configM.get(wxT("templateInfo/menuPosition"), 0);
        matchesTypeM = expandTemplateCommands(
            configM.get(wxT("templateInfo/matchesType"), matchesTypeM));
        if (!matchesTypeM)
            matchesTypeM = wxT(".*");
        matchesNameM = expandTemplateCommands(
            configM.get(wxT("templateInfo/matchesName"), matchesNameM));
        if (!matchesNameM)
            matchesNameM = wxT(".*");
        wxString matchesWhen = getBooleanAsString(true);
        matchesWhenM = getStringAsBoolean(expandTemplateCommands(
            configM.get(wxT("templateInfo/matchesWhen"), matchesWhen)));
    }
    else
    {
        menuCaptionM = templateFileNameM.GetName();
        menuPositionM = 0;
        matchesTypeM = wxT(".*");
        matchesNameM = wxT(".*");
        matchesWhenM = true;
    }
}
//-----------------------------------------------------------------------------
wxString TemplateDescriptor::expandTemplateCommands(const wxString& inputText) const
{
    wxString result = wxEmptyString;
    CodeTemplateProcessor tp(metadataItemM, 0);
    tp.processTemplateText(result, inputText, metadataItemM);
    return result;
 }
//-----------------------------------------------------------------------------
bool TemplateDescriptor::operator<(const TemplateDescriptor& right) const
{
    int p = getMenuPosition();
    int rp = right.getMenuPosition();
    if (p == rp)
        return (getMenuCaption().Cmp(right.getMenuCaption()) < 0);
    else
        return (p < rp);
}
//-----------------------------------------------------------------------------
wxString TemplateDescriptor::getMenuCaption() const
{
    if (!menuCaptionM.IsEmpty())
        return menuCaptionM;
    return templateFileNameM.GetName();
}
//-----------------------------------------------------------------------------
 bool TemplateDescriptor::matches(const MetadataItem* metadataItem) const
{
    wxRegEx typeRegEx(matchesTypeM, wxRE_ADVANCED);
    if (!typeRegEx.IsValid())
        throw FRError(_("Invalid regex"));
    if (typeRegEx.Matches(metadataItem->getTypeName()))
    {
        wxRegEx nameRegEx(matchesNameM, wxRE_ADVANCED);
        if (!nameRegEx.IsValid())
            throw FRError(_("Invalid regex"));
        if (nameRegEx.Matches(metadataItem->getName_()))
            return matchesWhenM;
    }
    return false;
}
//-----------------------------------------------------------------------------
//! needed in checkDescriptorsSorted() to sort on objects instead of pointers
bool templateDescriptorPointerLT(const TemplateDescriptorPtr left,
    const TemplateDescriptorPtr right)
{
    return *left < *right;
}
//-----------------------------------------------------------------------------
MetadataTemplateManager::MetadataTemplateManager(MetadataItem* metadataItem)
    : metadataItemM(metadataItem)
{
    collectDescriptors();
}
//-----------------------------------------------------------------------------
void MetadataTemplateManager::collectDescriptors()
{
    wxArrayString fileNames;
    // Collect predefined and user-defined template descriptors.
    // A user-defined descriptor will supercede a predefined one with the same
    // base name.
    // A user may also remove a predefined template by overriding it with one
    // that does not match any object.
    wxDir::GetAllFiles(config().getCodeTemplatesPath(), &fileNames,
        wxT("*.template"), wxDIR_FILES);
    wxString userPath = config().getUserCodeTemplatesPath();
    if (wxDir::Exists(userPath))
        wxDir::GetAllFiles(userPath, &fileNames, wxT("*.template"), wxDIR_FILES);
    descriptorsM.clear();
    for (wxString::size_type i = 0; i < fileNames.Count(); i++)
    {
        wxFileName fileName(fileNames[i]);
        TemplateDescriptorPtr tdp = findDescriptor(fileName.GetName());
        if (tdp)
        {
            // Present already - override it (and re-check matching).
            tdp->setTemplateFileName(fileName);
            if (!tdp->matches(metadataItemM))
                descriptorsM.remove(tdp);
        }
        else
        {
            tdp.reset(new TemplateDescriptor(fileName, metadataItemM));
            if (tdp->matches(metadataItemM))
                descriptorsM.push_back(tdp);
        }
    }
    // Sort everything by menu position.
    descriptorsM.sort(templateDescriptorPointerLT);
}
//-----------------------------------------------------------------------------
TemplateDescriptorPtr MetadataTemplateManager::findDescriptor(
    const wxString& baseFileName) const
{
    for (TemplateDescriptorList::const_iterator it = descriptorsBegin();
        it != descriptorsEnd(); ++it)
    {
        if ((*it)->getBaseFileName() == baseFileName)
            return (*it);
    }
    return TemplateDescriptorPtr();
}
//-----------------------------------------------------------------------------
TemplateDescriptorList::const_iterator MetadataTemplateManager::descriptorsBegin() const
{
    return descriptorsM.begin();
}
//-----------------------------------------------------------------------------
TemplateDescriptorList::const_iterator MetadataTemplateManager::descriptorsEnd() const
{
    return descriptorsM.end();
}
//-----------------------------------------------------------------------------