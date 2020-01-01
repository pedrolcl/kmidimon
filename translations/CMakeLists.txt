set( TRANSLATIONS
     kmidimon_cs.ts
     kmidimon_es.ts
     kmidimon_fr.ts
     kmidimon_ja.ts )

qt5_add_translation(QM_FILES ${TRANSLATIONS})

add_custom_target(translations ALL DEPENDS ${QM_FILES})

install( FILES ${QM_FILES}
         DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/kmidimon/translations/ )
