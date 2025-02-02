
#ifndef DIALOG_SHELL_MENU_H_
#define DIALOG_SHELL_MENU_H_

#include "Dialog.h"
#include "Verbosity.h"
#include "ui_DialogShellMenu.h"

#include <boost/optional.hpp>

struct MenuItem;
class MenuItemModel;

class DialogShellMenu final : public Dialog {
	Q_OBJECT

public:
	DialogShellMenu(QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
	~DialogShellMenu() override = default;

Q_SIGNALS:
	void restore(const QModelIndex &selection);

private Q_SLOTS:
	void currentChanged(const QModelIndex &current, const QModelIndex &previous);

private Q_SLOTS:
	void on_radioToSameDocument_toggled(bool checked);

private:
	void buttonBox_clicked(QAbstractButton *button);
	void buttonBox_accepted();
	void buttonNew_clicked();
	void buttonCopy_clicked();
	void buttonDelete_clicked();
	void buttonUp_clicked();
	void buttonDown_clicked();
	void connectSlots();

private:
	bool applyDialogChanges();
	bool validateFields(Verbosity verbosity);
	bool updateCurrentItem();
	bool updateCurrentItem(const QModelIndex &index);
	boost::optional<MenuItem> readFields(Verbosity verbosity);
	void updateButtonStates();
	void updateButtonStates(const QModelIndex &current);

private:
	Ui::DialogShellMenu ui;
	MenuItemModel *model_;
	QModelIndex deleted_;
};

#endif

