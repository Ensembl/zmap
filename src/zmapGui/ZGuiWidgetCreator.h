/*  File: ZGuiWidgetCreator.h
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

#ifndef ZGUIWIDGETCREATOR_H
#define ZGUIWIDGETCREATOR_H

#include <QString>
#include <QColor>

QT_BEGIN_NAMESPACE
class QWidget ;
class QPushButton ;
class QToolButton ;
class QLabel ;
class QLineEdit ;
class QGroupBox ;
class QGridLayout ;
class QHBoxLayout ;
class QVBoxLayout ;
class QCheckBox ;
class QAction ;
class QMenu ;
class QTableView ;
class QTreeView ;
class QListView ;
class QStandardItem ;
class QStandardItemModel ;
class QFrame ;
class QTabWidget ;
class QRadioButton ;
class QButtonGroup ;
class QTextEdit ;
class QComboBox ;
class QSpinBox ;
class QErrorMessage ;
class QItemSelectionModel ;
class QSlider ;
class QSignalMapper ;
QT_END_NAMESPACE

namespace ZMap
{

namespace GUI
{

//
// This just wraps widget creation with some exception trapping.
//

class ZGuiWidgetCreator
{
public:

    //
    // These are the basic QT types, i.e. stuff that I've not subclassed
    //
    static QWidget* createWidget(QWidget* parent = Q_NULLPTR) ;
    static QPushButton * createPushButton(const QString& data = QString()) ;
    static QToolButton * createToolButton() ;
    static QLabel * createLabel(const QString& data = QString()) ;
    static QLineEdit * createLineEdit() ;
    static QGroupBox * createGroupBox(const QString& data = QString()) ;
    static QGridLayout * createGridLayout() ;
    static QHBoxLayout* createHBoxLayout() ;
    static QVBoxLayout* createVBoxLayout() ;
    static QCheckBox* createCheckBox(const QString& data = QString()) ;
    static QMenu * createMenu(QWidget * parent = Q_NULLPTR ) ;
    static QAction* createAction(const QString& data, QWidget* parent = Q_NULLPTR ) ;
    static QTableView* createTableView() ;
    static QTreeView* createTreeView() ;
    static QListView* createListView() ;
    static QStandardItem* createStandardItem(const QString& data = QString()) ;
    static QStandardItemModel *createStandardItemModel() ;
    static QFrame *createFrame() ;
    static QTabWidget *createTabWidget() ;
    static QRadioButton *createRadioButton() ;
    static QButtonGroup * createButtonGroup() ;
    static QTextEdit *createTextEdit() ;
    static QComboBox *createComboBox() ;
    static QSpinBox *createSpinBox() ;
    static QErrorMessage *createErrorMessage(QWidget* parent) ;
    static QItemSelectionModel * createItemSelectionModel() ;
    static QSlider * createSlider() ;
    static QSignalMapper * createSignalMapper(QWidget* parent = Q_NULLPTR ) ;

private:
    ZGuiWidgetCreator() = delete ;
    ZGuiWidgetCreator(const ZGuiWidgetCreator&) = delete ;
    ZGuiWidgetCreator& operator=(const ZGuiWidgetCreator&) = delete ;
};

} // namespace GUI

} // namespace ZMap



#endif // ZGUIWIDGETCREATOR_H
