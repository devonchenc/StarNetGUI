#pragma once

#include <optional>
#include <QObject>
#include <QProcess>
#include <QString>

struct StarNetRunOptions
{
    QString inputFile;
    QString starlessOutput;
    QString maskOutput;
    QString starsOutput;
    std::optional<int> stride;
    bool upsample{ false };
    bool save8Bit{ false };
    bool disableHighlightsProtection{ false };
};

class StarNetRunner : public QObject
{
    Q_OBJECT

public:
    explicit StarNetRunner(QObject* parent = nullptr);
    ~StarNetRunner() override;

    void setExecutable(const QString& executable);

    bool isRunning() const;

public slots:
    void run(const StarNetRunOptions& options);

    void cancel();

signals:
    void started();
    void finished(int exitCode);

    void outputReceived(const QString& output);
    void errorReceived(const QString& error);

private slots:
    void onStarted();
    void onFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onReadyReadStandardOutput();
    void onReadyReadStandardError();
    void onErrorOccurred(QProcess::ProcessError error);

private:
    QProcess* _process{ nullptr };
    QString _executable;
};
