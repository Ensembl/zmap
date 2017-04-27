#include "ZGuiDialoguePreferences.h"
#include "ZGuiDialoguePreferencesStyle.h"
#include "ZGuiWidgetComboPreferences.h"
#include "ZGuiWidgetCreator.h"
#include "ZGuiTextDisplayDialogue.h"
#include "ZGuiPortValidator.h"
#include "ZGuiCoordinateValidator.h"
#include <stdexcept>
#include <sstream>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QMenu>
#include <QMenuBar>
#include <QAction>
#include <QLabel>
#include <QFrame>
#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>
#ifndef QT_NO_DEBUG
#include <QDebug>
#endif


namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGuiDialoguePreferences>::m_name ("ZGuiDialoguePreferences" ) ;
const QString ZGuiDialoguePreferences::m_display_name("ZMap Preferences"),

ZGuiDialoguePreferences::m_help_text_overview("The ZMap Configuration Window allows you to configure the appearance\n"
                                              "and operation of certains parts of ZMap.\n"
                                              "\n"
                                              "The Configuration Window has the following sections:\n"
                                              "\n"
                                              "The menubar with general operations such as showing this help.\n"
                                              "The section chooser where you can click on the section that you want to configure\n"
                                              "The section resources notebook which displays the elements you can configure for\n"
                                              "a particular section. As you select different sections the resources notebook changes\n"
                                              "to allow you to configure that section.\n"
                                              "\n"
                                              "After you have made your changes you can click:\n"
                                              "\n"
                                              "\"Cancel\" to discard them and quit the resources dialog\n"
                                              "\"Ok\" to apply them and quit the resources dialog\n"
                                              "\n"
                                              "To save settings for future sessions:\n"
                                              "\n"
                                              "Click \"File->Save\" to save settings for the current section only\n"
                                              "Click \"File->Save All\" to save settigns for all sections\n"
                                              "\n"
                                              "**IMPORTANT NOTE:** You probably SHOULDN'T SAVE CHANGES TO THE ADVANCED settings unless you are\n"
                                              "sure what you are doing. Saved settings will override settings from any peer program (e.g. Otter)\n"
                                              "for ALL future sessions and the Advanced settings may be key for your setup to work correctly.\n"
                                              "\n"
                                              "To reset a value:\n"
                                              "Clear the contents of a text box to unset the value. If you had previously saved changes, then\n"
                                              "save again to override the previously-saved value. The default values will then be used in your\n"
                                              "next session."),

ZGuiDialoguePreferences::m_help_text_title_overview("ZMap Preferences: Help") ;


ZGuiDialoguePreferences::ZGuiDialoguePreferences(QWidget *parent)
    : QMainWindow(parent),
      Util::InstanceCounter<ZGuiDialoguePreferences>(),
      Util::ClassName<ZGuiDialoguePreferences>(),
      m_top_layout(Q_NULLPTR),
      m_menu_file(Q_NULLPTR),
      m_menu_help(Q_NULLPTR),
      m_action_file_save(Q_NULLPTR),
      m_action_file_save_all(Q_NULLPTR),
      m_action_file_close(Q_NULLPTR),
      m_action_help_overview(Q_NULLPTR),
      m_combo_preferences(Q_NULLPTR),
      m_controls_display(Q_NULLPTR),
      m_controls_blixem(Q_NULLPTR),
      m_check_shrinkable(Q_NULLPTR),
      m_check_highlight(Q_NULLPTR),
      m_check_enable(Q_NULLPTR),
      m_check_scope(Q_NULLPTR),
      m_check_features(Q_NULLPTR),
      m_check_temporary(Q_NULLPTR),
      m_check_sleep(Q_NULLPTR),
      m_check_kill(Q_NULLPTR),
      m_entry_scope(Q_NULLPTR),
      m_entry_max(Q_NULLPTR),
      m_entry_host(Q_NULLPTR),
      m_entry_port(Q_NULLPTR),
      m_entry_config(Q_NULLPTR),
      m_entry_launch(Q_NULLPTR),
      m_button_cancel(Q_NULLPTR),
      m_button_OK(Q_NULLPTR),
      m_text_display_dialogue(Q_NULLPTR),
      m_port_validator(Q_NULLPTR),
      m_scope_validator(Q_NULLPTR),
      m_maxhomols_validator(Q_NULLPTR)
{
    if (!Util::canCreateQtItem())
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::ZGuiDialoguePreferences() ; can not instantiate without instances of QApplication existing ")) ;

    // create actions
    try
    {
        createFileMenuActions();
        createHelpMenuActions();
    }
    catch (std::exception& error)
    {
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::ZGuiDialoguePreferences() ; caught exception in attempt to create actions: ") + error.what() ) ;
    }
    catch (...)
    {
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::ZGuiDialoguePreferences() ; caught unknown exception in attempt to create actions ")) ;
    }

    // menu bar
    QMenuBar *menu_bar = Q_NULLPTR ;
    try
    {
        menu_bar = menuBar() ;
    }
    catch (std::exception & error)
    {
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::ZGuiDialoguePreferences() ; caught exception on attempt to create menu bar: ") + error.what()) ;
    }
    catch (...)
    {
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::ZGuiDialoguePreferences() ; caught unknown exception on attempt to create menu bar")) ;
    }
    if (!menu_bar)
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::ZGuiDialoguePreferences() ; unable to create menu bar ")) ;

    // menu bar style
    if (!(m_style = ZGuiDialoguePreferencesStyle::newInstance()))
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::ZGuiDialoguePreferences() ; unable to create style for this dialogue")) ;
    m_style->setParent(this) ;
    menu_bar->setStyle(m_style) ;

    // status bar
    QStatusBar *status_bar = Q_NULLPTR ;
    try
    {
        status_bar = statusBar() ;
    }
    catch (std::exception & error)
    {
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::ZGuiDialoguePreferences() ; caught exception on attempt to create status bar: ") + error.what()) ;
    }
    catch (...)
    {
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::ZGuiDialoguePreferences() ; caught unknown exception on attempt to create status bar ")) ;
    }
    if (!status_bar)
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::ZGuiDialoguePreferences() ; unable to create status bar ")) ;

    // central widget
    QWidget *widget = Q_NULLPTR ;
    if (!(widget = ZGuiWidgetCreator::createWidget()))
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::ZGuiDialoguePreferences() ; unable to create central widget ")) ;
    try
    {
        setCentralWidget(widget) ;
    }
    catch (std::exception& error)
    {
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::ZGuiDialoguePreferences() ; caught exception on attempt to set central widget: ") + error.what() ) ;
    }
    catch (...)
    {
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::ZGuiDialoguePreferences() ; caught unknown exception on attempt to set central widget")) ;
    }

    // top layout
    if (!(m_top_layout = ZGuiWidgetCreator::createVBoxLayout()))
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::ZGuiDialoguePreferences() ; unable to create top layout ")) ;
    widget->setLayout(m_top_layout) ;

    // create all menus
    try
    {
        createAllMenus() ;
    }
    catch (std::exception& error)
    {
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::ZGuiDialoguePreferences() ; caught exception in attempt to create all menus: ") + error.what() ) ;
    }
    catch (...)
    {
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::ZGuiDialoguePreferences() ; caught unknown exception in attempt to create all menus")) ;
    }

    // create various of the controls
    widget = Q_NULLPTR ;
    try
    {
        widget = createControls01() ;
    }
    catch (std::exception & error)
    {
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::ZGuiDialoguePreferences() ; caught exception in call to createControls01() ") + error.what() ) ;
    }
    catch (...)
    {
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::ZGuiDialoguePreferences() ; caught unknown exception in call to createControls01() ")) ;
    }
    if (!widget)
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::ZGuiDialoguePreferences() ; failed in call to createControls01() ")) ;
    m_top_layout->addWidget(widget) ;

    // frame separator
    QFrame *frame = Q_NULLPTR ;
    if (!(frame = ZGuiWidgetCreator::createFrame()))
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::ZGuiDialoguePreferences() : unable to create frame ")) ;
    frame->setFrameStyle(QFrame::HLine | QFrame::Plain) ;
    m_top_layout->addWidget(frame) ;

    // first widg of controls
    try
    {
        m_controls_display = createControls02() ;
    }
    catch (std::exception & error)
    {
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::ZGuiDialoguePreferences() ; caught exception on call to createControls02()  : ") + error.what() ) ;
    }
    catch (...)
    {
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::ZGuiDialoguePreferences() ; caught unknown exception on call to createControls02() ")) ;
    }
    if (!m_controls_display)
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::ZGuiDialoguePreferences() ; failed in call to createControls02() ")) ;
    m_top_layout->addWidget(m_controls_display) ;

    // second widg of controls
    try
    {
        m_controls_blixem = createControls03() ;
    }
    catch (std::exception & error)
    {
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::ZGuiDialoguePreferences() ; caught exception on call to createControls03(): ") + error.what() ) ;
    }
    catch (...)
    {
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::ZGuiDialoguePreferences() ; caught unknown exception on call to createControls03() ")) ;
    }
    if (!m_controls_blixem)
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::ZGuiDialoguePreferences() ; failed in call to createControls03() ")) ;
    m_top_layout->addWidget(m_controls_blixem) ;

    // another frame separator
    frame = Q_NULLPTR ;
    if (!(frame = ZGuiWidgetCreator::createFrame()))
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::ZGuiDialoguePreferences() : unable to create frame ")) ;
    frame->setFrameStyle(QFrame::HLine | QFrame::Plain) ;
    m_top_layout->addWidget(frame) ;
    frame = Q_NULLPTR ;

    // finally the buttons along the bottom
    try
    {
        widget = createControls07() ;
    }
    catch (std::exception & error)
    {
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::ZGuiDialoguePreferences() ; caught exception on call to createControls07() : ") + error.what() ) ;
    }
    catch (...)
    {
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::ZGuiDialoguePreferences() ; caught unknown exception on call to createControls07() ")) ;
    }
    if (!widget)
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::ZGuiDialoguePreferences() ; failed in attempt to call createControls07() ")) ;
    m_top_layout->addWidget(widget) ;

    // last but not least... and in fact, not actually last
    m_top_layout->addStretch(100) ;

    // other window settings
    //setAttribute(Qt::ApplicationModal) ; // might also want this
    setWindowTitle(m_display_name) ;
    setAttribute(Qt::WA_DeleteOnClose) ;

    // help dialogue
    if (!connect(this, SIGNAL(signal_display_help()), this, SLOT(slot_display_help_create())))
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::ZGuiDialoguePreferences() ; unable to connect help display signal to appropriate slot")) ;

    // finally, set the default to display
    m_combo_preferences->setPreferences(ZGPreferencesType::Display) ;
}

ZGuiDialoguePreferences::~ZGuiDialoguePreferences()
{
}


ZGuiDialoguePreferences* ZGuiDialoguePreferences::newInstance(QWidget *parent)
{
    ZGuiDialoguePreferences *thing = Q_NULLPTR ;

    try
    {
        thing = new ZGuiDialoguePreferences(parent) ;
    }
    catch (...)
    {
        thing = Q_NULLPTR ;
    }

    return thing ;
}

void ZGuiDialoguePreferences::closeEvent(QCloseEvent*)
{
    emit signal_received_close_event() ;
}

void ZGuiDialoguePreferences::createAllMenus()
{
    createFileMenu() ;
    QMenuBar* menu_bar = menuBar() ;
    if (menu_bar)
        menu_bar->addSeparator() ;
    createHelpMenu() ;
}

void ZGuiDialoguePreferences::createFileMenu()
{
    QMenuBar *menu_bar = menuBar() ;
    if (menu_bar)
    {
        if (!(m_menu_file = menu_bar->addMenu(QString("&File"))))
            throw std::runtime_error(std::string("ZGuiDialoguePreferences::createFileMenu() ; unable to create new menu ")) ;

        if (m_action_file_save)
            m_menu_file->addAction(m_action_file_save) ;
        if (m_action_file_save_all)
            m_menu_file->addAction(m_action_file_save_all) ;
        if (m_action_file_close)
            m_menu_file->addAction(m_action_file_close) ;
    }
    else
    {
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::createFileMenu() ; could not access menu bar")) ;
    }
}

void ZGuiDialoguePreferences::createHelpMenu()
{
    QMenuBar *menu_bar = menuBar() ;
    if (menu_bar)
    {
        if (!(m_menu_help = menu_bar->addMenu(QString("&Help"))))
            throw std::runtime_error(std::string("ZGuiDialoguePreferences::createHelpMenu() ; unable to create new menu ")) ;

        if (m_action_help_overview)
            m_menu_help->addAction(m_action_help_overview) ;
    }
    else
    {
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::createHelpMenu() ; could not access menu bar")) ;
    }

}

void ZGuiDialoguePreferences::createFileMenuActions()
{
    if (!m_action_file_save && (m_action_file_save = ZGuiWidgetCreator::createAction(QString("&Save"),this)))
    {
        m_action_file_save->setShortcut(QKeySequence::Save) ;
        m_action_file_save->setShortcutContext(Qt::WindowShortcut) ;
        m_action_file_save->setStatusTip(QString("Save preferences ")) ;
        addAction(m_action_file_save) ;
        connect(m_action_file_save, SIGNAL(triggered(bool)), this, SLOT(slot_action_file_save())) ;
    }

    if (!m_action_file_save_all && (m_action_file_save_all = ZGuiWidgetCreator::createAction(QString("Save all"))) )
    {
        m_action_file_save_all->setStatusTip(QString("Save all preferences ")) ;
        addAction(m_action_file_save_all) ;
        connect(m_action_file_save_all, SIGNAL(triggered(bool)), this, SLOT(slot_action_file_save_all())) ;
    }

    if (!m_action_file_close && (m_action_file_close = ZGuiWidgetCreator::createAction(QString("Close"))))
    {
        m_action_file_close->setShortcut(QKeySequence::Close) ;
        m_action_file_close->setShortcutContext(Qt::WindowShortcut) ;
        m_action_file_close->setStatusTip(QString("Close the preferences dialogue ")) ;
        addAction(m_action_file_close) ;
        connect(m_action_file_close, SIGNAL(triggered(bool)), this, SLOT(slot_action_file_close())) ;
    }
}

void ZGuiDialoguePreferences::createHelpMenuActions()
{
    if (!m_action_help_overview && (m_action_help_overview = ZGuiWidgetCreator::createAction(QString("Overview"),this)))
    {
        m_action_help_overview->setStatusTip(QString("Help for this dialogue ")) ;
        addAction(m_action_help_overview) ;
        connect(m_action_help_overview, SIGNAL(triggered(bool)), this, SLOT(slot_action_help_overview())) ;
    }
}


QWidget * ZGuiDialoguePreferences::createControls01()
{
    QWidget *widget = Q_NULLPTR ;
    if (!(widget = ZGuiWidgetCreator::createWidget()))
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::createControls01() ; unable to create top level widget ")) ;

    QHBoxLayout * layout = Q_NULLPTR ;
    if (!(layout = ZGuiWidgetCreator::createHBoxLayout()))
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::createControls01() ; unable to create top layout ")) ;
    widget->setLayout(layout) ;
    layout->setMargin(0) ;

    QLabel* label = Q_NULLPTR ;
    if (!(label = ZGuiWidgetCreator::createLabel(QString("Choose preferences:"))))
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::createControls01() ; unable to create label ")) ;
    layout->addWidget(label, 0, Qt::AlignLeft) ;

    if (!(m_combo_preferences = ZGuiWidgetComboPreferences::newInstance()))
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::createControls01() ; unable to create preferences combo box ")) ;
    if (!connect(m_combo_preferences, SIGNAL(currentIndexChanged(int)), this, SLOT(slot_control_panel_changed())))
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::createControls01() ; unable to connect preferences combo box to appropriate slot")) ;
    layout->addWidget(m_combo_preferences, 0, Qt::AlignLeft) ;

    layout->addStretch(100) ;

    return widget ;
}

// first thing; tab not needed... just want a bunch of controls...
QWidget *ZGuiDialoguePreferences::createControls02()
{
    QWidget* widget = Q_NULLPTR ;
    if (!(widget = ZGuiWidgetCreator::createWidget()))
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::createControls03() ; unable to create top level widget ")) ;

    QGridLayout *glayout = Q_NULLPTR ;
    QLabel *label = Q_NULLPTR ;
    if (!(glayout = ZGuiWidgetCreator::createGridLayout()))
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::createControls03() ; unable to create first hbox layout ")) ;
    widget->setLayout(glayout) ;
    glayout->setMargin(0) ;

    if (!(label = ZGuiWidgetCreator::createLabel(QString("Shrinkable window:"))))
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::createControls03() ; unable to create first label ")) ;
    if (!(m_check_shrinkable = ZGuiWidgetCreator::createCheckBox()))
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::createControls03() ; unable to create first check box")) ;
    glayout->addWidget(label, 0, 0, 1, 1, Qt::AlignRight) ;
    glayout->addWidget(m_check_shrinkable, 0, 1, 1, 1, Qt::AlignLeft) ;

    if (!(label = ZGuiWidgetCreator::createLabel(QString("Highlight filtered columns:"))))
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::createControls03() ; unable to create second label ")) ;
    if (!(m_check_highlight = ZGuiWidgetCreator::createCheckBox()))
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::createControls03() ; unable to create second check box ")) ;
    glayout->addWidget(label, 1, 0, 1, 1, Qt::AlignRight) ;
    glayout->addWidget(m_check_highlight, 1, 1, 1, 1, Qt::AlignLeft) ;

    if (!(label = ZGuiWidgetCreator::createLabel(QString("Enable annotation:"))))
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::createControls03() ; unable to create third label ")) ;
    if (!(m_check_enable = ZGuiWidgetCreator::createCheckBox()))
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::createControls03() ; unable to create third check box ")) ;
    glayout->addWidget(label, 2, 0, 1, 1, Qt::AlignRight) ;
    glayout->addWidget(m_check_enable, 2, 1, 1, 1, Qt::AlignLeft) ;

    return widget ;
}


// second thing; top level thing should be a tab, and its elements are widgets
QWidget *ZGuiDialoguePreferences::createControls03()
{
    QTabWidget * tab_widget = Q_NULLPTR ;
    if (!(tab_widget = ZGuiWidgetCreator::createTabWidget()))
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::createControls03() ; unable to create top level widget ")) ;

    // create widges to add as pages to the tab
    QWidget * widget = Q_NULLPTR ;
    if (!(widget = createControls04()))
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::createControls03() ; unable to create first widget ")) ;
    tab_widget->addTab(widget, QString("General")) ;

    if (!(widget = createControls05()))
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::createControls03() ; unable to create second widget ")) ;
    tab_widget->addTab(widget, QString("Pfetch socket server")) ;

    if (!(widget = createControls06()))
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::createControls03() ; unable to create third widget ")) ;
    tab_widget->addTab(widget, QString("Advanced")) ;

    return tab_widget ;
}

// "general" tab for blixem controls
QWidget* ZGuiDialoguePreferences::createControls04()
{
    QWidget * widget = Q_NULLPTR ;

    if (!(widget = ZGuiWidgetCreator::createWidget()))
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::createControls04() ; unable to create top level widget ")) ;
    QGridLayout *glayout = Q_NULLPTR ;
    if (!(glayout = ZGuiWidgetCreator::createGridLayout()))
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::createControls04() ; unable to create grid layout ")) ;
    widget->setLayout(glayout) ;
    glayout->setMargin(0) ;

    QLabel *label = Q_NULLPTR ;
    if (!(label = ZGuiWidgetCreator::createLabel(QString("Scope:"))))
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::createControls04() ; unable to create first label")) ;
    if (!(m_entry_scope = ZGuiWidgetCreator::createLineEdit()))
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::createControls04() ; unable to create first line edit ")) ;
    if (!(m_scope_validator = ZGuiCoordinateValidator::newInstance()))
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::createControls04() ; unable to create scope validator ")) ;
    m_scope_validator->setParent(this) ;
    m_entry_scope->setValidator(m_scope_validator) ;

    glayout->addWidget(label, 0, 0, 1, 1, Qt::AlignRight) ;
    glayout->addWidget(m_entry_scope, 0, 1, 1, 1) ;

    if (!(label = ZGuiWidgetCreator::createLabel(QString("Restrict scope to mark:"))))
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::createControls04() ; unable to create second label ")) ;
    if (!(m_check_scope = ZGuiWidgetCreator::createCheckBox()))
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::createControls04() ; unable to create check  box ")) ;
    glayout->addWidget(label, 1, 0, 1, 1, Qt::AlignRight) ;
    glayout->addWidget(m_check_scope, 1, 1, 1, 1, Qt::AlignLeft) ;

    if (!(label = ZGuiWidgetCreator::createLabel(QString("Restrict features to mark:"))))
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::createControls04() ; unable to create third label ")) ;
    if (!(m_check_features = ZGuiWidgetCreator::createCheckBox()))
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::createControls04() ; unable to create check box"))  ;
    glayout->addWidget(label, 2, 0, 1, 1, Qt::AlignRight) ;
    glayout->addWidget(m_check_features, 2, 1, 1, 1, Qt::AlignLeft) ;

    if (!(label = ZGuiWidgetCreator::createLabel(QString("Maximum homols shown:"))))
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::createControls04() ; unable to create fourth label")) ;
    if (!(m_entry_max = ZGuiWidgetCreator::createLineEdit()))
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::createControls04() ; unable to create line edit ")) ;
    if (!(m_maxhomols_validator = ZGuiCoordinateValidator::newInstance()))
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::createControls04() ; unable to max homols validator ")) ;
    m_maxhomols_validator->setParent(this) ;
    m_maxhomols_validator->setRange(0) ;
    m_entry_max->setValidator(m_maxhomols_validator) ;

    glayout->addWidget(label, 3, 0, 1, 1, Qt::AlignRight) ;
    glayout->addWidget(m_entry_max, 3, 1, 1, 1) ;

    return widget ;
}

// "pfetch" tab for blixem controls
QWidget* ZGuiDialoguePreferences::createControls05()
{
    QWidget *widget = Q_NULLPTR ;
    if (!(widget = ZGuiWidgetCreator::createWidget()))
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::createControls05() ; unable to create top level widget ")) ;
    QGridLayout* glayout = Q_NULLPTR ;
    if (!(glayout = ZGuiWidgetCreator::createGridLayout()))
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::createControls05() ; unable to create grid layout ")) ;
    widget->setLayout(glayout) ;
    glayout->setMargin(0) ;

    QLabel *label = Q_NULLPTR ;
    if (!(label = ZGuiWidgetCreator::createLabel(QString("Host network id:"))))
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::createControls05() ; unable to create first label ")) ;
    if (!(m_entry_host = ZGuiWidgetCreator::createLineEdit()))
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::createControls05() ; unable to create first line edit ")) ;
    glayout->addWidget(label, 0, 0, 1, 1, Qt::AlignRight) ;
    glayout->addWidget(m_entry_host, 0, 1, 1, 1) ;

    if (!(label = ZGuiWidgetCreator::createLabel(QString("Port:"))))
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::createControls05() ; unable to create second label ")) ;
    if (!(m_entry_port = ZGuiWidgetCreator::createLineEdit()))
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::createControls05() ; unable to create second line edit ")) ;
    if (!(m_port_validator = ZGuiPortValidator::newInstance()))
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::createControls05() ; unable to create port validator ")) ;
    m_port_validator->setParent(this) ;
    m_entry_port->setValidator(m_port_validator) ;

    glayout->addWidget(label, 1, 0, 1, 1, Qt::AlignRight) ;
    glayout->addWidget(m_entry_port, 1, 1, 1, 1) ;

    return widget ;
}


// "advanced" tab for blixem controls
QWidget* ZGuiDialoguePreferences::createControls06()
{
    QWidget *widget = Q_NULLPTR ;
    if (!(widget = ZGuiWidgetCreator::createWidget()))
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::createControls06() ; unable to create top level widget ")) ;
    QGridLayout * glayout = Q_NULLPTR ;
    if (!(glayout = ZGuiWidgetCreator::createGridLayout()))
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::createControls06() ; unable to create grid layout ")) ;
    widget->setLayout(glayout) ;
    glayout->setMargin(0) ;

    QLabel *label = Q_NULLPTR ;
    if (!(label = ZGuiWidgetCreator::createLabel(QString("Config file:"))))
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::createControls06() ; unable to create first label ")) ;
    if (!(m_entry_config = ZGuiWidgetCreator::createLineEdit()))
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::createControls06() ; unable to create line edit ")) ;
    glayout->addWidget(label, 0, 0, 1, 1, Qt::AlignRight) ;
    glayout->addWidget(m_entry_config, 0, 1, 1, 1) ;


    if (!(label = ZGuiWidgetCreator::createLabel(QString("Launch script:"))))
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::createControls06() ; unable to create second label ")) ;
    if (!(m_entry_launch = ZGuiWidgetCreator::createLineEdit()))
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::createControls06() ; unable to create line edit ")) ;
    glayout->addWidget(label, 1, 0, 1, 1, Qt::AlignRight) ;
    glayout->addWidget(m_entry_launch, 1, 1, 1, 1) ;


    if (!(label = ZGuiWidgetCreator::createLabel(QString("Keep temporary files:"))))
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::createControls06() ; unable to create third label ")) ;
    if (!(m_check_temporary = ZGuiWidgetCreator::createCheckBox()))
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::createControls06() ; unable to create first check box ")) ;
    glayout->addWidget(label, 2, 0, 1, 1, Qt::AlignRight) ;
    glayout->addWidget(m_check_temporary, 2, 1, 1, 1) ;

    if (!(label = ZGuiWidgetCreator::createLabel(QString("Sleep on startup:"))))
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::createControls06() ; unable to create fourth label ")) ;
    if (!(m_check_sleep = ZGuiWidgetCreator::createCheckBox()))
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::createControls06() ; unable to create second check box ")) ;
    glayout->addWidget(label, 3, 0, 1, 1, Qt::AlignRight) ;
    glayout->addWidget(m_check_sleep, 3, 1, 1, 1) ;

    if (!(label = ZGuiWidgetCreator::createLabel(QString("Kill Blixem on exit:"))))
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::createControls06() ; unable to create fifth label ")) ;
    if (!(m_check_kill = ZGuiWidgetCreator::createCheckBox()))
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::createControls06() ; unable to create third check box ")) ;
    glayout->addWidget(label, 4, 0, 1, 1, Qt::AlignRight) ;
    glayout->addWidget(m_check_kill, 4, 1, 1, 1) ;

    return widget ;
}

// button box at the bottom
QWidget *ZGuiDialoguePreferences::createControls07()
{
    QWidget* widget = Q_NULLPTR ;
    if (!(widget = ZGuiWidgetCreator::createWidget()))
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::createControls07() ; unable to create top level widget ")) ;
    QHBoxLayout * layout = Q_NULLPTR ;
    if (!(layout = ZGuiWidgetCreator::createHBoxLayout()))
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::createControls07() ; unable to create top layout for this widget ")) ;
    widget->setLayout(layout) ;
    layout->setMargin(0) ;

    // now for some buttons...
    if (!(m_button_cancel = ZGuiWidgetCreator::createPushButton(QString("Cancel"))))
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::createControls07() ; unable to create Cancel button ")) ;
    if (!connect(m_button_cancel, SIGNAL(clicked(bool)), this, SLOT(slot_action_file_close())))
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::createControls07() ; unable to connect cancel button to appropriate slot ")) ;
    layout->addWidget(m_button_cancel, 0, Qt::AlignLeft) ;
    layout->addStretch(100) ;

    if (!(m_button_OK = ZGuiWidgetCreator::createPushButton(QString("OK"))))
        throw std::runtime_error(std::string("ZGuiDialoguePreferences::createControls07() ; unable to create OK button ")) ;
    layout->addWidget(m_button_OK, 0, Qt::AlignRight) ;

    return widget ;
}

bool ZGuiDialoguePreferences::setPreferencesDisplayData(const ZGPreferencesDisplayData& data)
{
    bool result = false ;

    if (m_check_shrinkable)
    {
        m_check_shrinkable->setChecked(data.getShrinkableWindow()) ;
    }

    if (m_check_highlight)
    {
        m_check_highlight->setChecked(data.getHighlightFiltered()) ;
    }

    if (m_check_enable)
    {
        m_check_enable->setChecked(data.getEnableAnnotation()) ;
    }

    result = true ;

    return result ;
}

ZGPreferencesDisplayData ZGuiDialoguePreferences::getPreferencesDisplayData() const
{
    ZGPreferencesDisplayData data ;

    if (m_check_shrinkable)
    {
        data.setShrinkableWindow(m_check_shrinkable->isChecked()) ;
    }

    if (m_check_highlight)
    {
        data.setHighlightFiltered(m_check_highlight->isChecked()) ;
    }

    if (m_check_enable)
    {
        data.setEnableAnnotation(m_check_enable->isChecked()) ;
    }

    return data ;
}


bool ZGuiDialoguePreferences::setPreferencesBlixemData(const ZGPreferencesBlixemData& data)
{
    bool result = false ;

    if (m_entry_scope)
    {
        std::stringstream str ;
        str << data.getScope() ;
        m_entry_scope->setText(QString::fromStdString(str.str())) ;
    }

    if (m_check_scope)
    {
        m_check_scope->setChecked(data.getRestrictScope()) ;
    }

    if (m_check_features)
    {
        m_check_features->setChecked(data.getRestrictFeatures()) ;
    }

    if (m_entry_max)
    {
        std::stringstream str ;
        str << data.getMax() ;
        m_entry_max->setText(QString::fromStdString(str.str())) ;
    }

    if (m_entry_host)
    {
        m_entry_host->setText(QString::fromStdString(data.getNetworkID())) ;
    }

    if (m_entry_port)
    {
        std::stringstream str ;
        str << data.getPort() ;
        m_entry_port->setText(QString::fromStdString(str.str())) ;
    }

    if (m_entry_config)
    {
        m_entry_config->setText(QString::fromStdString(data.getConfigFile())) ;
    }

    if (m_entry_launch)
    {
        m_entry_launch->setText(QString::fromStdString(data.getLaunchScript())) ;
    }

    if (m_check_temporary)
    {
        m_check_temporary->setChecked(data.getKeepTemporary()) ;
    }

    if (m_check_sleep)
    {
        m_check_sleep->setChecked(data.getSleep()) ;
    }

    if (m_check_kill)
    {
        m_check_kill->setChecked(data.getKill()) ;
    }

    result = true ;

    return result ;
}

ZGPreferencesBlixemData ZGuiDialoguePreferences::getPreferencesBlixemData() const
{
    ZGPreferencesBlixemData data ;

    if (m_entry_scope)
    {
        uint32_t value = 0 ;
        bool ok = false ;
        value = m_entry_scope->text().toUInt(&ok) ;
        if (ok)
            data.setScope(value) ;
    }

    if (m_check_scope)
    {
        data.setRestrictScope(m_check_scope->isChecked()) ;
    }

    if (m_check_features)
    {
        data.setRestrictFeatures(m_check_features->isChecked()) ;
    }

    if (m_entry_max)
    {
        uint32_t value = 0 ;
        bool ok = false ;
        value = m_entry_max->text().toUInt(&ok) ;
        if (ok)
            data.setMax(value) ;
    }

    if (m_entry_host)
    {
        data.setNetworkID(m_entry_host->text().toStdString()) ;
    }

    if (m_entry_port)
    {
        uint32_t value = 0 ;
        bool ok = false ;
        value = m_entry_port->text().toUInt(&ok) ;
        if (ok)
            data.setPort(value) ;
    }

    if (m_entry_config)
    {
        data.setConfigFile(m_entry_config->text().toStdString()) ;
    }

    if (m_entry_launch)
    {
        data.setLaunchScript(m_entry_launch->text().toStdString()) ;
    }

    if (m_check_temporary)
    {
        data.setKeepTemporary(m_check_temporary->isChecked()) ;
    }

    if (m_check_sleep)
    {
        data.setSleep(m_check_sleep->isChecked()) ;
    }

    if (m_check_kill)
    {
        data.setKill(m_check_kill->isChecked()) ;
    }

    return data ;
}



////////////////////////////////////////////////////////////////////////////////
/// Slots
////////////////////////////////////////////////////////////////////////////////


void ZGuiDialoguePreferences::slot_action_file_save()
{

}

void ZGuiDialoguePreferences::slot_action_file_save_all()
{

}

void ZGuiDialoguePreferences::slot_action_file_close()
{
    emit signal_received_close_event() ;
}

void ZGuiDialoguePreferences::slot_action_help_overview()
{
    emit signal_display_help() ;
}

void ZGuiDialoguePreferences::slot_control_panel_changed()
{
    if (m_combo_preferences)
    {
        ZGPreferencesType preferences = m_combo_preferences->getPreferences() ;
        if (preferences == ZGPreferencesType::Blixem)
        {
            m_controls_display->hide() ;
            m_controls_blixem->show() ;
        }
        else if (preferences == ZGPreferencesType::Display)
        {
            m_controls_blixem->hide() ;
            m_controls_display->show() ;
        }
        resize(sizeHint()) ;
    }
}


void ZGuiDialoguePreferences::slot_display_help_create()
{
    if (!m_text_display_dialogue)
    {
        try
        {
            m_text_display_dialogue = ZGuiTextDisplayDialogue::newInstance() ;
            connect(m_text_display_dialogue, SIGNAL(signal_received_close_event()), this, SLOT(slot_display_help_closed())) ;
        }
        catch (...)
        {
            return ;
        }
    }
    m_text_display_dialogue->setText(m_help_text_overview) ;
    m_text_display_dialogue->setWindowTitle(m_help_text_title_overview) ;

    m_text_display_dialogue->show() ;
}


void ZGuiDialoguePreferences::slot_display_help_closed()
{
    if (m_text_display_dialogue)
    {
        m_text_display_dialogue->close() ;
        m_text_display_dialogue = Q_NULLPTR ;
    }
}

} // namespace GUI

} // namespace ZMap

