find_package(Qt6 REQUIRED COMPONENTS Quick)
find_package(Qt6 QUIET COMPONENTS Quick3D) # 3D line preview, optional at runtime

qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(raildeck-qt
    src/main.cpp
)

set_source_files_properties(qml/Theme.qml PROPERTIES
    QT_QML_SINGLETON_TYPE TRUE
)

qt_add_qml_module(raildeck-qt
    URI RailDeck
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/Theme.qml
        qml/StatusBar.qml
        qml/Speedometer.qml
        qml/ArcGauge.qml
        qml/EffortBar.qml
        qml/InfoTile.qml
        qml/CabButton.qml
        qml/AdvisorPanel.qml
        qml/LeverControl.qml
        qml/PlanningStrip.qml
        qml/TrackView3D.qml
    SOURCES
        src/trainbackend.h
        src/trainbackend.cpp
)

# qmltyperegistrations includes <trainbackend.h> by file name
target_include_directories(raildeck-qt PRIVATE src)

target_link_libraries(raildeck-qt PRIVATE
    Qt6::Quick
    traincore::traincore
)
