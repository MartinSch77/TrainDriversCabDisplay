#include "trainbackend.h"

#include <QVariantMap>

using namespace traincore;

TrainBackend::TrainBackend(QObject *parent)
    : QObject(parent)
{
    m_elapsed.start();
    connect(&m_timer, &QTimer::timeout, this, [this] {
        const double dt = m_elapsed.restart() / 1000.0;
        m_sim.tick(dt);
        emit stateChanged();
    });
    m_timer.start(33); // ~30 Hz UI refresh
}

void TrainBackend::send(Cmd cmd)
{
    m_sim.command(static_cast<Command>(cmd));
    emit stateChanged();
}

void TrainBackend::setLever(double percent)
{
    m_sim.setLever(percent);
    emit stateChanged();
}

QString TrainBackend::clockText() const
{
    const TrainState &s = m_sim.state();
    return QStringLiteral("%1:%2:%3")
        .arg(s.clockHour, 2, 10, QLatin1Char('0'))
        .arg(s.clockMinute, 2, 10, QLatin1Char('0'))
        .arg(s.clockSecond, 2, 10, QLatin1Char('0'));
}

QVariantList TrainBackend::routeProfile() const
{
    QVariantList list;
    for (const RouteSegment &seg : m_sim.route()) {
        QVariantMap m;
        m.insert(QStringLiteral("startM"), seg.startM);
        m.insert(QStringLiteral("limitKmh"), seg.limitKmh);
        m.insert(QStringLiteral("gradientPermille"), seg.gradientPermille);
        list.append(m);
    }
    return list;
}

QVariantList TrainBackend::routeStations() const
{
    QVariantList list;
    for (const RouteStation &st : m_sim.stations()) {
        QVariantMap m;
        m.insert(QStringLiteral("positionM"), st.positionM);
        m.insert(QStringLiteral("name"), QString::fromStdString(st.name));
        list.append(m);
    }
    return list;
}

QVariantList TrainBackend::alerts() const
{
    QVariantList list;
    for (const Alert &a : m_sim.state().alerts) {
        QVariantMap m;
        m.insert(QStringLiteral("severity"), static_cast<int>(a.severity));
        m.insert(QStringLiteral("text"), QString::fromStdString(a.text));
        list.append(m);
    }
    return list;
}
