set( TRANSLATIONS_FILES
     cs.ts
     es.ts
     fr.ts
     ja.ts )

qt5_add_translation(QM_FILES ${TRANSLATIONS_FILES})
add_custom_target(translations ALL DEPENDS ${QM_FILES})
install( FILES ${QM_FILES}
         DESTINATION ${CMAKE_INSTALL_DATADIR}/kmidimon/locale )
