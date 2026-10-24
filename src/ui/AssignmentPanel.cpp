#include "lms/ui/AssignmentPanel.h"

#include <QAbstractItemView>
#include <QComboBox>
#include <QDateTime>
#include <QDateTimeEdit>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QLocale>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QScrollArea>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTime>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QVariant>

#include <cstddef>
#include <cstdint>
#include <exception>
#include <stdexcept>
#include <string>
#include <string_view>

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
    // The date editor displays minutes, so save new deadlines at second 00.
    auto precise = date;
    precise.setTime(QTime(date.time().hour(), date.time().minute(), 0));
    return precise.toUTC().toString("yyyy-MM-ddTHH:mm:ss'Z'");
}

QString asReadableLocalTime(std::string_view utc)
{
    const QDateTime timestamp = QDateTime::fromString(
        QString::fromUtf8(utc.data(), static_cast<qsizetype>(utc.size())), Qt::ISODate);
    if (!timestamp.isValid()) return "Invalid date";
    return QLocale(QLocale::English).toString(
        timestamp.toLocalTime(), "dd MMM yyyy, HH:mm");
}
} // namespace

AssignmentPanel::AssignmentPanel(IAcademicManagementService& academic,
                                 ISubmissionService& submissions,
                                 const std::optional<Session>& session,
                                 QWidget* parent)
    : QWidget(parent),academic_(academic),submissions_(submissions),session_(session)
{
    auto* outer=new QVBoxLayout(this);
    auto* scroll=new QScrollArea(this);
    scroll->setWidgetResizable(true);
    outer->addWidget(scroll);
    auto* content=new QWidget(scroll);
    scroll->setWidget(content);
    auto* layout=new QVBoxLayout(content);
    auto* coursesForm=new QFormLayout;
    courses_=new QComboBox(this);
    coursesForm->addRow("Course:",courses_);
    layout->addLayout(coursesForm);
    auto* timeNote = new QLabel(
        "All times are displayed in your computer's local time.", this);
    timeNote->setStyleSheet("color: palette(mid);");
    layout->addWidget(timeNote);

    assignments_=new QTableWidget(this);
    assignments_->setColumnCount(4);
    assignments_->setHorizontalHeaderLabels(
        {"Assignment","Original due (local)","Your due (local)","Maximum mark"});
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
    assignmentDeadline_->setDisplayFormat("dd MMM yyyy, HH:mm");
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
    extensionDeadline_->setDisplayFormat("dd MMM yyyy, HH:mm");
    currentDeadline_=new QLabel("Choose an assignment and student.",extensionGroup_);
    currentDeadline_->setWordWrap(true);
    auto* grantButton=new QPushButton("Grant individual extension",extensionGroup_);
    extensionLayout->addRow("Assignment:",extensionAssignments_);
    extensionLayout->addRow("Enrolled student:",enrolledStudents_);
    extensionLayout->addRow("Current deadline:",currentDeadline_);
    extensionLayout->addRow("New deadline (local time):",extensionDeadline_);
    extensionLayout->addRow(grantButton);
    layout->addWidget(extensionGroup_);

    studentSubmissionGroup_=new QGroupBox("Submit assignment (Student)",this);
    auto* submissionLayout=new QVBoxLayout(studentSubmissionGroup_);
    auto* submissionForm=new QFormLayout;
    submissionAssignments_=new QComboBox(studentSubmissionGroup_);
    submissionText_=new QTextEdit(studentSubmissionGroup_);
    submissionText_->setPlaceholderText("Enter your answer here or attach a file.");
    submissionText_->setMaximumHeight(100);
    attachmentPath_=new QLineEdit(studentSubmissionGroup_);
    attachmentPath_->setReadOnly(true);
    attachmentPath_->setPlaceholderText("Optional PDF, DOCX or TXT file (maximum 10 MB)");
    auto* browse=new QPushButton("Choose file",studentSubmissionGroup_);
    auto* clearAttachment=new QPushButton("Clear file",studentSubmissionGroup_);
    auto* fileRow=new QHBoxLayout;
    fileRow->addWidget(attachmentPath_,1);
    fileRow->addWidget(browse);
    fileRow->addWidget(clearAttachment);
    submissionForm->addRow("Assignment:",submissionAssignments_);
    submissionForm->addRow("Your response:",submissionText_);
    submissionForm->addRow("Attachment:",fileRow);
    submissionLayout->addLayout(submissionForm);
    auto* rule=new QLabel(
        "First submissions may be late. A replacement must be submitted by your own "
        "effective deadline; previous versions are preserved.",studentSubmissionGroup_);
    rule->setWordWrap(true);
    submissionLayout->addWidget(rule);
    submitButton_=new QPushButton("Submit / Resubmit",studentSubmissionGroup_);
    submissionLayout->addWidget(submitButton_);

    auto* historyLabel=new QLabel("Submission history (newest first)",studentSubmissionGroup_);
    submissionLayout->addWidget(historyLabel);
    versionHistory_=new QTableWidget(studentSubmissionGroup_);
    versionHistory_->setColumnCount(4);
    versionHistory_->setHorizontalHeaderLabels(
        {"Version","Submitted (local)","Status","Attachment"});
    versionHistory_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    versionHistory_->verticalHeader()->setVisible(false);
    versionHistory_->setSelectionBehavior(QAbstractItemView::SelectRows);
    versionHistory_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    versionHistory_->setMaximumHeight(145);
    submissionLayout->addWidget(versionHistory_);

    auto* savedLabel=new QLabel("Selected version's text",studentSubmissionGroup_);
    submissionLayout->addWidget(savedLabel);
    savedText_=new QTextEdit(studentSubmissionGroup_);
    savedText_->setReadOnly(true);
    savedText_->setPlaceholderText("Select a version to review the submitted text.");
    savedText_->setMaximumHeight(90);
    submissionLayout->addWidget(savedText_);
    layout->addWidget(studentSubmissionGroup_);

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
    connect(submissionAssignments_,&QComboBox::currentIndexChanged,this,
            [this](int){refreshHistory();});
    connect(browse,&QPushButton::clicked,this,[this]{chooseFile();});
    connect(clearAttachment,&QPushButton::clicked,this,
            [this]{attachmentPath_->clear();});
    connect(submitButton_,&QPushButton::clicked,this,[this]{submitWork();});
    connect(versionHistory_,&QTableWidget::itemSelectionChanged,this,
            [this]{showSelectedVersion();});

    creationGroup_->hide();
    extensionGroup_->hide();
    studentSubmissionGroup_->hide();
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
        studentSubmissionGroup_->setVisible(!lecturer);
        // Discard unsent UI state whenever a new account logs in.
        submissionText_->clear();
        attachmentPath_->clear();
        savedText_->clear();
        savedVersions_.clear();
        versionHistory_->setRowCount(0);
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
    {
        const QSignalBlocker block(submissionAssignments_);
        submissionAssignments_->clear();
    }
    versionHistory_->setRowCount(0);
    savedVersions_.clear();
    savedText_->clear();
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
                    effective=asReadableLocalTime(
                        submissions_.effectiveDeadline(actor(),a.id,actor().userId()));
                }
                assignments_->setItem(i,0,new QTableWidgetItem(QString::fromStdString(a.title)));
                auto* originalTime = new QTableWidgetItem(asReadableLocalTime(a.deadlineUtc));
                originalTime->setToolTip(QString::fromStdString(a.deadlineUtc) + " UTC (exact)");
                assignments_->setItem(i,1,originalTime);
                auto* effectiveTime = new QTableWidgetItem(effective);
                if (!lecturer) effectiveTime->setToolTip(QString::fromStdString(
                    submissions_.effectiveDeadline(actor(),a.id,actor().userId())) + " UTC (exact)");
                assignments_->setItem(i,2,effectiveTime);
                assignments_->setItem(i,3,new QTableWidgetItem(QString::number(a.maximumMark,'f',2)));
                if(lecturer)
                    addChoice(extensionAssignments_,QString::fromStdString(a.title),a.id);
                else
                    addChoice(submissionAssignments_,QString::fromStdString(a.title),a.id);
            }
        }
        if(lecturer) {
            const auto students=academic_.listEnrolledStudentsForCourse(actor(),course);
            const QSignalBlocker block(enrolledStudents_);
            for(const auto& student:students)
                addChoice(enrolledStudents_,QString::fromStdString(student.username),student.id);
        }
        refreshEffectiveDeadline();
        if(!lecturer) refreshHistory();
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
        currentDeadline_->setText(asReadableLocalTime(effective));
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
void AssignmentPanel::chooseFile()
{
    const QString path=QFileDialog::getOpenFileName(this,"Choose submission attachment",
        QString(),"Allowed files (*.pdf *.docx *.txt)");
    if(!path.isEmpty()) attachmentPath_->setText(path);
}

void AssignmentPanel::submitWork()
{
    try {
        if(actor().role()!=Role::Student)
            throw AuthenticationError("Only students may submit work");
        const auto assignment=selection(submissionAssignments_);
        SubmissionContent content;
        content.text=submissionText_->toPlainText().toStdString();
        content.attachmentPath=attachmentPath_->text().toStdString();
        const auto prior=submissions_.listMyVersions(actor(),assignment);
        if(!prior.empty() && QMessageBox::question(this,"Confirm resubmission",
            "Create a new version of this assignment? The earlier versions will remain saved.")
            != QMessageBox::Yes) return;
        const auto saved=submissions_.submit(actor(),assignment,content);
        submissionText_->clear();
        attachmentPath_->clear();
        refreshHistory();
        QMessageBox::information(this,"Submission saved",
            QString("Version %1 was saved successfully.").arg(saved.versionNumber)
            +(saved.late?" It was recorded as late.":""));
    } catch(const std::exception& error) {
        showError(error);
    }
}

void AssignmentPanel::refreshHistory()
{
    savedVersions_.clear();
    versionHistory_->setRowCount(0);
    savedText_->clear();
    if(!session_ || actor().role()!=Role::Student ||
       submissionAssignments_->currentIndex()<0) return;
    try {
        savedVersions_=submissions_.listMyVersions(actor(),selection(submissionAssignments_));
        versionHistory_->setRowCount(static_cast<int>(savedVersions_.size()));
        for(int row=0;row<static_cast<int>(savedVersions_.size());++row) {
            const auto& version=savedVersions_[static_cast<std::size_t>(row)];
            versionHistory_->setItem(row,0,
                new QTableWidgetItem(QString::number(version.versionNumber)));
            versionHistory_->setItem(row,1,
                new QTableWidgetItem(asReadableLocalTime(version.submittedAtUtc)));
            versionHistory_->setItem(row,2,
                new QTableWidgetItem(version.late?"Late":"On time"));
            versionHistory_->setItem(row,3,
                new QTableWidgetItem(version.attachmentRef.empty()?"None":"Saved"));
        }
        if(!savedVersions_.empty()) versionHistory_->selectRow(0);
    } catch(const std::exception& error) {
        showError(error);
    }
}

void AssignmentPanel::showSelectedVersion()
{
    const int row=versionHistory_->currentRow();
    if(row<0 || row>=static_cast<int>(savedVersions_.size())) {
        savedText_->clear();
        return;
    }
    const auto& selected=savedVersions_[static_cast<std::size_t>(row)];
    savedText_->setPlainText(selected.text.empty()
        ? QString(selected.attachmentRef.empty() ? "(No text)" : "(File-only submission)")
        : QString::fromStdString(selected.text));
}

} // namespace lms
