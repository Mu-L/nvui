#ifndef NVUI_GUI_HPP
#define NVUI_GUI_HPP

#include <QObject>
#include <QMainWindow>
#include <QWidget>
#include <QFont>
#include <QToolBar>
#include <QLayout>
#include "decide_renderer.hpp"
#include "nvim.hpp"
#include "editor.hpp"
#include "titlebar.hpp"
#include "hlstate.hpp"
#include <iostream>
#include <memory>
#include <span>
#include <msgpack.hpp>
#include <QEvent>
#include <unordered_map>
#include <QSemaphore>
class Window;

#if defined(Q_OS_WIN)
#include "wineditor.hpp"
#endif

constexpr int tolerance = 10; //10px tolerance for resizing

using obj_ref_cb = void (*)(Window*, std::span<const Object>);

/// The main window class which holds the rest of the GUI components.
/// Fundamentally, the Neovim area is just 1 big text box.
/// However, there are additional features that we are trying to
/// support.
class Window : public QMainWindow
{
  Q_OBJECT
  template<typename R, typename E>
  using handler_func =
    std::function<
      std::tuple<std::optional<R>, std::optional<E>>
      (const ObjectArray&)>;
public:
  Window(
    QWidget* parent = nullptr,
    Nvim* nv = nullptr,
    int width = 0,
    int height = 0,
    bool custom_titlebar = false
  );
  /**
   * Registers all of the relevant gui event handlers
   * for handling Neovim redraw events, as well as a Neovim
   * notification handler for the 'redraw' method.
   */
  void register_handlers();
  /**
   * Sets a handler for the corresponding function
   * in Neovim's redraw notification.
   */
  void set_handler(std::string method, obj_ref_cb handler);
public slots:
  /**
   * Handles a 'redraw' Neovim notification.
   * TODO: Decide parameter type and how this will be called
   * (signals etc.)
   */
  void handle_redraw(Object dir_args);
  /**
   * Starts a resizing or moving operation depending on the coordinates
   * of p.
   */
  void resize_or_move(const QPointF& p);
  /**
   * Returns whether the window is frameless or not
   */
  inline bool is_frameless() const
  {
    return windowFlags() & Qt::FramelessWindowHint;
  }
  
  /**
   * Maximize the window.
   */
  void maximize();
private:
  /**
   * Listen for a notification with the method call "method",
   * and invoke the corresponding callback on the main (Qt) thread.
   * cb is passed the arguments array.
   */
  void listen_for_notification(
    std::string method,
    std::function<void (const ObjectArray&)> cb
  );
  /**
   * Listens for a request with the given name,
   * and then sends a response with the data returned
   * by the callback function.
   * Things like matching message id are handled by this function.
   */
  template<typename Res, typename Err>
  void handle_request(
    std::string req_name,
    handler_func<Res, Err> handler
  );
  /**
   * Disable the frameless window.
   * The window should be in frameless mode,
   * so windowState() & Qt::FramelessWindowHint should be true.
   */
  void disable_frameless_window();
  /**
   * Enable the frameless window.
   * The window should not be in frameless window mode,
   * so windowState() & Qt::FramelessWindowHint should be false.
   */
  void enable_frameless_window();
  /**
   * Hides the title bar.
   * This just always hides it,
   * since there is no reason not to, unlike with showing.
   */
  inline void hide_title_bar()
  {
    title_bar->hide();
  }
  
  /**
   * Shows the titlebar. Only activates if the window is a
   * frameless window (otherwise you would get two title bars,
   * the OS one and the custom one).
   */
  inline void show_title_bar()
  {
    if (!is_frameless()) return;
    title_bar->show();
  }

  /**
   * If fullscreen is true, shows the window in fullscreen mode,
   * otherwise reverts back to how it was before.
   */
  void set_fullscreen(bool fullscreen);
  /**
   * Turn the window fullscreen.
   * If the window is already fullscreen, this does nothing.
   */
  void fullscreen();
  /**
   * Un-fullscreen the window, restoring its original state.
   */
  void un_fullscreen();
  /**
   * Updates the title bar colors.
   * If titlebar_colors contains a value, then it uses
   * titlebar_colors's values.
   * Otherwise, it uses the default colors.
   */
  void update_titlebar_colors();
  QSemaphore semaphore;
  bool resizing;
  bool maximized = false;
  bool moving = false;
  std::unique_ptr<TitleBar> title_bar;
  HLState hl_state;
  Nvim* nvim;
  std::unordered_map<std::string, obj_ref_cb> handlers;
  QFlags<Qt::WindowState> prev_state;
  template<typename T>
  using opt = std::optional<T>;
  std::pair<opt<QColor>, opt<QColor>> titlebar_colors;
#if defined(USE_DIRECT2D)
  WinEditorArea editor_area;
#elif defined(USE_QPAINTER)
  EditorArea editor_area;
#endif
signals:
  void win_state_changed(Qt::WindowStates new_state);
  void resize_done(QSize size);
  void default_colors_changed(QColor fg, QColor bg);
protected:
  bool nativeEvent(const QByteArray& e_type, void* msg, long* result) override;
  void changeEvent(QEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;
  void moveEvent(QMoveEvent* event) override;
};

#endif // NVUI_WINDOW_HPP
