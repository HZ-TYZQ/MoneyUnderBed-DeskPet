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
    void theSourceReleaseTagCannotTriggerThePipeline();
    void theBinaryReleaseNotesPointAtTheSourceRelease();
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

// 对应源码单独成一个 Release，其标签**不得**匹配发布流水线的标签过滤。
// `v1.1.0-sources` 之类的名字会匹配 `v*.*.*` 并触发整条打包流水线，而 metadata
// 只接受 MAJOR.MINOR.PATCH，结果是一次注定失败的运行——与 docs/Decisions.md
// 第 14.1 节记录的 rc 标签陷阱是同一个。
void TestReleaseWorkflows::theSourceReleaseTagCannotTriggerThePipeline()
{
    const QString yaml = readFile(workflowRoot() + QStringLiteral("package.yml"));
    QVERIFY2(!yaml.isEmpty(), "package.yml is missing");

    // 从工作流里取出源码 Release 的标签构造式，避免测试和实现各写一份。
    const QRegularExpression assignment(
        // 自定义分隔符：正则里本身含有 `)"`，默认的 R"( )" 会提前结束。
        QStringLiteral(R"RX(tag="([^"]*)\$VERSION([^"]*)")RX"));
    const QRegularExpressionMatch match = assignment.match(yaml);
    QVERIFY2(match.hasMatch(),
             "package.yml no longer builds the source release tag as tag=\"...$VERSION...\"");

    const QStringList filters = tagFilters(yaml);
    QVERIFY(!filters.isEmpty());

    // 用真实版本号代入，确认成品标签落不进过滤式。
    for (const QString &version : {QStringLiteral("1.1.0"), QStringLiteral("2.0.0"),
                                   QStringLiteral("10.20.30")}) {
        const QString tag = match.captured(1) + version + match.captured(2);
        for (const QString &filter : filters) {
            QVERIFY2(!globMatches(filter, tag),
                     qPrintable(QStringLiteral("source release tag %1 matches the "
                                               "release filter %2 and would trigger "
                                               "a doomed pipeline run")
                                    .arg(tag, filter)));
        }
    }
}

// GPLv3 §6(d)：源码指引必须与目标代码放在一起。源码拆到另一个 Release 之后，
// 二进制 Release 的说明模板里必须留有指向它的链接，否则拆分本身就制造了
// 一次不合规的分发。
void TestReleaseWorkflows::theBinaryReleaseNotesPointAtTheSourceRelease()
{
    const QString notes =
        readFile(QStringLiteral(MUB_SOURCE_ROOT "/packaging/release-notes.md"));
    QVERIFY2(!notes.isEmpty(), "packaging/release-notes.md is missing");
    QVERIFY2(notes.contains(QStringLiteral("@SOURCES_TAG@")),
             "the binary release notes no longer link the corresponding-source release");

    const QString sourceNotes =
        readFile(QStringLiteral(MUB_SOURCE_ROOT "/packaging/sources-release-notes.md"));
    QVERIFY2(!sourceNotes.isEmpty(), "packaging/sources-release-notes.md is missing");

    // 工作流必须真的把占位符替换掉，否则发出去的说明里会留下 `@SOURCES_TAG@`。
    const QString yaml = readFile(workflowRoot() + QStringLiteral("package.yml"));
    QVERIFY2(yaml.contains(QStringLiteral("s/@SOURCES_TAG@/")),
             "package.yml does not substitute @SOURCES_TAG@ before publishing");
    QVERIFY2(yaml.contains(QStringLiteral("s/@VERSION@/")),
             "package.yml does not substitute @VERSION@ before publishing");
}

QTEST_APPLESS_MAIN(TestReleaseWorkflows)

#include "tst_releaseworkflows.moc"
