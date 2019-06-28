#ifndef BROWSERWINDOW_H
#define BROWSERWINDOW_H

#include <QApplication>
#include <QMainWindow>
#include <indigo_bus.h>

class QPlainTextEdit;
class QTreeView;
class ServiceModel;
class PropertyModel;
class QItemSelection;
class QVBoxLayout;
class QScrollArea;
class QIndigoServers;
struct PropertyNode;
struct TreeNode;



class BrowserWindow : public QMainWindow {
	Q_OBJECT
public:
	explicit BrowserWindow(QWidget *parent = nullptr);

signals:
	void enable_blobs(bool on);

public slots:
	void on_selection_changed(const QItemSelection &selected, const QItemSelection &deselected);
	void on_property_log(indigo_property* property, const char *message);
	void on_property_define_delete(indigo_property* property, const char *message);
	void on_blobs_changed(bool status);
	void on_bonjour_changed(bool status);
	void on_use_suffix_changed(bool status);
	void on_log_error();
	void on_log_info();
	void on_log_debug();
	void on_log_trace();
	void on_servers_act();
	void on_exit_act();
	void on_about_act();

private:
	QPlainTextEdit* mLog;
	QTreeView* mProperties;
	QScrollArea* mScrollArea;
	QWidget* form_panel;
	QVBoxLayout* form_layout;

	QIndigoServers *mIndigoServers;
	ServiceModel* mServiceModel;
	PropertyModel* mPropertyModel;
	PropertyNode* current_node;
	void clear_window();
};

#endif // BROWSERWINDOW_H
