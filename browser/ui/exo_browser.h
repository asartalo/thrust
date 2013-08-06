// Copyright (c) 2013 Stanislas Polu.
// See the LICENSE file.

#ifndef BREACH_BROWSER_UI_EXO_BROWSER_H_
#define BREACH_BROWSER_UI_EXO_BROWSER_H_

#include <vector>

#include "base/basictypes.h"
#include "base/callback_forward.h"
#include "base/memory/scoped_ptr.h"
#include "base/strings/string_piece.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "content/public/browser/web_contents_delegate.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/gfx/size.h"
#include "ui/gfx/point.h"

#if defined(TOOLKIT_GTK)
#include <gtk/gtk.h>
#include "ui/base/gtk/gtk_signal.h"

typedef struct _GtkToolItem GtkToolItem;
namespace views {
class Widget;
class ViewsDelegate;
}
#endif

class GURL;

namespace content {
class BrowserContext;
class SiteInstance;
class WebContents;
}

namespace breach {

class BreachDevToolsFrontend;
class BreachJavaScriptDialogManager;

class ExoBrowserWrap;
class ExoFrameWrap;


// ### ExoBrowser
// 
// This represents one window of the Breach ExoBrowser. The ExoBrowser exposes
// to Javascript the ability to add "frames" within that window (which are web
// views) using a fixed layout with visibility and z-index capabilities.
//
// The ExoBrowser initialization always come from Javascript and it is
// therefore aware of its associated Javascript wrapper (used to forward noti-
// fications or callbacks there).
//
// The ExoBrowser lives on the BrowserThread::UI thread, and should PostTask on
// the NodeJS thread whenever it wants to communicate with its JS Wrapper
class ExoBrowser : public content::WebContentsDelegate,
                   public content::NotificationObserver {
public:
  static const int kDefaultWindowWidth;
  static const int kDefaultWindowHeight;
  
  /****************************************************************************/
  /*                         STATIC INTERFACE                                 */
  /****************************************************************************/
  // ### Initialize
  // Runs one-time initialization at application startup.
  static void Initialize();

  // ### CreateNew
  // ```
  // @wrapper {ExoBrowserWrap} the wrapper associated with this ExoBrowser
  // @size    {Size} the initial size of the window
  // ```
  // Creates a new empty ExoBrowser window.
  static ExoBrowser* CreateNew(ExoBrowserWrap* wrapper,
                               const gfx::Size& size);

  // ### instances
  // Getter for all the currently working ExoBrowser instances.
  static std::vector<ExoBrowser*>& instances() { return instances_; }

  // ### KillAll
  // Kills all running instances and returns.
  static void KillAll();

  /****************************************************************************/
  /*                            PUBLIC INTERFACE                              */
  /****************************************************************************/
  // ### ~ExoBrowser
  virtual ~ExoBrowser();

  // ### AddFrame
  // ```
  // @frame {ExoFrame} the frame to add to this browser
  // ```
  // This methods adds a frame to this browser. It checks that this frame has
  // itself as a parent. The browser is not responsible for deleting the frame
  // as it is binded to its JS wrapper.
  // ExoFrame object should only be created from the JS wrappers and this
  // method can only be called from there.
  void AddFrame(ExoFrame* frame);

  // ### RemoveFrame
  // ```
  // @name     {string} the new frame name
  // ```
  // Removes the ExoFrame designated by `name` by removing it from the current
  // ExoBrowser window. The frame can be reassociated with a different browser.
  // The underlying ExoFrame is not deleted as its deletion is handled by the
  // JS wrapper.
  void RemoveFrame(const std::string& name);

  // ### Kill
  // Kills the ExoBrowser and remove all of its frame. The ExoBrowser object 
  // will not be deleted on kill (as it will be reclaimed when the JS object 
  // is deleted) but it will be marked as killed as it is not usable anymore
  void Kill();


  // ### is_killed
  // Returns whether the ExoBrowser is killed or not
  bool is_killed() { return is_killed_; }

  // ### WindowSize
  // Retrieves the native Window size
  gfx::Size size() { return PlatformSize(); }

  // ### WindowPosition
  // Retrieves the native Window position
  gfx::Point position() { return PlatformPosition(); }

  // ### window
  // Retrieves the NativeWindow object
  gfx::NativeWindow window() { return window_; }

  /****************************************************************************/
  /*                  WEBCONTENTSDELEGATE IMPLEMENTATION                      */
  /****************************************************************************/
  virtual content::WebContents* OpenURLFromTab(
      content::WebContents* source,
      const content::OpenURLParams& params) OVERRIDE;

  virtual void LoadingStateChanged(content::WebContents* source) OVERRIDE;

  virtual void RequestToLockMouse(content::WebContents* web_contents,
                                  bool user_gesture,
                                  bool last_unlocked_by_target) OVERRIDE;

  virtual void CloseContents(content::WebContents* source) OVERRIDE;

  virtual void AddNewContents(content::WebContents* source,
                              content::WebContents* new_contents,
                              WindowOpenDisposition disposition,
                              const gfx::Rect& initial_pos,
                              bool user_gesture,
                              bool* was_blocked) OVERRIDE;

  virtual void WebContentsCreated(content::WebContents* source_contents,
                                  int64 source_frame_id,
                                  const string16& frame_name,
                                  const GURL& target_url,
                                  content::WebContents* new_contents) OVERRIDE;

  virtual void DidNavigateMainFramePostCommit(
      content::WebContents* web_contents) OVERRIDE;

  virtual content::JavaScriptDialogManager* 
    GetJavaScriptDialogManager() OVERRIDE;


  virtual void ActivateContents(content::WebContents* contents) OVERRIDE;
  virtual void DeactivateContents(content::WebContents* contents) OVERRIDE;

  virtual void RendererUnresponsive(content::WebContents* source) OVERRIDE;
  virtual void WorkerCrashed(content::WebContents* source) OVERRIDE;

private:
  /****************************************************************************/
  /*                           PRIVATE INTERFACE                              */
  /****************************************************************************/
  explicit ExoBrowser(ExoBrowserWrap* wrapper);

  // ### FrameForWebContents
  // ```
  // @content {WebContents} a WebContents
  // ```
  // Retrieves within this browser, the frame associated with the provided 
  // WebContents. Returns NULL otherwise
  ExoFrame* FrameForWebContents(content::WebContents* contents);

  /****************************************************************************/
  /*                        STATIC PLATFORM INTERFACE                         */
  /****************************************************************************/
  // All the methods that begin with Platform need to be implemented by the
  // platform specific Browser implementation.

  // ### PlatformInitialize
  // Helper for one time initialization of application
  static void PlatformInitialize(const gfx::Size& default_window_size);

  // ### PlatformExit
  // Closes all windows and exits.
  static void PlatformExit();

  /****************************************************************************/
  /*                            PLATFORM INTERFACE                            */
  /****************************************************************************/
  // ### PlatformCleanup
  // Called from the destructor to let each platform do any necessary cleanup.
  void PlatformCleanUp();

  // ### PlatformCreateWindow
  // Creates the ExoBrowser window GUI.
  void PlatformCreateWindow(int width, int height);

  // ### PlatformSetTitle 
  // Set the title of ExoBrowser window.
  void PlatformSetTitle(const string16& title);

  // ### PlatformSize
  // Retrieves the size of the ExoBrowser window.
  gfx::Size PlatformSize();

  // ### PlatformPosition
  // Retrieves the position of the ExoBrowser window.
  gfx::Point PlatformPosition();


#if defined(OS_WIN) && !defined(USE_AURA)
  static ATOM RegisterWindowClass();
  static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
  static LRESULT CALLBACK EditWndProc(HWND, UINT, WPARAM, LPARAM);
#elif defined(TOOLKIT_GTK)
  CHROMEGTK_CALLBACK_0(Browser, gboolean, OnWindowDestroyed);
  CHROMEG_CALLBACK_3(Browser, gboolean, OnCloseWindowKeyPressed, GtkAccelGroup*,
                     GObject*, guint, GdkModifierType);
  CHROMEG_CALLBACK_3(Browser, gboolean, OnNewWindowKeyPressed, GtkAccelGroup*,
                     GObject*, guint, GdkModifierType);
  CHROMEG_CALLBACK_3(Browser, gboolean, OnHighlightURLView, GtkAccelGroup*,
                     GObject*, guint, GdkModifierType);
  CHROMEG_CALLBACK_3(Browser, gboolean, OnReloadKeyPressed, GtkAccelGroup*,
                     GObject*, guint, GdkModifierType);
#endif


  /****************************************************************************/
  /*                  NOTIFICATION OBSERVER IMPLEMENTATION                    */
  /****************************************************************************/
  virtual void Observe(int type,
                       const content::NotificationSource& source,
                       const content::NotificationDetails& details) OVERRIDE;


  /****************************************************************************/
  /*                               MEMBERS                                    */
  /****************************************************************************/
  scoped_ptr<BreachJavaScriptDialogManager>    dialog_manager_;

  gfx::NativeWindow                            window_;
  content::NotificationRegistrar               registrar_;

  std::map<std::string, ExoFrame*>             frames_;

  ExoBrowserWrap*                              wrapper_;

  bool                                         is_killed_;

#if defined(OS_WIN) && !defined(USE_AURA)
  WNDPROC                                      default_edit_wnd_proc_;
  static HINSTANCE                             instance_handle_;
#elif defined(TOOLKIT_GTK)
#endif

  // A static container of all the open instances. 
  static std::vector<ExoBrowser*>              s_instances;

  friend class ExoBrowserWrap;
  friend class ExoFrameWrap;
};

} // namespace breach
