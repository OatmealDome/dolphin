// Copyright 2019 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#include <string>

#include <MetalKit/MetalKit.h>

#include "Core/Boot/Boot.h"

#include "Common/CommonTypes.h"
#include "Common/FileUtil.h"
#include "Common/IniFile.h"
#include "Common/WindowSystemInfo.h"

#include "Core/BootManager.h"
#include "Core/ConfigManager.h"
#include "Core/Core.h"
#include "Core/Host.h"
#include "Core/HW/GCPad.h"
#include "Core/HW/Wiimote.h"
#include "Core/HW/SI/SI.h"
#include "Core/HW/SI/SI_Device.h"
#include "Core/State.h"

#include "InputCommon/ControllerEmu/ControllerEmu.h"
#include "InputCommon/ControllerInterface/ControllerInterface.h"
#include "InputCommon/ControllerInterface/Touch/ButtonManager.h"
#include "InputCommon/InputConfig.h"

#include "MainiOS.h"

#include "UICommon/UICommon.h"

#include "VideoCommon/RenderBase.h"

// The Core only supports using a single Host thread.
// If multiple threads want to call host functions then they need to queue
// sequentially for access.
std::mutex s_host_identity_lock;
Common::Event s_update_main_frame_event;
bool s_have_wm_user_stop = false;

// Our UIViewController
UIViewController* s_view_controller = nullptr;

void Host_NotifyMapLoaded()
{
}
void Host_RefreshDSPDebuggerWindow()
{
}

bool Host_UIBlocksControllerState()
{
  return false;
}

void Host_Message(HostMessageID id)
{
  if (id == HostMessageID::WMUserJobDispatch)
  {
    s_update_main_frame_event.Set();
  }
  else if (id == HostMessageID::WMUserStop)
  {
    s_have_wm_user_stop = true;
    if (Core::IsRunning())
      Core::QueueHostJob(&Core::Stop);
  }
}

void Host_UpdateTitle(const std::string& title)
{
}

void Host_UpdateDisasmDialog()
{
}

void Host_UpdateMainFrame()
{
}

void Host_RequestRenderWindowSize(int width, int height)
{
}

bool Host_UINeedsControllerState()
{
  return false;
}

bool Host_RendererHasFocus()
{
  return true;
}

bool Host_RendererIsFullscreen()
{
  return true;
}

void Host_YieldToUI()
{
}

void Host_UpdateProgressDialog(const char* caption, int position, int total)
{
}

void Host_TitleChanged()
{
}

static bool MsgAlert(const char* caption, const char* text, bool yes_no, Common::MsgType /*style*/)
{
  @autoreleasepool
  {
    // Log to console as a backup
    NSLog(@"MsgAlert - %s: %s (yes_no: %d)", caption, text, yes_no ? 1 : 0);

    // Don't continue if the view controller isn't available
    if (!s_view_controller)
    {
      return false;
    }

    NSCondition* condition = [[NSCondition alloc] init];
    
    __block bool yes_pressed = false;

    dispatch_async(dispatch_get_main_queue(), ^{
      UIAlertController* alert = [UIAlertController alertControllerWithTitle:[NSString stringWithUTF8String:caption]
                                    message:[NSString stringWithUTF8String:text]
                                    preferredStyle:UIAlertControllerStyleAlert];

      if (yes_no)
      {
        [alert addAction:[UIAlertAction actionWithTitle:@"Yes" style:UIAlertActionStyleDefault
          handler:^(UIAlertAction * action) {
            yes_pressed = true;

            [condition signal];
        }]];

        [alert addAction:[UIAlertAction actionWithTitle:@"No" style:UIAlertActionStyleDefault
          handler:^(UIAlertAction* action) {
            yes_pressed = false;

            [condition signal];
        }]];
      }
      else
      {
        [alert addAction:[UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault
          handler:^(UIAlertAction* action) {
            [condition signal];
        }]];
      }

      [s_view_controller presentViewController:alert animated:YES completion:nil];
    });

    // Wait for a button press
    [condition lock];
    [condition wait];
    [condition unlock];

    return yes_pressed;
  }
}

@implementation MainiOS

+ (void) applicationStart
{
  Common::RegisterMsgAlertHandler(&MsgAlert);
  
  NSString* user_directory = [MainiOS getUserFolder];
  UICommon::SetUserDirectory(std::string([user_directory UTF8String]));

  UICommon::CreateDirectories();

  NSFileManager* file_manager = [NSFileManager defaultManager];

  // Check if GCPadNew.ini exists
  NSString* gc_pad_path = [NSString stringWithUTF8String:File::GetUserPath(F_GCPADCONFIG_IDX).c_str()];
  if (![file_manager fileExistsAtPath:gc_pad_path])
  {
    NSString* bundle_gc_pad = [[NSBundle mainBundle] pathForResource:@"GCPadNew" ofType:@"ini"];
    [file_manager copyItemAtPath:bundle_gc_pad toPath:gc_pad_path error:nil];
  }

  // Check if WiimoteNew.ini exists
  NSString* wii_pad_path = [NSString stringWithUTF8String:File::GetUserPath(F_WIIPADCONFIG_IDX).c_str()];
  if (![file_manager fileExistsAtPath:wii_pad_path])
  {
    NSString* bundle_wii_pad = [[NSBundle mainBundle] pathForResource:@"WiimoteNew" ofType:@"ini"];
    [file_manager copyItemAtPath:bundle_wii_pad toPath:wii_pad_path error:nil];
  }

  // Set IMU IR if needed
  if (![[NSUserDefaults standardUserDefaults] boolForKey:@"imu_ir_initial_value_set"])
  {
    [MainiOS toggleImuIr:false];
    [[NSUserDefaults standardUserDefaults] setBool:true forKey:@"imu_ir_initial_value_set"];
  }

  // Check if WiimoteProfile.ini exists
  NSString* wiimote_profile_folder = [[[user_directory stringByAppendingPathComponent:@"Config"]
                                                        stringByAppendingPathComponent:@"Profiles"]
                                                        stringByAppendingPathComponent:@"Wiimote"];
  NSString* wiimote_profile_ini = [wiimote_profile_folder stringByAppendingPathComponent:@"WiimoteProfile.ini"];
  if (![file_manager fileExistsAtPath:wiimote_profile_folder])
  {
    [file_manager createDirectoryAtPath:wiimote_profile_folder withIntermediateDirectories:true
                  attributes:nil error:nil];
  }
  if (![file_manager fileExistsAtPath:wiimote_profile_ini])
  {
    NSString* bundle_profile = [[NSBundle mainBundle] pathForResource:@"WiimoteProfile" ofType:@"ini"];
    [file_manager copyItemAtPath:bundle_profile toPath:wiimote_profile_ini error:nil];
  }

  // Get the Dolphin.ini path
  std::string dolphin_config_path = File::GetUserPath(F_DOLPHINCONFIG_IDX);

  // Check if the Dolphin.ini file exists already
  if (![file_manager fileExistsAtPath:[NSString stringWithUTF8String:dolphin_config_path.c_str()]])
  {
    // Create a new Dolphin.ini
    IniFile dolphin_config;

    std::string gfx_backend;

    // Check if GPU Family 3 is supported
    id<MTLDevice> metalDevice = MTLCreateSystemDefaultDevice();
    if ([metalDevice supportsFeatureSet:MTLFeatureSet_iOS_GPUFamily3_v2])
    {
      gfx_backend = "Vulkan";
    }
    else
    {
      gfx_backend = "OGL";
    }

    dolphin_config.GetOrCreateSection("Core")->Set("GFXBackend", gfx_backend);

    dolphin_config.Save(dolphin_config_path);
  }

  UICommon::Init();

  if (g_controller_interface.IsInit())
    return;

  // Create a dummy WindowSystemInfo
  WindowSystemInfo wsi;
  wsi.type = WindowSystemType::IPhoneOS;

  g_controller_interface.Initialize(wsi);
  Pad::Initialize();
  Wiimote::Initialize(Wiimote::InitializeMode::DO_NOT_WAIT_FOR_WIIMOTES);
}

+ (NSString*)getUserFolder
{
#ifndef IPHONEOS_JAILBROKEN
  NSString* user_directory = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) objectAtIndex:0];
#else
  NSString* user_directory = @"/private/var/mobile/Documents/DolphiniOS";
#endif

  return user_directory;
}

+ (NSString*)getGfxBackend
{
  IniFile dolphin_config;
  dolphin_config.Load(File::GetUserPath(F_DOLPHINCONFIG_IDX));
  
  std::string gfx_backend;
  dolphin_config.GetOrCreateSection("Core")->Get("GFXBackend", &gfx_backend, "OGL");

  return [NSString stringWithUTF8String:gfx_backend.c_str()];
}

+ (void)importFiles:(NSSet<NSURL*>*)files
{
  NSFileManager* file_manager = [NSFileManager defaultManager];
  for (NSURL* url in files)
  {
    if (![url startAccessingSecurityScopedResource])
    {
      continue;
    }
    
    NSString* file = [url path];
    NSString* software_directory = [[MainiOS getUserFolder] stringByAppendingPathComponent:@"Software"];
    NSString* destination_path = [software_directory stringByAppendingPathComponent:[file lastPathComponent]];
    
    // Check if the file is new
    if (![file_manager fileExistsAtPath:destination_path])
    {
      // Move the file
      NSError* error = nil;
      [file_manager moveItemAtPath:file toPath:destination_path error:&error];
      
      if (error != nil)
      {
        return;
      }
      
      [url stopAccessingSecurityScopedResource];
    }
  }
}

+ (void)startEmulationWithFile:(NSString*)file viewController:(UIViewController*)viewController view:(UIView*)view
{
  s_view_controller = viewController;

  WindowSystemInfo wsi;
  wsi.type = WindowSystemType::IPhoneOS;
  wsi.display_connection = nullptr;
  wsi.render_surface = (__bridge void*)view;
  wsi.render_surface_scale = [UIScreen mainScreen].scale;  

  std::unique_ptr<BootParameters> boot = BootParameters::GenerateFromFile([file UTF8String]);

  std::unique_lock<std::mutex> guard(s_host_identity_lock);

  // No use running the loop when booting fails
  s_have_wm_user_stop = false;

  if (BootManager::BootCore(std::move(boot), wsi))
  {
    ButtonManager::Init(SConfig::GetInstance().GetGameID());
    static constexpr int TIMEOUT = 10000;
    static constexpr int WAIT_STEP = 25;
    int time_waited = 0;
    // A Core::CORE_ERROR state would be helpful here.
    while (!Core::IsRunning() && time_waited < TIMEOUT && !s_have_wm_user_stop)
    {
      [NSThread sleepForTimeInterval:WAIT_STEP];
      time_waited += WAIT_STEP;
    }
    while (Core::IsRunning())
    {
      guard.unlock();
      s_update_main_frame_event.Wait();
      guard.lock();
      Core::HostDispatchJobs();
    }
  }

  Core::Shutdown();
  ButtonManager::Shutdown();

  s_view_controller = nullptr;
}

+ (void)stopEmulation
{
  std::lock_guard<std::mutex> guard(s_host_identity_lock);
  Core::Stop();
  s_update_main_frame_event.Set();
}

+ (CGFloat)getGameAspectRatio
{
  if (!g_renderer)
    return 0;
  
  return g_renderer->CalculateDrawAspectRatio();
}

+ (CGRect)getRenderTargetRectangle
{
  if (!g_renderer)
    return CGRectZero;
  
  const MathUtil::Rectangle<int>& rect = g_renderer->GetTargetRectangle();
  return CGRectMake(rect.left, rect.top, rect.GetWidth(), rect.GetHeight());
}

+ (void)setDrawRectangleCustomOriginAsX:(int)x y:(int)y
{
  if (!g_renderer)
    return;
  
  g_renderer->SetDrawRectangleCustomOrigin(x, y);
  g_renderer->UpdateDrawRectangle();
}

+ (void)windowResized
{
  if (g_renderer)
    g_renderer->ResizeSurface();
}

+ (void)gamepadEventForButton:(int)button action:(int)action
{
  ButtonManager::GamepadEvent("Touchscreen", button, action);
}

+ (void)gamepadMoveEventForAxis:(int)axis value:(CGFloat)value
{
  ButtonManager::GamepadAxisEvent("Touchscreen", axis, value);
}

+ (void)toggleImuIr:(bool)enabled
{
  std::string wiimote_ini_path = File::GetUserPath(F_WIIPADCONFIG_IDX);
  IniFile wiimote_ini;
  wiimote_ini.Load(wiimote_ini_path);
  wiimote_ini.GetOrCreateSection("Wiimote1")->Set("IMUIR/Enabled", enabled);
  wiimote_ini.Save(wiimote_ini_path);
}

+ (void)toggleSidewaysWiimote:(bool)enabled reload_wiimote:(bool)reload_wiimote
{
  std::string wiimote_ini_path = File::GetUserPath(F_WIIPADCONFIG_IDX);
  IniFile wiimote_ini;
  wiimote_ini.Load(wiimote_ini_path);
  wiimote_ini.GetOrCreateSection("Wiimote1")->Set("Options/Sideways Wiimote", enabled);
  wiimote_ini.Save(wiimote_ini_path);

  if (reload_wiimote)
  {
    Wiimote::LoadConfig();
  }
}

+ (bool)IsTouchscreenConnectedToController:(int)i input_config:(InputConfig*)input_config
{
  std::string android_str = "Android";
  
  ControllerEmu::EmulatedController* controller = input_config->GetController(i);
  std::string device = controller->GetDefaultDevice().ToString();
  
  return device.compare(0, android_str.size(), android_str) == 0;
}

+ (bool)isTouchscreenDeviceConnected:(bool)check_wii
{
  for (size_t i = 0; i < 4; i++)
  {
    InputConfig* gc_input_config = Pad::GetConfig();    
    if (SConfig::GetInstance().m_SIDevice[i] != SerialInterface::SIDEVICE_NONE)
    {
      if ([MainiOS IsTouchscreenConnectedToController:i input_config:gc_input_config])
      {
        return true;
      }
    }

    if (check_wii)
    {
      InputConfig* wii_input_config = Wiimote::GetConfig();
      if (g_wiimote_sources[i] == 1) // Emulated
      {
        if ([MainiOS IsTouchscreenConnectedToController:i input_config:wii_input_config])
        {
          return true;
        }
      }
    }
  }

  return false;
}

@end
