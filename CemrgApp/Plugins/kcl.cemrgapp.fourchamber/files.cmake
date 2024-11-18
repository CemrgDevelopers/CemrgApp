set(SRC_CPP_FILES

)

set(INTERNAL_CPP_FILES
  kcl_cemrgapp_fourchamber_Activator.cpp
  FourChamberView.cpp
  FourChamberLandmarksView.cpp
  FourChamberGuidePointsView.cpp
  CemrgDataInteractor.cpp
)

set(UI_FILES
  src/internal/FourChamberViewControls.ui
  src/internal/FourChamberLandmarksViewControls.ui
  src/internal/FourChamberViewPointLabelSelect.ui
  src/internal/FourChamberLandmarksViewPointLabel.ui
  src/internal/FourChamberViewIdentifyLabels.ui
  src/internal/Meshtools3DParameterUI.ui
  src/internal/LabelsSelectorUI.ui
  src/internal/VentricularFibresParameter.ui
  src/internal/FourChamberGuidePointsViewControls.ui
  src/internal/FourChamberGuidePointsViewSelector.ui
)

set(MOC_H_FILES
  src/internal/kcl_cemrgapp_fourchamber_Activator.h
  src/internal/FourChamberView.h
  src/internal/FourChamberLandmarksView.h
  src/internal/FourChamberGuidePointsView.h
  src/internal/CemrgDataInteractor.h
)

# list of resource files which can be used by the plug-in
# system without loading the plug-ins shared library,
# for example the icon used in the menu and tabs for the
# plug-in views in the workbench
set(CACHED_RESOURCE_FILES
  resources/icon.xpm
  plugin.xml
)

# list of Qt .qrc files which contain additional resources
# specific to this plugin
set(QRC_FILES

)

set(CPP_FILES )

foreach(file ${SRC_CPP_FILES})
  set(CPP_FILES ${CPP_FILES} src/${file})
endforeach(file ${SRC_CPP_FILES})

foreach(file ${INTERNAL_CPP_FILES})
  set(CPP_FILES ${CPP_FILES} src/internal/${file})
endforeach(file ${INTERNAL_CPP_FILES})
