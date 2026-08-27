#include "app/DiagnosticPrivacy.h"

#include <QDir>
#include <QRegularExpression>

namespace mub::app {

QString redactDiagnosticText(QString text)
{
    const QString home = QDir::homePath();
    if (home.size() > 1) {
        text.replace(home, QStringLiteral("<home>"), Qt::CaseSensitive);
        const QString nativeHome = QDir::toNativeSeparators(home);
        if (nativeHome != home) {
            text.replace(nativeHome, QStringLiteral("<home>"), Qt::CaseInsensitive);
        }
    }

    static const QRegularExpression environmentAssignment(
        QStringLiteral(R"(\b(PATH|HOME|USER|USERNAME)=(("[^"]*")|('[^']*')|\S+))"));
    text.replace(environmentAssignment, QStringLiteral("\\1=<redacted>"));
    return text;
}

} // namespace mub::app
