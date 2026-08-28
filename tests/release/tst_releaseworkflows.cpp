#include <QFile>
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <QTest>

// 发布用工作流的接线断言。
//
// 计划第 10 节要求：`v1.1.0` 标签必须同时运行 Build and test 与 Package
// candidates。这两件事只写在 YAML 里，没有任何编译期约束，改坏了要等到真正
// 打标签时才发现——而那时标签已经推出去了。本测试把它变成本地就能失败的检查。
//
// 本测试只做文本断言，不解析完整 YAML：需要守住的是「标签过滤是否存在、是否
// 一致」，而不是 YAML 语义。
class TestReleaseWorkflows final : public QObject
{
    Q_OBJECT

private slots:
    void bothReleaseWorkflowsTriggerOnVersionTags();
    void theTagFilterMatchesTheReleaseTag();
    void theRetiredProbeWorkflowIsGone();
    void theCurrentChecklistDoesNotDependOnTheProbe();
};

namespace {

QString workflowRoot()
{
    return QStringLiteral(MUB_SOURCE_ROOT "/.github/workflows/");
}

QString readFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }
    return QString::fromUtf8(file.readAll());
}

// 取出 `on:` 块里 `tags:` 下的过滤式。只看第一个 `tags:`，正式工作流不会有第二个。
QStringList tagFilters(const QString &yaml)
{
    QStringList filters;
    const QStringList lines = yaml.split(QLatin1Char('\n'));
    qsizetype index = 0;
    for (; index < lines.size(); ++index) {
        if (lines.at(index).trimmed() == QLatin1String("tags:")) {
            break;
        }
    }
    for (++index; index < lines.size(); ++index) {
        const QString trimmed = lines.at(index).trimmed();
        if (trimmed.isEmpty() || trimmed.startsWith(QLatin1Char('#'))) {
            continue;
        }
        if (!trimmed.startsWith(QLatin1String("- "))) {
            break;
        }
        QString value = trimmed.mid(2).trimmed();
        value.remove(QLatin1Char('"'));
        value.remove(QLatin1Char('\''));
        filters.append(value);
    }
    return filters;
}

// GitHub 的标签过滤用 glob，不是正则。这里只需要覆盖工作流实际使用的 `*`。
bool globMatches(const QString &pattern, const QString &text)
{
    QString expression;
    for (const QChar character : pattern) {
        if (character == QLatin1Char('*')) {
            expression += QLatin1String("[^/]*");
        } else {
            expression += QRegularExpression::escape(character);
        }
    }
    const QRegularExpression regex(QLatin1Char('^') + expression + QLatin1Char('$'));
    return regex.match(text).hasMatch();
}

} // namespace

void TestReleaseWorkflows::bothReleaseWorkflowsTriggerOnVersionTags()
{
    for (const QString &name : {QStringLiteral("build.yml"), QStringLiteral("package.yml")}) {
        const QString yaml = readFile(workflowRoot() + name);
        QVERIFY2(!yaml.isEmpty(), qPrintable(name + QStringLiteral(" is missing")));
        const QStringList filters = tagFilters(yaml);
        QVERIFY2(!filters.isEmpty(),
                 qPrintable(name + QStringLiteral(" has no tag trigger")));
    }

    // 两条流水线的过滤式必须逐字一致，否则会出现只跑一半的标签。
    QCOMPARE(tagFilters(readFile(workflowRoot() + QStringLiteral("build.yml"))),
             tagFilters(readFile(workflowRoot() + QStringLiteral("package.yml"))));
}

void TestReleaseWorkflows::theTagFilterMatchesTheReleaseTag()
{
    const QStringList filters =
        tagFilters(readFile(workflowRoot() + QStringLiteral("build.yml")));

    for (const QString &tag : {QStringLiteral("v1.1.0"), QStringLiteral("v1.2.3")}) {
        bool matched = false;
        for (const QString &filter : filters) {
            matched = matched || globMatches(filter, tag);
        }
        QVERIFY2(matched, qPrintable(QStringLiteral("no filter matches %1").arg(tag)));
    }
}

void TestReleaseWorkflows::theRetiredProbeWorkflowIsGone()
{
    // 阶段 1 的窗口探针工作流已随阶段 5 删除：它只构建 probes/window/，不监听
    // 标签，却被 1.0 清单列为产物门。probes/window/ 的源码作为
    // docs/WindowsFeasibilityResults.md 的证据来源保留，但不再由 CI 构建。
    QVERIFY(!QFile::exists(workflowRoot() + QStringLiteral("probe-windows.yml")));
}

void TestReleaseWorkflows::theCurrentChecklistDoesNotDependOnTheProbe()
{
    const QString checklist =
        readFile(QStringLiteral(MUB_SOURCE_ROOT "/docs/ReleaseChecklist-1.1.md"));
    QVERIFY2(!checklist.isEmpty(), "docs/ReleaseChecklist-1.1.md is missing");

    // 只看勾选项。说明段落里解释「这一项为什么没了」是允许的，成为验收条件
    // 才是问题——1.0 清单正是把它写成了第 1 节的产物门。
    for (const QString &line : checklist.split(QLatin1Char('\n'))) {
        const QString trimmed = line.trimmed();
        if (!trimmed.startsWith(QLatin1String("- [")))  {
            continue;
        }
        QVERIFY2(!trimmed.contains(QStringLiteral("probe"))
                     && !trimmed.contains(QStringLiteral("探针")),
                 qPrintable(QStringLiteral("checklist item still gates on the "
                                           "retired window probe: %1")
                                .arg(trimmed)));
    }
}

QTEST_APPLESS_MAIN(TestReleaseWorkflows)

#include "tst_releaseworkflows.moc"
