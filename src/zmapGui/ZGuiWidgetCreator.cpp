/*  File: ZGuiWidgetCreator.cpp
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

#include "ZGuiWidgetCreator.h"

#include <QWidget>
#include <QPushButton>
#include <QToolButton>
#include <QLabel>
#include <QLineEdit>
#include <QGroupBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QAction>
#include <QMenu>
#include <QTreeView>
#include <QListView>
#include <QTableView>
#include <QStandardItemModel>
#include <QFrame>
#include <QTabWidget>
#include <QRadioButton>
#include <QButtonGroup>
#include <QTextEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QErrorMessage>
#include <QItemSelectionModel>
#include <QSlider>
#include <QSignalMapper>

#include "ZGuiFileImportStrandValidator.h"
#include "ZGuiCoordinateValidator.h"
#include "ZGuiWidgetSelectSequence.h"
#include "ZGuiWidgetButtonColour.h"


namespace ZMap
{

namespace GUI
{

QWidget* ZGuiWidgetCreator::createWidget(QWidget* parent)
{
    QWidget* widget = Q_NULLPTR ;

    try
    {
        widget = new QWidget(parent) ;
    }
    catch (...)
    {
        widget = Q_NULLPTR ;
    }

    return widget ;
}


QPushButton *ZGuiWidgetCreator::createPushButton(const QString& data)
{
    QPushButton *button = Q_NULLPTR ;

    try
    {
        button = new QPushButton(data) ;
    }
    catch (...)
    {
        button = Q_NULLPTR ;
    }

    return button ;
}

QToolButton * ZGuiWidgetCreator::createToolButton()
{
    QToolButton * button = Q_NULLPTR ;

    try
    {
        button = new QToolButton ;
    }
    catch (...)
    {
        button = Q_NULLPTR ;
    }

    return button ;
}


QLabel* ZGuiWidgetCreator::createLabel(const QString& data)
{
    QLabel *label = Q_NULLPTR ;

    try
    {
        label = new QLabel(data) ;
    }
    catch (...)
    {
        label = Q_NULLPTR ;
    }

    return label ;
}

QLineEdit * ZGuiWidgetCreator::createLineEdit()
{
    QLineEdit *edit = Q_NULLPTR ;

    try
    {
        edit = new QLineEdit ;
    }
    catch (...)
    {
        edit = Q_NULLPTR ;
    }

    return edit ;
}


QGroupBox * ZGuiWidgetCreator::createGroupBox(const QString&data)
{
    QGroupBox * box = Q_NULLPTR ;

    try
    {
        box = new QGroupBox(data) ;
    }
    catch (...)
    {
        box = Q_NULLPTR ;
    }

    return box ;
}


QGridLayout* ZGuiWidgetCreator::createGridLayout()
{
    QGridLayout * layout = Q_NULLPTR ;

    try
    {
        layout = new QGridLayout ;
    }
    catch (...)
    {
        layout = Q_NULLPTR ;
    }

    return layout ;
}

QHBoxLayout* ZGuiWidgetCreator::createHBoxLayout()
{
    QHBoxLayout *layout = Q_NULLPTR ;

    try
    {
        layout = new QHBoxLayout ;
    }
    catch (...)
    {
        layout = Q_NULLPTR ;
    }

    return layout ;
}

QVBoxLayout* ZGuiWidgetCreator::createVBoxLayout()
{
    QVBoxLayout * layout = Q_NULLPTR ;

    try
    {
        layout = new QVBoxLayout ;
    }
    catch (...)
    {
        layout = Q_NULLPTR ;
    }

    return layout ;
}

QCheckBox *ZGuiWidgetCreator::createCheckBox(const QString& data)
{
    QCheckBox * box = Q_NULLPTR ;

    try
    {
        box = new QCheckBox(data) ;
    }
    catch (...)
    {
        box = Q_NULLPTR ;
    }

    return box ;
}

QMenu* ZGuiWidgetCreator::createMenu(QWidget* parent)
{
    QMenu * menu = Q_NULLPTR ;

    try
    {
        menu = new QMenu(parent) ;
    }
    catch (...)
    {
        menu = Q_NULLPTR ;
    }

    return menu ;
}

QAction* ZGuiWidgetCreator::createAction(const QString& data, QWidget* parent)
{
    QAction * action = Q_NULLPTR ;

    try
    {
        action = new QAction(data, parent) ;
    }
    catch (...)
    {
        action = Q_NULLPTR ;
    }

    return action ;
}

QTableView *ZGuiWidgetCreator::createTableView()
{
    QTableView *view = Q_NULLPTR ;

    try
    {
        view = new QTableView ;
    }
    catch (...)
    {
        view = Q_NULLPTR ;
    }

    return view ;
}

QTreeView* ZGuiWidgetCreator::createTreeView()
{
    QTreeView *view = Q_NULLPTR ;

    try
    {
        view = new QTreeView ;
    }
    catch (...)
    {
        view = Q_NULLPTR ;
    }

    return view ;
}

QListView * ZGuiWidgetCreator::createListView()
{
    QListView * view = Q_NULLPTR ;

    try
    {
        view = new QListView ;
    }
    catch (...)
    {
        view = Q_NULLPTR ;
    }

    return view ;
}

QStandardItem* ZGuiWidgetCreator::createStandardItem(const QString& data)
{
    QStandardItem* item = Q_NULLPTR ;

    try
    {
        item = new QStandardItem(data) ;
    }
    catch (...)
    {
        item = Q_NULLPTR ;
    }

    return item ;
}



QStandardItemModel* ZGuiWidgetCreator::createStandardItemModel()
{
    QStandardItemModel *model = Q_NULLPTR ;

    try
    {
        model = new QStandardItemModel ;
    }
    catch (...)
    {
        model = Q_NULLPTR ;
    }

    return model ;
}

QFrame *ZGuiWidgetCreator::createFrame()
{
    QFrame *frame = Q_NULLPTR ;

    try
    {
        frame = new QFrame ;
    }
    catch (...)
    {
        frame = Q_NULLPTR ;
    }

    return frame ;
}

QTabWidget* ZGuiWidgetCreator::createTabWidget()
{
    QTabWidget* widget = Q_NULLPTR ;

    try
    {
        widget = new QTabWidget ;
    }
    catch (...)
    {
        widget = Q_NULLPTR ;
    }

    return widget ;
}

QRadioButton* ZGuiWidgetCreator::createRadioButton()
{
    QRadioButton *button = Q_NULLPTR ;

    try
    {
        button = new QRadioButton ;
    }
    catch (...)
    {
        button = Q_NULLPTR ;
    }

    return button ;
}


QButtonGroup* ZGuiWidgetCreator::createButtonGroup()
{
    QButtonGroup * group = Q_NULLPTR ;

    try
    {
        group = new QButtonGroup ;
    }
    catch (...)
    {
        group = Q_NULLPTR ;
    }

    return group ;
}


QTextEdit *ZGuiWidgetCreator::createTextEdit()
{
    QTextEdit *edit = Q_NULLPTR ;

    try
    {
        edit = new QTextEdit ;
    }
    catch (...)
    {
        edit = Q_NULLPTR ;
    }

    return edit ;
}


QComboBox *ZGuiWidgetCreator::createComboBox()
{
    QComboBox *box = Q_NULLPTR ;

    try
    {
        box = new QComboBox ;
    }
    catch (...)
    {
        box = Q_NULLPTR ;
    }

    return box ;

}


QSpinBox *ZGuiWidgetCreator::createSpinBox()
{
    QSpinBox *box = Q_NULLPTR ;

    try
    {
        box = new QSpinBox ;
    }
    catch (...)
    {
        box = Q_NULLPTR ;
    }

    return box ;
}


QErrorMessage *ZGuiWidgetCreator::createErrorMessage(QWidget* parent)
{
    QErrorMessage *message = Q_NULLPTR ;

    try
    {
        message = new QErrorMessage(parent) ;
    }
    catch (...)
    {
        message = Q_NULLPTR ;
    }

    return message ;
}




QItemSelectionModel* ZGuiWidgetCreator::createItemSelectionModel()
{
    QItemSelectionModel * model = Q_NULLPTR ;

    try
    {
        model = new QItemSelectionModel ;
    }
    catch (...)
    {
        model = Q_NULLPTR ;
    }

    return model ;
}



QSlider *ZGuiWidgetCreator::createSlider()
{
    QSlider *slider = Q_NULLPTR ;


    try
    {
        slider = new QSlider ;
    }
    catch (...)
    {
        slider = Q_NULLPTR ;
    }

    return slider ;
}


QSignalMapper* ZGuiWidgetCreator::createSignalMapper(QWidget* parent)
{
    QSignalMapper * mapper = Q_NULLPTR ;

    try
    {
        mapper = new QSignalMapper(parent) ;
    }
    catch (...)
    {
        mapper = Q_NULLPTR ;
    }

    return mapper ;
}







} // namespace GUI

} // namespace ZMap

