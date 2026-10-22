/*
 * Audacity: A Digital Audio Editor
 */
#pragma once

#include <map>
#include <set>
#include <vector>

#include <QObject>
#include <QPointer>
#include <QRect>

#include "framework/global/async/asyncable.h"
#include "framework/global/modularity/ioc.h"
#include "framework/rcommand/commandable.h"
#include "framework/rcommand/icommanddispatcher.h"
#include "framework/actions/iactionsdispatcher.h"
#include "framework/ui/inavigationcontroller.h"
#include "framework/ui/iuiconfiguration.h"
#include "framework/ui/imainwindow.h"

#include "userinputblocker.h"

class QQuickWindow;
class QWindow;

namespace muse::uicomponents {
class WindowView;
}

namespace au::screencap {
//! Handles the screencap commands. "Capture coordinates" are device pixels from
//! the main window's top-left corner: what capture() writes and rect() returns.
class ScreencapController : public QObject, public muse::rcommand::Commandable, public muse::Contextable,
    public muse::async::Asyncable
{
    Q_OBJECT

    muse::ContextInject<muse::rcommand::ICommandDispatcher> commandDispatcher{ this };
    muse::ContextInject<muse::actions::IActionsDispatcher> actionsDispatcher{ this };
    muse::ContextInject<muse::ui::INavigationController> navigation{ this };
    muse::ContextInject<muse::ui::IMainWindow> mainWindow{ this };
    muse::GlobalInject<muse::ui::IUiConfiguration> uiConfiguration;

public:
    explicit ScreencapController(const muse::modularity::ContextPtr& ctx);

    void init();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    using Request = muse::rcommand::Request;
    using Response = muse::rcommand::Response;
    using OnResponse = muse::rcommand::ICommandDispatcher::OnResponse;

    Response looks(const Request& request) const;
    Response setLook(const Request& request);
    Response controls(const Request& request) const;
    Response trigger(const Request& request);
    Response rect(const Request& request) const;
    void popups(const Request& request, const OnResponse& onResponse);
    Response closePopups(const Request& request);
    Response capture(const Request& request);
    void dispatch(const Request& request, const OnResponse& onResponse);
    Response setUserInput(const Request& request);

    muse::ui::INavigationControl* findControl(const Request& request, muse::Ret& ret) const;
    std::vector<muse::uicomponents::WindowView*> openViews() const;
    bool anyOpenViewRendered() const;
    muse::Val popupList() const;
    void answerPopupWaiters();

    QQuickWindow* mainQuickWindow() const;
    QRect toCaptureRect(const QWindow* window, const QRectF& windowRect) const;

    std::map<const QWindow*, uint64_t> m_shownOrder;
    uint64_t m_showCounter = 0;
    std::set<const QWindow*> m_rendered;
    std::vector<std::pair<Request, OnResponse> > m_popupWaiters;
    UserInputBlocker m_userInputBlocker;
};
}
