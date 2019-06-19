#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSplitter>
#include <QTreeView>
#include <QMenuBar>
#include <QPlainTextEdit>
#include <QScrollArea>
#include <sys/time.h>
#include "browserwindow.h"
#include "servicemodel.h"
#include "propertymodel.h"
#include "indigoclient.h"
#include "qindigoproperty.h"
#include "logger.h"
#include "conf.h"


BrowserWindow::BrowserWindow(QWidget *parent) : QMainWindow(parent) {
	setWindowTitle(tr("INDIGO Control Panel"));
	resize(1024, 768);

	//  Set central widget of window
	QWidget *central = new QWidget;
	setCentralWidget(central);


	//  Set the root layout to be a VBox
	QVBoxLayout *rootLayout = new QVBoxLayout;
	rootLayout->setSpacing(0);
	rootLayout->setContentsMargins(0, 0, 0, 0);
	rootLayout->setSizeConstraint(QLayout::SetMinimumSize);
	central->setLayout(rootLayout);

	// Create menubar
	QMenuBar *menu = new QMenuBar;
	QMenu *file = new QMenu("&File");
	QAction *servers_act = new QAction(tr("&Servers"), this);
	file->addAction(servers_act);
	QAction *exit_act = new QAction(tr("&Exit"), this);
	file->addAction(exit_act);
	menu->addMenu(file);

	QMenu *settings = new QMenu("&Settings");
	QAction *blobs_act = new QAction(tr("&Ebable BLOBs"), this);
	blobs_act->setCheckable(true);
	blobs_act->setChecked(conf.blobs_enabled);
	settings->addAction(blobs_act);
	menu->addMenu(settings);

	QAction *bonjour_act = new QAction(tr("&Enable auto connect"), this);
	bonjour_act->setCheckable(true);
	bonjour_act->setChecked(conf.auto_connect);
	settings->addAction(bonjour_act);
	menu->addMenu(settings);

	rootLayout->addWidget(menu);

	connect(exit_act, &QAction::triggered, this, &BrowserWindow::on_exit_act);
	connect(blobs_act, &QAction::toggled, this, &BrowserWindow::on_blobs_changed);
	connect(bonjour_act, &QAction::toggled, this, &BrowserWindow::on_bonjour_changed);

	// Create properties viewing area
	QWidget *view = new QWidget;
	QVBoxLayout *propertyLayout = new QVBoxLayout;
	propertyLayout->setSpacing(5);
	propertyLayout->setContentsMargins(5, 5, 5, 5);
	propertyLayout->setSizeConstraint(QLayout::SetMinimumSize);
	view->setLayout(propertyLayout);
	rootLayout->addWidget(view);

	mProperties = new QTreeView;
	form_panel = new QWidget();
	form_layout = new QVBoxLayout();
	form_layout->setMargin(0);
	form_panel->setLayout(form_layout);

	mScrollArea = new QScrollArea();
	mScrollArea->setWidgetResizable(true);
	mScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	mScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	form_layout->addWidget(mScrollArea);
	mScrollArea->setMinimumWidth(600);


	QSplitter* hSplitter = new QSplitter;
	hSplitter->addWidget(mProperties);
	hSplitter->addWidget(form_panel);
	hSplitter->setStretchFactor(0, 45);
	hSplitter->setStretchFactor(2, 55);
	propertyLayout->addWidget(hSplitter, 85);


	//  Create log viewer
	mLog = new QPlainTextEdit;
	mLog->setReadOnly(true);
	propertyLayout->addWidget(mLog, 15);

	mServiceModel = new ServiceModel("_indigo._tcp");

	mPropertyModel = new PropertyModel();
	mProperties->setHeaderHidden(true);
	mProperties->setModel(mPropertyModel);

	connect(&IndigoClient::instance(), &IndigoClient::property_defined, mPropertyModel, &PropertyModel::define_property);
	connect(&IndigoClient::instance(), &IndigoClient::property_changed, mPropertyModel, &PropertyModel::update_property);
	connect(&IndigoClient::instance(), &IndigoClient::property_deleted, mPropertyModel, &PropertyModel::delete_property);

	connect(&IndigoClient::instance(), &IndigoClient::property_defined, this, &BrowserWindow::on_property_log);
	connect(&IndigoClient::instance(), &IndigoClient::property_changed, this, &BrowserWindow::on_property_log);
	connect(&IndigoClient::instance(), &IndigoClient::property_deleted, this, &BrowserWindow::on_property_log);

	connect(&Logger::instance(), &Logger::log_in_window, this, &BrowserWindow::on_property_log);

	connect(mProperties->selectionModel(), &QItemSelectionModel::selectionChanged, this, &BrowserWindow::on_selection_changed);

	current_node = nullptr;

	//  Start up the client
	IndigoClient::instance().start();
}


void BrowserWindow::on_property_log(indigo_property* property, const char *message) {
	char timestamp[16];
	char log_line[512];
	struct timeval tmnow;

	if (!message) return;

	//printf("CCCCC-> %s\n", message);

	gettimeofday(&tmnow, NULL);
	strftime(timestamp, sizeof(log_line), "%H:%M:%S", localtime((const time_t *) &tmnow.tv_sec));
	snprintf(timestamp + 8, sizeof(timestamp) - 8, ".%03ld", tmnow.tv_usec/1000);
	if (property)
		snprintf(log_line, 512, "%s '%s'.%s - %s", timestamp, property->device, property->name, message);
	else
		snprintf(log_line, 512, "%s %s", timestamp, message);

	mLog->appendPlainText(log_line); // Adds the message to the widget
}

void BrowserWindow::clear_window() {
	QWidget* ppanel = new QWidget();
	QVBoxLayout* playout = new QVBoxLayout;
	playout->setSizeConstraint(QLayout::SetMinimumSize);
	ppanel->setLayout(playout);
	mScrollArea->setWidget(ppanel);
	mScrollArea->setWidgetResizable(true);
	ppanel->show();
}

void BrowserWindow::on_selection_changed(const QItemSelection &selected, const QItemSelection &) {
	fprintf(stderr, "SELECTION CHANGED\n");

	//  Deal with the outgoing selection
	if (current_node != nullptr) {
		fprintf(stderr, "SELECTION CHANGED no current node\n");
		clear_window();
	}

	if (selected.indexes().empty()) {
		fprintf(stderr, "SELECTION CHANGED selected.indexes().empty()\n");
		clear_window();
		current_node = nullptr;
		return;
	}

	QModelIndex s = selected.indexes().front();
	TreeNode* n = static_cast<TreeNode*>(s.internalPointer());
	if (n != nullptr) {
		fprintf(stderr, "SELECTION CHANGED n->node_type == %d\n", n->node_type);
		clear_window();
	}

	if (n != nullptr && n->node_type == TREE_NODE_PROPERTY) {
		fprintf(stderr, "SELECTION CHANGED n->node_type == TREE_NODE_PROPERTY\n");

		PropertyNode* p = reinterpret_cast<PropertyNode*>(n);
		QIndigoProperty* ip = new QIndigoProperty(p->property);
		current_node = p;

		QWidget* ppanel = new QWidget();
		QVBoxLayout* playout = new QVBoxLayout;
		playout->setSpacing(10);
		playout->setContentsMargins(10, 10, 10, 10);
		playout->setSizeConstraint(QLayout::SetMinimumSize);
		ppanel->setLayout(playout);
		playout->addWidget(ip);
		mScrollArea->setWidget(ppanel);
		ppanel->show();

		//  Connect to update signals coming from indigo bus
		connect(mPropertyModel, &PropertyModel::property_updated, ip, &QIndigoProperty::property_update);
	} else if (n != nullptr && n->node_type == TREE_NODE_GROUP) {
		fprintf(stderr, "SELECTION CHANGED n->node_type == TREE_NODE_GROUP\n");
		GroupNode* g = reinterpret_cast<GroupNode*>(n);
		QWidget* ppanel = new QWidget();
		QVBoxLayout* playout = new QVBoxLayout;
		playout->setSpacing(10);
		playout->setContentsMargins(10, 10, 10, 10);
		playout->setSizeConstraint(QLayout::SetMinimumSize);
		ppanel->setLayout(playout);

        	//  Iterate properties
		for (int i = 0; i < g->children.count; i++) {
			PropertyNode* p = g->children.nodes[i];
			QIndigoProperty* ip = new QIndigoProperty(p->property);
			playout->addWidget(ip);
			connect(mPropertyModel, &PropertyModel::property_updated, ip, &QIndigoProperty::property_update);

		}
		playout->addStretch(); // Fill the vertical space available

		mScrollArea->setWidget(ppanel);
		mScrollArea->setWidgetResizable(true);
		ppanel->show();
	} else if (n != nullptr && n->node_type == TREE_NODE_DEVICE) {
		fprintf(stderr, "SELECTION CHANGED n->node_type == TREE_NODE_DEVICE\n");
		clear_window();

		/*
		GroupNode* d = reinterpret_cast<GroupNode*>(n);
                QWidget* ppanel = new QWidget();
                QVBoxLayout* playout = new QVBoxLayout;
                playout->setSpacing(10);
                playout->setContentsMargins(10, 10, 25, 10);
                playout->setSizeConstraint(QLayout::SetMinimumSize);
                ppanel->setLayout(playout);

                //  Iterate groups
                for (int i = 0; i < d->children.count; i++) {
			PropertyNode* p = d->children.nodes[i];
                        QIndigoProperty* ip = new QIndigoProperty(p->property);
                        fprintf(stderr, "POPER\n");
                        playout->addWidget(ip);
                        connect(mPropertyModel, &PropertyModel::property_updated, ip, &QIndigoProperty::property_update);
                }
                playout->addStretch(); // Fill the vertical space available
                mScrollArea->setWidget(ppanel);
                mScrollArea->setWidgetResizable(true);
                ppanel->show();
		*/
	}
}

void BrowserWindow::on_exit_act() {
	QApplication::quit();
}

void BrowserWindow::on_blobs_changed(bool status) {
	conf.blobs_enabled = status;
	printf ("%s\n", __FUNCTION__);
}

void BrowserWindow::on_bonjour_changed(bool status) {
	conf.auto_connect = status;
	printf ("%s\n", __FUNCTION__);
}
