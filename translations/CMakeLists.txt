set( TRANSLATIONS
     kmidimon_cs.ts
     kmidimon_es.ts
     kmidimon_fr.ts
     kmidimon_ja.ts )

if (QT_VERSION VERSION_LESS 5.15.0)
    qt5_add_translation(QM_FILES ${TRANSLATIONS})
else()
    qt_add_translation(QM_FILES ${TRANSLATIONS})
endif()

add_custom_target(translations ALL DEPENDS ${QM_FILES})

install( FILES ${QM_FILES}
         DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/kmidimon/translations/ )
