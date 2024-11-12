project(kcl_cemrgapp_fourchamber)

mitk_create_plugin(
  EXPORT_DIRECTIVE FOURCHAMBER_EXPORT
  EXPORTED_INCLUDE_SUFFIXES src
  MODULE_DEPENDS MitkSegmentation MitkQtWidgetsExt MitkCore MitkCemrgAppModule
  PACKAGE_DEPENDS ITK
)
