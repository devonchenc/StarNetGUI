#pragma once

#include <memory>
#include <QMainWindow>
#include <QSettings>
#include "StarNetRunner.h"

class QLineEdit;
class QCheckBox;
class QProgressBar;
class QPlainTextEdit;
class QLabel;
class QPushButton;
class CFilePathLineEdit;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() = default;

private:
    void setupUI();

    bool updateProgress(const QString& output);

    QSettings settings() const;

private:
    CFilePathLineEdit* _inputEdit{ nullptr };
    QLineEdit* _starlessOutputEdit{ nullptr };
    QLineEdit* _maskOutputEdit{ nullptr };
    QLineEdit* _starsOutputEdit{ nullptr };

    QProgressBar* _progressBar{ nullptr };
    QPlainTextEdit* _logEdit{ nullptr };
    QPushButton* _processButton{ nullptr };

    std::unique_ptr<StarNetRunner> _runner;
};
