/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/
#include <mitkIOUtil.h>
#include <mitkSurface.h>
#include <mitkImageCast.h>
#include <mitkITKImageImport.h>
#include <mitkCommandLineParser.h>
#include <mitkImagePixelReadAccessor.h>


#include <QBuffer>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QTextStream>

#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

#include <cctype>
#include <cstdlib>
#include <iostream>
#include <limits>

int compareVersions(const QString &v1, const QString &v2)
{
  QList<int> t1;
  foreach (QString t, v1.split('.'))
  {
    t1.append(t.toInt());
  }

  QList<int> t2;
  foreach (QString t, v2.split('.'))
  {
    t2.append(t.toInt());
  }

  // pad with 0
  if (t1.size() < t2.size())
  {
    for (int i = 0; i < t2.size() - t1.size(); ++i)
      t1.append(0);
  }
  else
  {
    for (int i = 0; i < t1.size() - t2.size(); ++i)
      t2.append(0);
  }

  for (int i = 0; i < t1.size(); ++i)
  {
    if (t1.at(i) < t2.at(i))
      return -1;
    else if (t1.at(i) > t2.at(i))
      return 1;
  }
  return 0;
}

/*int checkUpdates(QTextStream &out)
{
  out << "Checking for updates... ";
  out.flush();
  QNetworkAccessManager manager;
  QNetworkReply *reply =
    manager.get(QNetworkRequest(QUrl("https://www.mitk.org/wiki/PluginGeneratorUpdate?action=raw")));

  QEventLoop eventLoop;
  reply->connect(reply, SIGNAL(finished()), &eventLoop, SLOT(quit()));
  eventLoop.exec();

  QByteArray data = reply->readAll();

  if (data.isEmpty())
  {
    qCritical() << "A network error occured:" << reply->errorString();
    return EXIT_FAILURE;
  }
  delete reply;

  QBuffer buffer(&data);
  buffer.open(QIODevice::ReadOnly);
  bool versionFound = false;
  while (buffer.canReadLine())
  {
    QString line = buffer.readLine();
    if (!versionFound)
    {
      line = line.trimmed();
      if (line.isEmpty())
        continue;

      if (compareVersions(PLUGIN_GENERATOR_VERSION, line) < 0)
      {
        versionFound = true;
        out << "New version " << line << " available.\n";
      }
      else
      {
        // no update available
        out << "No update available.\n";
        out.flush();
        return EXIT_SUCCESS;
      }
    }
    else
    {
      out << line;
    }
  }

  if (!versionFound)
  {
    qCritical() << "Update information not readable.";
    return EXIT_FAILURE;
  }

  out << '\n';
  out.flush();

  return EXIT_SUCCESS;
}*/

bool readAnswer(char defaultAnswer)
{
  std::string line;
  std::cin >> std::noskipws >> line;

  // consume the new line character
  std::cin.clear();
  std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

  char answer = defaultAnswer;
  if (!line.empty() && line[0] != '\n')
  {
    answer = std::tolower(line[0]);
  }

  if (answer == 'y')
    return true;
  if (answer == 'n')
    return false;
  if (defaultAnswer == 'y')
    return true;
  return false;
}

void createFilePathMapping(const QString &templateName,
                           const QString &baseInDir,
                           const QString &baseOutDir,
                           QHash<QString, QString> &fileNameMapping)
{
  QFileInfo info(templateName);
  if (info.isDir())
  {
    QStringList subEntries = QDir(templateName).entryList();
    foreach (QString subEntry, subEntries)
    {
      createFilePathMapping(templateName + "/" + subEntry, baseInDir, baseOutDir, fileNameMapping);
    }
    return;
  }

  fileNameMapping[templateName] = QString(templateName).replace(baseInDir, baseOutDir);
}

QHash<QString, QString> createTemplateFileMapping(const QString &qrcBase,
                                                  const QString &baseOutDir,
                                                  const QHash<QString, QString> &fileNameMapping)
{
  QHash<QString, QString> filePathMapping;
  createFilePathMapping(qrcBase, qrcBase, baseOutDir, filePathMapping);

  QMutableHashIterator<QString, QString> i(filePathMapping);
  while (i.hasNext())
  {
    i.next();
    QHashIterator<QString, QString> j(fileNameMapping);
    while (j.hasNext())
    {
      j.next();
      i.setValue(i.value().replace(j.key(), j.value()));
    }
  }

  return filePathMapping;
}

bool generateFiles(const QHash<QString, QString> &parameters, const QHash<QString, QString> &filePathMapping)
{
  QHashIterator<QString, QString> paths(filePathMapping);
  while (paths.hasNext())
  {
    paths.next();
    QFile templ(paths.key());
    templ.open(QIODevice::ReadOnly);
    QByteArray templContent = templ.readAll();

    QHashIterator<QString, QString> i(parameters);
    while (i.hasNext())
    {
      i.next();
      templContent.replace(i.key(), QByteArray(i.value().toLatin1()));
    }

    QFile outTempl(paths.value());
    QDir dir(QFileInfo(outTempl).dir());
    if (!dir.exists())
    {
      if (!dir.mkpath(dir.absolutePath()))
      {
        qCritical() << "Could not create directory" << dir.absolutePath();
        return EXIT_FAILURE;
      }
    }

    if (!outTempl.open(QIODevice::WriteOnly))
    {
      qCritical() << outTempl.errorString();
      return false;
    }
    outTempl.write(templContent);
  }

  return true;
}

int main(int argc, char** argv)
{

  // std::cout << argc << '\n';
  // for(int i=0;i<argc;i++) {
  //   std::cout << argv[i] << '\n';
  // }

  mitkCommandLineParser parser;
  parser.setCategory("Post processing");
  parser.setTitle("Plugin Generation");
  parser.setContributor("CEMRG, KCL");
  parser.setDescription("Generate a new plugin");

  // Use Unix-style argument names
  parser.setArgumentPrefix("--", "-");
//   parser.setStrictModeEnabled(true);

//   // Add command line argument names
  // parser.addArgument(
  //       "help", "h", mitkCommandLineParser::Bool,
  //       "  Show this help text");
  parser.addArgument(
        "out-dir", "o", mitkCommandLineParser::String,
        "  Output directory", QDir::tempPath().toStdString());
  parser.addArgument(
        "license", "l", mitkCommandLineParser::String,
        "  Path to a file containing license information", ":/COPYRIGHT_HEADER");
  parser.addArgument(
        "vendor", "v", mitkCommandLineParser::String,
        "  The vendor of the generated code", "German Cancer Research Center (DKFZ)");
  parser.addArgument(
        "quiet", "q", mitkCommandLineParser::Bool,
        "  Do not print additional information");
  parser.addArgument(
        "confirm-all", "y", mitkCommandLineParser::Bool,
        "  Answer all questions with 'yes'", "");

  parser.addArgument(
        "plugin-symbolic-name", "ps", mitkCommandLineParser::String,
        "* The plugin's symbolic name");

  parser.addArgument(
        "plugin-name", "pn", mitkCommandLineParser::String,
        "  The plug-in's human readable name");

  parser.addArgument(
        "view-class", "vc", mitkCommandLineParser::String,
        "  The View's' class name");
  parser.addArgument(
        "view-name", "vn", mitkCommandLineParser::String,
        "* The View's human readable name");

  parser.addArgument(
        "project-copyright", "", mitkCommandLineParser::String,
        "  Path to a file containing copyright information", ":/LICENSE");
  parser.addArgument(
        "project-name", "", mitkCommandLineParser::String,
        "  The project name");

  parser.addArgument(
        "project-app-name", "", mitkCommandLineParser::String,
        "  The application name");

  auto parsedArgs = parser.parseArguments(argc, argv);

  for (const auto &parsedArg : parsedArgs) {
    std::cout << "Argument: " << parsedArg.first << " " << us::any_cast<std::string>(parsedArg.second) << '\n';
  }

  

  if (parsedArgs.empty()){
      MITK_ERROR << "Arguments not parsed correctly";
      return EXIT_FAILURE;
  }


  // Show a help message
  if (parsedArgs.end() != parsedArgs.find("help"))
  {
//     MITK_INFO << "A CTK plug-in generator for MITK (version " << PLUGIN_GENERATOR_VERSION << ")\n\n"
    MITK_INFO  << parser.helpText() << "\n[* - options are required]\n";
    return EXIT_SUCCESS;
  }

  std::string output_dir_arg = QDir::tempPath().toStdString();
  std::string license_arg = QDir::tempPath().toStdString(); // to-do: change to actual license path or make obligatory
  std::string vendor_arg = "CEMRG, Imperial College";
  auto quiet = false;
  auto autoConfirm = false;
  std::string ps_arg = "plugin_symbolic_name";
  std::string pn_arg = "plugin_name";
  std::string vc_arg = "view_class";
  std::string vn_arg = "view_name";
  std::string project_copyright_arg = ":/LICENSE";
  std::string project_name_arg = "Project Name";
  std::string project_app_name_arg = "ProjectAppName";

  /*bool noNetwork = parsedArgs.contains("no-networking");

  if (parsedArgs.contains("check-update"))
  {
    if (noNetwork)
    {
      out << "Network support disabled. Cannot check for updates.\n";
      out.flush();
    }
    else
    {
      return checkUpdates(out);
    }
  }
  else
  {
    // always check for updates if no-networking is not given
    if (!noNetwork)
    {
      checkUpdates(out);
    }
  }*/

  // Check arguments

  // Project options
  QString projectName = QString::fromStdString(us::any_cast<std::string>(parsedArgs["project-name"]));
  MITK_INFO << projectName.toStdString();
  QString projectAppName = QString::fromStdString(us::any_cast<std::string>(parsedArgs["project-app-name"]));
  MITK_INFO << projectAppName.toStdString();
  QString copyrightPath = QString::fromStdString(us::any_cast<std::string>(parsedArgs["project-copyright"]));
  MITK_INFO << copyrightPath.toStdString();

  bool createProject = !projectName.isEmpty();
  if (createProject && projectAppName.isEmpty())
  {
    projectAppName = projectName;
  }
  MITK_INFO << "Finished parsing project names";

  QString pluginSymbolicName = QString::fromStdString(us::any_cast<std::string>(parsedArgs["plugin-symbolic-name"]));
  if (pluginSymbolicName.isEmpty())
  {
    qCritical() << "Required argument 'plugin-symbolic-name' missing.";
    qCritical("%s%s%s", "Type '", qPrintable(projectAppName), " -h' for help");
    return EXIT_FAILURE;
  }

  QString pluginTarget(pluginSymbolicName);
  pluginTarget.replace('.', '_');

  QString activatorClass = pluginTarget + "_Activator";

  MITK_INFO << "Finished parsing plugin params";

  QString outDir = QString::fromStdString(us::any_cast<std::string>(parsedArgs["out-dir"]));
  QString licensePath = QDir::fromNativeSeparators(QString::fromStdString(us::any_cast<std::string>(parsedArgs["license"])));
  QString pluginExportDirective = pluginSymbolicName.split('.').last().toUpper() + "_EXPORT";

  QString pluginName = QString::fromStdString(us::any_cast<std::string>(parsedArgs["plugin-name"]));
  MITK_INFO << pluginName.toStdString();

  if (pluginName.isEmpty())
  {
    QStringList toks = pluginSymbolicName.split('.');
    pluginName = toks.last();
    pluginName[0] = pluginName[0].toUpper();
  }

  MITK_INFO << us::any_cast<std::string>(parsedArgs["vendor"]);
  QString vendor = QString::fromStdString(us::any_cast<std::string>(parsedArgs["vendor"]));
  MITK_INFO << vendor.toStdString();

  QString viewName = QString::fromStdString(us::any_cast<std::string>(parsedArgs["view-name"]));
  if (viewName.isEmpty())
  {
    qCritical() << "Required argument 'view-name' missing.";
    qCritical("%s%s%s", "Type '", qPrintable(projectAppName), " -h' for help");
    return EXIT_FAILURE;
  }

  QStringList toks = viewName.split(QRegExp("\\s"), QString::SkipEmptyParts);
  QString viewClass = QString::fromStdString(us::any_cast<std::string>(parsedArgs["view-class"]));
  if (viewClass.isEmpty())
  {
    foreach (QString tok, toks)
    {
      QString tmp = tok;
      tmp[0] = tmp[0].toUpper();
      viewClass += tmp;
    }
  }

  QString viewId;
  if (viewId.isEmpty())
  {
    viewId = "org.mitk.views.";
    foreach (QString tok, toks)
    {
      viewId += tok.toLower();
    }
  }

  quiet = parsedArgs.end() != parsedArgs.find("quiet");
  autoConfirm = parsedArgs.end() != parsedArgs.find("confirm-all");

  if (!outDir.endsWith('/'))
    outDir += '/';
  if (createProject)
    outDir += projectName;
  else
    outDir += pluginSymbolicName;

  // Print the collected information
  if (!quiet)
  {
    if (createProject)
    {
        MITK_INFO << "Using the following information to create a project:\n\n";
        MITK_INFO  << "  Project Name:        " << projectName.toStdString() << '\n';
        MITK_INFO  << "  Application Name:    " << projectAppName.toStdString() << '\n';
        MITK_INFO  << "  Copyright File:      " << QDir::toNativeSeparators(copyrightPath).toStdString() << '\n';
    }
    else
    {
      MITK_INFO << "Using the following information to create a plug-in:\n\n";
    }

    MITK_INFO << "  License File:        " << QDir::toNativeSeparators(licensePath).toStdString() << '\n';
    MITK_INFO << "  Plugin-SymbolicName: " << pluginSymbolicName.toStdString() << '\n';
    MITK_INFO << "  Plugin-Name:         " << pluginName.toStdString() << '\n';
    MITK_INFO << "  Plugin-Vendor:       " << vendor.toStdString() << '\n';
    MITK_INFO << "  View Name:           " << viewName.toStdString() << '\n';
    MITK_INFO << "  View Id:             " << viewId.toStdString() << '\n';
    MITK_INFO << "  View Class:          " << viewClass.toStdString() << '\n';
    MITK_INFO << '\n';
    MITK_INFO << "Create in: " << outDir.toStdString() << '\n';
    MITK_INFO << '\n';

    if (!autoConfirm)
      MITK_INFO << "Continue [Y/n]? ";

    // out.flush();

    if (!autoConfirm && !readAnswer('y'))
    {
      MITK_INFO << "Aborting.\n";
      return EXIT_SUCCESS;
    }
  }

  // Check the output directory
  if (!QDir(outDir).exists())
  {
    if (!autoConfirm)
    {
      MITK_INFO << "Directory '" << outDir.toStdString() << "' does not exist. Create it [Y/n]? ";
    //   out.flush();
    }

    if (autoConfirm || readAnswer('y'))
    {
      if (!QDir().mkpath(outDir))
      {
        qCritical() << "Could not create directory:" << outDir;
        return EXIT_FAILURE;
      }
    }
    else
    {
      MITK_INFO << "Aborting.\n";
      return EXIT_SUCCESS;
    }
  }

  if (!QDir(outDir).entryList(QDir::AllEntries | QDir::NoDotAndDotDot).isEmpty())
  {
    if (!autoConfirm)
    {
      MITK_INFO << "Directory '" << outDir.toStdString() << "' is not empty. Continue [y/N]? ";
    //   out.flush();
    }

    if (!autoConfirm && !readAnswer('n'))
    {
      MITK_INFO << "Aborting.\n";
      return EXIT_SUCCESS;
    }
  }

  // Extract the license text
  QFile licenseFile(licensePath);
  if (!licenseFile.open(QIODevice::ReadOnly))
  {
    qCritical() << "Cannot open file" << licenseFile.fileName();
    return EXIT_FAILURE;
  }
  QString licenseText = licenseFile.readAll();
  licenseFile.close();

  QHash<QString, QString> parameters;
  if (createProject)
  {
    // Extract the copyright
    QFile copyrightFile(copyrightPath);
    if (!copyrightFile.open(QIODevice::ReadOnly))
    {
      qCritical() << "Cannot open file" << copyrightFile.fileName();
      return EXIT_FAILURE;
    }
    QString copyrighText = copyrightFile.readAll();
    copyrightFile.close();

    parameters["$(copyright)"] = copyrighText;
    parameters["$(project-name)"] = projectName;
    parameters["$(project-app-name)"] = projectAppName;
    parameters["$(project-plugins)"] = QString("Plugins/") + pluginSymbolicName + ":ON";

    QStringList toks = pluginTarget.split("_");
    QString projectPluginBase = toks[0] + "_" + toks[1];
    parameters["$(project-plugin-base)"] = projectPluginBase;
  }
  parameters["$(license)"] = licenseText;
  parameters["$(plugin-name)"] = pluginName;
  parameters["$(plugin-symbolic-name)"] = pluginSymbolicName;
  parameters["$(vendor)"] = vendor;
  parameters["$(plugin-target)"] = pluginTarget;
  parameters["$(plugin-export-directive)"] = pluginExportDirective;
  parameters["$(view-id)"] = viewId;
  parameters["$(view-name)"] = viewName;
  parameters["$(view-file-name)"] = viewClass;
  parameters["$(view-class-name)"] = viewClass;
  parameters["$(activator-file-name)"] = activatorClass;
  parameters["$(activator-class-name)"] = activatorClass;

  if (createProject)
  {
    QHash<QString, QString> projectFileNameMapping;
    projectFileNameMapping["TemplateApp"] = projectAppName;

    QHash<QString, QString> filePathMapping =
      createTemplateFileMapping("./ProjectTemplate", outDir, projectFileNameMapping);
    generateFiles(parameters, filePathMapping);
  }

  // QHash<QString, QString> pluginFileNameMapping;
  // pluginFileNameMapping["QmitkTemplateView"] = viewClass;
  // pluginFileNameMapping["mitkPluginActivator"] = activatorClass;

  // if (createProject)
  // {
  //   if (!outDir.endsWith('/'))
  //     outDir += '/';
  //   outDir += "Plugins/" + pluginSymbolicName;
  // }
  // QHash<QString, QString> filePathMapping =
  //   createTemplateFileMapping("./PluginTemplate", outDir, pluginFileNameMapping);
  // generateFiles(parameters, filePathMapping);

  return EXIT_SUCCESS;
}
