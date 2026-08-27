#include "ui/FirstRunWindow.h"

#include <QCheckBox>
#include <QLabel>
#include <QTest>

using mub::ui::FirstRunWindow;

class TestFirstRunWindow final : public QObject
{
    Q_OBJECT

private slots:
    void isASinglePageWithoutWizardSteps();
    void explainsMenuDragAndQuit();
    void doesNotOfferTheDesktopEntryOutsideAppImage();
    void offersTheDesktopEntryUncheckedByDefault();
    void reportsTheDesktopEntryChoice();
};

// 第 5.2 节：首次启动显示简短提示，不做多页欢迎向导。
void TestFirstRunWindow::isASinglePageWithoutWizardSteps()
{
    FirstRunWindow window(false);
    // 只有一段说明文字，没有分步控件。
    QCOMPARE(window.findChildren<QLabel *>().size(), 1);
}

void TestFirstRunWindow::explainsMenuDragAndQuit()
{
    FirstRunWindow window(false);
    const QLabel *text = window.findChild<QLabel *>();
    QVERIFY(text != nullptr);

    // 第 5.2 节点名了这三件事，缺一条就不算说明了操作方式。
    for (const QString &topic : {QStringLiteral("右键"), QStringLiteral("拖动"),
                                 QStringLiteral("退出")}) {
        QVERIFY2(text->text().contains(topic), qPrintable(topic));
    }
}

// 应用菜单入口只对 AppImage 有意义；Windows 免安装 ZIP 不创建快捷方式。
void TestFirstRunWindow::doesNotOfferTheDesktopEntryOutsideAppImage()
{
    FirstRunWindow window(false);
    QCOMPARE(window.findChildren<QCheckBox *>().size(), 0);
    QVERIFY(!window.wantsDesktopEntry());
}

// 第 5.2 节：询问，不静默写入。默认勾上就等于静默写入。
void TestFirstRunWindow::offersTheDesktopEntryUncheckedByDefault()
{
    FirstRunWindow window(true);
    QCheckBox *box = window.findChild<QCheckBox *>();
    QVERIFY(box != nullptr);
    QVERIFY(!box->isChecked());
    QVERIFY(!window.wantsDesktopEntry());
}

void TestFirstRunWindow::reportsTheDesktopEntryChoice()
{
    FirstRunWindow window(true);
    QCheckBox *box = window.findChild<QCheckBox *>();
    QVERIFY(box != nullptr);

    box->setChecked(true);
    QVERIFY(window.wantsDesktopEntry());
    box->setChecked(false);
    QVERIFY(!window.wantsDesktopEntry());
}

QTEST_MAIN(TestFirstRunWindow)
#include "tst_firstrunwindow.moc"
