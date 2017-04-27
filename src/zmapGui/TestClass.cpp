#include "TestClass.h"



#include <QApplication>
#include "MainWindow01.h"
#include "MainWindow02.h"
#include "MainWindow03.h"
#include "ZGCache.h"
#include "ZGSequence.h"

#include "TestCounter.h"
#include "ZDebug.h"

// some experimental stuff
#include "ZVNode.h"
#include "Number.h"
#include "TestContainer.h"
#include "SetTest.h"
#include "ZGFeaturesetSet.h"
#include "TestDummy.h"

// gui pac model
#include "ZGuiControl.h"
#include "ZGuiAbstraction.h"
#include "ZGuiPresentation.h"
#include "ZDummyControl.h"

#include "ZGMessageHeader.h"
#include "ZGPacMessageHeader.h"

// scene collection
#include "ZGuiScene.h"
#include "ZGuiSceneCollection.h"
#include "ZGuiMain.h"
#include "ZGuiMainCollection.h"

#include <memory>
#include <iostream>
#include <stdexcept>
#include <set>
#include <new>

template<typename T>
T randType(const T & range)
{
    return static_cast<T>((static_cast<double>(rand())/static_cast<double>(RAND_MAX)) * range) ;
}

TestClass::TestClass()
{

}


#include "ZInternalID.h"
Q_DECLARE_METATYPE(ZMap::GUI::ZInternalID)
#include "ZGTrackData.h"
Q_DECLARE_METATYPE(ZMap::GUI::ZGTrackData)
#include "ZGStyleData.h"
Q_DECLARE_METATYPE(ZMap::GUI::ZGStyleData)
#include "ZGFeatureData.h"
Q_DECLARE_METATYPE(ZMap::GUI::ZGFeatureData)
#include "ZGSourceType.h"
Q_DECLARE_METATYPE(ZMap::GUI::ZGSourceType)
#include "ZGStrandType.h"
Q_DECLARE_METATYPE(ZMap::GUI::ZGStrandType)
#include "ZGFrameType.h"
Q_DECLARE_METATYPE(ZMap::GUI::ZGFrameType)
#include "ZGZMapData.h"
Q_DECLARE_METATYPE(ZMap::GUI::ZGZMapData)
#include "ZGPreferencesType.h"
Q_DECLARE_METATYPE(ZMap::GUI::ZGPreferencesType)
#include "ZGTFTActionType.h"
Q_DECLARE_METATYPE(ZMap::GUI::ZGTFTActionType)

void TestClass::testFunction10()
{
    using namespace ZMap::GUI ;
    std::stringstream str;
    str << "TestClass::testFunction10(): " ;
    ZDebug::String2cout(str.str()) ;
    str.str(std::string()) ;

    //
    // this has to exist before you can create
    // any QT stuff
    //
    int argc = 0 ;
    QApplication q_application(argc, nullptr) ;

    // register use as QVariant metatype
    qRegisterMetaType<ZInternalID>("ZInternalID") ;
    qRegisterMetaType<ZGTrackData>("ZGTrackData") ;
    qRegisterMetaType<ZGStyleData>("ZGStyleData") ;
    qRegisterMetaType<ZGFeatureData>("ZGFeatureData") ;
    qRegisterMetaType<ZGSourceType>("ZGSourceType") ;
    qRegisterMetaType<ZGStrandType>("ZGStrandType") ;
    qRegisterMetaType<ZGFrameType>("ZGFrameType") ;
    qRegisterMetaType<ZGZMapData>("ZGZMapData") ;
    qRegisterMetaType<ZGPreferencesType>("ZGPreferencesType") ;

    ZDebug::instanceTest_All() ;

    //
    // create controllers and hook them up together
    //
    std::shared_ptr<ZGPacControl> gui_controller(std::make_shared<ZGuiControl>()),
            dummy_controller(std::make_shared<ZDummyControl>()) ;

    gui_controller->setPeer(dummy_controller) ;
    dummy_controller->setPeer(gui_controller) ;
    std::shared_ptr<QWidget> dummy_controller_widget = std::dynamic_pointer_cast<QWidget, ZGPacControl>(dummy_controller) ;
    if (dummy_controller_widget)

    std::cout << "gui_controller.use_count() = " << gui_controller.use_count() << std::endl ;
    std::cout << "dummy_controller.use_count() = " << dummy_controller.use_count() << std::endl ;

    //
    // start event handling
    //
    int q_result = q_application.exec() ;

    ZDebug::instanceTest_All() ;

    // unhook communications and finish
    gui_controller->detatchPeer() ;
    dummy_controller->detatchPeer() ;

    gui_controller.reset() ;
    dummy_controller.reset() ;

    ZDebug::instanceTest_All() ;

    str << "TestClass::testFunction10(); q_result = " << q_result ;
    ZDebug::String2cout(str.str()) ;
}


