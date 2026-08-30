#include "StarNetRunner.h"

#include <QDebug>

StarNetRunner::StarNetRunner(QObject* parent)
    : QObject(parent)
    , _process(new QProcess(this))
{
    connect(_process, &QProcess::started, this, &StarNetRunner::onStarted);
    connect(_process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this, &StarNetRunner::onFinished);
    connect(_process, &QProcess::readyReadStandardOutput, this, &StarNetRunner::onReadyReadStandardOutput);
    connect(_process, &QProcess::readyReadStandardError, this, &StarNetRunner::onReadyReadStandardError);
    connect(_process, &QProcess::errorOccurred, this, &StarNetRunner::onErrorOccurred);
}

StarNetRunner::~StarNetRunner()
{
    if (_process->state() != QProcess::NotRunning)
    {
        _process->kill();
        _process->waitForFinished(1000);
    }
}

void StarNetRunner::setExecutable(const QString& executable)
{
    _executable = executable;
}

bool StarNetRunner::isRunning() const
{
    return _process->state() != QProcess::NotRunning;
}

void StarNetRunner::run(const StarNetRunOptions& options)
{
    if (isRunning())
    {
        emit errorReceived(QStringLiteral("StarNet2 is already running."));
        return;
    }

    if (_executable.isEmpty())
    {
        emit errorReceived(QStringLiteral("StarNet2 executable is not configured."));
        return;
    }

    QStringList arguments;
    arguments << "--input" << options.inputFile << "--output" << options.starlessOutput;

    if (!options.maskOutput.isEmpty())
    {
        arguments << "--mask" << options.maskOutput;
    }

    if (!options.starsOutput.isEmpty())
    {
        arguments << "--unscreen" << options.starsOutput;
    }

    if (options.stride.has_value())
    {
        arguments << "--stride" << QString::number(options.stride.value());
    }

    if (options.upsample)
    {
        arguments << "--upsample";
    }

    if (options.save8Bit)
    {
        arguments << "--eight";
    }

    if (options.disableHighlightsProtection)
    {
        arguments << "--disable-highlights-protection";
    }

    _process->start(_executable, arguments);
}

void StarNetRunner::cancel()
{
    if (!isRunning())
        return;

    _process->terminate();

    if (!_process->waitForFinished(1000))
    {
        _process->kill();
    }
}

void StarNetRunner::onStarted()
{
    qDebug() << "StarNet2 started";

    emit started();
}

void StarNetRunner::onFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitStatus == QProcess::CrashExit)
    {
        emit errorReceived(QStringLiteral("StarNet2 terminated unexpectedly."));
    }

    emit finished(exitCode);
}

void StarNetRunner::onReadyReadStandardOutput()
{
    const QByteArray data = _process->readAllStandardOutput();

    qDebug() << "Output:" << QString::fromLocal8Bit(data);

    emit outputReceived(QString::fromLocal8Bit(data));
}

void StarNetRunner::onReadyReadStandardError()
{
    const QByteArray data = _process->readAllStandardError();

    qDebug() << "Error:" << QString::fromLocal8Bit(data);

    emit errorReceived(QString::fromLocal8Bit(data));
}

void StarNetRunner::onErrorOccurred(QProcess::ProcessError error)
{
    QString message;

    switch (error)
    {
    case QProcess::FailedToStart:
        message = QStringLiteral("Failed to start StarNet2.");
        break;

    case QProcess::Crashed:
        message = QStringLiteral("StarNet2 crashed.");
        break;

    case QProcess::Timedout:
        message = QStringLiteral("StarNet2 operation timed out.");
        break;

    case QProcess::WriteError:
        message = QStringLiteral("Failed to write to StarNet2 process.");
        break;

    case QProcess::ReadError:
        message = QStringLiteral("Failed to read from StarNet2 process.");
        break;

    case QProcess::UnknownError:
    default:
        message = QStringLiteral("Unknown StarNet2 process error.");
        break;
    }

    emit errorReceived(message);
}
