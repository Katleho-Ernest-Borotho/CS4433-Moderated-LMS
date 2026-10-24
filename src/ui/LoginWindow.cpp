#include "lms/ui/LoginWindow.h"
#include "lms/ui/AcademicPanel.h"
#include "lms/ui/AssignmentPanel.h"
#include "lms/ui/MarkingPanel.h"
#include "lms/ui/ResultsPanel.h"

#include <QAbstractItemView>
#include <QComboBox>
#include <QFont>
#include <QFormLayout>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QStackedWidget>
#include <QString>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QWidget>

#include <cstddef>
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

LoginWindow::LoginWindow(IAuthenticationService& authentication,
                         BootstrapService& bootstrap,
                         IAcademicManagementService& accounts,
                         ISubmissionService& submissions,
                         IMarkingService& marking, IModerationService& moderation)
    : authentication_(authentication), bootstrap_(bootstrap), accounts_(accounts), submissions_(submissions), marking_(marking), moderation_(moderation)
{
    setWindowTitle("Moderated Learning Management System");
    setMinimumSize(700, 520);
    resize(930, 680);

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

QWidget* LoginWindow::createAdminPanel()
{
    auto* panel = new QWidget;
    auto* layout = new QVBoxLayout(panel);
    layout->setSpacing(12);
    layout->addWidget(title("User management", panel));

    auto* form = new QFormLayout;
    newUsername_ = new QLineEdit(panel);
    newUsername_->setPlaceholderText("New username (3-40 characters)");
    newPassword_ = new QLineEdit(panel);
    newPassword_->setEchoMode(QLineEdit::Password);
    newPassword_->setPlaceholderText("Initial password, at least 12 characters");
    newPasswordConfirmation_ = new QLineEdit(panel);
    newPasswordConfirmation_->setEchoMode(QLineEdit::Password);
    newPasswordConfirmation_->setPlaceholderText("Confirm initial password");
    newRole_ = new QComboBox(panel);
    newRole_->addItem("Student", static_cast<int>(Role::Student));
    newRole_->addItem("Lecturer", static_cast<int>(Role::Lecturer));
    newRole_->addItem("Head of Department", static_cast<int>(Role::HeadOfDepartment));
    newRole_->addItem("Administrator", static_cast<int>(Role::Administrator));
    form->addRow("Username:", newUsername_);
    form->addRow("Initial password:", newPassword_);
    form->addRow("Confirm password:", newPasswordConfirmation_);
    form->addRow("Role:", newRole_);
    layout->addLayout(form);

    auto* create = new QPushButton("Create user", panel);
    connect(create, &QPushButton::clicked, this, [this] { createAccount(); });
    layout->addWidget(create);

    accountTable_ = new QTableWidget(panel);
    accountTable_->setColumnCount(4);
    accountTable_->setHorizontalHeaderLabels({"ID", "Username", "Role", "Status"});
    accountTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    accountTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    accountTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    accountTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    accountTable_->verticalHeader()->setVisible(false);
    layout->addWidget(accountTable_);

    auto* actions = new QHBoxLayout;
    auto* refresh = new QPushButton("Refresh users", panel);
    auto* deactivate = new QPushButton("Deactivate selected user", panel);
    connect(refresh, &QPushButton::clicked, this, [this] { refreshAccounts(); });
    connect(deactivate, &QPushButton::clicked, this, [this] { deactivateAccount(); });
    actions->addWidget(refresh);
    actions->addWidget(deactivate);
    layout->addLayout(actions);
    return panel;
}

QWidget* LoginWindow::createDashboardPage()
{
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(35, 25, 35, 25);
    layout->setSpacing(10);
    layout->addWidget(title("Moderated Learning Management System", page));

    dashboardGreeting_ = new QLabel(page);
    dashboardGreeting_->setAlignment(Qt::AlignCenter);
    layout->addWidget(dashboardGreeting_);

    adminTabs_ = new QTabWidget(page);
    adminPanel_ = createAdminPanel();
    academicPanel_ = new AcademicPanel(accounts_, session_, adminTabs_);
    adminTabs_->addTab(adminPanel_, "User management");
    adminTabs_->addTab(academicPanel_, "Academic management");
    layout->addWidget(adminTabs_);

    learningTabs_ = new QTabWidget(page);
    myCoursesTable_ = new QTableWidget(learningTabs_);
    myCoursesTable_->setColumnCount(3);
    myCoursesTable_->setHorizontalHeaderLabels({"Course code", "Course title", "Department ID"});
    myCoursesTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    myCoursesTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    assignmentsPanel_ = new AssignmentPanel(accounts_, submissions_, session_, learningTabs_);
    learningTabs_->addTab(myCoursesTable_, "My courses");
    learningTabs_->addTab(assignmentsPanel_, "Assignments and extensions");
    markingPanel_ = new MarkingPanel(accounts_, marking_, session_, learningTabs_);
    learningTabs_->addTab(markingPanel_, "Marking");
    resultsPanel_ = new ResultsPanel(moderation_, session_, learningTabs_);
    learningTabs_->addTab(resultsPanel_, "Results and moderation");
    layout->addWidget(learningTabs_);

    auto* information = new QLabel(
        "The LMS includes academic records, submissions, lecturer marking and moderated result release.",
        page);
    information->setWordWrap(true);
    information->setAlignment(Qt::AlignCenter);
    layout->addWidget(information);

    auto* logout = new QPushButton("Log out", page);
    connect(logout, &QPushButton::clicked, this, [this] { logOut(); });
    layout->addWidget(logout);
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

    const bool administrator = session_->role() == Role::Administrator;
    adminTabs_->setVisible(administrator);
    learningTabs_->setVisible(!administrator);
    learningTabs_->setTabVisible(1, session_->role()==Role::Lecturer ||
                                   session_->role()==Role::Student);
    learningTabs_->setTabVisible(2, session_->role()==Role::Lecturer);
    learningTabs_->setTabVisible(3, session_->role()==Role::HeadOfDepartment ||
                                   session_->role()==Role::Student);
    learningTabs_->setCurrentIndex(0);
    pages_->setCurrentWidget(dashboardPage_);
    if (administrator) {
        refreshAccounts();
        academicPanel_->refresh();
    } else {
        try {
            const auto courses = accounts_.listVisibleCourses(*session_);
            myCoursesTable_->setRowCount(static_cast<int>(courses.size()));
            for (int i = 0; i < static_cast<int>(courses.size()); ++i) {
                const auto& course = courses[static_cast<std::size_t>(i)];
                myCoursesTable_->setItem(i, 0,
                    new QTableWidgetItem(QString::fromStdString(course.code)));
                myCoursesTable_->setItem(i, 1,
                    new QTableWidgetItem(QString::fromStdString(course.title)));
                myCoursesTable_->setItem(i, 2,
                    new QTableWidgetItem(QString::number(course.departmentId)));
            }
            if(session_->role()==Role::Lecturer || session_->role()==Role::Student)
                assignmentsPanel_->refresh();
            if(session_->role()==Role::Lecturer)
                markingPanel_->refresh();
            if(session_->role()==Role::HeadOfDepartment ||
               session_->role()==Role::Student)
                resultsPanel_->refresh();
        } catch (const std::exception&) {
            QMessageBox::warning(this, "Courses", "Unable to load your assigned courses.");
            logOut();
        }
    }
}

void LoginWindow::createAccount()
{
    if (!session_) return;
    if (newPassword_->text() != newPasswordConfirmation_->text()) {
        newPassword_->clear();
        newPasswordConfirmation_->clear();
        QMessageBox::warning(this, "Cannot create user", "Passwords do not match.");
        return;
    }
    try {
        const auto name = newUsername_->text().trimmed().toStdString();
        const auto password = newPassword_->text().toStdString();
        const auto role = static_cast<Role>(newRole_->currentData().toInt());
        const auto result = accounts_.createUser(*session_, name, password, role);
        newUsername_->clear();
        newPassword_->clear();
        newPasswordConfirmation_->clear();
        refreshAccounts();
        academicPanel_->refresh();
        QMessageBox::information(this, "User created",
            "Created account for " + QString::fromStdString(result.username)
            + " (" + roleText(result.role) + ").");
    } catch (const AuthenticationError&) {
        newPassword_->clear();
        newPasswordConfirmation_->clear();
        QMessageBox::warning(this, "Access denied", "Your administrator session is no longer valid.");
        logOut();
    } catch (const std::exception& error) {
        newPassword_->clear();
        newPasswordConfirmation_->clear();
        QMessageBox::warning(this, "Cannot create user", error.what());
    }
}

void LoginWindow::refreshAccounts()
{
    if (!session_) return;
    try {
        const auto users = accounts_.listUsers(*session_);
        accountTable_->setRowCount(0);
        accountTable_->setRowCount(static_cast<int>(users.size()));
        for (int row = 0; row < static_cast<int>(users.size()); ++row) {
            const auto& user = users[static_cast<std::size_t>(row)];
            accountTable_->setItem(row, 0,
                new QTableWidgetItem(QString::number(static_cast<qlonglong>(user.id))));
            accountTable_->setItem(row, 1,
                new QTableWidgetItem(QString::fromStdString(user.username)));
            accountTable_->setItem(row, 2, new QTableWidgetItem(roleText(user.role)));
            accountTable_->setItem(row, 3,
                new QTableWidgetItem(user.active ? "Active" : "Inactive"));
        }
    } catch (const AuthenticationError&) {
        QMessageBox::warning(this, "Access denied", "Your administrator session is no longer valid.");
        logOut();
    } catch (const std::exception&) {
        QMessageBox::critical(this, "User list", "Could not load user records.");
    }
}

void LoginWindow::deactivateAccount()
{
    if (!session_) return;
    const int row = accountTable_->currentRow();
    if (row < 0 || !accountTable_->item(row, 0) || !accountTable_->item(row, 1)) {
        QMessageBox::information(this, "Select a user", "Select an account in the table first.");
        return;
    }

    const auto userId = accountTable_->item(row, 0)->text().toLongLong();
    const QString userName = accountTable_->item(row, 1)->text();
    if (QMessageBox::question(this, "Deactivate user",
           "Deactivate " + userName + "? The account will no longer be able to log in.")
        != QMessageBox::Yes) return;

    try {
        accounts_.deactivateUser(*session_, userId);
        refreshAccounts();
        QMessageBox::information(this, "User deactivated", userName + " is now inactive.");
    } catch (const AuthenticationError&) {
        QMessageBox::warning(this, "Access denied", "Your administrator session is no longer valid.");
        logOut();
    } catch (const std::exception& error) {
        QMessageBox::warning(this, "Cannot deactivate user", error.what());
    }
}

void LoginWindow::logOut()
{
    session_.reset();
    newUsername_->clear();
    newPassword_->clear();
    newPasswordConfirmation_->clear();
    loginPassword_->clear();
    accountTable_->setRowCount(0);
    myCoursesTable_->setRowCount(0);
    adminTabs_->hide();
    learningTabs_->hide();
    pages_->setCurrentWidget(loginPage_);
    loginUsername_->setFocus();
}

} // namespace lms
