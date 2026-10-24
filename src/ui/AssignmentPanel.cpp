#include "lms/ui/AssignmentPanel.h"

#include <QAbstractItemView>
#include <QComboBox>
#include <QDateTime>
#include <QDateTimeEdit>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QVariant>

#include <cstddef>
#include <cstdint>
#include <exception>
#include <stdexcept>
#include <string>

namespace lms {
namespace {
std::int64_t selection(const QComboBox* box)
{
    if (!box || box->currentIndex() < 0)
        throw std::invalid_argument("Select a course, assignment or student first");
    return box->currentData().toLongLong();
}

void addChoice(QComboBox* box, const QString& name, std::int64_t id)
{
    box->addItem(name,QVariant::fromValue(static_cast<qlonglong>(id)));
}

QString asUtc(const QDateTime& date)
{
    return date.toUTC().toString("yyyy-MM-ddTHH:mm:ss'Z'");
}
} // namespace

AssignmentPanel::AssignmentPanel(IAcademicManagementService& academic,
                                 ISubmissionService& submissions,
                                 const std::optional<Session>& session,
                                 QWidget* parent)
    : QWidget(parent),academic_(academic),submissions_(submissions),session_(session)
{
    auto* layout=new QVBoxLayout(this);
    auto* coursesForm=new QFormLayout;
    courses_=new QComboBox(this);
    coursesForm->addRow("Course:",courses_);
    layout->addLayout(coursesForm);

    assignments_=new QTableWidget(this);
    assignments_->setColumnCount(4);
    assignments_->setHorizontalHeaderLabels(
        {"Assignment","Original deadline (UTC)","My effective deadline (UTC)","Maximum mark"});
    assignments_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    assignments_->verticalHeader()->setVisible(false);
    assignments_->setSelectionBehavior(QAbstractItemView::SelectRows);
    assignments_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layout->addWidget(assignments_,1);

    creationGroup_=new QGroupBox("Create assignment (Lecturer)",this);
    auto* creationLayout=new QFormLayout(creationGroup_);
    assignmentTitle_=new QLineEdit(creationGroup_);
    assignmentTitle_->setPlaceholderText("Assignment title");
    assignmentDeadline_=new QDateTimeEdit(QDateTime::currentDateTime().addDays(7),creationGroup_);
    assignmentDeadline_->setCalendarPopup(true);
    assignmentDeadline_->setDisplayFormat("yyyy-MM-dd HH:mm");
    maximumMark_=new QDoubleSpinBox(creationGroup_);
    maximumMark_->setRange(0.01,1000000.0);
    maximumMark_->setDecimals(2);
    maximumMark_->setValue(100.0);
    auto* createButton=new QPushButton("Create assignment",creationGroup_);
    creationLayout->addRow("Title:",assignmentTitle_);
    creationLayout->addRow("Deadline (local time):",assignmentDeadline_);
    creationLayout->addRow("Maximum mark:",maximumMark_);
    creationLayout->addRow(createButton);
    layout->addWidget(creationGroup_);

    extensionGroup_=new QGroupBox("Individual deadline extension (Lecturer)",this);
    auto* extensionLayout=new QFormLayout(extensionGroup_);
    extensionAssignments_=new QComboBox(extensionGroup_);
    enrolledStudents_=new QComboBox(extensionGroup_);
    extensionDeadline_=new QDateTimeEdit(QDateTime::currentDateTime().addDays(14),extensionGroup_);
    extensionDeadline_->setCalendarPopup(true);
    extensionDeadline_->setDisplayFormat("yyyy-MM-dd HH:mm");
    currentDeadline_=new QLabel("Choose an assignment and student.",extensionGroup_);
    currentDeadline_->setWordWrap(true);
    auto* grantButton=new QPushButton("Grant individual extension",extensionGroup_);
    extensionLayout->addRow("Assignment:",extensionAssignments_);
    extensionLayout->addRow("Enrolled student:",enrolledStudents_);
    extensionLayout->addRow("Current deadline:",currentDeadline_);
    extensionLayout->addRow("New deadline (local time):",extensionDeadline_);
    extensionLayout->addRow(grantButton);
    layout->addWidget(extensionGroup_);

    auto* refreshButton=new QPushButton("Refresh assignments",this);
    layout->addWidget(refreshButton);

    connect(courses_,&QComboBox::currentIndexChanged,this,[this](int){loadCourse();});
    connect(extensionAssignments_,&QComboBox::currentIndexChanged,this,
            [this](int){refreshEffectiveDeadline();});
    connect(enrolledStudents_,&QComboBox::currentIndexChanged,this,
            [this](int){refreshEffectiveDeadline();});
    connect(createButton,&QPushButton::clicked,this,[this]{createAssignment();});
    connect(grantButton,&QPushButton::clicked,this,[this]{grantExtension();});
    connect(refreshButton,&QPushButton::clicked,this,[this]{refresh();});

    creationGroup_->hide();
    extensionGroup_->hide();
}

const Session& AssignmentPanel::actor() const
{
    if(!session_) throw AuthenticationError("Please log in again");
    return *session_;
}

void AssignmentPanel::showError(const std::exception& error)
{
    QMessageBox::warning(this,"Assignments",QString::fromUtf8(error.what()));
}

void AssignmentPanel::refresh()
{
    try {
        const auto role=actor().role();
        const bool lecturer=role==Role::Lecturer;
        if(role!=Role::Student && !lecturer)
            throw AuthenticationError("Assignments are not available for this role");
        creationGroup_->setVisible(lecturer);
        extensionGroup_->setVisible(lecturer);
        const auto courses=academic_.listVisibleCourses(actor());
        const QSignalBlocker block(courses_);
        const auto previous=courses_->currentData().toLongLong();
        courses_->clear();
        int selected=0;
        for(int i=0;i<static_cast<int>(courses.size());++i) {
            const auto& course=courses[static_cast<std::size_t>(i)];
            addChoice(courses_,QString::fromStdString(course.code+" - "+course.title),course.id);
            if(course.id==previous)selected=i;
        }
        if(courses_->count()>0)courses_->setCurrentIndex(selected);
        loadCourse();
    } catch(const std::exception& error) {
        showError(error);
    }
}

void AssignmentPanel::loadCourse()
{
    assignments_->setRowCount(0);
    {
        const QSignalBlocker block(extensionAssignments_);
        extensionAssignments_->clear();
    }
    {
        const QSignalBlocker block(enrolledStudents_);
        enrolledStudents_->clear();
    }
    currentDeadline_->setText("Choose an assignment and student.");
    if(!session_ || courses_->currentIndex()<0)return;
    try {
        const auto course=selection(courses_);
        const bool lecturer=actor().role()==Role::Lecturer;
        const auto records=academic_.listCourseAssignments(actor(),course);
        assignments_->setRowCount(static_cast<int>(records.size()));
        {
            const QSignalBlocker block(extensionAssignments_);
            for(int i=0;i<static_cast<int>(records.size());++i) {
                const auto& a=records[static_cast<std::size_t>(i)];
                QString effective="-";
                if(!lecturer) {
                    effective=QString::fromStdString(
                        submissions_.effectiveDeadline(actor(),a.id,actor().userId()));
                }
                assignments_->setItem(i,0,new QTableWidgetItem(QString::fromStdString(a.title)));
                assignments_->setItem(i,1,new QTableWidgetItem(QString::fromStdString(a.deadlineUtc)));
                assignments_->setItem(i,2,new QTableWidgetItem(effective));
                assignments_->setItem(i,3,new QTableWidgetItem(QString::number(a.maximumMark,'f',2)));
                if(lecturer)
                    addChoice(extensionAssignments_,QString::fromStdString(a.title),a.id);
            }
        }
        if(lecturer) {
            const auto students=academic_.listEnrolledStudentsForCourse(actor(),course);
            const QSignalBlocker block(enrolledStudents_);
            for(const auto& student:students)
                addChoice(enrolledStudents_,QString::fromStdString(student.username),student.id);
        }
        refreshEffectiveDeadline();
    } catch(const std::exception& error) {
        showError(error);
    }
}

void AssignmentPanel::refreshEffectiveDeadline()
{
    if(!session_ || actor().role()!=Role::Lecturer ||
       extensionAssignments_->currentIndex()<0 || enrolledStudents_->currentIndex()<0) {
        currentDeadline_->setText("Choose an assignment and student.");
        return;
    }
    try {
        const auto effective=submissions_.effectiveDeadline(actor(),
            selection(extensionAssignments_),selection(enrolledStudents_));
        currentDeadline_->setText(QString::fromStdString(effective));
    } catch(const std::exception& error) {
        currentDeadline_->setText(QString::fromUtf8(error.what()));
    }
}

void AssignmentPanel::createAssignment()
{
    try {
        academic_.createAssignment(actor(),selection(courses_),
            assignmentTitle_->text().trimmed().toStdString(),
            asUtc(assignmentDeadline_->dateTime()).toStdString(),
            maximumMark_->value());
        assignmentTitle_->clear();
        loadCourse();
        QMessageBox::information(this,"Assignment created","The assignment has been saved.");
    } catch(const std::exception& error) {
        showError(error);
    }
}

void AssignmentPanel::grantExtension()
{
    try {
        submissions_.grantExtension(actor(),selection(extensionAssignments_),
            selection(enrolledStudents_),
            asUtc(extensionDeadline_->dateTime()).toStdString());
        refreshEffectiveDeadline();
        QMessageBox::information(this,"Deadline extended",
            "The student's individual effective deadline has been updated.");
    } catch(const std::exception& error) {
        showError(error);
    }
}
} // namespace lms
