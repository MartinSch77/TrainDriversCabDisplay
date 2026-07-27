#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QTimer>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("RailDeck Pro"));
    QGuiApplication::setOrganizationName(QStringLiteral("RailDeck"));

    QQmlApplicationEngine engine;
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
                     [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);
    engine.loadFromModule("RailDeck", "Main");

    // Headless verification: raildeck-qt --screenshot out.png [--screenshot-delay ms]
    const QStringList args = QCoreApplication::arguments();
    const int shotIdx = args.indexOf(QStringLiteral("--screenshot"));
    if (shotIdx >= 0 && shotIdx + 1 < args.size() && !engine.rootObjects().isEmpty()) {
        const QString &path = args.at(shotIdx + 1);
        const int delayIdx = args.indexOf(QStringLiteral("--screenshot-delay"));
        const int delayMs = (delayIdx >= 0 && delayIdx + 1 < args.size())
                                ? args.at(delayIdx + 1).toInt()
                                : 1500;
        auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().constFirst());
        if (window != nullptr) {
            QTimer::singleShot(delayMs, window, [window, path] {
                window->grabWindow().save(path);
                QCoreApplication::quit();
            });
        }
    }

    return QGuiApplication::exec();
}
