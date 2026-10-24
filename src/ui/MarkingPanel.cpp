#include "lms/ui/MarkingPanel.h"

#include <QAbstractItemView>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QVariant>

#include <cstddef>
#include <exception>
#include <stdexcept>

namespace lms {
namespace {
std::int64_t choice(const QComboBox* box)
{
    if(!box || box->currentIndex()<0)
        throw std::invalid_argument("Select a course and assignment");
    return box->currentData().toLongLong();
}
void addChoice(QComboBox* box, const QString& text,std::int64_t id)
{
    box->addItem(text,QVariant::fromValue(static_cast<qlonglong>(id)));
}
}

MarkingPanel::MarkingPanel(IAcademicManagementService& academic,
    IMarkingService& marking,const std::optional<Session>& session,QWidget* parent)
    : QWidget(parent),academic_(academic),marking_(marking),session_(session)
{
    auto* layout=new QVBoxLayout(this);
    auto* selectionForm=new QFormLayout;
    courses_=new QComboBox(this);
    assignments_=new QComboBox(this);
    selectionForm->addRow("Assigned course:",courses_);
    selectionForm->addRow("Assignment:",assignments_);
    layout->addLayout(selectionForm);

    submissions_=new QTableWidget(this);
    submissions_->setColumnCount(4);
    submissions_->setHorizontalHeaderLabels({"Student","Latest version","Status","Marking"});
    submissions_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    submissions_->setSelectionBehavior(QAbstractItemView::SelectRows);
    submissions_->setSelectionMode(QAbstractItemView::SingleSelection);
    submissions_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    submissions_->verticalHeader()->setVisible(false);
    submissions_->setMinimumHeight(150);
    layout->addWidget(submissions_,1);

    auto* review=new QGroupBox("Review the selected submission",this);
    auto* form=new QFormLayout(review);
    submittedText_=new QPlainTextEdit(review);
    submittedText_->setReadOnly(true);
    submittedText_->setPlaceholderText("Select a student to inspect their latest version.");
    submittedText_->setMaximumHeight(105);
    form->addRow("Submitted text:",submittedText_);
    download_=new QPushButton("Save latest attached file...",review);
    download_->setEnabled(false);
    form->addRow("Attachment:",download_);

    earnedMark_=new QDoubleSpinBox(review);
    earnedMark_->setRange(0,1000000.0);
    earnedMark_->setDecimals(2);
    form->addRow("Earned mark:",earnedMark_);
    feedback_=new QPlainTextEdit(review);
    feedback_->setMaximumHeight(70);
    feedback_->setPlaceholderText("Required written feedback");
    form->addRow("Feedback:",feedback_);
    revisionReason_=new QPlainTextEdit(review);
    revisionReason_->setMaximumHeight(55);
    revisionReason_->setPlaceholderText("Required when revising an existing mark");
    form->addRow("Revision reason:",revisionReason_);
    auto* save=new QPushButton("Save provisional mark",review);
    form->addRow(save);
    penalty_=new QLabel("Late penalty is calculated when you save.",review);
    penalty_->setWordWrap(true);
    form->addRow("Assessment:",penalty_);
    status_=new QLabel("Results remain hidden until HoD moderation and release.",review);
    status_->setWordWrap(true);
    form->addRow(status_);
    layout->addWidget(review);

    auto* refreshButton=new QPushButton("Refresh submissions",this);
    layout->addWidget(refreshButton);

    connect(courses_,&QComboBox::currentIndexChanged,this,[this](int){loadAssignments();});
    connect(assignments_,&QComboBox::currentIndexChanged,this,[this](int){loadSubmissions();});
    connect(submissions_,&QTableWidget::itemSelectionChanged,this,[this]{selectSubmission();});
    connect(save,&QPushButton::clicked,this,[this]{saveMark();});
    connect(download_,&QPushButton::clicked,this,[this]{saveAttachment();});
    connect(refreshButton,&QPushButton::clicked,this,[this]{refresh();});
}

const Session& MarkingPanel::actor() const
{
    if(!session_)throw AuthenticationError("Sign in again");
    return *session_;
}

void MarkingPanel::showError(const std::exception& error)
{
    QMessageBox::warning(this,"Marking",QString::fromUtf8(error.what()));
}

void MarkingPanel::refresh()
{
    rows_.clear();
    submissions_->setRowCount(0);
    submittedText_->clear();
    feedback_->clear();
    revisionReason_->clear();
    status_->setText("Results remain hidden until HoD moderation and release.");
    download_->setEnabled(false);
    try {
        if(actor().role()!=Role::Lecturer)
            throw AuthenticationError("Marking is available to assigned lecturers only");
        const auto courses=academic_.listVisibleCourses(actor());
        const QSignalBlocker block(courses_);
        const auto selected=courses_->currentData().toLongLong();
        courses_->clear();
        int keep=0;
        for(int i=0;i<static_cast<int>(courses.size());++i) {
            const auto& c=courses[static_cast<std::size_t>(i)];
            addChoice(courses_,QString::fromStdString(c.code+" - "+c.title),c.id);
            if(c.id==selected)keep=i;
        }
        if(courses_->count()>0)courses_->setCurrentIndex(keep);
        loadAssignments();
    } catch(const std::exception& error) { showError(error); }
}

void MarkingPanel::loadAssignments()
{
    const QSignalBlocker block(assignments_);
    const auto selected=assignments_->currentData().toLongLong();
    assignments_->clear();
    if(!session_||courses_->currentIndex()<0) {
        loadSubmissions();
        return;
    }
    try {
        const auto items=academic_.listCourseAssignments(actor(),choice(courses_));
        int keep=0;
        for(int i=0;i<static_cast<int>(items.size());++i) {
            const auto& a=items[static_cast<std::size_t>(i)];
            addChoice(assignments_,QString::fromStdString(a.title),a.id);
            if(a.id==selected)keep=i;
        }
        if(assignments_->count()>0)assignments_->setCurrentIndex(keep);
    } catch(const std::exception& error) { showError(error); }
    loadSubmissions();
}

void MarkingPanel::loadSubmissions()
{
    rows_.clear();
    submissions_->setRowCount(0);
    selectSubmission();
    if(!session_||assignments_->currentIndex()<0)return;
    try {
        const auto assignments=academic_.listCourseAssignments(actor(),choice(courses_));
        for(const auto& a:assignments)
            if(a.id==choice(assignments_))assignmentMaximum_=a.maximumMark;
        earnedMark_->setMaximum(assignmentMaximum_);
        rows_=marking_.listSubmissions(actor(),choice(assignments_));
        submissions_->setRowCount(static_cast<int>(rows_.size()));
        for(int row=0;row<static_cast<int>(rows_.size());++row) {
            const auto& sub=rows_[static_cast<std::size_t>(row)];
            const auto existing=marking_.currentResult(actor(),sub.submissionId);
            QString markStatus="Not marked";
            if(existing) {
                if(existing->markedVersionId!=sub.latestVersionId)
                    markStatus="Needs re-mark";
                else markStatus="Provisional";
            }
            submissions_->setItem(row,0,new QTableWidgetItem(QString::fromStdString(sub.studentUsername)));
            submissions_->setItem(row,1,new QTableWidgetItem(QString::number(sub.latestVersionNumber)));
            submissions_->setItem(row,2,new QTableWidgetItem(QString::fromStdString(sub.state)));
            submissions_->setItem(row,3,new QTableWidgetItem(markStatus));
        }
    } catch(const std::exception& error) { showError(error); }
}

void MarkingPanel::selectSubmission()
{
    submittedText_->clear();
    feedback_->clear();
    revisionReason_->clear();
    penalty_->setText("Late penalty is calculated when you save.");
    download_->setEnabled(false);
    const int index=submissions_->currentRow();
    if(!session_||index<0||index>=static_cast<int>(rows_.size()))return;
    try {
        const auto& sub=rows_[static_cast<std::size_t>(index)];
        submittedText_->setPlainText(sub.text.empty()?QString("(File-only submission)"):
                                     QString::fromStdString(sub.text));
        download_->setEnabled(!sub.attachmentRef.empty());
        const auto existing=marking_.currentResult(actor(),sub.submissionId);
        if(existing) {
            earnedMark_->setValue(existing->earnedMark);
            feedback_->setPlainText(QString::fromStdString(existing->feedback));
            if(existing->markedVersionId!=sub.latestVersionId)
                status_->setText("A newer version was submitted. Mark this newest version again.");
            else
                status_->setText("Existing provisional mark. Changes require an audit reason.");
        } else {
            earnedMark_->setValue(0.0);
            status_->setText("New assessment — not yet marked.");
        }
    } catch(const std::exception& error) { showError(error); }
}

void MarkingPanel::saveMark()
{
    const int index=submissions_->currentRow();
    if(index<0||index>=static_cast<int>(rows_.size())) {
        QMessageBox::information(this,"Select a submission","Select a student submission first.");
        return;
    }
    try {
        const auto submissionId=rows_[static_cast<std::size_t>(index)].submissionId;
        const auto result=marking_.mark(actor(),submissionId,earnedMark_->value(),
            feedback_->toPlainText().toStdString(),revisionReason_->toPlainText().toStdString());
        penalty_->setText(QString("Earned %1  |  Late penalty %2  |  Provisional final %3")
            .arg(result.earnedMark,0,'f',2)
            .arg(result.penaltyMark,0,'f',2)
            .arg(result.finalMark,0,'f',2));
        revisionReason_->clear();
        status_->setText("Saved for HoD moderation. This result is not visible to the student.");
        // Refresh state without removing the success message.
        const auto username=rows_[static_cast<std::size_t>(index)].studentUsername;
        loadSubmissions();
        for(int i=0;i<static_cast<int>(rows_.size());++i)
            if(rows_[static_cast<std::size_t>(i)].studentUsername==username)
                submissions_->selectRow(i);
        penalty_->setText(QString("Earned %1  |  Late penalty %2  |  Provisional final %3")
            .arg(result.earnedMark,0,'f',2)
            .arg(result.penaltyMark,0,'f',2)
            .arg(result.finalMark,0,'f',2));
        QMessageBox::information(this,"Provisional assessment saved",
            "The mark and feedback are saved. Student access remains blocked.");
    } catch(const std::exception& error) { showError(error); }
}

void MarkingPanel::saveAttachment()
{
    const int index=submissions_->currentRow();
    if(index<0||index>=static_cast<int>(rows_.size()))return;
    try {
        const auto& sub=rows_[static_cast<std::size_t>(index)];
        const auto pos=sub.attachmentRef.find_last_of('.');
        const std::string ext=pos==std::string::npos?"":sub.attachmentRef.substr(pos);
        const QString target=QFileDialog::getSaveFileName(
            this,"Save attached submission",QString("submission")+QString::fromStdString(ext));
        if(target.isEmpty())return;
        // Authorisation is checked by the service immediately before file access.
        const std::string bytes=marking_.readLatestAttachment(actor(),sub.submissionId);
        QFile file(target);
        if(!file.open(QIODevice::WriteOnly))
            throw std::runtime_error("Cannot write the selected attachment file");
        if(file.write(bytes.data(),static_cast<qint64>(bytes.size()))
            !=static_cast<qint64>(bytes.size())) {
            file.close();
            file.remove();
            throw std::runtime_error("Unable to save the full attachment");
        }
        file.close();
        QMessageBox::information(this,"Attachment saved","The submission file was saved.");
    } catch(const std::exception& error) { showError(error); }
}
} // namespace lms
