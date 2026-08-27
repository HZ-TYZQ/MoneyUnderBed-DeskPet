#include "app/RotatingLogWriter.h"

#include <QFileInfo>

#include <algorithm>
#include <utility>

namespace mub::app {

RotatingLogWriter::RotatingLogWriter(QString path, const qint64 maxBytes)
    : path_(std::move(path))
    , maxBytes_(std::max<qint64>(1, maxBytes))
    , file_(path_)
{
}

bool RotatingLogWriter::open()
{
    if (file_.isOpen()) {
        return true;
    }
    if (QFileInfo(path_).size() >= maxBytes_ && !rotate()) {
        return false;
    }
    file_.setFileName(path_);
    return file_.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
}

bool RotatingLogWriter::writeLine(const QByteArray &line)
{
    if (!open()) {
        return false;
    }

    // 一条异常巨大的消息也不能突破边界；保留尾部换行供诊断工具逐行读取。
    QByteArray bounded = line.left(std::max<qint64>(0, maxBytes_ - 1));
    bounded.append('\n');
    if (file_.size() > 0 && file_.size() + bounded.size() > maxBytes_) {
        if (!rotate() || !open()) {
            return false;
        }
    }
    const bool ok = file_.write(bounded) == bounded.size();
    file_.flush();
    return ok;
}

QString RotatingLogWriter::path() const
{
    return file_.isOpen() ? path_ : QString();
}

bool RotatingLogWriter::rotate()
{
    file_.close();
    const QString previous = path_ + QStringLiteral(".1");
    if (QFile::exists(previous) && !QFile::remove(previous)) {
        return false;
    }
    if (QFile::exists(path_) && !QFile::rename(path_, previous)) {
        return false;
    }
    // 兼容阶段 7 的旧日志：旧实现只在下次启动时轮转，归档文件可能已经
    // 超过新上限。迁移时同样收紧，保证两代文件都受严格边界约束。
    if (QFileInfo(previous).size() > maxBytes_
        && !QFile::resize(previous, maxBytes_)) {
        return false;
    }
    return true;
}

} // namespace mub::app
