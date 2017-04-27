/*  File: ZGuiDialogueNewSource.cpp
 *  Author: Steve Miller (sm23@sanger.ac.uk)
 *  Copyright (c) 2006-2016: Genome Research Ltd.
 *-------------------------------------------------------------------
 * ZMap is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * or see the on-line version at http://www.gnu.org/copyleft/gpl.txt
 *-------------------------------------------------------------------
 * This file is part of the ZMap genome database user interface
 * and was written by
 *
 * Steve Miller (sm23@sanger.ac.uk)
 *
 * Description:
 *-------------------------------------------------------------------
 */

#include "ZGuiDialogueNewSource.h"
#include "ZGuiWidgetCreator.h"
#include "ZGuiDialogueSelectItems.h"
#include "ZGuiWidgetComboSource.h"
#include "ZGuiPortValidator.h"

#include <stdexcept>
#include <memory>
#include <sstream>

#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QPushButton>
#include <QLineEdit>
#include <QToolButton>
#include <QIcon>
#include <QPixmap>
#include <QStackedLayout>
#include <QStringList>
#include <QCheckBox>
#ifndef QT_NO_DEBUG
#  include <QDebug>
#endif


namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGuiDialogueNewSource>::m_name("ZGuiDialogueNewSource") ;
const QString ZGuiDialogueNewSource::m_display_name("ZMap Create Source");
const QString ZGuiDialogueNewSource::m_default_host("ensembldb.ensembl.org") ;
const QChar ZGuiDialogueNewSource::m_list_delimiter(';') ;


ZGuiDialogueNewSource::ZGuiDialogueNewSource(QWidget *parent)
    : QMainWindow(parent),
      Util::InstanceCounter<ZGuiDialogueNewSource>(),
      Util::ClassName<ZGuiDialogueNewSource>(),
      m_top_layout(Q_NULLPTR),
      m_controls_ensembl(Q_NULLPTR),
      m_controls_fileurl(Q_NULLPTR),
      m_entry_source_type(Q_NULLPTR),
      m_button_cancel(Q_NULLPTR),
      m_button_OK(Q_NULLPTR),
      m_entry_source_name(Q_NULLPTR),
      m_entry_database(Q_NULLPTR),
      m_entry_dbprefix(Q_NULLPTR),
      m_entry_host(Q_NULLPTR),
      m_entry_port(Q_NULLPTR),
      m_entry_username(Q_NULLPTR),
      m_entry_password(Q_NULLPTR),
      m_entry_featuresets(Q_NULLPTR),
      m_entry_biotypes(Q_NULLPTR),
      m_entry_source_fileurl(Q_NULLPTR),
      m_entry_pathurl(Q_NULLPTR),
      m_button_lookupdb(Q_NULLPTR),
      m_button_lookup_featuresets(Q_NULLPTR),
      m_button_lookup_biotypes(Q_NULLPTR),
      m_port_validator(Q_NULLPTR),
      m_check_load_dna(Q_NULLPTR)
{
    if (!Util::canCreateQtItem())
        throw std::runtime_error(std::string("ZGuiDialogueNewSource::ZGuiDialogueNewSource() ; unable to create instance without qApp existing")) ;

    QWidget* widget = Q_NULLPTR ;
    if (!(widget = ZGuiWidgetCreator::createWidget()))
        throw std::runtime_error(std::string("ZGuiDialogueNewSource::ZGuiDialogueNewSource() ; unable to create central widget ")) ;
    try
    {
        setCentralWidget(widget) ;
    }
    catch (std::exception& error)
    {
        throw std::runtime_error(std::string("ZGuiDialogueNewSource::ZGuiDialogueNewSource() ; caught exception on attempt to set central widget: ") + error.what() ) ;
    }
    catch (...)
    {
        throw std::runtime_error(std::string("ZGuiDialogueNewSource::ZGuiDialogueNewSource() ; caught unknown exception on attempt to set central widget ")) ;
    }

    // layout manager for the central widget
    if (!(m_top_layout = ZGuiWidgetCreator::createVBoxLayout()))
        throw std::runtime_error(std::string("ZGuiDialogueNewSource::ZGuiDialogueNewSource() ; unable to create top layout ")) ;
    widget->setLayout(m_top_layout) ;

    // first control box
    QWidget * control_box = Q_NULLPTR ;
    if (!(control_box = createControls01()))
        throw std::runtime_error(std::string("ZGuiDialogueNewSource::ZGuiDialogueNewSource() ; unable to create first control box ")) ;
    m_top_layout->addWidget(control_box) ;

    // frame as a separator
    QFrame *frame = Q_NULLPTR ;
    if (!(frame = ZGuiWidgetCreator::createFrame()))
        throw std::runtime_error(std::string("ZGuiDialogueNewSource::ZGuiDialogueNewSource() : unable to create frame ")) ;
    frame->setFrameStyle(QFrame::HLine | QFrame::Plain) ;
    m_top_layout->addWidget(frame) ;

    // second control box
    if (!(m_controls_ensembl = createControls02()))
        throw std::runtime_error(std::string("ZGuiDialogueNewSource::ZGuiDialogueNewSource() ; unable to create widget from createControls02() ")) ;
    m_top_layout->addWidget(m_controls_ensembl) ;
    //m_controls_ensembl->hide() ;

    // third control box
    if (!(m_controls_fileurl = createControls03()))
        throw std::runtime_error(std::string("ZGuiDialogueNewSource::ZGuiDialogueNewSource() ; unable to create widget from createControls03() ")) ;
    m_top_layout->addWidget(m_controls_fileurl) ;
    //m_controls_fileurl->hide() ;

    // second separator
    if (!(frame = ZGuiWidgetCreator::createFrame()))
        throw std::runtime_error(std::string("ZGuiDialogueNewSource::ZGuiDialogueNewSource() : unable to create frame ")) ;
    frame->setFrameStyle(QFrame::HLine | QFrame::Plain) ;
    m_top_layout->addWidget(frame) ;

    // final control box
    if (!(control_box = createControls04()))
        throw std::runtime_error(std::string("ZGuiDialogueNewSource::ZGuiDialogueNewSource() ; unable to create layout from createControls04() ")) ;
    m_top_layout->addWidget(control_box) ;

    // general window settings
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed) ;
    setWindowTitle(m_display_name) ;
    setAttribute(Qt::WA_DeleteOnClose) ;

    m_top_layout->addStretch(100) ;

    // finally set the default type
    m_entry_source_type->setSource(ZGSourceType::Ensembl) ;
}


ZGuiDialogueNewSource::~ZGuiDialogueNewSource()
{
}

ZGuiDialogueNewSource* ZGuiDialogueNewSource::newInstance(QWidget* parent)
{
    ZGuiDialogueNewSource * source = Q_NULLPTR ;

    try
    {
        source = new ZGuiDialogueNewSource(parent) ;
    }
    catch (...)
    {
        source = Q_NULLPTR ;
    }

    return source ;
}

void ZGuiDialogueNewSource::closeEvent(QCloseEvent *)
{
    emit signal_received_close_event() ;
}


// top controls; just to switch between different source types
QWidget *ZGuiDialogueNewSource::createControls01()
{
    QWidget *widget = Q_NULLPTR ;
    if (!(widget = ZGuiWidgetCreator::createWidget()))
        throw std::runtime_error(std::string("ZGuiDialogueNewSource::createControls01() ; unable to create widget ")) ;

    QHBoxLayout * layout = Q_NULLPTR ;
    if (!(layout = ZGuiWidgetCreator::createHBoxLayout()))
        throw std::runtime_error(std::string("ZGuiDialogueNewSource::createControls01() ; unable to create layout ")) ;
    widget->setLayout(layout) ;

    QLabel *label = Q_NULLPTR ;
    if (!(label = ZGuiWidgetCreator::createLabel(QString("Source type: "))))
        throw std::runtime_error(std::string("ZGuiDialogueNewSource::createControls01() ; unable to create label ")) ;
    layout->addWidget(label, 0, Qt::AlignLeft) ;

    if (!(m_entry_source_type = ZGuiWidgetComboSource::newInstance()))
        throw std::runtime_error(std::string("ZGuiDialogueNewSource::createControls01() ; unable to create source type entry ")) ;
    if (!connect(m_entry_source_type, SIGNAL(currentIndexChanged(int)), this, SLOT(slot_control_panel_changed())))
        throw std::runtime_error(std::string("ZGuiDialogueNewSource::createControls01() ; unable to connect the source entry to appropriate slot")) ;
   layout->addWidget(m_entry_source_type, 0, Qt::AlignLeft) ;

    // add a stretch at the end
    layout->addStretch(100) ;

    return widget ;
}

// ensembl data entries
QWidget* ZGuiDialogueNewSource::createControls02()
{
    QWidget* widget = Q_NULLPTR ;

    if (!(widget = ZGuiWidgetCreator::createWidget()))
        throw std::runtime_error(std::string("ZGuiDialogueNewSource::createControls02() ; unable to create widget ")) ;

    // now add two grid layouts to this one...
    QGridLayout *grid = Q_NULLPTR ;
    if (!(grid = ZGuiWidgetCreator::createGridLayout()))
        throw std::runtime_error(std::string("ZGuiDialogueNewSource::createControls02() ; unable to create first grid layout ")) ;
    widget->setLayout(grid) ;

    int row = 0 ;
    QLabel *label = Q_NULLPTR ;
    if (!(label = ZGuiWidgetCreator::createLabel(QString("Source name:"))))
        throw std::runtime_error(std::string("ZGuiDialogueNewSource::createControls02() ; unable to create 'source name ' label")) ;
    if (!(m_entry_source_name = ZGuiWidgetCreator::createLineEdit()))
        throw std::runtime_error(std::string("ZGuiDialogueNewSource::createControls02() ; unable to create 'source name' line entry ")) ;
    grid->addWidget(label, row, 0, 1, 1, Qt::AlignRight) ;
    grid->addWidget(m_entry_source_name, row, 1, 1, 1) ;
    ++row ;

    if (!(label = ZGuiWidgetCreator::createLabel(QString("Database:"))))
        throw std::runtime_error(std::string("ZGuiDialogueNewSource::createControls02() ; unable to create 'database' label")) ;
    if (!(m_entry_database = ZGuiWidgetCreator::createLineEdit()))
        throw std::runtime_error(std::string("ZGuiDialogueNewSource::createControls02() ; unable to create 'database' line edit ")) ;
    if (!(m_button_lookupdb = ZGuiWidgetCreator::createToolButton()))
        throw std::runtime_error(std::string("ZGuiDialogueNewSource::createControls02() ; unable to create 'database lookup' tool button")) ;
    if (!connect(m_button_lookupdb, SIGNAL(clicked(bool)), this, SLOT(slot_ensembl_lookup_db())))
        throw std::runtime_error(std::string("ZGuiDialogueNewSource::createControls02() ; unable to connect button lookup db to slot ")) ;
    m_button_lookupdb->setIcon(QPixmap(":/icon-search.png")) ;
    grid->addWidget(label, row, 0, 1, 1, Qt::AlignRight) ;
    grid->addWidget(m_entry_database, row, 1, 1, 1) ;
    grid->addWidget(m_button_lookupdb, row, 2, 1, 1, Qt::AlignLeft) ;
    ++row ;

    if (!(label = ZGuiWidgetCreator::createLabel(QString("DB prefix:"))))
        throw std::runtime_error(std::string("ZGuiDialogueNewSource::createControls02() ; unable to create 'db prefix' label ")) ;
    if (!(m_entry_dbprefix = ZGuiWidgetCreator::createLineEdit()))
        throw std::runtime_error(std::string("ZGuiDialogueNewSource::createControls02() ; unable to create 'db prefix' line edit ")) ;
    grid->addWidget(label, row, 0, 1, 1, Qt::AlignRight) ;
    grid->addWidget(m_entry_dbprefix, row, 1, 1, 1) ;
    ++row ;

    row = 0 ;
    if (!(label = ZGuiWidgetCreator::createLabel(QString("Host:"))))
        throw std::runtime_error(std::string("ZGuiDialogueNewSource::createControls02() ; unable to create 'host' label ")) ;
    if (!(m_entry_host = ZGuiWidgetCreator::createLineEdit()))
        throw std::runtime_error(std::string("ZGuiDialogueNewSource::createControls02() ; unable to create 'host' line edit ")) ;
    m_entry_host->setText(m_default_host) ;
    grid->addWidget(label, row, 3, 1, 1, Qt::AlignRight) ;
    grid->addWidget(m_entry_host, row, 4, 1, 1) ;
    ++row ;

    if (!(label = ZGuiWidgetCreator::createLabel(QString("Port:"))))
        throw std::runtime_error(std::string("ZGuiDialogueNewSource::createControls02() ; unable to create 'port' label")) ;
    if (!(m_entry_port = ZGuiWidgetCreator::createLineEdit()))
        throw std::runtime_error(std::string("ZGuiDialogueNewSource::createControls02() ; unable to create 'port' line edit ")) ;
    if (!(m_port_validator = ZGuiPortValidator::newInstance()))
        throw std::runtime_error(std::string("ZGuiDialogueNewSource::createControls02() ; unable to create 'port' validator ")) ;
    m_port_validator->setParent(this) ;
    m_entry_port->setValidator(m_port_validator) ;
    grid->addWidget(label, row, 3, 1, 1, Qt::AlignRight) ;
    grid->addWidget(m_entry_port, row, 4, 1, 1) ;
    ++row ;

    if (!(label = ZGuiWidgetCreator::createLabel(QString("Username:"))))
        throw std::runtime_error(std::string("ZGuiDialogueNewSource::createControls02() ; unable to create 'username' label ")) ;
    if (!(m_entry_username = ZGuiWidgetCreator::createLineEdit()))
        throw std::runtime_error(std::string("ZGuiDialogueNewSource::createControls02() ; unable to create 'username' line edit ")) ;
    grid->addWidget(label, row, 3, 1, 1, Qt::AlignRight) ;
    grid->addWidget(m_entry_username, row, 4, 1, 1) ;
    ++row ;

    if (!(label = ZGuiWidgetCreator::createLabel(QString("Password:"))))
        throw std::runtime_error(std::string("ZGuiDialogueNewSource::createControls02() ; unable to create 'password' label")) ;
    if (!(m_entry_password = ZGuiWidgetCreator::createLineEdit()))
        throw std::runtime_error(std::string("ZGuiDialogueNewSource::createControls02() ; unable to create 'password' line edit ")) ;
    grid->addWidget(label, row, 3, 1, 1, Qt::AlignRight) ;
    grid->addWidget(m_entry_password, row, 4, 1, 1 ) ;
    m_entry_password->setEchoMode(QLineEdit::EchoMode::Password) ;
    ++row ;

    // next lot across the bottom
    if (!(label = ZGuiWidgetCreator::createLabel(QString("Featuresets:"))))
        throw std::runtime_error(std::string("ZGuiDialogueNewSource::createControls02() ; unable to create 'featuresets' label ")) ;
    if (!(m_entry_featuresets = ZGuiWidgetCreator::createLineEdit()))
        throw std::runtime_error(std::string("ZGuiDialogueNewSource::createControls02() ; unable to create 'featuresets' line entry ")) ;
    if (!(m_button_lookup_featuresets = ZGuiWidgetCreator::createToolButton()))
        throw std::runtime_error(std::string("ZGuiDialogueNewSource::createControls02() ; unable to create 'featuresets' tool button")) ;
    if (!connect(m_button_lookup_featuresets, SIGNAL(clicked(bool)), this, SLOT(slot_ensembl_lookup_featuresets())))
        throw std::runtime_error(std::string("ZGuiDialogueNewSource::createControls02() ; unable to connect button lookup featuresets to slot ")) ;
    m_button_lookup_featuresets->setIcon(QPixmap(":/icon-search.png")) ;
    grid->addWidget(label, row, 0, 1, 1, Qt::AlignRight) ;
    grid->addWidget(m_entry_featuresets, row, 1, 1, 4) ;
    grid->addWidget(m_button_lookup_featuresets, row, 5, 1, 1, Qt::AlignLeft) ;
    ++row ;

    if (!(label = ZGuiWidgetCreator::createLabel(QString("Biotypes:"))))
        throw std::runtime_error(std::string("ZGuiDialogueNewSource::createControls02() ; unable to create 'biotypes' label ")) ;
    if (!(m_entry_biotypes = ZGuiWidgetCreator::createLineEdit()))
        throw std::runtime_error(std::string("ZGuiDialogueNewSource::createControls02() ; unable to create 'biotypes' line edit ")) ;
    if (!(m_button_lookup_biotypes = ZGuiWidgetCreator::createToolButton()))
        throw std::runtime_error(std::string("ZGuiDialogueNewSource::createControls02() ; unable to create 'biotypes' tool button ")) ;
    if (!connect(m_button_lookup_biotypes, SIGNAL(clicked(bool)), this, SLOT(slot_ensembl_lookup_biotypes())))
        throw std::runtime_error(std::string("ZGuiDialogueNewSource::createControls02()  ; unable to connect button lookup biotypes to slot ")) ;
    m_button_lookup_biotypes->setIcon(QPixmap(":/icon-search.png")) ;
    grid->addWidget(label, row, 0, 1, 1, Qt::AlignRight) ;
    grid->addWidget(m_entry_biotypes, row, 1, 1, 4) ;
    grid->addWidget(m_button_lookup_biotypes, row, 5, 1, 1, Qt::AlignLeft) ;
    ++row ;

    if (!(label = ZGuiWidgetCreator::createLabel(QString("Load DNA:"))))
        throw std::runtime_error(std::string("ZGuiDialogueNewSource::createControls02() ; unable to create 'load dna' label ")) ;
    if (!(m_check_load_dna = ZGuiWidgetCreator::createCheckBox()))
        throw std::runtime_error(std::string("ZGuiDialogueNewSource::createControls02() ; unable to create 'load dna' check box ")) ;
    grid->addWidget(label, row, 0, 1, 1, Qt::AlignRight) ;
    grid->addWidget(m_check_load_dna, row, 1, 1, 1, Qt::AlignLeft) ;

    return widget ;
}

// ensembl labels and data entries
QWidget* ZGuiDialogueNewSource::createControls03()
{
    QWidget* widget = Q_NULLPTR ;

    if (!(widget = ZGuiWidgetCreator::createWidget()))
        throw std::runtime_error(std::string("ZGuiDialogueNewSource::createControl03() ; unable to create widget  ")) ;

    QGridLayout *grid = Q_NULLPTR ;
    if (!(grid = ZGuiWidgetCreator::createGridLayout()))
        throw std::runtime_error(std::string("ZGuiDialogueNewSource::createControls03() ; unable to create grid layout ")) ;
    widget->setLayout(grid) ;

    int row = 0 ;
    QLabel * label = Q_NULLPTR ;
    if (!(label = ZGuiWidgetCreator::createLabel(QString("Source name:"))))
        throw std::runtime_error(std::string("ZGuiDialogueNewSource::createControls03() ; unable to create source name label ")) ;
    if (!(m_entry_source_fileurl = ZGuiWidgetCreator::createLineEdit()))
        throw std::runtime_error(std::string("ZGuiDialogueNewSource::createControls03() ; unable to create entry for source fileurl ")) ;
    grid->addWidget(label, row, 0, 1, 1, Qt::AlignRight) ;
    grid->addWidget(m_entry_source_fileurl, row, 1, 1, 1) ;
    ++row ;

    if (!(label = ZGuiWidgetCreator::createLabel(QString("Path/URL:"))))
        throw std::runtime_error(std::string("ZGuiDialogueNewSource::createControls03() ; unable to create path/url label ")) ;
    if (!(m_entry_pathurl = ZGuiWidgetCreator::createLineEdit()))
        throw std::runtime_error(std::string("ZGuiDialogueNewSource::createControls03() ; unable to create line edit for path/url ")) ;
    grid->addWidget(label, row, 0, 1, 1, Qt::AlignRight) ;
    grid->addWidget(m_entry_pathurl, row, 1, 1, 1) ;

    return widget ;
}

QWidget *ZGuiDialogueNewSource::createControls04()
{
    QWidget* widget = Q_NULLPTR ;
    if (!(widget = ZGuiWidgetCreator::createWidget()))
        throw std::runtime_error(std::string("ZGuiDialogueNewSource::createControls04() ; unable to create widget ")) ;

    QHBoxLayout* layout = Q_NULLPTR ;
    if (!(layout = ZGuiWidgetCreator::createHBoxLayout()))
        throw std::runtime_error(std::string("ZGuiDialogueNewSource::createControls04() ; unable to create layout ")) ;
    widget->setLayout(layout) ;

    if (!(m_button_cancel = ZGuiWidgetCreator::createPushButton(QString("Cancel"))))
        throw std::runtime_error(std::string("ZGuiDialogueNewSource::createControls04() ; unable to create cancel button ")) ;
    if (!connect(m_button_cancel, SIGNAL(clicked(bool)), this, SLOT(slot_close())))
        throw std::runtime_error(std::string("ZGuiDialogueNewSource::createControls04() ; unable to connect cancel button to slot_close() ")) ;
    layout->addWidget(m_button_cancel, 0, Qt::AlignLeft) ;
    layout->addStretch(100) ;
    if (!(m_button_OK = ZGuiWidgetCreator::createPushButton(QString("OK"))))
        throw std::runtime_error(std::string("ZGuiDialogueNewSource::createControls04() ; unable to create OK button ")) ;
    layout->addWidget(m_button_OK, 0, Qt::AlignRight) ;

    //widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed) ;

    return widget ;
}


bool ZGuiDialogueNewSource::setNewSourceData(const ZGNewSourceData& data)
{
    bool result = false ;
    ZGSourceType type = data.getSourceType() ;

    if (m_entry_source_type)
    {
        m_entry_source_type->setSource(type) ;
    }

    if (m_entry_source_name)
    {
        m_entry_source_name->setText(QString::fromStdString(data.getSourceName())) ;
    }

    if (m_entry_source_fileurl)
    {
        m_entry_source_fileurl->setText(QString::fromStdString(data.getSourceName())) ;
    }

    if (m_entry_pathurl)
    {
        m_entry_pathurl->setText(QString::fromStdString(data.getPathURL())) ;
    }

    if (m_entry_database)
    {
        m_entry_database->setText(QString::fromStdString(data.getDatabase())) ;
    }

    if (m_entry_dbprefix)
    {
        m_entry_dbprefix->setText(QString::fromStdString(data.getPrefix())) ;
    }

    if (m_entry_host)
    {
        m_entry_host->setText(QString::fromStdString(data.getHost())) ;
    }

    if (m_entry_port)
    {
        std::stringstream str ;
        str << data.getPort() ;
        m_entry_port->setText(QString::fromStdString(str.str())) ;
    }

    if (m_entry_username)
    {
        m_entry_username->setText(QString::fromStdString(data.getUsername())) ;
    }

    if (m_entry_password)
    {
        m_entry_password->setText(QString::fromStdString(data.getPassword())) ;
    }

    if (m_entry_featuresets)
    {
        std::set<std::string> featuresets = data.getFeaturesets() ;
        QStringList string_list ;
        for (auto it = featuresets.begin() ; it != featuresets.end() ; ++it)
            string_list.append(QString::fromStdString(*it)) ;
        m_entry_featuresets->setText(string_list.join(m_list_delimiter)) ;
    }

    if (m_entry_biotypes)
    {
        std::set<std::string> biotypes = data.getBiotypes() ;
        QStringList string_list ;
        for (auto it = biotypes.begin() ; it != biotypes.end() ; ++it)
            string_list.append(QString::fromStdString(*it)) ;
        m_entry_biotypes->setText(string_list.join(m_list_delimiter)) ;
    }
    result = true ;

    return result ;
}

ZGNewSourceData ZGuiDialogueNewSource::getNewSourceData() const
{
    ZGNewSourceData data ;
    ZGSourceType type = ZGSourceType::Invalid ;

    if (m_entry_source_type)
    {
        type = m_entry_source_type->getSourceType() ;
        data.setSourceType(type) ;
    }

    if (m_entry_source_name)
    {
        if (type == ZGSourceType::Ensembl)
            data.setSourceName(m_entry_source_name->text().toStdString()) ;
        else if (type == ZGSourceType::FileURL)
            data.setSourceName(m_entry_source_fileurl->text().toStdString()) ;
    }

    if (m_entry_pathurl)
    {
        data.setPathURL(m_entry_pathurl->text().toStdString()) ;
    }

    if (m_entry_database)
    {
        data.setDatabase(m_entry_database->text().toStdString()) ;
    }

    if (m_entry_dbprefix)
    {
        data.setPrefix(m_entry_dbprefix->text().toStdString()) ;
    }

    if (m_entry_host)
    {
        data.setHost(m_entry_host->text().toStdString()) ;
    }

    if (m_entry_port)
    {
        uint32_t value = m_entry_port->text().toUInt() ;
        data.setPort(value) ;
    }

    if (m_entry_username)
    {
        data.setUsername(m_entry_username->text().toStdString()) ;
    }

    if (m_entry_password)
    {
        data.setPassword(m_entry_password->text().toStdString()) ;
    }

    if (m_entry_featuresets)
    {
        std::set<std::string> featuresets ;
        QStringList string_list = m_entry_featuresets->text().split(m_list_delimiter, QString::SplitBehavior::SkipEmptyParts) ;
        for (auto it = string_list.begin() ; it != string_list.end() ; ++it)
            featuresets.insert(it->toStdString()) ;
        data.setFeaturesets(featuresets) ;
    }

    if (m_entry_biotypes)
    {
        std::set<std::string> biotypes ;
        QStringList string_list = m_entry_biotypes->text().split(m_list_delimiter, QString::SplitBehavior::SkipEmptyParts) ;
        for (auto it = string_list.begin() ; it != string_list.end() ; ++it)
            biotypes.insert(it->toStdString()) ;
        data.setBiotypes(biotypes) ;
    }


    return data ;
}








////////////////////////////////////////////////////////////////////////////////
/// Slots
////////////////////////////////////////////////////////////////////////////////

// this swops over control panels... note the resize mechanism that is used
void ZGuiDialogueNewSource::slot_control_panel_changed()
{
    if (m_entry_source_type)
    {
        ZGSourceType type = m_entry_source_type->getSourceType() ;
        if (type == ZGSourceType::Ensembl)
        {
            m_controls_fileurl->hide() ;
            m_controls_ensembl->show() ;
        }
        else if (type == ZGSourceType::FileURL)
        {
            m_controls_ensembl->hide() ;
            m_controls_fileurl->show() ;
        }
        resize(sizeHint());
    }
}

void ZGuiDialogueNewSource::slot_close()
{
    emit signal_received_close_event() ;
}

void ZGuiDialogueNewSource::slot_action_file_close()
{
   emit signal_received_close_event() ;
}


void ZGuiDialogueNewSource::slot_ensembl_lookup_db()
{
    std::unique_ptr<ZGuiDialogueSelectItems> select_items(ZGuiDialogueSelectItems::newInstance()) ;
    select_items->setWindowTitle(QString("ZMap Select Database ")) ;
    select_items->setModal(true) ;
    if (select_items->exec() == QDialog::Accepted)
    {
#ifndef QT_NO_DEBUG
        qDebug() << "ZGuiDialogueNewSource::slot_ensembl_lookup_db() ; accept has been given " ;
#endif
        QStringList selected_items = select_items->getSelectedData() ;
        if (m_entry_database && selected_items.size())
            m_entry_database->setText(*selected_items.begin()) ;
    }
}

void ZGuiDialogueNewSource::slot_ensembl_lookup_featuresets()
{
    std::unique_ptr<ZGuiDialogueSelectItems> select_items(ZGuiDialogueSelectItems::newInstance()) ;
    select_items->setWindowTitle(QString("ZMap Select Featuresets")) ;
    select_items->setModal(true) ;
    if (select_items->exec() == QDialog::Accepted)
    {
#ifndef QT_NO_DEBUG
        qDebug() << "ZGuiDialogueNewSource::slot_ensembl_lookup_featuresets() ; accept has been given " ;
#endif
        QStringList selected_items = select_items->getSelectedData() ;
        if (m_entry_featuresets)
            m_entry_featuresets->setText(selected_items.join(m_list_delimiter)) ;
    }
}

void ZGuiDialogueNewSource::slot_ensembl_lookup_biotypes()
{
    std::unique_ptr<ZGuiDialogueSelectItems> select_items(ZGuiDialogueSelectItems::newInstance()) ;
    select_items->setWindowTitle(QString("ZMap Select Biotypes")) ;
    select_items->setModal(true) ;
    if (select_items->exec() == QDialog::Accepted)
    {
#ifndef QT_NO_DEBUG
        qDebug() << "ZGuiDialogueNewSource::slot_ensembl_lookup_biotypes() ; accept has been given " ;
#endif
        QStringList selected_items = select_items->getSelectedData() ;
        if (m_entry_biotypes)
            m_entry_biotypes->setText(selected_items.join(m_list_delimiter)) ;
    }
}

} // namespace GUI

} // namespace ZMap

