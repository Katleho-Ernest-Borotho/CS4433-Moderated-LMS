#include "lms/ui/LoginWindow.h"

#include <QFont>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QStackedWidget>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

#include <exception>
#include <string>

namespace lms {
namespace {
QString roleText(Role role)
{
    switch (role) {
    case Role::Student: return "Student";
    case Role::Lecturer: return "Lecturer";
    case Role::HeadOfDepartment: return "Head of Department";
    case Role::Administrator: return "Administrator";
    }
    return "Unknown";
}

QLabel* title(const QString& value, QWidget* parent)
{
    auto* label = new QLabel(value, parent);
    label->setAlignment(Qt::AlignCenter);
    QFont font = label->font();
    font.setPointSize(16);
    font.setBold(true);
    label->setFont(font);
    return label;
}
} // namespace

LoginWindow::LoginWindow(IAuthenticationService& authentication, BootstrapService& bootstrap)
    : authentication_(authentication), bootstrap_(bootstrap)
{
    setWindowTitle("Moderated Learning Management System");
    setMinimumSize(600, 400);

    pages_ = new QStackedWidget(this);
    setCentralWidget(pages_);

    setupPage_ = createSetupPage();
    loginPage_ = createLoginPage();
    dashboardPage_ = createDashboardPage();
    pages_->addWidget(setupPage_);
    pages_->addWidget(loginPage_);
    pages_->addWidget(dashboardPage_);
    pages_->setCurrentWidget(bootstrap_.needsSetup() ? setupPage_ : loginPage_);
}

QWidget* LoginWindow::createSetupPage()
{
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(65, 35, 65, 35);
    layout->setSpacing(16);
    layout->addWidget(title("Initial administrator setup", page));

    auto* description = new QLabel(
        "No accounts exist yet. Create the first administrator to manage LMS users.", page);
    description->setWordWrap(true);
    description->setAlignment(Qt::AlignCenter);
    layout->addWidget(description);

    auto* form = new QFormLayout;
    setupUsername_ = new QLineEdit(page);
    setupUsername_->setPlaceholderText("3-40 letters, numbers, . _ -");
    setupPassword_ = new QLineEdit(page);
    setupPassword_->setEchoMode(QLineEdit::Password);
    setupPassword_->setPlaceholderText("At least 12 characters");
    setupConfirmation_ = new QLineEdit(page);
    setupConfirmation_->setEchoMode(QLineEdit::Password);
    form->addRow("Username:", setupUsername_);
    form->addRow("Password:", setupPassword_);
    form->addRow("Confirm password:", setupConfirmation_);
    layout->addLayout(form);

    auto* create = new QPushButton("Create administrator", page);
    connect(create, &QPushButton::clicked, this, [this] { setupAdministrator(); });
    layout->addWidget(create);
    layout->addStretch();
    return page;
}

QWidget* LoginWindow::createLoginPage()
{
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(85, 45, 85, 45);
    layout->setSpacing(18);
    layout->addWidget(title("Moderated Learning Management System", page));
    auto* introduction = new QLabel("Sign in to continue", page);
    introduction->setAlignment(Qt::AlignCenter);
    layout->addWidget(introduction);

    auto* form = new QFormLayout;
    loginUsername_ = new QLineEdit(page);
    loginUsername_->setPlaceholderText("Username");
    loginPassword_ = new QLineEdit(page);
    loginPassword_->setEchoMode(QLineEdit::Password);
    loginPassword_->setPlaceholderText("Password");
    form->addRow("Username:", loginUsername_);
    form->addRow("Password:", loginPassword_);
    layout->addLayout(form);

    auto* submit = new QPushButton("Log in", page);
    connect(submit, &QPushButton::clicked, this, [this] { attemptLogin(); });
    connect(loginPassword_, &QLineEdit::returnPressed, submit, &QPushButton::click);
    layout->addWidget(submit);
    layout->addStretch();
    return page;
}

QWidget* LoginWindow::createDashboardPage()
{
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(65, 45, 65, 45);
    layout->setSpacing(18);
    layout->addWidget(title("Moderated Learning Management System", page));
    dashboardGreeting_ = new QLabel(page);
    dashboardGreeting_->setAlignment(Qt::AlignCenter);
    layout->addWidget(dashboardGreeting_);

    auto* information = new QLabel(
        "Login verified. The role-specific academic screens are not implemented yet.", page);
    information->setAlignment(Qt::AlignCenter);
    information->setWordWrap(true);
    layout->addWidget(information);

    auto* logout = new QPushButton("Log out", page);
    connect(logout, &QPushButton::clicked, this, [this] {
        session_.reset();
        loginPassword_->clear();
        pages_->setCurrentWidget(loginPage_);
        loginUsername_->setFocus();
    });
    layout->addWidget(logout);
    layout->addStretch();
    return page;
}

void LoginWindow::setupAdministrator()
{
    if (setupPassword_->text() != setupConfirmation_->text()) {
        QMessageBox::warning(this, "Account setup", "Passwords do not match.");
        return;
    }
    try {
        const QString name = setupUsername_->text().trimmed();
        bootstrap_.createInitialAdministrator(name.toStdString(),
                                              setupPassword_->text().toStdString());
        setupPassword_->clear();
        setupConfirmation_->clear();
        loginUsername_->setText(name);
        pages_->setCurrentWidget(loginPage_);
        loginPassword_->setFocus();
        QMessageBox::information(this, "Administrator created",
                                 "Your administrator account is ready. Please log in.");
    } catch (const std::exception& error) {
        QMessageBox::warning(this, "Account setup", error.what());
    }
}

void LoginWindow::attemptLogin()
{
    try {
        session_ = authentication_.logIn(loginUsername_->text().trimmed().toStdString(),
                                         loginPassword_->text().toStdString());
        loginPassword_->clear();
        showDashboard();
    } catch (const AuthenticationError&) {
        loginPassword_->clear();
        QMessageBox::warning(this, "Login failed", "Invalid username or password.");
    } catch (const std::exception&) {
        loginPassword_->clear();
        QMessageBox::critical(this, "Login error", "Could not access user records.");
    }
}

void LoginWindow::showDashboard()
{
    if (!session_) return;
    dashboardGreeting_->setText(
        "Signed in as " + QString::fromStdString(session_->username())
        + " (" + roleText(session_->role()) + ")");
    pages_->setCurrentWidget(dashboardPage_);
}

} // namespace lms
