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
#include "CemrgFourChamberCmd.h"

bool CemrgFourChamberCmd::CheckCarpDirectory(QString dir) {
    bool success = false;
    if (dir == "") {
        MITK_ERROR << "CARP directory not set!";
        return success;
    }

    QDir carpDir(dir);
    if (!carpDir.exists()) {
        MITK_ERROR << "CARP directory does not exist!";
        return success;
    }
    
    // iterate carpDir and check for required files
    QStringList requiredFiles = {"mguvc", "GlVTKConvert", "GlRuleFibers", "GlElemCenters", "carp.pt", "igbextract"};
    QDirIterator it(carpDir, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString file = it.next();
        for (int ix = 0; ix < requiredFiles.size(); ix++) {
            if (file.contains(requiredFiles.at(ix))) {
                requiredFiles.removeOne(requiredFiles.at(ix));
            }
        }
    }

    if (!requiredFiles.isEmpty()) {
        MITK_WARN << "CARP directory does not contain all required files! Below a list of missing files:";
        for (int ix = 0; ix < requiredFiles.size(); ix++) {
            MITK_WARN << requiredFiles.at(ix).toStdString();
        }
    }
    success = requiredFiles.isEmpty(); // means all files were found

    return success;
}

bool CemrgFourChamberCmd::ExecuteCarpless(QString executableName, QStringList arguments, QString outputPath, bool isOutputFile) {
    bool res = false;
    if (carpless) {
        MITK_WARN << "CARPless mode!";
        MITK_INFO << this->PrintFullCommand(executableName, arguments);
    } else {
        res = this->ExecuteCommand(executableName, arguments, outputPath, isOutputFile);
    }
    return res;
}

bool CemrgFourChamberCmd::CalculateUvcs(QString base_dir, FourChamberSubfolders fourch_sdirs, QString mesh_sdir, QString meshname, QString input_tags_parfile, QString etags_sdir, QString apex_sdir) {
    QString etags_folder = base_dir + "/" + fourch_sdirs.PAR + "/" + etags_sdir;
    QString mesh = base_dir + "/" + fourch_sdirs.MESH + "/" + mesh_sdir + "/" + meshname;
    QString surf_folder = base_dir + "/" + fourch_sdirs.UVC; 
    QString surf_folder_la = base_dir + "/" + fourch_sdirs.UVC_LA;
    QString surf_folder_ra = base_dir + "/" + fourch_sdirs.UVC_RA;

    std::cout << input_tags_parfile.toStdString() << '\n';
    std::cout << apex_sdir.toStdString() << '\n';

    QStringList n_list;
    // constant arguments for mguvc
    const bool LAPSOL = true;
    const bool C_APEX = true;
    const QString ID_SOL = "$MESH/UVC_ek";

    // constant arguments for GlVTKConvert
    const bool TRIM_N = true;

    MITK_INFO << "Calculating UVCs for the BiV mesh...";
    QFile::copy(etags_folder + "/etags.sh", surf_folder + "/BiV/etags.sh");

    // mguvc
    bool mguvc_success = ExecuteMguvc(surf_folder, "BiV/BiV", "biv", "biv", "20", "BiV/etags.sh", "BiV/uvc/", LAPSOL, false);
    
    if (!mguvc_success) {
        MITK_ERROR << "mguvc failed!";
        return false;
    }

    n_list << "BiV/uvc/BiV.uvc_phi.dat";
    n_list << "BiV/uvc/BiV.uvc_z.dat";   
    n_list << "BiV/uvc/BiV.uvc_ven.dat";
    n_list << "BiV/uvc/BiV.uvc_rho.dat";
    bool glvtkconvert_success = ExecuteGlVTKConvert(surf_folder, "BiV/BiV", n_list, "BiV/uvc/uvc", TRIM_N);

    if (!glvtkconvert_success) {
        MITK_ERROR << "GlVTKConvert (BiV/uvc/uvc) failed!";
        return false;
    }

    n_list.clear();

    // GlVTKConvert 
    n_list << "BiV/uvc/BiV.sol_apba_lap.dat";
    n_list << "BiV/uvc/BiV.sol_rvendo_lap.dat";
    n_list << "BiV/uvc/BiV.sol_endoepi_lap.dat";
    n_list << "BiV/uvc/BiV.sol_lvendo_lap.dat";
    glvtkconvert_success = ExecuteGlVTKConvert(surf_folder, "BiV/BiV", n_list, "BiV/uvc/laplace", TRIM_N);

    if (!glvtkconvert_success) {
        MITK_ERROR << "GlVTKConvert (BiV/uvc/laplace) failed!";
        return false;
    }

    n_list.clear();

    MITK_INFO << "Calculating UVCs for the LA mesh...";
    QFile::copy(etags_folder + "/etags_la.sh", surf_folder_la + "/la/etags.sh");

    // mguvc
    mguvc_success = ExecuteMguvc(surf_folder_la, "la/la", "lv", "lv", "20", "la/etags.sh", "la/uvc/", LAPSOL, C_APEX, ID_SOL);

    if (!mguvc_success) {
        MITK_ERROR << "mguvc (1) failed!";
        return false;
    }

    // GlVTKConvert
    n_list <<"la/uvc/la.uvc_phi.dat";
    n_list <<"la/uvc/la.uvc_z.dat";
    n_list <<"la/uvc/la.uvc_ven.dat";
    n_list <<"la/uvc/la.uvc_rho.dat";
    glvtkconvert_success = ExecuteGlVTKConvert(surf_folder_la, "la/la", n_list, "la/uvc/uvc", TRIM_N);

    if (!glvtkconvert_success) {
        MITK_ERROR << "GlVTKConvert (3) failed!";
        return false;
    }

    n_list.clear();

    MITK_INFO << "Calculating UVCs for the RA mesh...";
    QFile::copy(etags_folder + "/etags_ra.sh", surf_folder_ra + "/ra/etags.sh");

    // mguvc
    mguvc_success = ExecuteMguvc(surf_folder_ra, "ra/ra", "lv", "lv", "20", "ra/etags.sh", "ra/uvc/", LAPSOL, C_APEX, ID_SOL);

    if (!mguvc_success) {
        MITK_ERROR << "mguvc (ra/uvc/) failed!";
        return false;
    }

    // GlVTKConvert
    n_list <<"ra/uvc/ra.uvc_phi.dat";
    n_list <<"ra/uvc/ra.uvc_z.dat";
    n_list <<"ra/uvc/ra.uvc_ven.dat";
    n_list <<"ra/uvc/ra.uvc_rho.dat";
    glvtkconvert_success = ExecuteGlVTKConvert(surf_folder_ra, "ra/ra", n_list, "ra/uvc/uvc", TRIM_N);

    MITK_ERROR(!glvtkconvert_success) << "GlVTKConvert (ra/uvc/uvc) failed!";
    
    return glvtkconvert_success;
}

bool CemrgFourChamberCmd::ExecuteMguvc(QString directory, QString model_name, QString input_model, QString output_model, QString np, QString tags_file, QString output_dir, bool laplace_solution, bool custom_apex, QString id_solve) {
    QString output_path = directory + "/" + output_dir;
    QStringList arguments;

    if (!id_solve.isEmpty()) {
        arguments << "--ID="+id_solve;
    }

    arguments << "--model-name" << directory + "/" + model_name;
    arguments << "--input-model" << input_model << "--output-model" << output_model;
    arguments << "--np" << np;
    arguments << "--tags-file" << directory + "/" + tags_file;
    arguments << "--output-dir" << output_path;
    if (laplace_solution) {
        arguments << "--laplace-solution";
    }
    if (custom_apex) {
        arguments << "--custom-apex";
    }
    
    return ExecuteCarpless(mguvc(), arguments, output_path, false);
}

bool CemrgFourChamberCmd::ExecuteGlVTKConvert(QString directory, QString model, QStringList n_list, QString output_dir, bool trim_names) {
    QString output_path = directory + "/" + output_dir;
    QStringList arguments;

    arguments << "-m" << directory + "/" + model;
    for (int ix = 0; ix < n_list.size(); ix++) {
        arguments << "-n" << directory + "/" + n_list.at(ix);
    }
        
    arguments << "-o" << output_path;
    if (trim_names) {
            arguments << "--trim-names";
    }

    return ExecuteCarpless(GlVTKConvert(), arguments, output_path, false);
}

bool CemrgFourChamberCmd::ExecuteCarp_Pt(QString directory, QString meshname, QString par_sdir, QString parfile, QStringList stim_files, QString output_dir) {
    QString output_path = directory + "/" + output_dir;
    QString param_path = directory + "/" + par_sdir + "/" + parfile;

    QStringList arguments;
    arguments << "+F" << param_path;
    arguments << "-simID" << output_path;
    arguments << "-meshname" << directory + "/" + meshname;
    arguments << "-stimulus[0].vtx_file" << stim_files.at(0);
    arguments << "-stimulus[1].vtx_file" << stim_files.at(1);

    return ExecuteCarpless(carp_pt(), arguments, output_path, false);
}

bool CemrgFourChamberCmd::ExecuteIgbextract(QString directory, QString sdir, double small_f, double big_F, QString outname, QString name) {
    QString io_dir = directory + "/" + sdir;
    QString input_igb = io_dir + "/" + name;
    QFileInfo fi(input_igb); 
    
    if (outname.isEmpty()) {
        outname = fi.baseName();
    }

    if (!fi.exists()) {
        MITK_ERROR << ("IGB file [" + input_igb + "] not found").toStdString();
        return false;
    }

    outname += (outname.contains(".")) ? "" : ".dat";
    QString output_path = io_dir + "/" + outname;

    QStringList arguments;
    arguments << input_igb; 
    arguments << "-o" << "ascii";
    arguments << "-f" << QString::number(small_f);
    arguments << "-F" << QString::number(big_F);
    arguments << "-O" << output_path ;

    return ExecuteCarpless(igbextract(), arguments, output_path, true);

}

bool CemrgFourChamberCmd::ExecuteGlElemCenters(QString meshPath, QString outputPath) {
    QStringList arguments;

    arguments << "--meshname" << meshPath;
    arguments << "--output" << outputPath;

    return ExecuteCarpless(GlElemCenters(), arguments, outputPath, true);

}

QString CemrgFourChamberCmd::ExecuteSeg4ch(QString mode, QStringList modeArguments, QString expectedOutput) { 
    SetDockerImageSeg4ch();
    QString executablePath;
#if defined(__APPLE__)
    executablePath = "/usr/local/bin/";
#endif

    if (_base_directory.isEmpty()) {
        MITK_ERROR << "Base directory not set!";
        return "ERROR_IN_PROCESSING";
    }

    QStringList arguments = GetDockerArguments(_base_directory); 

    QString executableName = executablePath + "docker";
    QString outAbsolutePath = "ERROR_IN_PROCESSING";

    arguments << mode;

    for (int ix = 0; ix < modeArguments.size(); ix++) {
        arguments << modeArguments.at(ix);
    }

    bool successful = ExecuteCommand(executableName, arguments, _base_directory + "/" + expectedOutput);
    if(successful){
        MITK_INFO << ("Seg-4Ch command: " + mode + " successful").toStdString();
        outAbsolutePath = _base_directory + "/" + expectedOutput;
    } else{
        MITK_WARN << ("Error running 4Ch command: " + mode).toStdString();
    }

    return outAbsolutePath;
}

QStringList CemrgFourChamberCmd::GetArgumentList(QString seg_name) {
    QStringList arguments;
    if (!seg_name.isEmpty()) {
        arguments << "--seg-name" << seg_name;
    }
    if (!_points_file.isEmpty()) {
        arguments << "--points-json" << _points_file;
    }
    if (!_origin_spacing_file.isEmpty()) {
        arguments << "--origin-spacing-json" << _origin_spacing_file;
    }
    if (!_labels_file.isEmpty()) {
        arguments << "--labels-file" << _labels_file;
    }

    return arguments;
}

QString CemrgFourChamberCmd::DockerOriginAndSpacing(QString segName, QString dicomDir, QString outputName) {
    
    QStringList modeArguments;
    modeArguments << "--seg-name" << segName;
    modeArguments << "--dicom-dir" << dicomDir;
    modeArguments << "--output-name" << outputName;

    return ExecuteSeg4ch("origin", modeArguments, outputName);
}

QString CemrgFourChamberCmd::DockerExtractSurfaces(QString baseDirectory, QString parFolder, QString inputTagsFilename, QString apexSeptumFolder, QString meshname) {
    SetDockerImageFourch();
    QString executablePath;
#if defined(__APPLE__)
    executablePath = "/usr/local/bin/";
#endif
    QString executableName = executablePath + "docker";
    QString outAbsolutePath = "ERROR_IN_PROCESSING";

    QDir home(baseDirectory);
    QStringList arguments = GetDockerArguments(home.absolutePath());

    QString fourchCmd = "surfs";

    arguments << fourchCmd;
    arguments << "--par-folder" << home.relativeFilePath(parFolder);
    arguments << "--input-tags-setup" << home.relativeFilePath(inputTagsFilename);
    arguments << "--apex-septum-setup" << home.relativeFilePath(apexSeptumFolder);
    arguments << "--meshname" << home.relativeFilePath(meshname);

    QStringList outputs; 
    outputs << home.absolutePath() + "/surfaces_uvc_LA/la/la.vtk";
    outputs << home.absolutePath() + "/surfaces_uvc_RA/ra/ra.vtk";

    bool successful = ExecuteCommand(executableName, arguments, outputs.at(0));
    successful = successful && IsOutputSuccessful(outputs.at(1));

    if(successful){
        MITK_INFO << ("4Ch command: " + fourchCmd + " successful").toStdString();
        outAbsolutePath = outputs.at(0);
    } else{
        MITK_WARN << ("Error running 4Ch command: " + fourchCmd).toStdString();
    }

    return outAbsolutePath;
}

QString CemrgFourChamberCmd::DockerCorrectFibres(QString baseDirectory, QString meshname) {
    SetDockerImageFourch();
    QString executablePath;
#if defined(__APPLE__)
    executablePath = "/usr/local/bin/";
#endif
    QString executableName = executablePath + "docker";
    QString outAbsolutePath = "ERROR_IN_PROCESSING";

    QDir home(baseDirectory);
    QStringList arguments = GetDockerArguments(home.absolutePath());

    QString fourchCmd = "correctfibres";
    arguments << fourchCmd;
    arguments << "--meshname" << home.relativeFilePath(meshname);

    QString outputPath = home.absolutePath() + "/" + meshname + "_corrected.lon";
    bool successful = ExecuteCommand(executableName, arguments, outputPath);

    if(successful){
        MITK_INFO << ("4Ch command: " + fourchCmd + " successful").toStdString();
        outAbsolutePath = outputPath;
    } else{
        MITK_WARN << ("Error running 4Ch command: " + fourchCmd).toStdString();
    }

    return outAbsolutePath;
}

QString CemrgFourChamberCmd::DockerLaplacePrep(QString baseDirectory, QString atrium, QString afibSubdir, QString surfEndo, QString surfEpi) {
    SetDockerImageFourch();
    QString executablePath;
#if defined(__APPLE__)
    executablePath = "/usr/local/bin/";
#endif
    QString executableName = executablePath + "docker";
    QString outAbsolutePath = "ERROR_IN_PROCESSING";

    QDir home(baseDirectory);
    QStringList arguments = GetDockerArguments(home.absolutePath());

    QString fourchCmd = "laplace_prep";
    arguments << fourchCmd;
    arguments << "--mesh-path" << home.relativeFilePath(afibSubdir);
    arguments << "--atrium" << atrium;
    arguments << "--surf-endo" << surfEndo;
    arguments << "--surf-epi" << surfEpi;

    QString outputPath = home.absolutePath() + "/" + afibSubdir + "/" + atrium + "/" + surfEndo + ".vtx";
    bool successful = ExecuteCommand(executableName, arguments, outputPath);

    if(successful){
        MITK_INFO << ("4Ch command: " + fourchCmd + " successful").toStdString();
        outAbsolutePath = outputPath;
    } else{
        MITK_WARN << ("Error running 4Ch command: " + fourchCmd).toStdString();
    }

    return outAbsolutePath;    
}

QString CemrgFourChamberCmd::DockerSurfaceToVolume(QString baseDirectory, QString atrium, QString fibresEndo, QString fibresEpi, QString meshPathSuffix) {
    SetDockerImageFourch();
    QString executablePath;
#if defined(__APPLE__)
    executablePath = "/usr/local/bin/";
#endif
    QString executableName = executablePath + "docker";
    QString outAbsolutePath = "ERROR_IN_PROCESSING";

    QDir home(baseDirectory);
    QStringList arguments = GetDockerArguments(home.absolutePath());

    QString fourchCmd = "surf2vol";

    arguments << fourchCmd;
    arguments << "--atrium" << atrium;
    arguments << "--fibres-endo" << fibresEndo;
    arguments << "--fibres-epi" << fibresEpi;
    arguments << "--mesh-path" << meshPathSuffix;

    QString outputPath = home.absolutePath() + "/" + meshPathSuffix + "/" + atrium + "/" + atrium + "_fibres_l_sheet.vtk";
    bool successful = ExecuteCommand(executableName, arguments, outputPath);

    if(successful){
        MITK_INFO << ("4Ch command: " + fourchCmd + " successful").toStdString();
        outAbsolutePath = outputPath;
    } else{
        MITK_WARN << ("Error running 4Ch command: " + fourchCmd).toStdString();
    }

    return outAbsolutePath;
}

QString CemrgFourChamberCmd::DockerDefineTags(QString baseDirectory, QString dataSubdir, QString atriaSubdir, QString meshname, QString parfolder, QString inputTagsFilename, QString bbSettingsFilename) {
    SetDockerImageFourch();
    QString executablePath;
#if defined(__APPLE__)
    executablePath = "/usr/local/bin/";
#endif
    QString executableName = executablePath + "docker";
    QString outAbsolutePath = "ERROR_IN_PROCESSING";

    QDir home(baseDirectory);
    QStringList arguments = GetDockerArguments(home.absolutePath());

    QString presimFolder = home.relativeFilePath(dataSubdir);

    QString fourchCmd = "tags";
    arguments << fourchCmd;
    arguments << "--data-subdir" << home.relativeFilePath(dataSubdir);
    arguments << "--mesh-path" << home.relativeFilePath(atriaSubdir);
    arguments << "--meshname" << meshname;
    arguments << "--par-folder" << home.relativeFilePath(parfolder);
    arguments << "--input-tags-setup" << inputTagsFilename;
    arguments << "--bb-settings" << bbSettingsFilename;

    // msh_bb = f"{msh}_AV_FEC_BB"
    QString mshBB = meshname + "_AV_FEC_BB";
    QString outputPath = home.relativeFilePath(dataSubdir + "/" + mshBB + ".vtk");
    bool successful = ExecuteCommand(executableName, arguments, outputPath);

    if(successful){
        MITK_INFO << ("4Ch command: " + fourchCmd + " successful").toStdString();
        outAbsolutePath = outputPath;
    } else{
        MITK_WARN << ("Error running 4Ch command: " + fourchCmd).toStdString();
    }

    return outAbsolutePath;
}
QString CemrgFourChamberCmd::DockerSurfsPresim(QString baseDirectory, QString dataSubdir, QString parfolder, QString inputTagsFilename, QString mapSettingsFilename, QString apexCoordFilename, QString saCoordFilename) {
    SetDockerImageFourch();
    QString executablePath;
#if defined(__APPLE__)
    executablePath = "/usr/local/bin/";
#endif

    QString executableName = executablePath + "docker";
    QString outAbsolutePath = "ERROR_IN_PROCESSING";

    QDir home(baseDirectory);
    QStringList arguments = GetDockerArguments(home.absolutePath());

    QString fourchCmd = "presim";
    arguments << fourchCmd;
    arguments << "--data-subdir" << dataSubdir;
    arguments << "--par-folder" << parfolder;
    arguments << "--input-tags-setup" << inputTagsFilename;
    arguments << "--map-settings" << mapSettingsFilename;
    arguments << "--fch-apex" << apexCoordFilename;
    arguments << "--fch-sa" << saCoordFilename;

    QString mshBB = "myocardium_AV_FEC_BB"; // matching name in docker

    QString outputPath = home.absolutePath() + "/" + dataSubdir + "/surfaces_simulation/surfaces_rings/SVC.surf";

    bool successful = ExecuteCommand(executableName, arguments, outputPath);

    if(successful){
        MITK_INFO << ("4Ch command: " + fourchCmd + " successful").toStdString();
        outAbsolutePath = outputPath;
    } else{
        MITK_WARN << ("Error running 4Ch command: " + fourchCmd).toStdString();
    }

    return outAbsolutePath;

    // after calling this function, CARP needs to be called too
}
QString CemrgFourChamberCmd::DockerSplitFec(QString baseDirectory, QString meshPath, QString meshname, QString parfolder, QString inputTagsFilename, QString lvlrvTagsFilename) {
    SetDockerImageFourch();
    QString executablePath;
#if defined(__APPLE__)
    executablePath = "/usr/local/bin/";
#endif
    QString executableName = executablePath + "docker";
    QString outAbsolutePath = "ERROR_IN_PROCESSING";

    QDir home(baseDirectory);
    QStringList arguments = GetDockerArguments(home.absolutePath());

    QString fourchCmd = "fec";
    arguments << fourchCmd;
    arguments << "--mesh-path" << home.relativeFilePath(meshPath);
    arguments << "--meshname" << meshname;
    arguments << "--par-folder" << home.relativeFilePath(parfolder);
    arguments << "--input-tags-setup" << inputTagsFilename;
    arguments << "--lvlrv-tags" << lvlrvTagsFilename;

    QString outputPath = home.absolutePath() + "/sims_folder/" + meshname + "_lvrv.vtk";

    bool successful = ExecuteCommand(executableName, arguments, outputPath);

    if(successful){
        MITK_INFO << ("4Ch command: " + fourchCmd + " successful").toStdString();
        outAbsolutePath = outputPath;
    } else{
        MITK_WARN << ("Error running 4Ch command: " + fourchCmd).toStdString();
    }

    return QString();
}
//////////
/**
 * @brief DockerLandmarks function performs a specific task using Docker.
 *
 * This function takes several input parameters and executes a specific task using Docker.
 * The task involves processing a mesh and generating landmarks based on the provided input.
 *
 * @param baseDirectory The base directory ('heartFolder') where the task will be executed.
 * @param meshname The name of the mesh file to be processed (do not include extension, relative path to mesh).
 * @param surface The surface type to be used for landmark generation (endo or epi).
 * @param parfolder The folder containing the necessary parameter files (relative path to baseDirectory).
 * @param inputTagsFilename The filename of the input tags setup file (name only)
 * @param raaApexFile The filename of the RAA apex file (relative path to baseDirectory).
 * @param outputFolder The folder where the output will be saved (should be "atrial_fibres/UAC")
 *
 * @return The absolute path of the output file, or "ERROR_IN_PROCESSING" if an error occurred.
 */
//////////
QString CemrgFourChamberCmd::DockerLandmarks(QString baseDirectory, QString meshname, QString surface, QString parfolder, QString inputTagsFilename, QString raaApexFile, QString outputFolder) {
    SetDockerImageFourch();
    QString executablePath;
#if defined(__APPLE__)
    executablePath = "/usr/local/bin/";
#endif
    QString executableName = executablePath + "docker";
    QString outAbsolutePath = "ERROR_IN_PROCESSING";

    surface = surface.toLower();

    if (surface.contains("endo")) {
        surface = "endo";
    } else if (surface.contains("epi")) {
        surface = "epi";
    } else {
        MITK_ERROR << "Surface not recognized!";
        return outAbsolutePath;
    }

    QDir home(baseDirectory);
    QStringList arguments = GetDockerArguments(home.absolutePath());

    QString fourchCmd = "landmarks";
    arguments << fourchCmd;
    arguments << "--meshname" << home.relativeFilePath(meshname);
    arguments << "--surface" << surface;
    arguments << "--par-folder" << home.relativeFilePath(parfolder);
    arguments << "--input-tags-setup" << home.relativeFilePath(inputTagsFilename);
    arguments << "--raa-apex-file" << home.relativeFilePath(raaApexFile);
    arguments << "--output-folder" << home.relativeFilePath(outputFolder);

    QString expectedOutput = home.absoluteFilePath(outputFolder + "/landmarks.json");
    bool successful = ExecuteCommand(executableName, arguments, expectedOutput);

    if(successful){
        MITK_INFO << ("4Ch command: " + fourchCmd + " successful").toStdString();
        outAbsolutePath = expectedOutput;
    } else{
        MITK_WARN << ("Error running 4Ch command: " + fourchCmd).toStdString();
    }

    return outAbsolutePath;

}

QString CemrgFourChamberCmd::DockerMeshtoolGeneric(QString directory, QString command, QString subcommand, QStringList meshtoolArgs, QString expectedOutput) {
    SetDockerImageOpenCarp();
    QString executablePath;
#if defined(__APPLE__)
    executablePath = "/usr/local/bin/";
#endif
    QString executableName = executablePath + "docker";
    QString outAbsolutePath = "ERROR_IN_PROCESSING";

    QDir home(directory);
    QStringList arguments = QStringList();
    arguments << "run" << "--rm" << ("--volume=" + home.absolutePath() + ":/shared:z") << "--workdir=/shared";
    arguments << "docker.opencarp.org/opencarp/opencarp:latest";
    arguments << "meshtool";

    arguments << command;

    if (!subcommand.isEmpty()) {
        arguments << subcommand;
    }
    arguments << meshtoolArgs;

    QString outputPath = home.absolutePath() + "/" + expectedOutput;
    bool successful = ExecuteCommand(executableName, arguments, outputPath);

    if(successful){
        MITK_INFO << ("meshtool command: " + command + " successful").toStdString();
        outAbsolutePath = outputPath;
    } else{
        MITK_WARN << ("Error running meshtool command: " + command).toStdString();
    }

    return outAbsolutePath;
}

bool CemrgFourChamberCmd::ExecuteGlRuleFibers(VentricularFibresParams vfib) {
    QStringList arguments;

    arguments << "--meshname" << vfib.meshname();
    arguments << "--type" << vfib.type();
    arguments << "--apex_to_base" << vfib.apex_to_base() ;
    arguments << "--epi" << vfib.epi() ;
    arguments << "--lv" << vfib.lv() ;
    arguments << "--rv" << vfib.rv();
    arguments << "--alpha_endo" << vfib.a_endo();
    arguments << "--alpha_epi" << vfib.a_epi();
    arguments << "--beta_endo" << vfib.b_endo();
    arguments << "--beta_epi" << vfib.b_epi();
    arguments << "--output" << vfib.output();

    return ExecuteCarpless(GlRuleFibers(), arguments, vfib.output(), true);
    
}

bool CemrgFourChamberCmd::ExecuteGlRuleFibers(QString directory, QString m, QString type, QString a, QString e, QString l, QString r, double a_endo, double a_epi, double b_endo, double b_epi, QString output_pre) {
    VentricularFibresParams vfib;
    vfib.SetDirectory(directory); 
    vfib.SetMeshname(m);
    vfib.SetType(type);
    vfib.SetApexToBase(a);
    vfib.SetEpi(e);
    vfib.SetLV(l);
    vfib.SetRV(r);
    vfib.SetAlphaEndo(a_endo);
    vfib.SetAlphaEpi(a_epi);
    vfib.SetBetaEndo(b_endo);
    vfib.SetBetaEpi(b_epi);
    vfib.SetOutput(output_pre);

    return ExecuteGlRuleFibers(vfib);
}
