/*
   Copyright (C) 2005 korewaisai
   korewaisai@yahoo.co.jp

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.
*/
#include <QtGui>

#include "Crontab.h"
#include "CronModel.h"

void dumpIndex(const QModelIndex &idx, QString h);

QVariant CronModel::data(const QModelIndex &idx, int role) const
{
    if (!idx.isValid() || role != Qt::DisplayRole)
        return QVariant();

    if (idx.parent().isValid() || isOneUser()) {
        TCommand *cmnd = getTCommand(idx);
        switch (idx.column()) {
        case 0:
            return cmnd->time;
        case 1:
            return cmnd->user;
        case 2:
            return cmnd->command;
        }
    } else {
        if (idx.column() == 0)
            return  getCrontab(idx)->cronOwner;
    }

    return QVariant();

}

QModelIndex CronModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    if (crontabs->count() > 1) {
        CronType *t = static_cast<CronType*>(index.internalPointer());
        if (t->type == CronType::COMMAND) {
            TCommand *cmnd = static_cast<TCommand*>(t);
            Crontab *cron = cmnd->parent;
            return createIndex(crontabs->indexOf(cron), 0, cron);
        }
    }

    return QModelIndex();
}

QModelIndex CronModel::index(int row, int column, const QModelIndex &parent) const
{

    if (!parent.isValid()) {
        if (isOneUser()) {
            if (row < (*crontabs)[0]->tCommands.count())
                return createIndex(row, column, (*crontabs)[0]->tCommands[row]);
        } else {
            if (row >= 0 && row < crontabs->count())
                return createIndex(row, column, (*crontabs)[row]);
        }
    } else {
        if (!isOneUser()) {
            Crontab *cron = getCrontab(parent);
            if (row < cron->tCommands.count())
                return createIndex(row, column, cron->tCommands[row]);
        }
    }

    return QModelIndex();
}

Qt::ItemFlags CronModel::flags(const QModelIndex &idx) const
{
    if (!idx.isValid())
        return nullptr;

    if (isOneUser() || idx.parent().isValid())
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled |
                Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
    else
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDropEnabled;
}


int CronModel::rowCount(const QModelIndex &parent) const
{

    if (parent.isValid()) {
        if (!parent.parent().isValid()) {
            if (!isOneUser())
                return getCrontab(parent)->tCommands.count();
        }
    } else {
        if (isOneUser())
            return (*crontabs)[0]->tCommands.count();
        else
            return crontabs->count();
    }

    return 0;
}

QVariant CronModel::headerData(int section, Qt::Orientation orientation,
                               int role) const
{

    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
        case 0:
            return tr("Time");
        case 1:
            return tr("User");
        case 2:
            return tr("command");
        }
    }

    return QVariant();
}

QModelIndex CronModel::removeCComand(const QModelIndex &idx)
{

    if (!idx.isValid())
        return QModelIndex();

    int cronPos;
    int cmndPos;
    QModelIndex del;
    if (idx.parent().isValid()) {
        cronPos = idx.parent().row();
        cmndPos = idx.row();
        del = idx.parent();
    } else {
        cronPos = 0;
        cmndPos = idx.row();
        del = QModelIndex();
    }

    beginRemoveRows(del, cmndPos, cmndPos);

    delete crontabs->at(cronPos)->tCommands[cmndPos];
    crontabs->at(cronPos)->tCommands.removeAt(cmndPos);

    endRemoveRows();

    if (rowCount(del) > 0)
        return index(0, 0, del);
    else if (!isOneUser())
        return del;

    return QModelIndex();
}

QModelIndex CronModel::insertTCommand(const QModelIndex &idx, TCommand *cmnd)
{

    // insert cron command at insert position(idx)
    // if insert position is crontab, cron command be added at the end.
    //	dumpIndex(idx, "CronModel::insertTCommand");
    //	qDebug() << "CronModel::insertTCommand time=" << cmnd->time;

    int cronPos;
    int cmndPos;
    QModelIndex ins;
    if (idx.isValid()) {
        if (idx.parent().isValid()) {
            cronPos = idx.parent().row();
            cmndPos = idx.row() + 1;
            ins = idx.parent();
        } else {
            if (isOneUser()) {
                cronPos = 0;
                cmndPos = idx.row() + 1;
                ins = QModelIndex();
            } else {
                cronPos = idx.row();
                cmndPos = 0;
                ins = idx;
            }
        }
    } else {
        cronPos = 0;
        cmndPos = 0;
        ins = QModelIndex();
    }

    beginInsertRows(ins, cmndPos, cmndPos);

    crontabs->at(cronPos)->tCommands.insert(cmndPos,cmnd);

    endInsertRows();

    return index(cmndPos, 0, ins);
}

void CronModel::tCommandChanged(const QModelIndex &idx)
{

    QModelIndex from = index(idx.row(), 0, idx.parent());
    QModelIndex to = index(idx.row(), 3, idx.parent());
    emit dataChanged(from, to);

}

TCommand *CronModel::getTCommand(const QModelIndex &idx) const
{
    if (!idx.isValid())
        return nullptr;

    if (isOneUser())
        return static_cast<TCommand*>(idx.internalPointer());

    if (idx.parent().isValid())
        return static_cast<TCommand*>(idx.internalPointer());

    return nullptr;
}

Crontab *CronModel::getCrontab(const QModelIndex &idx) const
{
    if (!idx.isValid() && crontabs->count() == 0)
        return nullptr;

    if (isOneUser())
        return (*crontabs)[0];

    if (idx.parent().isValid())
        return static_cast<Crontab*>(idx.parent().internalPointer());

    return static_cast<Crontab*>(idx.internalPointer());
}

QModelIndex CronModel::searchTCommand(TCommand *cmnd) const
{

    if (isOneUser()) {
        for(int i=0; i<rowCount(QModelIndex()); i++) {
            QModelIndex idx = index(i, 0, QModelIndex());
            if (reinterpret_cast<uintptr_t>(getTCommand(idx)) == reinterpret_cast<uintptr_t>(cmnd))
                return idx;
        }
    } else {
        for (int i=0; i<rowCount(QModelIndex()); i++){
            QModelIndex pidx = index(i, 0, QModelIndex());
            for (int j=0; j<rowCount(pidx); j++) {
                QModelIndex idx = index(j, 0, pidx);
                if (reinterpret_cast<uintptr_t>(getTCommand(idx)) == reinterpret_cast<uintptr_t>(cmnd))
                    return idx;
            }
        }
    }
    return QModelIndex();
}

bool CronModel::dropMimeData ( const QMimeData*, Qt::DropAction,
                               int row, int , const QModelIndex &parent )
{
    //				   (0, --)
    //	   ----------
    //	   |        |  (-1, 0)
    //	   ----------
    //	               (1, --)
    //
    // ========================================
    //
    //				   (0, --)
    //	   ----------
    //	   |        |  (-1, 0)
    //	   ----------
    //	               (1, --)
    //	       ----------
    //	       |        |  (-1, 0, 0)
    //	       ----------
    //	               (1, 0)
    //	       ----------
    //	       |        |  (-1, 1, 0)
    //	       ----------
    //	               (1, --)
    //

    //	qDebug() << "CronModel::dropMimeData:row=" << row;
    //	dumpIndex(parent, "CronModel::dropMimeData");
    if (isOneUser()) {
        if (row < 0 && !parent.isValid())
            return false;
    } else {
        if (!parent.isValid())
            return false;
    }

    QModelIndex ins;
    QModelIndex next;
    if (row <= 0) {
        ins = parent;
        if (ins.isValid() &&
                (isOneUser() || ins.parent().isValid()))
            next = index(ins.row()+1, 0, ins.parent());
        else
            next = index(0, 0, ins);
    } else {
        ins = index(row-1, 0, parent);
        next = index(row, 0, parent);
    }
    //	dumpIndex(ins, "CronModel::dropMimeData insert : ");

    if (reinterpret_cast<uintptr_t>(getTCommand(ins)) == reinterpret_cast<uintptr_t>(drag) ||
            reinterpret_cast<uintptr_t>(getTCommand(next)) == reinterpret_cast<uintptr_t>(drag))
        return false;

    TCommand *t = new TCommand();
    *t = *drag;
    Crontab *c = getCrontab(ins);
    if (c->cronOwner != "/etc/crontab")
        t->user = c->cronOwner;
    t->parent = c;

    insertTCommand(ins, t);

    QModelIndex del = searchTCommand(drag);
    removeCComand(del);

    drag = nullptr;

    emit moveTCommand(t);


    return false;
}

void CronModel::dragTCommand(const QModelIndex &idx)
{
    drag = getTCommand(idx);
}
