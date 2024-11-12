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

// Qt
#include <QDir>
#include <QDirIterator>
#include <QString>
#include <QStringList>
#include <QFile>
#include <QFileInfo>

// MITK
#include <mitkIOUtil.h>
#include <mitkImageCast.h>
#include <mitkITKImageImport.h>
#include <mitkMorphologicalOperations.h>

// ITK
#include <itkConnectedComponentImageFilter.h>
#include <itkLabelShapeKeepNObjectsImageFilter.h>
#include <itkImageRegionIteratorWithIndex.h>
#include <itkBinaryBallStructuringElement.h>
#include <itkGrayscaleDilateImageFilter.h>
#include <itkRelabelComponentImageFilter.h>

#include "CemrgCommonUtils.h"
#include "CemrgAtrialModellingToolCmd.h"

/// AtialToolkitMode {UAC, FIBREMAP, SCALARMAP, LATFIELD, STIM, LABELS, VIS, PARFILE}
void CemrgAtrialModellingToolCmd::SetModeOfOperation(AtialToolkitMode atkm) {
    switch (atkm) {
        case UAC: mode = "uac"; break;
        case FIBREMAP: mode = "fibremap"; break;
        case SCALARMAP: mode = "scalarmap"; break;
        case LATFIELD: mode = "latfield"; break;
        case STIM: mode = "stim"; break;
        case LABELS: mode = "labels"; break;
        case VIS: mode = "vis"; break;
        case PARFILE: mode = "getparfile"; break;
        default: mode= ""; break;
    }
}

QString CemrgAtrialModellingToolCmd::GetParfile(AtrialToolkitParamfiles atkm, QString meshname) {
    QString parfileName = "";
    switch (atkm) {
        case RAA_PARAM: parfileName = "carpf_laplace_RAA.par"; break;
        case PV2_4Ch_PARAM: parfileName = "carpf_laplace_PV2_4Ch.par"; break;
        case PA_4Ch_PARAM: parfileName = "carpf_laplace_PA_4Ch.par"; break;
        case LS_PARAM: parfileName = "carpf_laplace_LS.par"; break;
        case single_LR_P_PARAM: parfileName = "carpf_laplace_single_LR_P.par"; break;
        case single_LR_A_PARAM: parfileName = "carpf_laplace_single_LR_A.par"; break;
        case EE_RA_PARAM: parfileName = "carpf_laplace_EE_RA.par"; break;
        case PV4_PARAM: parfileName = "carpf_laplace_PV4.par"; break;
        case LAA_4Ch_PARAM: parfileName = "carpf_laplace_LAA_4Ch.par"; break;
        case alpha_PARAM: parfileName = "carpf_laplace_alpha.par"; break;
        case PV1_PARAM: parfileName = "carpf_laplace_PV1.par"; break;
        case PV3_4Ch_PARAM: parfileName = "carpf_laplace_PV3_4Ch.par"; break;
        case single_UD_P_PARAM: parfileName = "carpf_laplace_single_UD_P.par"; break;
        case alpha_RA_PARAM: parfileName = "carpf_laplace_alpha_RA.par"; break;
        case PA_4Ch_RA_PARAM: parfileName = "carpf_laplace_PA_4Ch_RA.par"; break;
        case EE_PARAM: parfileName = "carpf_laplace_EE.par"; break;
        case LS_4Ch_RA_PARAM: parfileName = "carpf_laplace_LS_4Ch_RA.par"; break;
        case LS_4Ch_PARAM: parfileName = "carpf_laplace_LS_4Ch.par"; break;
        case LAA_PARAM: parfileName = "carpf_laplace_LAA.par"; break;
        case PV4_4Ch_PARAM: parfileName = "carpf_laplace_PV4_4Ch.par"; break;
        case PV3_PARAM: parfileName = "carpf_laplace_PV3.par"; break;
        case beta_PARAM: parfileName = "carpf_laplace_beta.par"; break;
        case beta_RA_PARAM: parfileName = "carpf_laplace_beta_RA.par"; break;
        case PA_RA_PARAM: parfileName = "carpf_laplace_PA_RA.par"; break;
        case PV2_PARAM: parfileName = "carpf_laplace_PV2.par"; break;
        case PA_PARAM: parfileName = "carpf_laplace_PA.par"; break;
        case PV1_4Ch_PARAM: parfileName = "carpf_laplace_PV1_4Ch.par"; break;
        case single_UD_A_PARAM: parfileName = "carpf_laplace_single_UD_A.par"; break;
        case LS_RA_PARAM: parfileName = "carpf_laplace_LS_RA.par"; break;
        default: parfileName = ""; break;
    }
    
    if (parfileName.isEmpty()) {
        return "ERROR_INVALID_PARFILE";
    }

    return GetParfile(parfileName, meshname);
}
QString CemrgAtrialModellingToolCmd::GetParfile(QString parfileName, QString meshname) {
    QString executableName, outAbsolutePath;
    QStringList arguments = PrepareDockerExecution(executableName, outAbsolutePath, AtialToolkitMode::PARFILE);

    if (arguments.isEmpty()) return "VOLUME_NOT_SET";

    arguments << "--lapsolve-par" << parfileName;
    if (!meshname.isEmpty()) {
        arguments << "lapsolve-msh" << meshname;
    }

    QString expectedOutput = home.absoluteFilePath(parfileName);
    bool successful = ExecuteCommand(executableName, arguments, expectedOutput);
    if (successful) {
        outAbsolutePath = expectedOutput;
    } 

    return outAbsolutePath;
}

QString CemrgAtrialModellingToolCmd::UniversalAtrialCoordinates(QString stage, QString layer, QString fibre, QString meshname, QStringList tags, QStringList landmarks, bool fourch, bool noraa, int scale) {
    QString executableName, outAbsolutePath;
    QStringList arguments = PrepareDockerExecution(executableName, outAbsolutePath, AtialToolkitMode::UAC);

    if (arguments.isEmpty()) return "VOLUME_NOT_SET";

    arguments << "--uac-stage" << stage; 
    arguments << "--atrium" << atrium;
    arguments << "--layer" << layer;
    if (!fibre.isEmpty()) 
        arguments << "--fibre" << fibre;

    arguments << "--msh" << meshname;

    if (tags.size() > 0) {
        arguments << "--tags";
        for (int ix = 0; ix < tags.size(); ix++) {
            arguments << tags.at(ix);
        }
    }

    if (landmarks.size() > 0) {
        arguments << "--landmarks" << home.relativeFilePath(landmarks.at(0));
        if (landmarks.size() > 1) {
            arguments << "--regions" << home.relativeFilePath(landmarks.at(1));
        }
    }

    if (fourch) {
        arguments << "--fourch";
    }

    if (noraa) {
        arguments << "--noraa";
    }

    arguments << QString::number(scale);

    QStringList outputs;
   if (stage.compare("1")) {
       outputs << "LSbc1.vtx" << "LSbc2.vtx" << "PAbc1.vtx" << "PAbc2.vtx";

   } else if (stage.compare("2a")){
       outputs << "AnteriorMesh.elem" << "AnteriorMesh.pts" << "PosteriorMesh.elem" << "PosteriorMesh.pts";

   } else if (stage.compare("2b")){
       outputs << "Labelled_Coords_2D_Rescaling_v3_C.elem" << "Labelled_Coords_2D_Rescaling_v3_C.pts";
   }

   QString expectedOutput = home.absoluteFilePath(outputs.at(0));
   bool successful = ExecuteCommand(executableName, arguments, expectedOutput);
    if(successful){
        outAbsolutePath = expectedOutput;
    }

    return outAbsolutePath;
}

QString CemrgAtrialModellingToolCmd::FibreMapping(QString layer, QString fibre, QString meshname, bool msh_endo_epi, QString outputSuffix, bool fourch, QString biproj) {
    QString executableName, outAbsolutePath;
    QStringList arguments = PrepareDockerExecution(executableName, outAbsolutePath, AtialToolkitMode::FIBREMAP);

    if (arguments.isEmpty()) return "VOLUME_NOT_SET";

    arguments << "--atrium" << atrium;
    arguments << "--layer" << layer;
    arguments << "--fibre" << fibre;
    arguments << "--msh" << meshname;
    arguments << "--output" << outputSuffix;

    if (fourch) {
       arguments << "--fourch";
   }

   if (msh_endo_epi){
       arguments << "--msh-endo" << "Labelled";
       arguments << "--msh-epi" << "Labelled";
   }

   if (!biproj.isEmpty())
        arguments << "--fibre-biproj" << biproj;

   QString omsh = outputSuffix + "_" + layer + "_" + fibre;
   omsh += layer.contains("bilayer") ? "_Bilayer" : "";

   QStringList outputs;
   outputs << omsh+".pts" << omsh+".elem";

   QString outPath = home.absoluteFilePath(outputs.at(0));
   bool successful = ExecuteCommand(executableName, arguments, outPath);

    if(successful){
        outAbsolutePath = outPath;
    } 

    return outAbsolutePath;
}

QString CemrgAtrialModellingToolCmd::ScalarMapping(QString meshname, bool bb, AtrialToolkitScalarMap scalatSuffix) {
    QString executableName, outAbsolutePath;

    QString scalarFilesuffix;
    switch (scalatSuffix) {
        case SAN: scalarFilesuffix = "SAN"; break;
        case CT: scalarFilesuffix = "CT"; break;
        case PM: scalarFilesuffix = "PM"; break;
        case BB: scalarFilesuffix = "BB"; break;
        default: scalarFilesuffix = "";
    }

    if (scalarFilesuffix.isEmpty()) {
        return "ERROR_INVALID_SCALAR";
    }

    QStringList arguments = PrepareDockerExecution(executableName, outAbsolutePath, AtialToolkitMode::SCALARMAP);

    if (arguments.isEmpty()) return "VOLUME_NOT_SET";

    arguments << "--atrium" << atrium;
    arguments << "--msh" << meshname;
    if (bb) {
        arguments << "--bb";
    }

    arguments << "--scalar-file-suffix" << scalarFilesuffix;

    QString expectedOutput = home.absoluteFilePath("MappedScalar_"+scalarFilesuffix+".dat");
    bool successful = ExecuteCommand(executableName, arguments, expectedOutput);
    if (successful) {
        outAbsolutePath = expectedOutput;
    }

    return outAbsolutePath;
}

QString CemrgAtrialModellingToolCmd::Labels(QString labelsStage, bool labelsLandmarks, double labelsThresh, QString meshname, QStringList landmarks, int scale) {
    QString executableName, outAbsolutePath;
    QStringList arguments = PrepareDockerExecution(executableName, outAbsolutePath, AtialToolkitMode::LABELS);

    if (arguments.isEmpty()) return "VOLUME_NOT_SET";

    arguments << "--labels-stage" << labelsStage;
    if (labelsLandmarks) {
        arguments << "--labels-landmarks";
    }
    arguments << "--labels-thresh" << QString::number(labelsThresh);
    arguments << "--msh" << meshname;
    arguments << "--landmarks" << home.relativeFilePath(landmarks.at(0));
    if (landmarks.size() > 1) {
        arguments << "--regions" << home.relativeFilePath(landmarks.at(1));
    }
    arguments << QString::number(scale);

    QStringList outputs;
    outputs << "Labelled_Coords_2D_Rescaling_v3_C.elem" << "Labelled_Coords_2D_Rescaling_v3_C.pts";

    QString expectedOutput = home.absoluteFilePath(outputs.at(0));
    bool successful = ExecuteCommand(executableName, arguments, expectedOutput);
    if (successful) {
        outAbsolutePath = expectedOutput;
    }

    return outAbsolutePath;
}

// helpers
QStringList CemrgAtrialModellingToolCmd::PrepareDockerExecution(QString &executableName, QString &outAbsolutePath, AtialToolkitMode atkm) {
    SetDockerImageUac(dockerTag);
    executableName = GetDockerExecutableName();
    outAbsolutePath = "ERROR_IN_PROCESSING";

    if (!IsVolumeSet()) {
        return QStringList();
    }

    SetModeOfOperation(atkm);

    QStringList arguments = GetDockerArguments(dataVolume);
    arguments << mode;

    return arguments;
}
