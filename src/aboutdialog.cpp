#include "aboutdialog.h"
#include "ui_aboutdialog.h"

#include "version.h"

namespace Ripes {

AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent), m_ui(new Ui::AboutDialog) {

  const QString name = "Ripes";
  const QString url = "https://github.com/silvigon/Jumping-Ripes";

  QString info = QString();
  info.append("<p><b>" + name + "</b><br/>" + getRipesVersion() + "</p>");
  info.append("<p><a href=\"" + url + "\">" + url + "</a></p>");

  m_ui->setupUi(this);
  m_ui->infoLabel->setText(info);
  m_ui->infoLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
  m_ui->infoLabel->setOpenExternalLinks(true);

  setWindowTitle("About");
}

AboutDialog::~AboutDialog() { delete m_ui; }

} // namespace Ripes
