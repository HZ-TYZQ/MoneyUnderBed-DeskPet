#include "ui/TrayIcon.h"

#include "core/AppMetadata.h"
#include "dialogue/DialogueData.h"

#include <QAction>
#include <QIcon>
#include <QLoggingCategory>
#include <QMenu>
#include <QSystemTrayIcon>

namespace mub::ui {

namespace {

Q_LOGGING_CATEGORY(lcTray, "mub.ui.tray")

// 应用与托盘图标使用作者头像素材中的 `natural`（平静睁眼），
// 由项目所有者于 2026-08-27 选定。第 6 节要求图标取自现有角色素材，
// 不自行绘制新形象；这里直接使用已登记的原件副本，未生成衍生文件，
// 因此不需要在 assets/MANIFEST.md 增加衍生素材条目。
} // namespace

bool TrayIcon::isAvailable()
{
    return QSystemTrayIcon::isSystemTrayAvailable();
}

TrayIcon::TrayIcon(QObject *parent)
    : QObject(parent)
{
    if (!isAvailable()) {
        // 原生 GNOME 通常没有托盘。这是主要降级路径，不是错误。
        qCInfo(lcTray) << "no system tray available; relying on relaunch to recall";
        return;
    }

    menu_ = std::make_unique<QMenu>();
    QMenu *menu = menu_.get();

    show_ = menu->addAction(tr("显示角色"));
    active_ = menu->addAction(tr("活跃模式"));
    active_->setCheckable(true);
    menu->addSeparator();
    QAction *quit = menu->addAction(tr("退出"));

    connect(show_, &QAction::triggered, this, &TrayIcon::showCharacterRequested);
    connect(active_, &QAction::toggled, this, [this](const bool checked) {
        emit modeChangeRequested(checked ? core::ActivityMode::Active
                                         : core::ActivityMode::Quiet);
    });
    connect(quit, &QAction::triggered, this, &TrayIcon::quitRequested);

    tray_ = new QSystemTrayIcon(this);
    tray_->setIcon(QIcon(dialogue::faceAssetPath(QStringLiteral("natural"))));
    tray_->setToolTip(metadata::displayName());
    tray_->setContextMenu(menu);

    // 左键点击托盘图标等同于「显示角色」，这是桌面上的普遍预期。
    connect(tray_, &QSystemTrayIcon::activated, this,
            [this](const QSystemTrayIcon::ActivationReason reason) {
                if (reason == QSystemTrayIcon::Trigger) {
                    emit showCharacterRequested();
                }
            });
    tray_->show();
}

TrayIcon::~TrayIcon() = default;

bool TrayIcon::isActive() const
{
    return tray_ != nullptr;
}

void TrayIcon::setMode(const core::ActivityMode mode)
{
    if (active_ == nullptr) {
        return;
    }
    QSignalBlocker blocker(active_);
    active_->setChecked(mode == core::ActivityMode::Active);
}

void TrayIcon::setCharacterVisible(const bool visible)
{
    if (show_ == nullptr) {
        return;
    }
    show_->setEnabled(!visible);
}

} // namespace mub::ui
