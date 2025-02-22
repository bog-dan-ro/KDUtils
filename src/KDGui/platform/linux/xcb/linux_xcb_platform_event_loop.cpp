/*
  This file is part of KDUtils.

  SPDX-FileCopyrightText: 2018-2023 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
  Author: Paul Lemire <paul.lemire@kdab.com>

  SPDX-License-Identifier: MIT

  Contact KDAB at <info@kdab.com> for commercial licensing options.
*/

#include "linux_xcb_platform_event_loop.h"
#include "linux_xcb_platform_integration.h"
#include "linux_xkb_keyboard.h"
#include <KDGui/window.h>

#include <KDFoundation/file_descriptor_notifier.h>

#include <KDFoundation/logging.h>

#include <xcb/xcb.h>

using namespace KDFoundation;
using namespace KDGui;

LinuxXcbPlatformEventLoop::LinuxXcbPlatformEventLoop(LinuxXcbPlatformIntegration *platformIntegration)
    : LinuxPlatformEventLoop()
    , m_platformIntegration{ platformIntegration }
{
    m_logger = m_platformIntegration->logger();

    m_keyboard = m_platformIntegration->keyboard();

    // Setup a notifier for the xcb connection. We have to manually register the notifier since
    // at this time, the event loop will not yet be set on the application (we are still in it's
    // constructor).
    const auto connection = m_platformIntegration->connection();
    auto xcbfd = xcb_get_file_descriptor(connection);
    SPDLOG_LOGGER_DEBUG(m_logger, "Registering xcb fd {} with event loop", xcbfd);
    m_xcbNotifier = std::make_unique<FileDescriptorNotifier>(xcbfd, FileDescriptorNotifier::NotificationType::Read);
    m_xcbNotifier->triggered.connect([this](const int &) {
        this->m_xcbEventsPending = true;
    });

    if (!registerNotifier(m_xcbNotifier.get()))
        SPDLOG_LOGGER_CRITICAL(m_logger, "Failed to register xcb fd {} with event loop", xcbfd);
}

LinuxXcbPlatformEventLoop::~LinuxXcbPlatformEventLoop()
{
}

void LinuxXcbPlatformEventLoop::waitForEvents(int timeout)
{
    // Process any xcb events already waiting for us
    processXcbEvents();

    // Call the base class to do the actual multiplexing
    LinuxPlatformEventLoop::waitForEvents(timeout);

    // Now we are awake again, check to see if we have any xcb events to process
    if (!m_xcbEventsPending)
        return;
    processXcbEvents();
    m_xcbEventsPending = false;
}

void LinuxXcbPlatformEventLoop::processXcbEvents()
{
    const auto connection = m_platformIntegration->connection();
    while (auto xcbEvent = xcb_poll_for_event(connection)) {
        // We don't care where the event came from (server or client), so remove that
        // from the event type. See:
        // https://stackoverflow.com/questions/60744214/what-does-the-0x80-mean-in-libxcb-code-sample
        const auto eventType = xcbEvent->response_type & ~0x80;

        switch (eventType) {
        case XCB_EXPOSE: {
            const auto *expose = reinterpret_cast<xcb_expose_event_t *>(xcbEvent);
            SPDLOG_LOGGER_DEBUG(m_logger,
                                "{}: Window {} exposed. Region to be redrawn at location ({}, {}), with dimension ({}, {})",
                                xcbEvent->sequence, expose->window, expose->x, expose->y, expose->width, expose->height);
            break;
        }

        case XCB_CLIENT_MESSAGE: {
            SPDLOG_LOGGER_DEBUG(m_logger, "Received a window client message");
            const auto clientEvent = reinterpret_cast<xcb_client_message_event_t *>(xcbEvent);
            auto window = m_platformIntegration->window(clientEvent->window);
            if (window && clientEvent->data.data32[0] == window->closeAtom()) {
                SPDLOG_LOGGER_DEBUG(m_logger, "Window closed by user");
                // TODO: Process on Window by means of an event so we can do more than
                // simply hiding the window.
                window->window()->visible = false;
            }
            break;
        }

        case XCB_CONFIGURE_NOTIFY: {
            const auto *configureEvent = reinterpret_cast<xcb_configure_notify_event_t *>(xcbEvent);
            const auto w = configureEvent->width;
            const auto h = configureEvent->height;
            auto window = m_platformIntegration->window(configureEvent->window);
            SPDLOG_LOGGER_DEBUG(m_logger, "{}: Resize of window to {} x {}", xcbEvent->sequence, w, h);
            window->handleResize(w, h);
            break;
        }

        case XCB_KEY_PRESS: {
            // Delegate the key events to the xkb keyboard object
            const auto keyEvent = reinterpret_cast<xcb_key_press_event_t *>(xcbEvent);
            m_keyboard->handleKeyPress(keyEvent);
            break;
        }

        case XCB_KEY_RELEASE: {
            // Delegate the key events to the xkb keyboard object
            const auto keyEvent = reinterpret_cast<xcb_key_release_event_t *>(xcbEvent);
            m_keyboard->handleKeyRelease(keyEvent);
            break;
        }

        case XCB_BUTTON_PRESS: {
            const auto buttonEvent = reinterpret_cast<xcb_button_press_event_t *>(xcbEvent);
            auto window = m_platformIntegration->window(buttonEvent->event);
            auto button = buttonEvent->detail;

            switch (button) {
            case 1:
            case 2:
            case 3:
            default: {
                SPDLOG_LOGGER_DEBUG(m_logger,
                                    "Mouse press event for button {} at pos ({}, {})",
                                    button,
                                    buttonEvent->event_x,
                                    buttonEvent->event_y);
                window->handleMousePress(
                        buttonEvent->time,
                        button,
                        buttonEvent->event_x,
                        buttonEvent->event_y);
                break;
            }

            case 4: {
                const int32_t xDelta = 0;
                const int32_t yDelta = 120;
                SPDLOG_LOGGER_DEBUG(m_logger,
                                    "Wheel event yDelta = {} at pos({}, {})",
                                    yDelta, buttonEvent->event_x, buttonEvent->event_y);
                window->handleMouseWheel(
                        buttonEvent->time,
                        xDelta,
                        yDelta);
                break;
            }

            case 5: {
                const int32_t xDelta = 0;
                const int32_t yDelta = -120;
                SPDLOG_LOGGER_DEBUG(m_logger,
                                    "Wheel event yDelta = {} at pos({}, {})",
                                    yDelta, buttonEvent->event_x, buttonEvent->event_y);
                window->handleMouseWheel(
                        buttonEvent->time,
                        xDelta,
                        yDelta);
                break;
            }

            case 6: {
                const int32_t xDelta = -120;
                const int32_t yDelta = 0;
                SPDLOG_LOGGER_DEBUG(m_logger,
                                    "Wheel event xDelta = {} at pos({}, {})",
                                    xDelta, buttonEvent->event_x, buttonEvent->event_y);
                window->handleMouseWheel(
                        buttonEvent->time,
                        xDelta,
                        yDelta);
                break;
            }

            case 7: {
                const int32_t xDelta = 120;
                const int32_t yDelta = 0;
                SPDLOG_LOGGER_DEBUG(m_logger,
                                    "Wheel event xDelta = {} at pos({}, {})",
                                    xDelta, buttonEvent->event_x, buttonEvent->event_y);
                window->handleMouseWheel(
                        buttonEvent->time,
                        xDelta,
                        yDelta);
                break;
            }
            }

            break;
        }

        case XCB_BUTTON_RELEASE: {
            const auto buttonEvent = reinterpret_cast<xcb_button_release_event_t *>(xcbEvent);
            auto window = m_platformIntegration->window(buttonEvent->event);
            SPDLOG_LOGGER_DEBUG(m_logger,
                                "Mouse release event for button {} at pos ({}, {})",
                                buttonEvent->detail,
                                buttonEvent->event_x,
                                buttonEvent->event_y);
            window->handleMouseRelease(
                    buttonEvent->time,
                    buttonEvent->detail,
                    buttonEvent->event_x,
                    buttonEvent->event_y);
            break;
        }

        case XCB_MOTION_NOTIFY: {
            const auto mouseMoveEvent = reinterpret_cast<xcb_motion_notify_event_t *>(xcbEvent);
            auto window = m_platformIntegration->window(mouseMoveEvent->event);
            SPDLOG_LOGGER_DEBUG(m_logger,
                                "{}: Mouse move event for button {} at pos ({}, {})",
                                xcbEvent->sequence,
                                mouseMoveEvent->detail,
                                mouseMoveEvent->event_x,
                                mouseMoveEvent->event_y);
            window->handleMouseMove(
                    mouseMoveEvent->time,
                    mouseMoveEvent->detail,
                    static_cast<int64_t>(mouseMoveEvent->event_x),
                    static_cast<int64_t>(mouseMoveEvent->event_y));
            break;
        }

        case XCB_ENTER_NOTIFY: {
            break;
        }

        case XCB_LEAVE_NOTIFY: {
            break;
        }

        default: {
            // TODO: Handle xkb events such as new keymap, keyboard change etc

            m_logger->warn("Unhandled event type: {}", xcbEvent->response_type);
            break;
        }
        }

        free(xcbEvent);
    }
}
