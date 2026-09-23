#include "MainWindow.h"

#include <QCheckBox>
#include <QFileDialog>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QSpinBox>
#include <QPlainTextEdit>
#include <QVBoxLayout>
#include <QWidget>
#include <QStyle>
#include <QRegularExpression>
#include <QDesktopServices>
#include <QSettings>

#include "FilePathLineEdit.h"
#include "StarNetRunner.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , _runner(std::make_unique<StarNetRunner>(this))
{
    setupUI();

    setWindowIcon(QIcon(":/icons/starnet.png"));

    connect(_runner.get(), &StarNetRunner::started, this, [this]()
        {
            _logEdit->appendPlainText("StarNet2 started.");
        });

    connect(_runner.get(), &StarNetRunner::outputReceived, this, [this](const QString& output)
        {
            if (!updateProgress(output))
            {
                _logEdit->appendPlainText("[INFO] " + output);
            }
        });

    connect(_runner.get(), &StarNetRunner::errorReceived, this, [this](const QString& error)
        {
            _logEdit->appendPlainText("[ERROR] " + error);
        });

    connect(_runner.get(), &StarNetRunner::finished, this, [this](int exitCode)
        {
            _processButton->setEnabled(true);

            if (exitCode == 0)
            {
                _logEdit->appendPlainText("[INFO] StarNet2 finished successfully.");
            }
            else
            {
                _logEdit->appendPlainText(QString("[ERROR] StarNet2 finished with exit code %1.").arg(exitCode));
            }
        });
}

void MainWindow::setupUI()
{
    setWindowTitle("StarNet2 GUI");
    resize(800, 700);

    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(6);

    // ============================================================
    // Top bar
    // ============================================================

    QFrame* topFrame = new QFrame(centralWidget);
    topFrame->setFrameShape(QFrame::StyledPanel);

    QLabel* executableLabel = new QLabel("StarNet2 Executable:", topFrame);

    QLineEdit* executableEdit = new QLineEdit(topFrame);
    executableEdit->setPlaceholderText("Path to starnet2.exe...");

    const QString starnetPath = QDir(QCoreApplication::applicationDirPath()).filePath("starnet2.exe");
    if (QFileInfo::exists(starnetPath))
    {
        executableEdit->setText(QDir::toNativeSeparators(starnetPath));
    }
    else
    {
        QSettings settings = this->settings();
        const QString path = settings.value("StarNet/executablePath").toString();
        if (QFileInfo(path).isFile())
        {
            executableEdit->setText(path);
        }
    }

    QPushButton* executableBrowseButton = new QPushButton("...", topFrame);
    executableBrowseButton->setFixedWidth(45);
    connect(executableBrowseButton, &QPushButton::clicked, this, [=]()
        {
            const QString fileName = QFileDialog::getOpenFileName(this, "Select StarNet2 Executable", executableEdit->text(), "Executable (*.exe)");
            if (!fileName.isEmpty())
            {
                executableEdit->setText(fileName);

                QSettings settings = this->settings();
                settings.setValue("StarNet/executablePath", fileName);
            }
        });

    QHBoxLayout* topLayout = new QHBoxLayout(topFrame);
    topLayout->setContentsMargins(12, 8, 12, 8);
    topLayout->setSpacing(8);
    topLayout->addWidget(executableLabel);
    topLayout->addWidget(executableEdit, 1);
    topLayout->addWidget(executableBrowseButton);

    mainLayout->addWidget(topFrame);

    // ------------------------------------------------------------
    // Section font
    // ------------------------------------------------------------

    QFont sectionFont;
    sectionFont.setBold(true);
    sectionFont.setPointSize(12);

    // ============================================================
    // Input
    // ============================================================

    QLabel* inputLabel = new QLabel("Input", centralWidget);
    inputLabel->setFont(sectionFont);

    mainLayout->addWidget(inputLabel);

    QLabel* inputTitle = new QLabel("Input Image:", centralWidget);
    inputTitle->setMinimumWidth(100);

    _inputEdit = new CFilePathLineEdit(centralWidget);
    _inputEdit->setPlaceholderText("Select input image...");
    connect(_inputEdit, &QLineEdit::textChanged, this, [this](const QString& text)
        {
            // Generate default output filename
            const QFileInfo fileInfo(text);
            _starlessOutputEdit->setText(fileInfo.path() + "/" + fileInfo.completeBaseName() + "_starless." + fileInfo.suffix());
            _maskOutputEdit->setText(fileInfo.path() + "/" + fileInfo.completeBaseName() + "_mask." + fileInfo.suffix());
            _starsOutputEdit->setText(fileInfo.path() + "/" + fileInfo.completeBaseName() + "_stars." + fileInfo.suffix());
        });

    QPushButton* inputBrowseButton = new QPushButton("", centralWidget);
    inputBrowseButton->setFixedWidth(45);
    inputBrowseButton->setIcon(style()->standardIcon(QStyle::SP_DirIcon));

    QHBoxLayout* inputLayout = new QHBoxLayout;
    inputLayout->setSpacing(6);
    inputLayout->addWidget(inputTitle);
    inputLayout->addWidget(_inputEdit, 1);
    inputLayout->addWidget(inputBrowseButton);

    mainLayout->addLayout(inputLayout);

    connect(inputBrowseButton, &QPushButton::clicked, this, [this]()
        {
            QString initialDir;
            const QString inputPath = _inputEdit->text();
            if (!inputPath.isEmpty())
            {
                const QFileInfo fileInfo(inputPath);
                initialDir = fileInfo.absolutePath();
            }

            const QString fileName = QFileDialog::getOpenFileName( this, "Select Input Image", initialDir, "Images (*.tif *.tiff *.png *.fits *.fit);;All Files (*)");
            if (fileName.isEmpty())
                return;

            _inputEdit->setText(fileName);
        });

    // ============================================================
    // Output
    // ============================================================

    QLabel* outputLabel = new QLabel("Output", centralWidget);
    outputLabel->setFont(sectionFont);

    mainLayout->addWidget(outputLabel);

    QLabel* outputTitle = new QLabel("Starless Image:", centralWidget);
    outputTitle->setMinimumWidth(100);

    _starlessOutputEdit = new QLineEdit(centralWidget);
    _starlessOutputEdit->setPlaceholderText("Starless output image...");

    QPushButton* outputBrowseButton = new QPushButton("", centralWidget);
    outputBrowseButton->setFixedWidth(45);
    outputBrowseButton->setIcon(style()->standardIcon(QStyle::SP_DirIcon));

    QHBoxLayout* outputLayout = new QHBoxLayout;
    outputLayout->setSpacing(6);
    outputLayout->addWidget(outputTitle);
    outputLayout->addWidget(_starlessOutputEdit, 1);
    outputLayout->addWidget(outputBrowseButton);

    mainLayout->addLayout(outputLayout);

    connect(outputBrowseButton, &QPushButton::clicked, this, [this]()
        {
            QString initialDir;
            const QString inputPath = _starlessOutputEdit->text();
            if (!inputPath.isEmpty())
            {
                const QFileInfo fileInfo(inputPath);
                initialDir = fileInfo.absolutePath();
            }

            const QString fileName = QFileDialog::getSaveFileName(this, "Select Starless Output", initialDir, "TIFF (*.tif *.tiff);;All Files (*)");
            if (!fileName.isEmpty())
                _starlessOutputEdit->setText(fileName);
        });

    // ============================================================
    // Optional Outputs
    // ============================================================

    QGroupBox* optionalGroup = new QGroupBox("Advanced Options", centralWidget);

    QVBoxLayout* optionalLayout = new QVBoxLayout(optionalGroup);
    optionalLayout->setContentsMargins(12, 12, 12, 12);
    optionalLayout->setSpacing(8);

    // ------------------------------------------------------------
    // Star Mask
    // ------------------------------------------------------------

    QCheckBox* maskCheckBox = new QCheckBox("Generate Star Mask", optionalGroup);
    maskCheckBox->setChecked(false);

    optionalLayout->addWidget(maskCheckBox);

    QHBoxLayout* maskLayout = new QHBoxLayout;
    maskLayout->setContentsMargins(20, 0, 0, 0);
    maskLayout->setSpacing(6);

    QLabel* maskTitle = new QLabel("Mask File:", optionalGroup);

    _maskOutputEdit = new QLineEdit(optionalGroup);
    _maskOutputEdit->setPlaceholderText("Star mask output...");

    QPushButton* maskBrowseButton = new QPushButton("", optionalGroup);
    maskBrowseButton->setFixedWidth(45);
    maskBrowseButton->setIcon(style()->standardIcon(QStyle::SP_DirIcon));
    connect(maskBrowseButton, &QPushButton::clicked, this, [this]()
        {
            QString initialDir;
            const QString inputPath = _maskOutputEdit->text();
            if (!inputPath.isEmpty())
            {
                const QFileInfo fileInfo(inputPath);
                initialDir = fileInfo.absolutePath();
            }

            const QString fileName = QFileDialog::getSaveFileName(this, "Select Star Mask Output", initialDir, "TIFF (*.tif *.tiff);;All Files (*)");
            if (!fileName.isEmpty())
                _maskOutputEdit->setText(fileName);
        });

    maskLayout->addWidget(maskTitle);
    maskLayout->addWidget(_maskOutputEdit, 1);
    maskLayout->addWidget(maskBrowseButton);

    optionalLayout->addLayout(maskLayout);

    // ------------------------------------------------------------
    // Star Layer
    // ------------------------------------------------------------

    QCheckBox* starsCheckBox = new QCheckBox("Generate Star Layer (unscreen)", optionalGroup);

    optionalLayout->addWidget(starsCheckBox);

    QLabel* starsTitle = new QLabel("Star Layer File:", optionalGroup);

    _starsOutputEdit = new QLineEdit(optionalGroup);
    _starsOutputEdit->setPlaceholderText("Star layer output...");

    QPushButton* starsBrowseButton = new QPushButton("", optionalGroup);
    starsBrowseButton->setFixedWidth(45);
    starsBrowseButton->setIcon(style()->standardIcon(QStyle::SP_DirIcon));
    connect(starsBrowseButton, &QPushButton::clicked, this, [this]()
        {
            QString initialDir;
            const QString inputPath = _starsOutputEdit->text();
            if (!inputPath.isEmpty())
            {
                const QFileInfo fileInfo(inputPath);
                initialDir = fileInfo.absolutePath();
            }

            const QString fileName = QFileDialog::getSaveFileName(this, "Select Star Layer Output", initialDir, "TIFF (*.tif *.tiff);;All Files (*)");
            if (!fileName.isEmpty())
                _starsOutputEdit->setText(fileName);
        });

    QHBoxLayout* starsLayout = new QHBoxLayout;
    starsLayout->setContentsMargins(20, 0, 0, 0);
    starsLayout->setSpacing(6);
    starsLayout->addWidget(starsTitle);
    starsLayout->addWidget(_starsOutputEdit, 1);
    starsLayout->addWidget(starsBrowseButton);

    optionalLayout->addLayout(starsLayout);

    // ------------------------------------------------------------
    // More Options
    // ------------------------------------------------------------

    QCheckBox* strideCheckBox = new QCheckBox("Stride Value:", this);
    QSpinBox* strideSpinBox = new QSpinBox(this);
    strideSpinBox->setRange(2, 512);
    strideSpinBox->setSingleStep(2);
    strideSpinBox->setValue(32);

    QHBoxLayout* strideLayout = new QHBoxLayout;
    strideLayout->addWidget(strideCheckBox);
    strideLayout->addWidget(strideSpinBox);
    strideLayout->addStretch();

    QCheckBox* upsampleCheckBox = new QCheckBox("Upsample Input Image (2x)", optionalGroup);
    QCheckBox* save8BitCheckBox = new QCheckBox("Save Output As 8-bit", optionalGroup);
    QCheckBox* disableHighlightsProtectionCheckBox = new QCheckBox("Disable Starless Output Highlights Protection", optionalGroup);

    QGridLayout* advancedOptionalLayout = new QGridLayout(optionalGroup);
    advancedOptionalLayout->setColumnStretch(0, 1);
    advancedOptionalLayout->setColumnStretch(1, 1);
    advancedOptionalLayout->addLayout(strideLayout, 0, 0);
    advancedOptionalLayout->addWidget(upsampleCheckBox, 0, 1);
    advancedOptionalLayout->addWidget(save8BitCheckBox, 1, 0);
    advancedOptionalLayout->addWidget(disableHighlightsProtectionCheckBox, 1, 1);

    optionalLayout->addLayout(advancedOptionalLayout);

    mainLayout->addWidget(optionalGroup);

    // ============================================================
    // Buttons
    // ============================================================

    _processButton = new QPushButton("Process", centralWidget);
    _processButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    _processButton->setMinimumHeight(40);
    _processButton->setMinimumWidth(180);

    QPushButton* openOutputFileButton = new QPushButton("Open Output File", centralWidget);
    openOutputFileButton->setIcon(style()->standardIcon(QStyle::SP_FileDialogDetailedView));
    openOutputFileButton->setMinimumHeight(40);
    openOutputFileButton->setMinimumWidth(180);

    QPushButton* openOutputFolderButton = new QPushButton("Open Output Folder", centralWidget);
    openOutputFolderButton->setIcon(style()->standardIcon(QStyle::SP_DirOpenIcon));
    openOutputFolderButton->setMinimumHeight(40);
    openOutputFolderButton->setMinimumWidth(180);

    QHBoxLayout* buttonLayout = new QHBoxLayout;
    buttonLayout->setSpacing(8);
    buttonLayout->addStretch();
    buttonLayout->addWidget(_processButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(openOutputFileButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(openOutputFolderButton);
    buttonLayout->addStretch();

    mainLayout->addLayout(buttonLayout);

    // ============================================================
    // Progress
    // ============================================================

    QGroupBox* progressGroup = new QGroupBox("Progress", centralWidget);

    _progressBar = new QProgressBar(progressGroup);
    _progressBar->setRange(0, 100);
    _progressBar->setValue(0);
    _progressBar->setTextVisible(true);

    QPushButton* cancelButton = new QPushButton("Cancel", progressGroup);
    cancelButton->setIcon(style()->standardIcon(QStyle::SP_DialogCancelButton));

    QHBoxLayout* progressLayout = new QHBoxLayout(progressGroup);
    progressLayout->setContentsMargins(12, 12, 12, 12);
    progressLayout->addWidget(_progressBar, 1);
    progressLayout->addWidget(cancelButton);

    mainLayout->addWidget(progressGroup);

    // ============================================================
    // Log
    // ============================================================

    QLabel* logLabel = new QLabel("Log", centralWidget);
    logLabel->setFont(sectionFont);

    QPushButton* clearLogButton = new QPushButton("Clear Log", centralWidget);
    clearLogButton->setMaximumWidth(100);
    clearLogButton->setIcon(style()->standardIcon(QStyle::SP_DialogResetButton));

    QHBoxLayout* logLayout = new QHBoxLayout;
    logLayout->setSpacing(6);
    logLayout->addWidget(logLabel);
    logLayout->addWidget(clearLogButton);

    mainLayout->addLayout(logLayout);

    _logEdit = new QPlainTextEdit(centralWidget);
    _logEdit->setReadOnly(true);
    _logEdit->setFont(QFont("Consolas", 9));
    _logEdit->setPlaceholderText("StarNet2 output will appear here...");

    mainLayout->addWidget(_logEdit, 1);

    // ============================================================
    // Connections
    // ============================================================

    // Enable / disable optional output fields.

    connect(maskCheckBox, &QCheckBox::toggled, _maskOutputEdit, &QLineEdit::setEnabled);
    connect(maskCheckBox, &QCheckBox::toggled, maskBrowseButton, &QPushButton::setEnabled);
    connect(starsCheckBox, &QCheckBox::toggled, _starsOutputEdit, &QLineEdit::setEnabled);
    connect(starsCheckBox, &QCheckBox::toggled, starsBrowseButton, &QPushButton::setEnabled);
    connect(strideCheckBox, &QCheckBox::toggled, strideSpinBox, &QSpinBox::setEnabled);

    _maskOutputEdit->setEnabled(maskCheckBox->isChecked());
    maskBrowseButton->setEnabled(maskCheckBox->isChecked());
    _starsOutputEdit->setEnabled(starsCheckBox->isChecked());
    starsBrowseButton->setEnabled(starsCheckBox->isChecked());
    strideSpinBox->setEnabled(strideCheckBox->isChecked());

    // Process
    connect(_processButton, &QPushButton::clicked, this, [=]()
        {
            if (executableEdit->text().isEmpty())
            {
                _logEdit->appendPlainText("[ERROR] Please fill in the StarNet2 executable path.");
                return;
            }

            if (_inputEdit->text().isEmpty())
            {
                _logEdit->appendPlainText("[ERROR] Please fill in the input file path.");
                return;
            }

            if (_starlessOutputEdit->text().isEmpty())
            {
                _logEdit->appendPlainText("[ERROR] Please fill in the starless output file path.");
                return;
            }

            _processButton->setEnabled(false);
            _runner->setExecutable(executableEdit->text());

            StarNetRunOptions options;
            options.inputFile = _inputEdit->text();
            options.starlessOutput = _starlessOutputEdit->text();
            options.maskOutput = maskCheckBox->isChecked() ? _maskOutputEdit->text() : "";
            options.starsOutput = starsCheckBox->isChecked() ? _starsOutputEdit->text() : "";
            options.stride = strideCheckBox->isChecked() ? std::optional<int>(strideSpinBox->value()) : std::nullopt;
            options.upsample = upsampleCheckBox->isChecked();
            options.save8Bit = save8BitCheckBox->isChecked();
            options.disableHighlightsProtection = disableHighlightsProtectionCheckBox->isChecked();
            _runner->run(options);
        });

    connect(openOutputFileButton, &QPushButton::clicked, this, [this]()
        {
            QDesktopServices::openUrl(QUrl::fromLocalFile(_starlessOutputEdit->text()));
        });

    connect(openOutputFolderButton, &QPushButton::clicked, this, [this]()
        {
            const QFileInfo fileInfo(_starlessOutputEdit->text());
            const QString folderPath = fileInfo.absolutePath();
            QDesktopServices::openUrl(QUrl::fromLocalFile(folderPath));
        });

    // Cancel
    connect(cancelButton, &QPushButton::clicked, this, [this]()
        {
            _runner->cancel();
            _progressBar->setValue(0);
        });

    // Clear log
    connect(clearLogButton, &QPushButton::clicked, this, [this]()
        {
            _logEdit->clear();
        });
}

bool MainWindow::updateProgress(const QString& output)
{
    static const QRegularExpression regex(R"(Working:\s*(\d+(?:\.\d+)?)%)");

    const auto match = regex.match(output);

    if (!match.hasMatch())
        return false;

    bool ok = false;
    const double progress = match.captured(1).toDouble(&ok);

    if (!ok)
        return false;

    _progressBar->setValue(qRound(progress));

    return true;
}

QSettings MainWindow::settings() const
{
    return QSettings(QCoreApplication::applicationDirPath() + "/settings.ini", QSettings::IniFormat);
}
