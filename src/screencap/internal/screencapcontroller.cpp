/*
 * Audacity: A Digital Audio Editor
 */
#include "screencapcontroller.h"

#include <algorithm>
#include <cmath>

#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QQuickItem>
#include <QQuickWindow>

#include "framework/uicomponents/qml/Muse/UiComponents/windowview.h"

#include "../screencapcommands.h"

#include "log.h"

using namespace au::screencap;
using namespace muse;
using namespace muse::rcommand;
using muse::uicomponents::WindowView;

static Ret error(const std::string& text)
{
    return make_ret(Ret::Code::BadArgs, text);
}

static Response answer(const Request& request, const Val& data)
{
    return make_response(request, make_ok(), std::any(data));
}

static std::string param(const Request& request, const std::string& key)
{
    return request.params.at(key).toString();
}

ScreencapController::ScreencapController(const modularity::ContextPtr& ctx)
    : muse::Contextable(ctx)
{
}

void ScreencapController::init()
{
    auto cd = commandDispatcher();
    cd->onRequest(this, SCREENCAP_LOOKS_COMMAND, [this](const Request& r) { return looks(r); });
    cd->onRequest(this, SCREENCAP_LOOK_COMMAND, [this](const Request& r) { return setLook(r); });
    cd->onRequest(this, SCREENCAP_CONTROLS_COMMAND, [this](const Request& r) { return controls(r); });
    cd->onRequest(this, SCREENCAP_TRIGGER_COMMAND, [this](const Request& r) { return trigger(r); });
    cd->onRequest(this, SCREENCAP_RECT_COMMAND, [this](const Request& r) { return rect(r); });
    cd->onRequest(this, SCREENCAP_POPUPS_COMMAND,
                  ICommandDispatcher::AsyncCallBack([this](const Request& r, const OnResponse& on) { popups(r, on); }));
    cd->onRequest(this, SCREENCAP_CLOSE_POPUPS_COMMAND, [this](const Request& r) { return closePopups(r); });
    cd->onRequest(this, SCREENCAP_CAPTURE_COMMAND, [this](const Request& r) { return capture(r); });
    cd->onRequest(this, SCREENCAP_DISPATCH_COMMAND,
                  ICommandDispatcher::AsyncCallBack([this](const Request& r, const OnResponse& on) { dispatch(r, on); }));
    cd->onRequest(this, SCREENCAP_USER_INPUT_COMMAND, [this](const Request& r) { return setUserInput(r); });

    //! NOTE Watch every window being shown, to know when a popup has drawn its first frame
    qApp->installEventFilter(this);

    //! NOTE From the start: some marks the pointer leaves are app state, not hover (the
    //! ruler keeps showing the last pointer position), and nothing clears them later
    m_userInputBlocker.setBlocking(true);
}

bool ScreencapController::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::Show || event->type() == QEvent::Hide) {
        if (QQuickWindow* window = qobject_cast<QQuickWindow*>(watched)) {
            m_rendered.erase(window);
            if (event->type() == QEvent::Show) {
                m_shownOrder[window] = ++m_showCounter;
                //! NOTE frameSwapped comes from the render thread; the receiver context queues it here
                connect(window, &QQuickWindow::frameSwapped, this, [this, window]() {
                    if (window->isVisible()) {
                        m_rendered.insert(window);
                        answerPopupWaiters();
                    }
                }, static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::SingleShotConnection));
            } else {
                m_shownOrder.erase(window);
            }
        }
    }
    return QObject::eventFilter(watched, event);
}

// ---- Look ----

Response ScreencapController::looks(const Request& request) const
{
    ValList themes;
    for (const ui::ThemeInfo& theme : uiConfiguration()->themes()) {
        themes.push_back(Val(theme.codeKey));
    }
    ValList accents;
    for (const QString& color : uiConfiguration()->possibleAccentColors()) {
        accents.push_back(Val(color.toLower()));
    }
    return answer(request, Val(ValMap { { "themes", Val(themes) }, { "accents", Val(accents) } }));
}

Response ScreencapController::setLook(const Request& request)
{
    const std::string theme = param(request, "theme");
    const ui::ThemeList themes = uiConfiguration()->themes();
    const bool knownTheme = std::any_of(themes.begin(), themes.end(), [&](const ui::ThemeInfo& t) { return t.codeKey == theme; });
    if (!knownTheme) {
        return make_response(request, error("unknown theme: " + theme));
    }

    const ui::ThemeCode previousTheme = uiConfiguration()->currentTheme().codeKey;
    uiConfiguration()->setFollowSystemTheme(false);
    uiConfiguration()->setCurrentTheme(theme);

    //! NOTE The accent palette depends on the theme: check the accent against the new theme's
    if (request.params.contains("accent")) {
        const QString accent = QString::fromStdString(param(request, "accent")).toLower();
        const QStringList accents = uiConfiguration()->possibleAccentColors();
        const bool knownAccent = std::any_of(accents.begin(), accents.end(), [&](const QString& c) { return c.toLower() == accent; });
        if (!knownAccent) {
            uiConfiguration()->setCurrentTheme(previousTheme);
            return make_response(request, error("not an accent color of theme " + theme + ": " + accent.toStdString()));
        }
        uiConfiguration()->setCurrentThemeStyleValue(ui::ThemeStyleKey::ACCENT_COLOR, Val(accent));
    }
    return make_response(request, make_ok());
}

// ---- Navigation ----

static Val navigationItem(const ui::INavigation* nav)
{
    return Val(ValMap {
        { "name", Val(nav->name()) },
        { "enabled", Val(nav->enabled()) },
        { "active", Val(nav->active()) },
    });
}

Response ScreencapController::controls(const Request& request) const
{
    const std::string section = param(request, "section");
    const std::string panel = param(request, "panel");

    ValList items;
    for (ui::INavigationSection* s : navigation()->sections()) {
        if (section.empty()) {
            items.push_back(navigationItem(s));
            continue;
        }
        if (s->name().toStdString() != section) {
            continue;
        }
        for (ui::INavigationPanel* p : s->panels()) {
            if (panel.empty()) {
                items.push_back(navigationItem(p));
            } else if (p->name().toStdString() == panel) {
                for (ui::INavigationControl* c : p->controls()) {
                    items.push_back(navigationItem(c));
                }
            }
        }
    }
    return answer(request, Val(ValMap { { "items", Val(items) } }));
}

ui::INavigationControl* ScreencapController::findControl(const Request& request, Ret& ret) const
{
    const std::string section = param(request, "section");
    const std::string panel = param(request, "panel");
    const std::string control = param(request, "control");

    for (ui::INavigationSection* s : navigation()->sections()) {
        if (s->name().toStdString() != section) {
            continue;
        }
        for (ui::INavigationPanel* p : s->panels()) {
            if (p->name().toStdString() != panel) {
                continue;
            }
            for (ui::INavigationControl* c : p->controls()) {
                if (c->name().toStdString() == control) {
                    ret = make_ok();
                    return c;
                }
            }
        }
    }
    ret = error("no control " + section + " / " + panel + " / " + control);
    return nullptr;
}

Response ScreencapController::trigger(const Request& request)
{
    Ret ret;
    ui::INavigationControl* control = findControl(request, ret);
    if (!control) {
        return make_response(request, ret);
    }
    if (!control->enabled()) {
        return make_response(request, error("control is disabled: " + control->name().toStdString()));
    }
    control->trigger();
    return make_response(request, make_ok());
}

Response ScreencapController::rect(const Request& request) const
{
    Ret ret;
    const ui::INavigationControl* control = findControl(request, ret);
    if (!control) {
        return make_response(request, ret);
    }
    const QQuickItem* item = control->visualItem();
    if (!item || !item->window()) {
        return make_response(request, error("control has no visual item: " + control->name().toStdString()));
    }

    const QRectF sceneRect = item->mapRectToScene(QRectF(0, 0, item->width(), item->height()));
    const QRect r = toCaptureRect(item->window(), sceneRect);
    return answer(request, Val(ValMap {
        { "x", Val(r.x()) }, { "y", Val(r.y()) }, { "width", Val(r.width()) }, { "height", Val(r.height()) },
    }));
}

// ---- Popups ----

std::vector<WindowView*> ScreencapController::openViews() const
{
    //! NOTE A WindowView owns its window as the window's QObject parent. Going from the
    //! window finds every view; searching item trees misses views declared inside docks,
    //! whose content isn't a QObject descendant of the main window's content item.
    std::vector<WindowView*> views;
    for (QWindow* window : QGuiApplication::allWindows()) {
        WindowView* view = qobject_cast<WindowView*>(window->QObject::parent());
        if (view && view->isOpened()) {
            views.push_back(view);
        }
    }

    //! NOTE Oldest first: the order they are stacked in
    std::sort(views.begin(), views.end(), [this](const WindowView* a, const WindowView* b) {
        auto order = [this](const WindowView* v) {
            auto it = m_shownOrder.find(v->window());
            return it != m_shownOrder.end() ? it->second : 0;
        };
        return order(a) < order(b);
    });
    return views;
}

bool ScreencapController::anyOpenViewRendered() const
{
    for (const WindowView* view : openViews()) {
        if (m_rendered.count(view->window())) {
            return true;
        }
    }
    return false;
}

Val ScreencapController::popupList() const
{
    ValList list;
    for (const WindowView* view : openViews()) {
        const QWindow* window = view->window();
        const QRect r = toCaptureRect(window, QRectF(0, 0, window->width(), window->height()));
        list.push_back(Val(ValMap {
            { "name", Val(view->objectName()) },
            { "title", Val(window->title()) },
            { "rendered", Val(m_rendered.count(window) > 0) },
            { "x", Val(r.x()) }, { "y", Val(r.y()) }, { "width", Val(r.width()) }, { "height", Val(r.height()) },
        }));
    }
    return Val(ValMap { { "popups", Val(list) } });
}

void ScreencapController::popups(const Request& request, const OnResponse& onResponse)
{
    if (!request.params.at("wait").toBool() || anyOpenViewRendered()) {
        onResponse(answer(request, popupList()));
        return;
    }
    m_popupWaiters.emplace_back(request, onResponse);
}

void ScreencapController::answerPopupWaiters()
{
    if (m_popupWaiters.empty() || !anyOpenViewRendered()) {
        return;
    }
    const Val list = popupList();
    auto waiters = std::move(m_popupWaiters);
    m_popupWaiters.clear();
    for (const auto& [request, onResponse] : waiters) {
        onResponse(answer(request, list));
    }
}

Response ScreencapController::closePopups(const Request& request)
{
    std::vector<WindowView*> views = openViews();
    //! NOTE Newest first: a popup opened from another closes before its opener
    for (auto it = views.rbegin(); it != views.rend(); ++it) {
        (*it)->close(/*force*/ true);
    }
    return answer(request, Val(ValMap { { "closed", Val(static_cast<int>(views.size())) } }));
}

// ---- Capture ----

QQuickWindow* ScreencapController::mainQuickWindow() const
{
    return qobject_cast<QQuickWindow*>(mainWindow()->qWindow());
}

QRect ScreencapController::toCaptureRect(const QWindow* window, const QRectF& windowRect) const
{
    const QWindow* main = mainWindow()->qWindow();
    const qreal dpr = main->devicePixelRatio();
    const QPointF offset = window->geometry().topLeft() - main->geometry().topLeft();
    const QRectF r = windowRect.translated(offset);
    return QRect(QPoint(std::lround(r.left() * dpr), std::lround(r.top() * dpr)),
                 QSize(std::lround(r.width() * dpr), std::lround(r.height() * dpr)));
}

Response ScreencapController::capture(const Request& request)
{
    const QString path = QString::fromStdString(param(request, "path"));
    const QFileInfo file(path);
    if (path.isEmpty() || !file.isAbsolute() || !file.absoluteDir().exists()) {
        return make_response(request, error("path must be absolute and its folder must exist: " + path.toStdString()));
    }

    QQuickWindow* main = mainQuickWindow();
    IF_ASSERT_FAILED(main) {
        return make_response(request, make_ret(Ret::Code::InternalError));
    }

    //! NOTE Work in device pixels: drop the images' device pixel ratio so QPainter doesn't scale
    QImage image = main->grabWindow().convertToFormat(QImage::Format_ARGB32_Premultiplied);
    image.setDevicePixelRatio(1.0);
    {
        QPainter painter(&image);
        for (const WindowView* view : openViews()) {
            QQuickWindow* window = qobject_cast<QQuickWindow*>(view->window());
            if (!window) {
                continue;
            }
            QImage popup = window->grabWindow();
            popup.setDevicePixelRatio(1.0);
            painter.drawImage(toCaptureRect(window, QRectF(0, 0, window->width(), window->height())).topLeft(), popup);
        }
    }

    const bool cropped = request.params.contains("x") || request.params.contains("y")
                         || request.params.contains("width") || request.params.contains("height");
    if (cropped) {
        const QRect crop(request.params.at("x").toInt(), request.params.at("y").toInt(),
                         request.params.at("width").toInt(), request.params.at("height").toInt());
        if (crop.isEmpty() || !image.rect().contains(crop)) {
            return make_response(request, error("crop is outside the capture (" + std::to_string(image.width()) + "x"
                                                + std::to_string(image.height()) + ")"));
        }
        image = image.copy(crop);
    }

    if (!image.save(path, "PNG")) {
        return make_response(request, make_ret(Ret::Code::InternalError, "could not write " + path.toStdString()));
    }
    return answer(request, Val(ValMap {
        { "path", Val(path) }, { "width", Val(image.width()) }, { "height", Val(image.height()) },
    }));
}

// ---- Dispatch ----

void ScreencapController::dispatch(const Request& request, const OnResponse& onResponse)
{
    const std::string uri = param(request, "uri");
    if (uri.rfind("command://", 0) == 0) {
        commandDispatcher()->dispatch(CommandQuery(uri)).onResolve(this, [request, onResponse](const Response& response) {
            onResponse(make_response(request, response.ret, response.data));
        });
        return;
    }
    if (uri.rfind("action://", 0) == 0) {
        //! NOTE Actions answer nothing; their handlers run synchronously
        actionsDispatcher()->dispatch(actions::ActionQuery(uri));
        onResponse(make_response(request, make_ok()));
        return;
    }
    onResponse(make_response(request, error("not a command:// or action:// URI: " + uri)));
}

// ---- User input ----

Response ScreencapController::setUserInput(const Request& request)
{
    if (!UserInputBlocker::isSupported()) {
        return make_response(request, error("ignoring the user's input isn't supported on this platform"));
    }
    m_userInputBlocker.setBlocking(!request.params.at("enabled").toBool());
    return make_response(request, make_ok());
}
