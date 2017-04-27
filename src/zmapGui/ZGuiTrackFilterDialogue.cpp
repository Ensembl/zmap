/*  File: ZGuiTrackFilterDialogue.cpp
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


#include "ZGuiTrackFilterDialogue.h"
#include "ZGuiWidgetCreator.h"
#include <stdexcept>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QComboBox>

namespace ZMap
{

namespace GUI
{

template <> const std::string Util::ClassName<ZGuiTrackFilterDialogue>::m_name("ZGuiTrackFilterDialogue") ;
const QString ZGuiTrackFilterDialogue::m_display_name("ZMap Track Filter") ;
const int ZGuiTrackFilterDialogue::m_bases_to_match_min(0),
    ZGuiTrackFilterDialogue::m_bases_to_match_max(50) ;

ZGuiTrackFilterDialogue::ZGuiTrackFilterDialogue(QWidget* parent)
    : QDialog(parent),
      Util::InstanceCounter<ZGuiTrackFilterDialogue>(),
      Util::ClassName<ZGuiTrackFilterDialogue>(),
      m_top_layout(Q_NULLPTR),
      m_box1(Q_NULLPTR),
      m_box2(Q_NULLPTR),
      m_box3(Q_NULLPTR),
      m_top_label(Q_NULLPTR),
      m_bases_to_match(Q_NULLPTR),
      m_button_set_to_current(Q_NULLPTR),
      m_button_filter_all(Q_NULLPTR),
      m_button_cancel(Q_NULLPTR),
      m_button_unfilter(Q_NULLPTR),
      m_button_filter(Q_NULLPTR),
      m_combo_box1(Q_NULLPTR),
      m_combo_box2(Q_NULLPTR),
      m_combo_box3(Q_NULLPTR),
      m_combo_box4(Q_NULLPTR),
      m_combo_box5(Q_NULLPTR),
      m_combo_box6(Q_NULLPTR)
{

    if (!(m_top_layout = ZGuiWidgetCreator::createVBoxLayout()))
        throw std::runtime_error(std::string("ZGuiTrackFilterDialogue::ZGuiTrackFilterDialogue() ; could not create top_layout ")) ;
    setLayout(m_top_layout) ;

    // first box
    QWidget* widget = Q_NULLPTR ;
   if (!(widget  = createBox1()))
        throw std::runtime_error(std::string("ZGuiTrackFilterDialogue::ZGuiTrackFilterDialogue() ; could not create box1 ")) ;
    m_top_layout->addWidget(widget) ;

    // second box
    if (!(widget  = createBox2()))
        throw std::runtime_error(std::string("ZGuiTrackFilterDialogue::ZGuiTrackFilterDialogue() ; could not create box2")) ;
    m_top_layout->addWidget(widget) ;

    // third section
    if (!(widget = createBox3() ))
        throw std::runtime_error(std::string("ZGuiTrackFilterDialogue::ZGuiTrackFilterDialogue() ; could not create box3")) ;
    m_top_layout->addWidget(widget) ;

    // button layout
    QHBoxLayout *button_layout = Q_NULLPTR ;
    if (!(button_layout = ZGuiWidgetCreator::createHBoxLayout()))
        throw std::runtime_error(std::string("ZGuiTrackFilterDialogue::ZGuiTrackFilterDialogue() ; could not create button_layout ")) ;
    m_top_layout->addLayout(button_layout) ;

    // accept reject buttons
    if (!(m_button_cancel = ZGuiWidgetCreator::createPushButton(QString("Cancel"))))
        throw std::runtime_error(std::string("ZGuiTrackFilterDialogue::ZGuiTrackFilterDialogue() ; unable to create cancel button ")) ;
    if (!(m_button_unfilter = ZGuiWidgetCreator::createPushButton(QString("Unfilter"))))
        throw std::runtime_error(std::string("ZGuiTrackFilterDialogue::ZGuiTrackFilterDialogue() ; unable to create unfilter button ")) ;
    if (!(m_button_filter = ZGuiWidgetCreator::createPushButton(QString("Filter"))))
        throw std::runtime_error(std::string("ZGuiTrackFilterDialogue::ZGuiTrackFilterDialogue() ; unable to create filter button ")) ;
    button_layout->addWidget(m_button_cancel) ;
    button_layout->addStretch(100) ;
    button_layout->addWidget(m_button_unfilter) ;
    button_layout->addWidget(m_button_filter) ;
    if (!connect(m_button_cancel, SIGNAL(clicked(bool)), this, SLOT(reject())))
        throw std::runtime_error(std::string("ZGuiTrackFilterDialogue::ZGuiTrackFilterDialogue() ; could not connect m_button_cancel")) ;
    if (!connect(m_button_unfilter, SIGNAL(clicked(bool)), this, SLOT(slot_unfilter())))
        throw std::runtime_error(std::string("ZGuiTrackFilterDialogue::ZGuiTrackFilterDialogue() ; could not connect m_button_unfilter")) ;
    if (!connect(m_button_filter, SIGNAL(clicked(bool)), this, SLOT(accept())))
        throw std::runtime_error(std::string("ZGuiTrackFilterDialogue::ZGuiTrackFilterDialogue() ; could not connect m_button_filter")) ;

    // other window settings
    setWindowTitle(m_display_name) ;
    setModal(true) ;
}

ZGuiTrackFilterDialogue::~ZGuiTrackFilterDialogue()
{
}

ZGuiTrackFilterDialogue* ZGuiTrackFilterDialogue::newInstance(QWidget *parent)
{
    ZGuiTrackFilterDialogue *dialogue = Q_NULLPTR ;

    try
    {
        dialogue = new ZGuiTrackFilterDialogue(parent) ;
    }
    catch (...)
    {
        dialogue = Q_NULLPTR ;
    }

    return dialogue ;
}

QWidget* ZGuiTrackFilterDialogue::createBox1()
{
    QGroupBox * box = Q_NULLPTR ;

    if (!(box = ZGuiWidgetCreator::createGroupBox(QString("Current filter/target features:"))))
        throw std::runtime_error(std::string("ZGuiTrackFilterDialogue::creatBox1() ; unable to create group box ")) ;

    QHBoxLayout *layout = Q_NULLPTR ;
    if (!(layout = ZGuiWidgetCreator::createHBoxLayout()))
        throw std::runtime_error(std::string("ZGuiTrackFilterDialogue::createBox1() ; unable to create layout ")) ;

    if (!(m_top_label = ZGuiWidgetCreator::createLabel(QString("default text for this container"))))
        throw std::runtime_error(std::string("ZGuiTrackFilterDialogue::createBox1() ; unable to create m_top_label ")) ;
    layout->addWidget(m_top_label) ;

    // add the layout to the group_box
    box->setLayout(layout) ;

    return box ;
}

QWidget* ZGuiTrackFilterDialogue::createBox2()
{
    QGroupBox *box = Q_NULLPTR ;

    if (!(box = ZGuiWidgetCreator::createGroupBox(QString("Change filter options:"))))
        throw std::runtime_error(std::string("ZGuiTrackFilterDialogue::createBox2() ; unable to create group box ")) ;

    QGridLayout *layout = Q_NULLPTR ;
    if (!(layout = ZGuiWidgetCreator::createGridLayout()))
        throw std::runtime_error(std::string("ZGuiTrackFilterDialogue::createBox2() ; unable to create layout for this box ")) ;

    QLabel *label = Q_NULLPTR ;
    int row = 0 ;

    if (!(label = ZGuiWidgetCreator::createLabel(QString("Type of match: "))))
        throw std::runtime_error(std::string("ZGuiTrackFilterDialogue::createBox2() ; unable to create label for type of match ")) ;
    layout->addWidget(label, row, 0, Qt::AlignRight) ;
    if (!(m_combo_box1 = createComboBox1()))
        throw std::runtime_error(std::string("ZGuiTrackFilterDialogue::createBox2() ; unable to create combo box for type of match ")) ;
    layout->addWidget(m_combo_box1, row, 1, Qt::AlignLeft) ;
    ++row ;

    if (!(label = ZGuiWidgetCreator::createLabel(QString("+/- bases for matching:"))))
        throw std::runtime_error(std::string("ZGuiTrackFilterDialogue::createBox2() ; unable to create label for +/- bases for matching ")) ;
    layout->addWidget(label, row, 0, Qt::AlignRight) ;
    if (!(m_bases_to_match = ZGuiWidgetCreator::createSpinBox()))
        throw std::runtime_error(std::string("ZGuiTrackFilterDialogue::createBox2() ; unable to create spin box for number of bases to match "))  ;
    m_bases_to_match->setMinimum(m_bases_to_match_min) ;
    m_bases_to_match->setMaximum(m_bases_to_match_max) ;
    layout->addWidget(m_bases_to_match, row, 1, Qt::AlignLeft) ;
    ++row ;

    if (!(label = ZGuiWidgetCreator::createLabel(QString("Part of feature filter to match from:"))))
        throw std::runtime_error(std::string("ZGuiTrackFilterDialogue::createBox2() ; unable to create label for part of feature to match from ")) ;
    layout->addWidget(label, row, 0, Qt::AlignRight) ;
    if (!(m_combo_box2 = createComboBox2()))
        throw std::runtime_error(std::string("ZGuiTrackFilterDialogue::createBox2() ; unable to create combo box for part of feature to match from ")) ;
    layout->addWidget(m_combo_box2, row, 1, Qt::AlignLeft) ;
    ++row ;

    if (!(label = ZGuiWidgetCreator::createLabel(QString("Part of target features to match to:"))))
        throw std::runtime_error(std::string("ZGuiTrackFilterDialogue::createBox2() ; unable to create label for part of target features to match to ")) ;
    layout->addWidget(label, row, 0, Qt::AlignRight) ;
    if (!(m_combo_box3 = createComboBox3()))
        throw std::runtime_error(std::string("ZGuiTrackFilterDialogue::createBox2() ; unable to create combo box for part of target features to match to ")) ;
    layout->addWidget(m_combo_box3, row, 1, Qt::AlignLeft) ;
    ++row ;

    if (!(label = ZGuiWidgetCreator::createLabel(QString("Action to perform on features:"))))
        throw std::runtime_error(std::string("ZGuiTrackFilterDialogue::createBox2() ; unable to create label for action to perform on features ")) ;
    layout->addWidget(label, row, 0, Qt::AlignRight) ;
    if (!(m_combo_box4 = createComboBox4()))
        throw std::runtime_error(std::string("ZGuiTrackFilterDialogue::createBox2() ; unable to create combo box for action to perform on features")) ;
    layout->addWidget(m_combo_box4, row, 1, Qt::AlignLeft) ;
    ++row ;

    if (!(label = ZGuiWidgetCreator::createLabel(QString("Features/tracks to filter:"))))
        throw std::runtime_error(std::string("ZGuiTrackFilterDialogue::createBox2() ; unable to create label for features/tracks to filter")) ;
    layout->addWidget(label, row, 0, Qt::AlignRight) ;
    if (!(m_combo_box5 = createComboBox5()))
        throw std::runtime_error(std::string("ZGuiTrackFilterDialogue::createBox2() ; unable to create combo box for features/tracks to filter ")) ;
        layout->addWidget(m_combo_box5, row, 1, Qt::AlignLeft) ;

    box->setLayout(layout) ;

    return box ;
}

QWidget* ZGuiTrackFilterDialogue::createBox3()
{
    QWidget *widget = Q_NULLPTR ;
    if (!(widget = ZGuiWidgetCreator::createWidget()))
        throw std::runtime_error(std::string("ZGuiTrackFilterDialogue::createBox3() ; unable to create widget ")) ;

    QHBoxLayout* hbox = Q_NULLPTR ;
    if (!(hbox = ZGuiWidgetCreator::createHBoxLayout()))
        throw std::runtime_error(std::string("ZGuiTrackFilterDialogue::createBox3() ; unable to create hbox layout ")) ;
    widget->setLayout(hbox) ;
    hbox->setMargin(0) ;

    // now we need two group boxes to be added to this thing... each of which has a couple of
    // buttons etc...
    QGroupBox *box = Q_NULLPTR ;
    if (!(box = ZGuiWidgetCreator::createGroupBox(QString("Change filter feature(s):"))))
        throw std::runtime_error(std::string("ZGuiTrackFilterDialogue::createBox3() ; unable to create first group box thing ")) ;
    QHBoxLayout *layout = Q_NULLPTR ;
    if (!(layout = ZGuiWidgetCreator::createHBoxLayout()))
        throw std::runtime_error(std::string("ZGuiTrackFilterDialogue::createBox3() ; unable to create first hbox layout ")) ;
    // add a button to this layout
    if (!(m_button_set_to_current = ZGuiWidgetCreator::createPushButton(QString("Set to current highlight"))))
        throw std::runtime_error(std::string("ZGuiTrackFilterDialogue::createBox3() ; unable to create button_set_to_current ")) ;
    layout->addWidget(m_button_set_to_current) ;

    box->setLayout(layout) ;
    hbox->addWidget(box) ;

    if (!(box = ZGuiWidgetCreator::createGroupBox(QString("Change target features:"))))
        throw std::runtime_error(std::string("ZGuiTrackFilterDialogue::createBox3() ; unable to create second group box thing")) ;
    if (!(layout = ZGuiWidgetCreator::createHBoxLayout()))
        throw std::runtime_error(std::string("ZGuiTrackFilterDialogue::createBox3() ; unable to second hbox layout ")) ;
    // add button and combo box to this layout
    if (!(m_button_filter_all = ZGuiWidgetCreator::createPushButton(QString("Filter all tracks"))))
        throw std::runtime_error(std::string("ZGuiTrackFilterDialogue::createBox3() ; unable to create button_filter_all")) ;
    layout->addWidget(m_button_filter_all) ;
    if (!(m_combo_box6 = createComboBox6()))
        throw std::runtime_error(std::string("ZGuiTrackFilterDialogue::createBox3() ; unable to create combo box second group box ")) ;
    layout->addWidget(m_combo_box6) ;

    // add the layout to the group box
    box->setLayout(layout) ;
    hbox->addWidget(box) ;

    return widget ;
}


QComboBox * ZGuiTrackFilterDialogue::createComboBox1()
{
    QComboBox *box = Q_NULLPTR ;

    box = ZGuiWidgetCreator::createComboBox() ;

    if (box)
    {
        box->insertItem(0, QString("Partial (allow missing exons/gaps)")) ;
        box->insertItem(1, QString("Exact")) ;
    }

    return box ;
}

QComboBox * ZGuiTrackFilterDialogue::createComboBox2()
{
    QComboBox *box = Q_NULLPTR ;

    box = ZGuiWidgetCreator::createComboBox() ;

    if (box)
    {
        box->insertItem(0, QString("Matches or exons")) ;
        box->insertItem(1, QString("Gaps or introns")) ;
        box->insertItem(2, QString("CDS")) ;
        box->insertItem(3, QString("Overall start/end")) ;
    }

    return box ;
}


QComboBox * ZGuiTrackFilterDialogue::createComboBox3()
{
    QComboBox *box = Q_NULLPTR ;

    box = ZGuiWidgetCreator::createComboBox() ;

    if (box)
    {
        box->insertItem(0, QString("Matches or exons")) ;
        box->insertItem(1, QString("Gaps or introns")) ;
        box->insertItem(2, QString("CDS")) ;
        box->insertItem(3, QString("Overall start/end")) ;
    }

    return box ;
}


QComboBox * ZGuiTrackFilterDialogue::createComboBox4()
{
    QComboBox *box = Q_NULLPTR ;

    box = ZGuiWidgetCreator::createComboBox() ;

    if (box)
    {
        box->insertItem(0, QString("Show matching features")) ;
        box->insertItem(1, QString("Show non-matching features")) ;
        box->insertItem(2, QString("Show all matches/non-matches")) ;
    }

    return box ;
}


QComboBox * ZGuiTrackFilterDialogue::createComboBox5()
{
    QComboBox *box = Q_NULLPTR ;

    box = ZGuiWidgetCreator::createComboBox() ;

    if (box)
    {
        box->insertItem(0, QString("Do not apply to filter feature(s)")) ;
        box->insertItem(1, QString("Do not apply to filter track")) ;
        box->insertItem(2, QString("Apply to all features/track")) ;
    }

    return box ;
}


QComboBox * ZGuiTrackFilterDialogue::createComboBox6()
{
    QComboBox *box = Q_NULLPTR ;

    box = ZGuiWidgetCreator::createComboBox() ;

    if (box)
    {
        for (int i=0; i<250; ++i)
            box->insertItem(i, QString("Track name here ")) ;
    }

    return box ;
}

////////////////////////////////////////////////////////////////////////////////
/// Slots
////////////////////////////////////////////////////////////////////////////////

void ZGuiTrackFilterDialogue::slot_unfilter()
{

}

} // namespace GUI

} // namespace ZMap
