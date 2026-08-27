#pragma once

#include <QDialog>

class QPlainTextEdit;

namespace mub::platform {
struct BackendCapabilities;
}

namespace mub::ui {

// 关于窗口。
//
// docs/Decisions.md 第 5.3 节要求展示：程序版本、代码许可证、第三方字体
// 及其许可证、素材作者、原视频链接、素材限制和非官方声明；并提供
// 「复制诊断信息」。
//
// 素材来源与限制不是可选说明：第 12.3 节规定项目必须保留作者、原视频、
// 原网盘和素材授权说明，第 12.4 节的 R18 红线同样在此展示。
class AboutWindow final : public QDialog
{
    Q_OBJECT

public:
    AboutWindow(QString backendName, bool trayAvailable, QWidget *parent = nullptr);

private:
    void copyDiagnostics();

    QString backendName_;
    bool trayAvailable_ = false;
};

// 诊断信息文本。
//
// 第 5.3 节：包含程序版本、Qt 版本、系统、窗口后端、屏幕信息和托盘可用性；
// **不得**包含完整环境变量、用户文件内容或其他无关隐私。
// 单独暴露出来是为了让内容可以被测试逐条核对。
QString diagnosticsText(const QString &backendName, bool trayAvailable);

} // namespace mub::ui
