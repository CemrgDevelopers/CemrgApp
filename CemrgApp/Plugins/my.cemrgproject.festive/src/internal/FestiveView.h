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
 * Fetal Motion Corrected Slice to Volume Registration (Festive) Plugin
 *
 * Cardiac Electromechanics Research Group
 * http://www.cemrg.co.uk/
 * orod.razeghi@kcl.ac.uk
 *
 * This software is distributed WITHOUT ANY WARRANTY or SUPPORT!
 *
=========================================================================*/

#ifndef FestiveView_h
#define FestiveView_h

#include <berryISelectionListener.h>
#include <QmitkAbstractView.h>
#include <CemrgCommandLine.h>
#include "ui_FestiveViewUILogin.h"
#include "ui_FestiveViewUIReconst.h"
#include "ui_FestiveViewControls.h"


class FestiveView : public QmitkAbstractView {

    // this is needed for all Qt objects that should have a Qt meta-object
    // (everything that derives from QObject and wants to have signal/slots)
    Q_OBJECT

public:

    static const std::string VIEW_ID;
    virtual void CreateQtPartControl(QWidget *parent);

protected slots:

    /// \brief Called when the user clicks the GUI button
    void LoadDICOM();
    void ProcessIMGS();
    void ConvertNII();
    void MaskIMGS();
    void SaveMASK();
    void EstablishConnection();
    void CopyServer();
    void Reconstruction();
    void Download();
    void Reset();

protected:

    virtual void SetFocus();
    /// \brief called by QmitkFunctionality when DataManager's selection has changed
    virtual void OnSelectionChanged(berry::IWorkbenchPart::Pointer source, const QList<mitk::DataNode::Pointer>& nodes);

    Ui::FestiveViewUILogin m_UILogin;
    Ui::FestiveViewUIReconst m_UIReconst;
    Ui::FestiveViewControls m_Controls;

private:

    QString userID;
    QString server;
    QString outName;
    QString directory;
    QStringList imgsList;
    std::unique_ptr<CemrgCommandLine> cmd;
};

#endif // FestiveView_h
