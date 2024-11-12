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
 * Four Chamber Tools (inherits CemrgCommandLine)
 *
 * Cardiac Electromechanics Research Group
 * http://www.cemrgapp.com
 * orod.razeghi@kcl.ac.uk
 *
 * This software is distributed WITHOUT ANY WARRANTY or SUPPORT!
 *
=========================================================================*/

#ifndef CemrgAtrialModellingToolCmd_h
#define CemrgAtrialModellingToolCmd_h

#include <QString>
#include <QDir>

#include <mitkIOUtil.h>
#include <mitkImage.h>
#include <mitkCommon.h>

// VTK
#include <vtkSmartPointer.h>
#include <vtkConnectivityFilter.h>
#include <vtkIdList.h>
#include <vtkRegularPolygonSource.h>

// ITK
#include <itkImage.h>
#include <itkImageRegionIterator.h>

#include "FourChamberCommon.h"
#include "CemrgCommandLine.h"

enum AtialToolkitMode
{
    UAC,
    FIBREMAP,
    SCALARMAP,
    LATFIELD,
    STIM,
    LABELS,
    VIS,
    PARFILE
};
enum AtrialToolkitParamfiles {
    RAA_PARAM,
    PV2_4Ch_PARAM,
    PA_4Ch_PARAM,
    LS_PARAM,
    single_LR_P_PARAM,
    single_LR_A_PARAM,
    EE_RA_PARAM,
    PV4_PARAM,
    LAA_4Ch_PARAM,
    alpha_PARAM,
    PV1_PARAM,
    PV3_4Ch_PARAM,
    single_UD_P_PARAM,
    alpha_RA_PARAM,
    PA_4Ch_RA_PARAM,
    EE_PARAM,
    LS_4Ch_RA_PARAM,
    LS_4Ch_PARAM,
    LAA_PARAM,
    PV4_4Ch_PARAM,
    PV3_PARAM,
    beta_PARAM,
    beta_RA_PARAM,
    PA_RA_PARAM,
    PV2_PARAM,
    PA_PARAM,
    PV1_4Ch_PARAM,
    single_UD_A_PARAM,
    LS_RA_PARAM
};

enum AtrialToolkitScalarMap {
    SAN, 
    CT, 
    PM, 
    BB
};
class MITKCEMRGAPPMODULE_EXPORT CemrgAtrialModellingToolCmd : public CemrgCommandLine {

public:
    mitkClassMacro(CemrgAtrialModellingToolCmd, CemrgCommandLine)
    // [uac|fibremap|scalarmap|latfield|stim|labels|vis|getparfile

    void SetModeOfOperation(AtialToolkitMode atkm);

    QString GetParfile(AtrialToolkitParamfiles atkm, QString meshname="");
    QString GetParfile(QString parfileName, QString meshname = ""); // meshname="" means use default name

    QString UniversalAtrialCoordinates(QString stage, QString layer, QString fibre, QString meshname, QStringList tags, QStringList landmarks, bool fourch=false, bool noraa=false, int scale=1000);
    inline QString UacStage1(QString layer, QString fibre, QString meshname, QStringList tags, QStringList landmarks, bool fourch = false, bool noraa = false, int scale = 1000) { return UniversalAtrialCoordinates("1", layer, fibre, meshname, tags, landmarks, fourch, noraa, scale); };
    inline QString UacStage2a(QString layer, QString fibre, QString meshname, QStringList tags, QStringList landmarks, bool fourch = false, bool noraa = false, int scale = 1000) { return UniversalAtrialCoordinates("2a", layer, fibre, meshname, tags, landmarks, fourch, noraa, scale); };
    inline QString UacStage2b(QString layer, QString fibre, QString meshname, QStringList tags, QStringList landmarks, bool fourch = false, bool noraa = false, int scale = 1000) { return UniversalAtrialCoordinates("2b", layer, fibre, meshname, tags, landmarks, fourch, noraa, scale); };

    QString FibreMapping(QString layer, QString fibre, QString meshname, bool msh_endo_epi, QString outputSuffix="Fibre", bool fourch=false, QString biproj="100");
    QString ScalarMapping(QString meshname, bool bb, AtrialToolkitScalarMap scalatSuffix);
    QString Labels(QString labelsStage, bool labelsLandmarks, double labelsThresh, QString meshname, QStringList landmarks, int scale = 1000);

    // Helper functions
    QStringList PrepareDockerExecution(QString &executableName, QString &outAbsolutePath, AtialToolkitMode atkm);

    // inlines 
    inline void SetDockerTag(QString tag) { dockerTag = tag; };
    inline void SetVolume(QString volume) { dataVolume = volume; home = QDir(dataVolume); };
    inline void SetAtrium(QString atriumName) { atrium = atriumName; };
    inline void SetAtriumToLA() { atrium = "la"; };
    inline void SetAtriumToRA() { atrium = "ra"; };
    inline bool IsVolumeSet() { return dataVolume.isEmpty(); };

protected:
private:
    QString dockerTag; 
    QString dataVolume; // directory mounted in /data inside container
    QDir home;
    QString mode; // mode of operation of container
    QString atrium; // atrium name "la" or "ra"
};
#endif // CemrgAtrialModellingToolCmd_h
