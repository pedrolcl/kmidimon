foreach(S IN ITEMS 48 64 128 256 512)
    install (FILES kmidimon_${S}.png
             DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/${S}x${S}/apps
             RENAME kmidimon.png)
endforeach()

install ( FILES kmidimon.svgz
          DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/apps )
