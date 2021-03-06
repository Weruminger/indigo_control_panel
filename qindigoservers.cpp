// Copyright (c) 2019 Rumen G.Bogdanovski & David Hulse
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHORS 'AS IS' AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


#include "qindigoservers.h"

QIndigoServers::QIndigoServers(QWidget *parent): QDialog(parent)
{
	setWindowTitle("Available Services");

	m_server_list = new QListWidget;
	m_view_box = new QWidget();
	m_button_box = new QDialogButtonBox;
	m_service_line = new QLineEdit;
	m_service_line->setMinimumWidth(300);
	m_service_line->setToolTip(
		"service formats:\n"
		"        service@hostname:port\n"
		"        hostname:port\n"
		"        hostname\n"
		"\nservice can be any user defined name,\n"
		"if ommited hostname will be used."
	);
	//m_add_button = m_button_box->addButton(tr("Add service"), QDialogButtonBox::ActionRole);
	m_add_button = new QPushButton(" &Add ");
	m_add_button->setDefault(true);
	m_remove_button = m_button_box->addButton(tr("Remove selected"), QDialogButtonBox::ActionRole);
	m_remove_button->setToolTip(
		"Remove highlighted service.\n"
		"Only manually added services can be removed."
	);
	m_close_button = m_button_box->addButton(tr("Close"), QDialogButtonBox::ActionRole);

	QVBoxLayout* viewLayout = new QVBoxLayout;
	viewLayout->setContentsMargins(0, 0, 0, 0);
	viewLayout->addWidget(m_server_list);

	m_add_service_box = new QWidget();
	QHBoxLayout* addLayout = new QHBoxLayout;
	addLayout->setContentsMargins(0, 0, 0, 0);
	addLayout->addWidget(m_service_line);
	addLayout->addWidget(m_add_button);
	m_add_service_box->setLayout(addLayout);

	viewLayout->addWidget(m_add_service_box);
	m_view_box->setLayout(viewLayout);

	QHBoxLayout* horizontalLayout = new QHBoxLayout;
	horizontalLayout->addWidget(m_button_box);

	QVBoxLayout* mainLayout = new QVBoxLayout;
	mainLayout->addWidget(m_view_box);
	mainLayout->addLayout(horizontalLayout);

	setLayout(mainLayout);

	QObject::connect(m_server_list, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(highlightChecked(QListWidgetItem*)));
	QObject::connect(m_add_button, SIGNAL(clicked()), this, SLOT(onAddManualService()));
	QObject::connect(m_remove_button, SIGNAL(clicked()), this, SLOT(onRemoveManualService()));
	QObject::connect(m_close_button, SIGNAL(clicked()), this, SLOT(close()));
}


void QIndigoServers::onConnectionChange(QIndigoService &indigo_service) {
	QString service_name = indigo_service.name();
	indigo_debug("Connection State Change [%s] connected = %d\n", service_name.toUtf8().constData(), indigo_service.connected());
	QListWidgetItem* item = 0;
	for(int i = 0; i < m_server_list->count(); ++i){
		item = m_server_list->item(i);
		QString service = getServiceName(item);
		if (service == service_name) {
			if (indigo_service.connected())
				item->setCheckState(Qt::Checked);
			else
				item->setCheckState(Qt::Unchecked);
			break;
		}
	}
}


void QIndigoServers::onAddService(QIndigoService &indigo_service) {
	QListWidgetItem* item = new QListWidgetItem(
		indigo_service.name() +
		tr(" @ ") +
		indigo_service.host() +
		tr(":") +
		QString::number(indigo_service.port())
	);

	item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
	if (indigo_service.connected())
		item->setCheckState(Qt::Checked);
	else
		item->setCheckState(Qt::Unchecked);

	if (indigo_service.isQZeroConfService) {
		//item->setForeground(QBrush(QColor("#99FF00")));
		item->setData(Qt::DecorationRole,QIcon(":resource/bonjour_service.png"));
	} else {
		//item->setForeground(QBrush(QColor("#FFFFFF")));
		item->setData(Qt::DecorationRole,QIcon(":resource/manual_service.png"));
	}
	m_server_list->addItem(item);
}


void QIndigoServers::onAddManualService() {
	int port = 7624;
	QString hostname;
	QString service;
	QString service_str = m_service_line->text().trimmed();
	if (service_str.isEmpty()) {
		indigo_debug("Trying to add empty service!");
		return;
	}
	QStringList parts = service_str.split(':', QString::SkipEmptyParts);
	if (parts.size() > 2) {
		indigo_error("%s(): Service format error.\n",__FUNCTION__);
		return;
	} else if (parts.size() == 2) {
		port = atoi(parts.at(1).toUtf8().constData());
	}
	QStringList parts2 = parts.at(0).split('@', QString::SkipEmptyParts);
	if (parts2.size() > 2) {
		indigo_error("%s(): Service format error.\n",__FUNCTION__);
		return;
	} else if (parts2.size() == 2) {
		service = parts2.at(0);
		hostname = parts2.at(1);
	} else {
		hostname = parts2.at(0);
		service = parts2.at(0);
		int index = service.indexOf(QChar('.'));
		if (index > 0) {
			service.truncate(index);
		}
	}

	QIndigoService indigo_service(service.toUtf8(), hostname.toUtf8(), port);
	emit(requestAddManualService(indigo_service));
	m_service_line->setText("");
	indigo_debug("ADD: Service '%s' host '%s' port = %d\n", service.toUtf8().constData(), hostname.toUtf8().constData(), port);
}


void QIndigoServers::onRemoveService(QIndigoService &indigo_service) {
	QString service_name = indigo_service.name();
	QListWidgetItem* item = 0;
	for(int i = 0; i < m_server_list->count(); ++i){
		item = m_server_list->item(i);
		QString service = getServiceName(item);
		if (service == service_name) {
			delete item;
			break;
		}
	}
}


void QIndigoServers::highlightChecked(QListWidgetItem *item){
	QString service = getServiceName(item);
	if(item->checkState() == Qt::Checked)
		emit(requestConnect(service));
	else
		emit(requestDisconnect(service));
}


void QIndigoServers::onRemoveManualService() {
	QModelIndex index = m_server_list->currentIndex();
	QString service = index.data(Qt::DisplayRole).toString();
	int pos = service.indexOf('@');
	if (pos > 0) service.truncate(pos);
	service = service.trimmed();
	indigo_debug("TO BE REMOVED: [%s]\n", service.toUtf8().constData());
	emit(requestRemoveManualService(service));
}


QString QIndigoServers::getServiceName(QListWidgetItem* item) {
	QString service = item->text();
	int pos = service.indexOf('@');
	if (pos > 0) service.truncate(pos);
	service = service.trimmed();
	return service;
}
