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
CEMRG CMD APP TEMPLATE
This app serves as a template for the command line apps to be implemented
in the framework.
=========================================================================*/

// Qmitk
#include <mitkIOUtil.h>
#include <mitkSurface.h>
#include <mitkImageCast.h>
#include <mitkITKImageImport.h>
#include <mitkCommandLineParser.h>
#include <mitkImagePixelReadAccessor.h>

// VTK
#include <vtkClipPolyData.h>
#include <vtkImplicitBoolean.h>
#include <vtkPlane.h>
#include <vtkSphere.h>
#include <vtkDataSetSurfaceFilter.h>
#include <vtkPolyDataConnectivityFilter.h>
#include <vtkCleanPolyData.h>
#include <vtkMath.h>
#include <vtkFloatArray.h>
#include <vtkPolyData.h>
#include <vtkCellData.h>
#include <vtkPolyDataNormals.h>
#include <vtkIdList.h>

// ITK
#include <itkPoint.h>
#include <itkResampleImageFilter.h>
#include <itkImageRegionIterator.h>
#include <itkGrayscaleErodeImageFilter.h>
#include <itkImageRegionIteratorWithIndex.h>
#include <itkBinaryBallStructuringElement.h>
#include <itkNearestNeighborInterpolateImageFunction.h>
#include <itkImageFileWriter.h>

// Qt
#include <QtDebug>
#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>


// C++ Standard
#include <algorithm>
#include <string>
#include <numeric>

// CemrgApp
#include <CemrgScar3D.h>
#include <CemrgCommonUtils.h>

int main(int argc, char *argv[]) {
    mitkCommandLineParser parser;

    // Set general information about your command-line app
    parser.setCategory("Utilities");
    parser.setTitle("JSON file reader/writer Command-line App");
    parser.setContributor("CEMRG, KCL");
    parser.setDescription("JSON file reader/writer Command-line App");

    // How should arguments be prefixed
    parser.setArgumentPrefix("--", "-");

    // Add arguments. Unless specified otherwise, each argument is optional.
    // See mitkCommandLineParser::addArgument() for more information.
    // parser.addArgument(
    //   "input-path", "p", mitkCommandLineParser::String,
    //   "Input Directory Path", "Path of directory containing LGE files.",
    //   us::Any(), false);
    parser.addArgument(
        "input", "i", mitkCommandLineParser::String,
        "JSON file", "Full path of JSON file.",
        us::Any(), false);
    parser.addArgument( // optional
        "verbose", "v", mitkCommandLineParser::Bool,
        "Verbose Output", "Whether to produce verbose output");

    // Parse arguments.
    // This method returns a mapping of long argument names to their values.
    auto parsedArgs = parser.parseArguments(argc, argv);

    if (parsedArgs.empty()) {
        return EXIT_FAILURE;
    }

    if (parsedArgs["input"].Empty()) {
        MITK_INFO << parser.helpText();
        return EXIT_FAILURE;
    }

    // Parse, cast and set required arguments
    // auto inFilename = us::any_cast<std::string>(parsedArgs["input-path"]);
    MITK_INFO << "Parsing mandatory arguments";
    auto inFilename2 = us::any_cast<std::string>(parsedArgs["input"]);

    // Default values for optional arguments
    auto verbose = false;

    // Parse, cast and set optional argument
    if (parsedArgs.end() != parsedArgs.find("verbose")) {
        verbose = us::any_cast<bool>(parsedArgs["verbose"]);
    }

    MITK_INFO << "Program starting...";
    try {
        // Code the functionality of the cmd app here.
        MITK_INFO(verbose) << "Verbose mode ON.";
        QFileInfo fi(QString::fromStdString(inFilename2));
        QString directory = fi.absolutePath();
        QString fname = fi.completeBaseName();
        QString newName = "new_file.json";

        MITK_INFO(verbose) << "Reading input file";
        QJsonObject in_json = CemrgCommonUtils::ReadJSONFile(directory, fname);

        QStringList ky = in_json.keys();
        MITK_INFO << "Showing keys and values";
        for (int ix = 0; ix < ky.size(); ix++) {
            QString this_key = ky.at(ix);
            std::string msg = this_key.toStdString();
            msg += (in_json[this_key].isArray()) ? ": Array" : ": other";
            MITK_INFO << msg;
        }

        MITK_INFO(verbose) << "Creating a JSON object - saving";
        QStringList new_keys = (QStringList() << "a" << "b" << "c");
        QStringList new_values = (QStringList() << "0.0,1.3,4.8,2" << "2" << "23,29.3,1.1");
        QStringList types = (QStringList() << "array" << "int" << "array");
        
        QJsonObject new_json = CemrgCommonUtils::CreateJSONObject(new_keys, new_values, types);
        CemrgCommonUtils::WriteJSONFile(new_json, directory, "original");
        CemrgCommonUtils::WriteJSONFile(new_json, directory, "modified");

        MITK_INFO(verbose) << "Modifying input JSON object";
        CemrgCommonUtils::ModifyJSONFile(directory, "modified", "a", "2.9,3", "array");

        MITK_INFO(verbose) << "Goodbye!";
    }
    catch (const std::exception &e) {
        MITK_ERROR << e.what();
        return EXIT_FAILURE;
    }
    catch (...) {
        MITK_ERROR << "Unexpected error";
        return EXIT_FAILURE;
    }
}
