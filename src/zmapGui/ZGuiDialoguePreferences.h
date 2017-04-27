#ifndef ZGUIDIALOGUEPREFERENCES_H
#define ZGUIDIALOGUEPREFERENCES_H

#include "InstanceCounter.h"
#include <cstddef>
#include <string>
#include <QApplication>
#include <QMainWindow>

QT_BEGIN_NAMESPACE
class QVBoxLayout ;
class QMenu ;
class QAction ;
class QCheckBox ;
class QLineEdit ;
class QPushButton ;
QT_END_NAMESPACE

#include "ZInternalID.h"
#include "Utilities.h"
#include "InstanceCounter.h"
#include "ClassName.h"
#include "ZGPreferencesDisplayData.h"
#include "ZGPreferencesBlixemData.h"

namespace ZMap
{

namespace GUI
{

class ZGuiDialoguePreferencesStyle ;
class ZGuiWidgetComboPreferences ;
class ZGuiTextDisplayDialogue ;
class ZGuiPortValidator ;
class ZGuiCoordinateValidator ;

class ZGuiDialoguePreferences : public QMainWindow,
        public Util::InstanceCounter<ZGuiDialoguePreferences>,
        public Util::ClassName<ZGuiDialoguePreferences>
{

    Q_OBJECT

public:

    enum class Preferences
    {
        Display,
        Blixem
    };

    static ZGuiDialoguePreferences* newInstance(QWidget *parent= Q_NULLPTR) ;

    ~ZGuiDialoguePreferences() ;

    bool setPreferencesDisplayData(const ZGPreferencesDisplayData& data) ;
    ZGPreferencesDisplayData getPreferencesDisplayData() const ;

    bool setPreferencesBlixemData(const ZGPreferencesBlixemData& data) ;
    ZGPreferencesBlixemData getPreferencesBlixemData() const ;

signals:
    void signal_received_close_event() ;
    void signal_display_help() ;

public slots:

private slots:
    void slot_action_file_save() ;
    void slot_action_file_save_all() ;
    void slot_action_file_close() ;
    void slot_action_help_overview() ;
    void slot_control_panel_changed() ;
    void slot_display_help_create() ;
    void slot_display_help_closed() ;

protected:
    void closeEvent(QCloseEvent*) Q_DECL_OVERRIDE ;

private:

    explicit ZGuiDialoguePreferences(QWidget *parent = Q_NULLPTR);
    ZGuiDialoguePreferences(const ZGuiDialoguePreferences&) = delete ;
    ZGuiDialoguePreferences& operator=(const ZGuiDialoguePreferences& ) = delete ;

    static const QString m_display_name,
        m_help_text_overview,
        m_help_text_title_overview ;

    void createAllMenus() ;
    void createFileMenu() ;
    void createHelpMenu() ;
    void createFileMenuActions() ;
    void createHelpMenuActions() ;

    QWidget* createControls01() ;
    QWidget* createControls02() ;
    QWidget* createControls03() ;
    QWidget* createControls04() ;
    QWidget* createControls05() ;
    QWidget* createControls06() ;
    QWidget* createControls07() ;

    ZGuiDialoguePreferencesStyle *m_style ;
    QVBoxLayout *m_top_layout ;
    QMenu *m_menu_file,
        *m_menu_help ;
    QAction *m_action_file_save,
        *m_action_file_save_all,
        *m_action_file_close,
        *m_action_help_overview ;
    ZGuiWidgetComboPreferences *m_combo_preferences ;
    QWidget *m_controls_display,
        *m_controls_blixem ;
    QCheckBox *m_check_shrinkable,
        *m_check_highlight,
        *m_check_enable,
        *m_check_scope,
        *m_check_features,
        *m_check_temporary,
        *m_check_sleep,
        *m_check_kill ;
    QLineEdit *m_entry_scope,
        *m_entry_max,
        *m_entry_host,
        *m_entry_port,
        *m_entry_config,
        *m_entry_launch ;
    QPushButton *m_button_cancel,
        *m_button_OK ;
    ZGuiTextDisplayDialogue *m_text_display_dialogue ;
    ZGuiPortValidator *m_port_validator ;
    ZGuiCoordinateValidator *m_scope_validator,
        *m_maxhomols_validator ;
};


} // namespace GUI

} // namespace ZMap


#endif // ZGUIDIALOGUEPREFERENCES_H
