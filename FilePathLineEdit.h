#pragma once

#include <QLineEdit>
#include <QMimeData>

class CFilePathLineEdit : public QLineEdit
{
public:
    explicit CFilePathLineEdit(QWidget* parent = nullptr)
        : QLineEdit(parent)
    {
        setAcceptDrops(true);
    }

protected:
    void dragEnterEvent(QDragEnterEvent* event) override
    {
        if (event->mimeData()->hasUrls())
        {
            event->acceptProposedAction();
        }
    }

    void dropEvent(QDropEvent* event) override
    {
        const auto urls = event->mimeData()->urls();

        if (!urls.isEmpty())
        {
            const QString filePath = urls.first().toLocalFile();

            if (!filePath.isEmpty())
            {
                setText(filePath);
                event->acceptProposedAction();
            }
        }
    }
};
