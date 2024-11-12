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
#include <QFileInfo>
#include <QProcess>
#include <QMessageBox>

// C++ Standard
#include <algorithm>
#include <string>
#include <numeric>

// CemrgApp
#include <CemrgScar3D.h>
#include <CemrgCommandLine.h>
#include <CemrgCommonUtils.h>
#include <CemrgMultilabelSegmentationUtils.h>

int main(int argc, char* argv[]) {
    mitkCommandLineParser parser;

    // Set general information about your command-line app
    parser.setCategory("Pre processing");
    parser.setTitle("Resample/Reorient Command-line App");
    parser.setContributor("CEMRG, KCL");
    parser.setDescription("Resample and reorient nifti files.");

    // How should arguments be prefixed
    parser.setArgumentPrefix("--", "-");

    // Add arguments. Unless specified otherwise, each argument is optional.
    parser.addArgument(
        "input", "i", mitkCommandLineParser::String,
        "Image file path", "Full path of file (.nii or .nrrd).",
        us::Any(), false);
    parser.addArgument(
        "output", "o", mitkCommandLineParser::String,
        "Output filename", "Where to save the output. If no path is given, the output will be saved in the input directory.",
        us::Any(), false);
    parser.addArgument( // optional
        "spacing", "spacing", mitkCommandLineParser::String,
        "Spacing string (comma-separated)", "String of the form 'x,y,z' (default=1,1,1)");
    parser.addArgument( // optional
        "sigma-fraction", "sigma", mitkCommandLineParser::String,
        "Scalar determining the width of the interpolation function", "Calculated as spacing[x]*sigma-fraction (default=0.5)");
    parser.addArgument( // optional
        "alpha-fraction", "alpha", mitkCommandLineParser::String,
        "Scalar determining the cutoff distance over which the function is calculated", "Common values: 0=NN, 1, 2, or 3 (default=3)");
    parser.addArgument( // optional
        "verbose", "v", mitkCommandLineParser::Bool,
        "Verbose Output", "Whether to produce verbose output (default=false)");

    // Parse arguments.
    // This method returns a mapping of long argument names to their values.
    auto parsedArgs = parser.parseArguments(argc, argv);

    if (parsedArgs.empty()) {
        return EXIT_FAILURE;
    }

    if (parsedArgs["input"].Empty() || parsedArgs["output"].Empty()) {
        MITK_INFO << parser.helpText();
        return EXIT_FAILURE;
    }

    // Parse, cast and set required arguments
    auto inFilename = us::any_cast<std::string>(parsedArgs["input"]);
    auto outFilename = us::any_cast<std::string>(parsedArgs["output"]);

    // Default values for optional arguments
    auto verbose = false;
    std::string spacingStr = "1,1,1";
    std::string sigmaStr = "0.5";
    std::string alphaStr = "3.0";

    // Parse, cast and set optional arguments
    if (parsedArgs.end() != parsedArgs.find("verbose")) {
        verbose = us::any_cast<bool>(parsedArgs["verbose"]);
    }
    if (parsedArgs.end() != parsedArgs.find("sigma-fraction")) {
        sigmaStr = us::any_cast<std::string>(parsedArgs["sigma-fraction"]);
    }
    if (parsedArgs.end() != parsedArgs.find("alpha-fraction")) {
        alphaStr = us::any_cast<std::string>(parsedArgs["alpha-fraction"]);
    }
    if (parsedArgs.end() != parsedArgs.find("spacing")) {
        spacingStr = us::any_cast<std::string>(parsedArgs["spacing"]);
    }

    try {
        // Code the functionality of the cmd app here.
        MITK_INFO(verbose) << "Verbose mode ON.";
        MITK_INFO << "Input file: " << inFilename;
        MITK_INFO(verbose) << "Output file: " << outFilename;
        MITK_INFO << "Spacing: " << spacingStr;

        bool ok1, ok2;
        double sigmaFraction = QString::fromStdString(sigmaStr).toDouble(&ok1);
        if (!ok1) {
            MITK_ERROR << "Wrong input sigma-fraction: " << sigmaStr;
            return EXIT_FAILURE;
        }
        double alphaFraction = QString::fromStdString(alphaStr).toDouble(&ok2);
        if (!ok2) {
            MITK_ERROR << "Wrong input alpha-fraction: " << alphaStr;
            return EXIT_FAILURE;
        }

        MITK_INFO << "Sigma fraction: " << sigmaFraction;
        MITK_INFO << "Alpha fraction: " << alphaFraction;

        // parse input and output filenames
        QString filepath = QString::fromStdString(inFilename);
        QFileInfo fi(filepath);
        QString outpath = QString::fromStdString(outFilename);
        
        if (outpath.contains(mitk::IOUtil::GetDirectorySeparator())) {
            outpath = outpath;
        } else {
            outpath = fi.absolutePath() + mitk::IOUtil::GetDirectorySeparator() + outpath;
        }

        if (outpath.endsWith(".nii") || outpath.endsWith(".nrrd")) {
            outpath = outpath;
        } else {
            outpath = outpath + ".nii";
        }
        
        MITK_INFO(verbose) << "Parsing spacing string";
        QStringList spacingList = QString::fromStdString(spacingStr).split(",");
        std::vector<double> spacing;
        for (int ix = 0; ix < spacingList.size(); ix++) {
            bool ok; 
            double val = spacingList[ix].toDouble(&ok);
            if (!ok) {
                MITK_ERROR << "Invalid spacing string";
                return EXIT_FAILURE;
            }
            spacing.push_back(val);
        }

        MITK_INFO(verbose) << "Loading image";
        mitk::Image::Pointer seg = mitk::IOUtil::Load<mitk::Image>(inFilename);
        if (!seg) {
            MITK_ERROR << "Failed to load image";
            return EXIT_FAILURE;
        }

        MITK_INFO << "Resampling and smoothing";
        std::unique_ptr<CemrgMultilabelSegmentationUtils> multilabelUtils = std::unique_ptr<CemrgMultilabelSegmentationUtils>(new CemrgMultilabelSegmentationUtils());
        mitk::Image::Pointer seg_smooth = multilabelUtils->ResampleSmoothLabel(seg, spacing, sigmaFraction, alphaFraction);

        MITK_INFO(verbose) << "Saving image to " << outpath.toStdString();
        mitk::IOUtil::Save(seg_smooth, outpath.toStdString());

        MITK_INFO << "Goodbye!";
        return EXIT_SUCCESS;

    } catch (const std::exception &e) {
        MITK_ERROR << e.what();
        return EXIT_FAILURE;
    } catch (...) {
        MITK_ERROR << "Unexpected error";
        return EXIT_FAILURE;
    }
}
