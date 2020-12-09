/*
   Copyright (C) 2005 korewaisai
   korewaisai@yahoo.co.jp

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.
*/
#include <QButtonGroup>
#include <QComboBox>
#include <QGroupBox>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QTextEdit>
#include <QtGui>

#include "Crontab.h"
#include "VariableModel.h"
#include "VariableEdit.h"
#include "VariableView.h"
#include "Clib.h"

VariableEdit::VariableEdit(QWidget *parent)
    : QWidget(parent)
{

    QFrame *sepFrame;
    QPushButton *newButton;
    QPushButton *deleteButton;
    QGroupBox *varGroup;
    QGridLayout *g;
    QHBoxLayout *h, *h2;
    QVBoxLayout *v;

    variableModel = new VariableModel();

    QVBoxLayout *mainLayout = new QVBoxLayout;
    {
        mainLayout->addWidget((commentEdit = new QTextEdit()));

        mainLayout->addWidget((varGroup = new QGroupBox(tr("Variables"))));
        varGroup->setLayout((g = new QGridLayout));
        {
            g->addLayout((h = new QHBoxLayout), 0, 0, 1, 2);
            {
                h->addWidget(new QLabel(tr("Mail:")));
                h->addWidget((mailOffRadio = new QRadioButton(tr("Don't send"))));
                h->addWidget((mailOnRadio = new QRadioButton(tr("Send"))));
                h->addWidget(mailToRadio = new QRadioButton(tr("To")));
                h->addWidget((userCombo = new QComboBox()));
            }
            g->addWidget((sepFrame = new QFrame()), 1, 0, 1, 2);
            g->addLayout((v = new QVBoxLayout), 2, 0, 1, 1);
            {
                v->addWidget(variableView = new VariableView(variableModel));
                v->addLayout((h2 = new QHBoxLayout));
                {
                    h2->addStretch();
                    h2->addWidget((newButton =
                            new QPushButton(QIcon(":/images/filenew.png"),
                                            tr("New"))));
                    h2->addWidget((deleteButton =
                            new QPushButton(QIcon(":/images/editdelete.png"),
                                            tr("Delete"))));
                }
            }

            g->addLayout((v = new QVBoxLayout), 2, 1, 1, 1);
            {
                v->addLayout((h = new QHBoxLayout));
                {
                    h->addWidget(new QLabel(tr("Name:")));
                    h->addWidget((nameEdit = new QLineEdit()));
                }
                v->addLayout((h = new QHBoxLayout));
                {
                    h->addWidget(new QLabel(tr("Value:")));
                    h->addWidget((valueEdit = new QLineEdit()));
                }
                v->addWidget(new QLabel(tr("Comment:")));
                v->addWidget((varCommentEdit = new QTextEdit()));
            }
        }
    }
    setLayout(mainLayout);

    userCombo->addItems(Clib::allUsers());

    sepFrame->setFrameShape( QFrame::HLine );
    sepFrame->setFrameShadow( QFrame::Sunken );

    connect(variableView, SIGNAL(changeVar(Variable *)),
            this, SLOT(varViewSelected(Variable *)));

    connect(commentEdit, SIGNAL(textChanged()),
            this, SLOT(commentChanged()));
    connect(nameEdit, SIGNAL(textEdited(const QString&)),
            this, SLOT(varEdited(const QString&)));
    connect(valueEdit, SIGNAL(textEdited(const QString&)),
            this, SLOT(valEdited(const QString&)));
    connect(varCommentEdit, SIGNAL(textChanged()),
            this, SLOT(varCommentChanged()));
    connect(deleteButton, SIGNAL(clicked()),
            this, SLOT(deleteClicked()));
    connect(newButton, SIGNAL(clicked()),
            this, SLOT(newClicked()));
    connect(mailOnRadio, SIGNAL(toggled(bool)),
            this, SLOT(mailOnClicked(bool)));
    connect(mailOffRadio, SIGNAL(toggled(bool)),
            this, SLOT(mailOffClicked(bool)));
    connect(mailToRadio, SIGNAL(toggled(bool)),
            this, SLOT(mailToClicked(bool)));
    connect(userCombo, SIGNAL(activated(int)),
            this, SLOT(userActivated(int)));

    viewChanging = true;
    varViewChanging = true;

}

void VariableEdit::changeCurrent(Crontab *cron, TCommand *)
{
    if (cron == nullptr) {
        setEnabled(false);
        return;
    }

    setEnabled(true);
    viewChanging = true;

    crontab = cron;
    commentEdit->setPlainText(cron->comment);
    variableModel->resetData(&cron->variables);
    variableView->resetView();
    setMailCombo(cron->variables);

    viewChanging = false;
}

void VariableEdit::varViewSelected(Variable *var)
{
    if (var == nullptr) {
        nameEdit->setEnabled(false);
        valueEdit->setEnabled(false);
        varCommentEdit->setEnabled(false);
        return;
    }
    nameEdit->setEnabled(true);
    valueEdit->setEnabled(true);
    varCommentEdit->setEnabled(true);
    variable = var;
    varViewChanging = true;
    nameEdit->setText(var->name);
    valueEdit->setText(var->value);
    nameEdit->setCursorPosition(0);
    valueEdit->setCursorPosition(0);
    varCommentEdit->setPlainText(var->comment);
    varViewChanging = false;
}

void VariableEdit::commentChanged()
{
    if (!viewChanging) {
        crontab->comment = commentEdit->toPlainText();
        emit dataChanged();
    }
}
void VariableEdit::varEdited(const QString &str)
{
    variable->name = str;
    variableView->varDataChanged();
    emit dataChanged();
}
void VariableEdit::valEdited(const QString &str)
{
    variable->value = str;
    variableView->varDataChanged();
    emit dataChanged();
}
void VariableEdit::varCommentChanged()
{
    if (!varViewChanging) {
        variable->comment = varCommentEdit->toPlainText();
        emit dataChanged();
    }
}
void VariableEdit::deleteClicked()
{
    varViewChanging = true;
    variableView->removeVariable();
    setMailCombo(*(variableModel->variables));
    emit dataChanged();
    varViewChanging = false;
}

void VariableEdit::newClicked()
{
    variableView->insertVariable();
    emit dataChanged();
}

void VariableEdit::mailOnClicked(bool state)
{
    if ( state )
        setMailVar(0);

}
void VariableEdit::mailOffClicked(bool state)
{
    if ( state )
        setMailVar(1);

}
void VariableEdit::mailToClicked(bool state)
{
    if ( state )
        setMailVar(2);

}
void VariableEdit::userActivated(int)
{
    setMailVar(-1);
}

void VariableEdit::setMailVar(int mailFlag)
{
    int curFlag = 0;
    Variable *v;
    foreach(v, crontab->variables) {
        if (v->name == "MAILTO") {
            if (v->value == "\"\"") curFlag = 1;
            else curFlag = 2;
            break;
        }
    }
    if ( mailFlag != -1 ) {
        if (curFlag == mailFlag)
            return;
    }else{
        if (curFlag != 2)
            return;
        mailFlag = 2;
    }
    if (mailFlag == 0){
        if (curFlag == 1 || curFlag == 2) {
            int i = crontab->variables.indexOf(v);
            crontab->variables.removeAt(i);
        }
    } else if (mailFlag == 1) {
        if (curFlag == 0) {
            crontab->variables << new Variable("MAILTO","\"\"","Don't send mail to anyone");
        } else if(curFlag == 2) {
            v->value = "\"\"";
            v->comment = "Don't send mail to anyone";
        }
    } else if (mailFlag == 2) {
        QString u = userCombo->currentText();
        if (curFlag == 0) {
            QString c = "Send mail to \"" + u + "\"";
            crontab->variables << new Variable("MAILTO",u,c);
        } else if(curFlag == 1) {
            v->value = u;
            v->comment = "Send mail to \""+u+"\"";
        } else if(curFlag == 2) {
            if ( v->value != u ) {
                v->value = u;
                v->comment = "Send mail to \""+u+"\"";
            }else{
                return;
            }
        }
    }
    variableView->resetView();
    emit dataChanged();

}
void VariableEdit::setMailCombo(const QList<Variable*> &var)
{
    bool mvar = false;
    foreach (Variable *v, var) {
        if (v->name == "MAILTO") {
            mvar = true;
            if (v->value == "\"\"") {
                mailOffRadio->setChecked(true);
                userCombo->setCurrentIndex(userCombo->findText(crontab->cronOwner));
            } else {
                mailToRadio->setChecked(true);
                userCombo->setCurrentIndex(userCombo->findText(v->value));
            }
            break;
        }
    }
    if (!mvar) {
        mailOnRadio->setChecked(true);
        userCombo->setCurrentIndex(userCombo->findText(crontab->cronOwner));
    }
}