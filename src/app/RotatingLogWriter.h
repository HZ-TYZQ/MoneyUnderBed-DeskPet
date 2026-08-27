#pragma once

#include <QFile>
#include <QString>

namespace mub::app {

// 固定两代、严格字节上限的本地日志写入器。
// 不做任何联网或上传；调用方只传入已经筛选过的诊断文本。
class RotatingLogWriter
{
public:
    RotatingLogWriter(QString path, qint64 maxBytes);

    bool open();
    bool writeLine(const QByteArray &line);
    QString path() const;

private:
    bool rotate();

    QString path_;
    qint64 maxBytes_;
    QFile file_;
};

} // namespace mub::app
