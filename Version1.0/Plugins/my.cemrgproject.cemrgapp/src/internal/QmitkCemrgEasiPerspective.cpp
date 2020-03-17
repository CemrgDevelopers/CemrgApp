/*=========================================================================
  
 Program:   Medical Imaging & Interaction Toolkit
 Language:  C++
 Date:      $Date$
 Version:   $Revision$
 
 Copyright (c) German Cancer Research Center, Division of Medical and
 Biological Informatics. All rights reserved.
 See MITKCopyright.txt or http://www.mitk.org/copyright.html for details.
 
 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notices for more information.
 
 =========================================================================*/
/*=========================================================================
 *
 * Eikonal Activation Simulation (EASI) Plugin for MITK
 *
 * Cardiac Electromechanics Research Group
 * http://www.cemrg.co.uk/
 * orod.razeghi@kcl.ac.uk
 *
 * This software is distributed WITHOUT ANY WARRANTY or SUPPORT!
 *
=========================================================================*/

#include "QmitkCemrgEasiPerspective.h"
#include "berryIViewLayout.h"


QmitkCemrgEasiPerspective::QmitkCemrgEasiPerspective() {

}

QmitkCemrgEasiPerspective::QmitkCemrgEasiPerspective(const QmitkCemrgEasiPerspective& other) : QObject() {

    Q_UNUSED(other)
    throw std::runtime_error("Copy constructor not implemented");
}

void QmitkCemrgEasiPerspective::CreateInitialLayout(berry::IPageLayout::Pointer layout) {

    QString editorArea = layout->GetEditorArea();
    layout->AddView("my.cemrgproject.views.easiview", berry::IPageLayout::LEFT, 0.17f, editorArea);
    berry::IFolderLayout::Pointer folder = layout->CreateFolder(
                "folder", berry::IPageLayout::BOTTOM, 0.6f, "my.cemrgproject.views.easiview");
    folder->AddView("org.mitk.views.datamanager");
    berry::IViewLayout::Pointer lo = layout->GetViewLayout("my.cemrgproject.views.easiview");
    lo->SetCloseable(false);
}
