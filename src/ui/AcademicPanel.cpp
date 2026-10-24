#include "lms/ui/AcademicPanel.h"

#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QVariant>

#include <exception>
#include <stdexcept>

namespace lms {
namespace {
QTableWidget* makeTable(QWidget* parent, const QStringList& headings)
{
    auto* table = new QTableWidget(parent);
    table->setColumnCount(headings.size());
    table->setHorizontalHeaderLabels(headings);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->verticalHeader()->setVisible(false);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setMinimumHeight(130);
    return table;
}

std::int64_t selectedId(const QComboBox* box)
{
    if (!box || box->currentIndex() < 0)
        throw std::invalid_argument("Select an item first");
    return box->currentData().toLongLong();
}

void addId(QComboBox* box, const QString& text, std::int64_t id)
{
    box->addItem(text, QVariant::fromValue(static_cast<qlonglong>(id)));
}
} // namespace

AcademicPanel::AcademicPanel(IAcademicManagementService& service,
                             const std::optional<Session>& currentSession,
                             QWidget* parent)
    : QWidget(parent), service_(service), session_(currentSession)
{
    auto* outer = new QVBoxLayout(this);
    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    outer->addWidget(scroll);
    auto* content = new QWidget;
    scroll->setWidget(content);
    auto* layout = new QVBoxLayout(content);
    layout->setSpacing(12);

    auto* deptGroup = new QGroupBox("Departments", content);
    auto* deptLayout = new QVBoxLayout(deptGroup);
    departmentName_ = new QLineEdit(deptGroup);
    departmentName_->setPlaceholderText("New department name");
    auto* deptButtons = new QHBoxLayout;
    auto* addDept = new QPushButton("Create department", deptGroup);
    auto* editDept = new QPushButton("Rename selected department", deptGroup);
    deptButtons->addWidget(addDept);
    deptButtons->addWidget(editDept);
    departments_ = new QComboBox(deptGroup);
    departmentsTable_ = makeTable(deptGroup, {"ID", "Department"});
    deptLayout->addWidget(departmentName_);
    deptLayout->addLayout(deptButtons);
    deptLayout->addWidget(departments_);
    deptLayout->addWidget(departmentsTable_);
    layout->addWidget(deptGroup);

    auto* courseGroup = new QGroupBox("Courses", content);
    auto* courseLayout = new QVBoxLayout(courseGroup);
    auto* form = new QFormLayout;
    courseDepartments_ = new QComboBox(courseGroup);
    courseCode_ = new QLineEdit(courseGroup);
    courseCode_->setPlaceholderText("e.g. CS4433");
    courseTitle_ = new QLineEdit(courseGroup);
    courseTitle_->setPlaceholderText("Course title");
    form->addRow("Department:", courseDepartments_);
    form->addRow("Course code:", courseCode_);
    form->addRow("Course title:", courseTitle_);
    courseLayout->addLayout(form);
    auto* addCourse = new QPushButton("Create course", courseGroup);
    courseLayout->addWidget(addCourse);
    courses_ = new QComboBox(courseGroup);
    auto* editCourseButton = new QPushButton("Edit selected course", courseGroup);
    courseLayout->addWidget(courses_);
    courseLayout->addWidget(editCourseButton);
    coursesTable_ = makeTable(courseGroup, {"ID", "Code", "Course", "Department ID"});
    courseLayout->addWidget(coursesTable_);
    layout->addWidget(courseGroup);

    auto* allocationGroup = new QGroupBox("Role and course allocations", content);
    auto* allocationLayout = new QFormLayout(allocationGroup);
    hodUsers_ = new QComboBox(allocationGroup);
    hodDepartments_ = new QComboBox(allocationGroup);
    lecturerUsers_ = new QComboBox(allocationGroup);
    lecturerCourses_ = new QComboBox(allocationGroup);
    studentUsers_ = new QComboBox(allocationGroup);
    studentCourses_ = new QComboBox(allocationGroup);
    auto* assignHodButton = new QPushButton("Assign HoD to department", allocationGroup);
    auto* assignLecturerButton = new QPushButton("Assign lecturer to course", allocationGroup);
    auto* enrolButton = new QPushButton("Enrol student in course", allocationGroup);
    allocationLayout->addRow("Head of Department:", hodUsers_);
    allocationLayout->addRow("Department:", hodDepartments_);
    allocationLayout->addRow(assignHodButton);
    allocationLayout->addRow("Lecturer:", lecturerUsers_);
    allocationLayout->addRow("Course:", lecturerCourses_);
    allocationLayout->addRow(assignLecturerButton);
    allocationLayout->addRow("Student:", studentUsers_);
    allocationLayout->addRow("Course:", studentCourses_);
    allocationLayout->addRow(enrolButton);
    layout->addWidget(allocationGroup);

    auto* refreshButton = new QPushButton("Refresh departments, courses and users", content);
    layout->addWidget(refreshButton);
    layout->addStretch();

    connect(addDept, &QPushButton::clicked, this, [this] { createDepartment(); });
    connect(editDept, &QPushButton::clicked, this, [this] { editDepartment(); });
    connect(addCourse, &QPushButton::clicked, this, [this] { createCourse(); });
    connect(editCourseButton, &QPushButton::clicked, this, [this] { editCourse(); });
    connect(assignHodButton, &QPushButton::clicked, this, [this] { assignHoD(); });
    connect(assignLecturerButton, &QPushButton::clicked, this, [this] { assignLecturer(); });
    connect(enrolButton, &QPushButton::clicked, this, [this] { enrolStudent(); });
    connect(refreshButton, &QPushButton::clicked, this, [this] { refresh(); });
}

const Session& AcademicPanel::actor() const
{
    if (!session_) throw AuthenticationError("No active session");
    return *session_;
}

void AcademicPanel::reportFailure(const std::exception& error)
{
    QMessageBox::warning(this, "Academic management", QString::fromUtf8(error.what()));
}

void AcademicPanel::refresh()
{
    try {
        const auto departments = service_.listDepartments(actor());
        const auto courses = service_.listAllCourses(actor());
        const auto users = service_.listUsers(actor());

        departments_->clear(); courseDepartments_->clear(); hodDepartments_->clear();
        departmentsTable_->setRowCount(static_cast<int>(departments.size()));
        for (int i=0;i<static_cast<int>(departments.size());++i) {
            const auto& dept=departments[static_cast<std::size_t>(i)];
            const QString name=QString::fromStdString(dept.name);
            addId(departments_,name,dept.id);
            addId(courseDepartments_,name,dept.id);
            addId(hodDepartments_,name,dept.id);
            departmentsTable_->setItem(i,0,new QTableWidgetItem(QString::number(dept.id)));
            departmentsTable_->setItem(i,1,new QTableWidgetItem(name));
        }

        courses_->clear(); lecturerCourses_->clear(); studentCourses_->clear();
        coursesTable_->setRowCount(static_cast<int>(courses.size()));
        for (int i=0;i<static_cast<int>(courses.size());++i) {
            const auto& course=courses[static_cast<std::size_t>(i)];
            const QString code=QString::fromStdString(course.code);
            addId(courses_,code,course.id);
            addId(lecturerCourses_,code,course.id);
            addId(studentCourses_,code,course.id);
            coursesTable_->setItem(i,0,new QTableWidgetItem(QString::number(course.id)));
            coursesTable_->setItem(i,1,new QTableWidgetItem(code));
            coursesTable_->setItem(i,2,new QTableWidgetItem(QString::fromStdString(course.title)));
            coursesTable_->setItem(i,3,new QTableWidgetItem(QString::number(course.departmentId)));
        }

        hodUsers_->clear(); lecturerUsers_->clear(); studentUsers_->clear();
        for (const auto& user : users) {
            if (!user.active) continue;
            const QString name=QString::fromStdString(user.username);
            if (user.role == Role::HeadOfDepartment) addId(hodUsers_,name,user.id);
            if (user.role == Role::Lecturer) addId(lecturerUsers_,name,user.id);
            if (user.role == Role::Student) addId(studentUsers_,name,user.id);
        }
    } catch (const std::exception& error) {
        reportFailure(error);
    }
}

void AcademicPanel::createDepartment()
{
    try {
        service_.createDepartment(actor(),departmentName_->text().trimmed().toStdString());
        departmentName_->clear();
        refresh();
    } catch (const std::exception& error) { reportFailure(error); }
}

void AcademicPanel::editDepartment()
{
    try {
        const auto id=selectedId(departments_);
        const QString before=departments_->currentText();
        bool ok=false;
        const QString value=QInputDialog::getText(this,"Rename department",
            "Department name:",QLineEdit::Normal,before,&ok);
        if (!ok) return;
        service_.updateDepartment(actor(),Department{id,value.trimmed().toStdString()});
        refresh();
    } catch (const std::exception& error) { reportFailure(error); }
}

void AcademicPanel::createCourse()
{
    try {
        service_.createCourse(actor(),selectedId(courseDepartments_),
                              courseCode_->text().trimmed().toStdString(),
                              courseTitle_->text().trimmed().toStdString());
        courseCode_->clear();
        courseTitle_->clear();
        refresh();
    } catch (const std::exception& error) { reportFailure(error); }
}

void AcademicPanel::editCourse()
{
    try {
        const auto id=selectedId(courses_);
        const auto existing=service_.listAllCourses(actor());
        const auto it=std::find_if(existing.begin(),existing.end(),
            [id](const Course& course){return course.id==id;});
        if (it==existing.end()) throw std::invalid_argument("Course not found");
        bool ok=false;
        const QString title=QInputDialog::getText(this,"Update course title",
            "Course title:",QLineEdit::Normal,QString::fromStdString(it->title),&ok);
        if (!ok) return;
        auto modified=*it;
        modified.title=title.trimmed().toStdString();
        service_.updateCourse(actor(),modified);
        refresh();
    } catch (const std::exception& error) { reportFailure(error); }
}

void AcademicPanel::assignHoD()
{
    try {
        service_.assignHoD(actor(),selectedId(hodUsers_),selectedId(hodDepartments_));
        QMessageBox::information(this,"HoD assignment","Department assigned.");
    } catch (const std::exception& error) { reportFailure(error); }
}

void AcademicPanel::assignLecturer()
{
    try {
        service_.assignLecturer(actor(),selectedId(lecturerUsers_),selectedId(lecturerCourses_));
        QMessageBox::information(this,"Teaching assignment","Lecturer assigned.");
    } catch (const std::exception& error) { reportFailure(error); }
}

void AcademicPanel::enrolStudent()
{
    try {
        service_.enrolStudent(actor(),selectedId(studentUsers_),selectedId(studentCourses_));
        QMessageBox::information(this,"Enrolment","Student enrolled.");
    } catch (const std::exception& error) { reportFailure(error); }
}
} // namespace lms
