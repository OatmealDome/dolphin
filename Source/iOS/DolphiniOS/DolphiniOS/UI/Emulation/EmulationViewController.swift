// Copyright 2019 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

import Foundation
import MetalKit
import UIKit
import GameController

class EmulationViewController: UIViewController, UIGestureRecognizerDelegate
{
  @objc public var softwareFile: String = ""
  @objc public var softwareName: String = ""
  @objc public var isWii: Bool = false
  
  @IBOutlet weak var m_metal_view: MTKView!
  @IBOutlet weak var m_eagl_view: EAGLView!
  @IBOutlet weak var m_gc_pad_view: TCGameCubePad!
  @IBOutlet weak var m_wii_pad_view: TCWiiPad!
  
  required init?(coder: NSCoder)
  {
    super.init(coder: coder)
  }
  
  override func viewDidLoad()
  {
    self.navigationItem.title = self.softwareName;
    
    var renderer_view: UIView
    if (MainiOS.getGfxBackend() == "Vulkan")
    {
      renderer_view = m_metal_view
    }
    else
    {
      renderer_view = m_eagl_view
    }
    
    renderer_view.isHidden = false
    
    if (self.isWii)
    {
      m_wii_pad_view.isUserInteractionEnabled = true
      m_wii_pad_view.isHidden = false
    }
    else
    {
      m_gc_pad_view.isUserInteractionEnabled = true
      m_gc_pad_view.isHidden = false
    }
    
    setupTapGestureRecognizer(m_wii_pad_view)
    setupTapGestureRecognizer(m_gc_pad_view)
    
    let has_seen_alert = UserDefaults.standard.bool(forKey: "seen_double_tap_two_fingers_alert")
    if (!has_seen_alert)
    {
      let alert = UIAlertController(title: "Note", message: "Double tap the screen with two fingers fast to reveal the top bar.", preferredStyle: .alert)
      
      alert.addAction(UIAlertAction(title: "OK", style: .default, handler: { action in
        self.navigationController!.setNavigationBarHidden(true, animated: true)
        
        UserDefaults.standard.set(true, forKey: "seen_double_tap_two_fingers_alert")
      }))
      
      self.present(alert, animated: true, completion: nil)
    }
    else
    {
      self.navigationController!.setNavigationBarHidden(true, animated: true)
    }
    
    let wiimote_queue = DispatchQueue(label: "org.dolphin-emu.ios.wiimote-initial-queue")
    wiimote_queue.async
    {
      // Wait for aspect ratio to be set
      while (MainiOS.getGameAspectRatio() == 0)
      {
      }
      
      // Create the Wiimote pointer values
      DispatchQueue.main.sync
      {
        self.m_wii_pad_view.recalculatePointerValues()
      }
    }
    
    let queue = DispatchQueue(label: "org.dolphin-emu.ios.emulation-queue")
    queue.async
    {
      MainiOS.startEmulation(withFile: self.softwareFile, viewController: self, view: renderer_view)
      
      DispatchQueue.main.async {
        self.performSegue(withIdentifier: "toSoftwareTable", sender: nil)
      }
    }
    ObserveForGameControllers()
    connectControllers()
  }
  
  override func viewWillTransition(to size: CGSize, with coordinator: UIViewControllerTransitionCoordinator)
  {
    super.viewWillTransition(to: size, with: coordinator)
    
    // Perform an "animation" alongside the transition and tell Dolphin that
    // the window has resized after it is finished
    coordinator.animate(alongsideTransition: nil, completion: { _ in
      MainiOS.windowResized()
      self.m_wii_pad_view.recalculatePointerValues()
    })
  }
  
  func setupTapGestureRecognizer(_ view: TCView)
  {
    // Add a gesture recognizer for two finger double tapping
    let tap_recognizer = UITapGestureRecognizer(target: self, action: #selector(doubleTapped))
    tap_recognizer.numberOfTapsRequired = 2
    tap_recognizer.numberOfTouchesRequired = 2
    tap_recognizer.delegate = self
    
    view.real_view!.addGestureRecognizer(tap_recognizer)
  }
  
  @IBAction func doubleTapped(_ sender: UITapGestureRecognizer)
  {
    // Ignore double taps on things that can be tapped
    if (sender.view != nil)
    {
      let hit_view = sender.view!.hitTest(sender.location(in: sender.view), with: nil)
      if (hit_view != sender.view)
      {
        return
      }
    }
    
    let is_hidden = self.navigationController!.isNavigationBarHidden
    if (!is_hidden)
    {
      self.additionalSafeAreaInsets.top = 0
    }
    else
    {
      // This inset undoes any changes the navigation bar made to the safe area
      self.additionalSafeAreaInsets.top = -(self.navigationController!.navigationBar.bounds.height)
    }
    
    self.navigationController!.setNavigationBarHidden(!is_hidden, animated: true)
    
  }
  
  func gestureRecognizer(_ gestureRecognizer: UIGestureRecognizer, shouldRequireFailureOf otherGestureRecognizer: UIGestureRecognizer) -> Bool {
    if (gestureRecognizer is UITapGestureRecognizer && otherGestureRecognizer is UILongPressGestureRecognizer)
    {
      return true
    }
    
    return false
  }
  
  @IBAction func exitButtonPressed(_ sender: Any)
  {
    let alert = UIAlertController(title: "Stop Emulation", message: "Do you really want to stop the emulation? All unsaved data will be lost.", preferredStyle: .alert)
    
    alert.addAction(UIAlertAction(title: "No", style: .default, handler: nil))
    alert.addAction(UIAlertAction(title: "Yes", style: .destructive, handler: { action in
      MainiOS.stopEmulation()
    }))
    
    self.present(alert, animated: true, completion: nil)
  }
    
  // Function to run intially to lookout for any MFI or Remote Controllers in the area
  func ObserveForGameControllers() {
    NotificationCenter.default.addObserver(self, selector: #selector(connectControllers), name: NSNotification.Name.GCControllerDidConnect, object: nil)
    NotificationCenter.default.addObserver(self, selector: #selector(disconnectControllers), name: NSNotification.Name.GCControllerDidDisconnect, object: nil)
  }
    
    // This Function is called when a controller is connected to the Apple TV
  @objc func connectControllers() {
    //Used to register the Nimbus Controllers to a specific Player Number
    var indexNumber = 0
    // Run through each controller currently connected to the system
    for controller in GCController.controllers() {
    //Check to see whether it is an extended Game Controller (Such as a Nimbus)
      if controller.extendedGamepad != nil {
        if (self.isWii) {
        m_wii_pad_view.isHidden = true
        controller.playerIndex = GCControllerPlayerIndex.init(rawValue: indexNumber)!
        indexNumber += 1
        setupControllerControls(controller: controller)
        }
        else {
            m_gc_pad_view.isHidden = true
            controller.playerIndex = GCControllerPlayerIndex.init(rawValue: indexNumber)!
            indexNumber += 1
            setupControllerControls(controller: controller)
        }
      }
    }
  }

  @objc func disconnectControllers() {
    if (self.isWii) {
    m_wii_pad_view.isHidden = false
        }
        else {
            m_gc_pad_view.isHidden = false
        }
  }

  func setupControllerControls(controller: GCController) {
    //Function that check the controller when anything is moved or pressed on it
    controller.extendedGamepad?.valueChangedHandler = {
      (gamepad: GCExtendedGamepad, element: GCControllerElement) in
      // Add movement in here for sprites of the controllers
      self.controllerInputDetected(gamepad: gamepad, element: element, index: controller.playerIndex.rawValue)
    }
  }
 
  func controllerInputDetected(gamepad: GCExtendedGamepad, element: GCControllerElement, index: Int) {
    if #available(iOS 13.0, *) {
    if (self.isWii) {
      switch element {
        case gamepad.buttonA:
            MainiOS.gamepadEvent(forButton: Int32(TCButtonType.WIIMOTE_BUTTON_A.rawValue), action: Int32(gamepad.buttonA.isPressed ? 1 : 0))
            case gamepad.buttonB:
                MainiOS.gamepadEvent(forButton: Int32(TCButtonType.WIIMOTE_BUTTON_1.rawValue), action: Int32(gamepad.buttonB.isPressed ? 1 : 0))
            case gamepad.buttonX:
                MainiOS.gamepadEvent(forButton: Int32(TCButtonType.WIIMOTE_BUTTON_B.rawValue), action: Int32(gamepad.buttonX.isPressed ? 1 : 0))
            case gamepad.buttonY:
                MainiOS.gamepadEvent(forButton: Int32(TCButtonType.WIIMOTE_BUTTON_2.rawValue), action: Int32(gamepad.buttonY.isPressed ? 1 : 0))
            case gamepad.rightShoulder:
                MainiOS.gamepadEvent(forButton: Int32(TCButtonType.NUNCHUK_BUTTON_Z.rawValue), action: Int32(gamepad.rightShoulder.value == 0 ? 0 : 1))
            case gamepad.leftShoulder:
                MainiOS.gamepadEvent(forButton: Int32(TCButtonType.NUNCHUK_BUTTON_C.rawValue), action: Int32(gamepad.leftShoulder.value == 0 ? 0 : 1))
            case gamepad.buttonMenu:
                MainiOS.gamepadEvent(forButton: Int32(TCButtonType.WIIMOTE_BUTTON_PLUS.rawValue), action: Int32(gamepad.buttonMenu.value == 0 ? 0 : 1))
            case gamepad.leftThumbstick:
                MainiOS.gamepadMoveEvent(forAxis: Int32(TCButtonType.NUNCHUK_STICK.rawValue+1), value: CGFloat(-gamepad.leftThumbstick.up.value))
                MainiOS.gamepadMoveEvent(forAxis: Int32(TCButtonType.NUNCHUK_STICK.rawValue+2), value: CGFloat(gamepad.leftThumbstick.down.value))
                MainiOS.gamepadMoveEvent(forAxis: Int32(TCButtonType.NUNCHUK_STICK.rawValue+3), value: CGFloat(-gamepad.leftThumbstick.left.value))
                MainiOS.gamepadMoveEvent(forAxis: Int32(TCButtonType.NUNCHUK_STICK.rawValue+4), value: CGFloat(gamepad.leftThumbstick.right.value))
            case gamepad.rightThumbstick:
                MainiOS.gamepadMoveEvent(forAxis: Int32(TCButtonType.WIIMOTE_IR.rawValue+1), value: CGFloat(-gamepad.rightThumbstick.up.value))
                MainiOS.gamepadMoveEvent(forAxis: Int32(TCButtonType.WIIMOTE_IR.rawValue+2), value: CGFloat(gamepad.rightThumbstick.down.value))
                MainiOS.gamepadMoveEvent(forAxis: Int32(TCButtonType.WIIMOTE_IR.rawValue+3), value: CGFloat(-gamepad.rightThumbstick.left.value))
                MainiOS.gamepadMoveEvent(forAxis: Int32(TCButtonType.WIIMOTE_IR.rawValue+4), value: CGFloat(gamepad.rightThumbstick.right.value))
            case gamepad.dpad:
                MainiOS.gamepadEvent(forButton: Int32(TCButtonType.WIIMOTE_UP.rawValue), action: gamepad.dpad.up.isPressed ? 1 : 0)
                MainiOS.gamepadEvent(forButton: Int32(TCButtonType.WIIMOTE_DOWN.rawValue), action: gamepad.dpad.down.isPressed ? 1 : 0)
                MainiOS.gamepadEvent(forButton: Int32(TCButtonType.WIIMOTE_LEFT.rawValue), action: gamepad.dpad.left.isPressed ? 1 : 0)
                MainiOS.gamepadEvent(forButton: Int32(TCButtonType.WIIMOTE_RIGHT.rawValue), action: gamepad.dpad.right.isPressed ? 1 : 0)
            case gamepad.leftTrigger:
                MainiOS.gamepadMoveEvent(forAxis: Int32(TCButtonType.WIIMOTE_BUTTON_MINUS.rawValue), value: CGFloat(gamepad.leftTrigger.value))
            case gamepad.rightTrigger:
                MainiOS.gamepadMoveEvent(forAxis: Int32(TCButtonType.WIIMOTE_SHAKE_X.rawValue), value: CGFloat(gamepad.rightTrigger.value))
                MainiOS.gamepadMoveEvent(forAxis: Int32(TCButtonType.WIIMOTE_SHAKE_Y.rawValue), value: CGFloat(gamepad.rightTrigger.value))
                MainiOS.gamepadMoveEvent(forAxis: Int32(TCButtonType.WIIMOTE_SHAKE_Z.rawValue), value: CGFloat(gamepad.rightTrigger.value))
                MainiOS.gamepadMoveEvent(forAxis: Int32(TCButtonType.NUNCHUK_SHAKE_X.rawValue), value: CGFloat(gamepad.rightTrigger.value))
                MainiOS.gamepadMoveEvent(forAxis: Int32(TCButtonType.NUNCHUK_SHAKE_Y.rawValue), value: CGFloat(gamepad.rightTrigger.value))
                MainiOS.gamepadMoveEvent(forAxis: Int32(TCButtonType.NUNCHUK_SHAKE_Z.rawValue), value: CGFloat(gamepad.rightTrigger.value))
        default:
            NSLog("oeuf")
        }
        }
    else {
        switch element {
        case gamepad.buttonA:
            MainiOS.gamepadEvent(forButton: Int32(TCButtonType.BUTTON_A.rawValue), action: Int32(gamepad.buttonA.isPressed ? 1 : 0))
        case gamepad.buttonB:
            MainiOS.gamepadEvent(forButton: Int32(TCButtonType.BUTTON_X.rawValue), action: Int32(gamepad.buttonB.isPressed ? 1 : 0))
        case gamepad.buttonX:
            MainiOS.gamepadEvent(forButton: Int32(TCButtonType.BUTTON_B.rawValue), action: Int32(gamepad.buttonX.isPressed ? 1 : 0))
        case gamepad.buttonY:
            MainiOS.gamepadEvent(forButton: Int32(TCButtonType.BUTTON_Y.rawValue), action: Int32(gamepad.buttonY.isPressed ? 1 : 0))
        case gamepad.rightShoulder:
            MainiOS.gamepadEvent(forButton: Int32(TCButtonType.BUTTON_Z.rawValue), action: Int32(gamepad.rightShoulder.value == 0 ? 0 : 1))
        case gamepad.buttonMenu:
            MainiOS.gamepadEvent(forButton: Int32(TCButtonType.BUTTON_START.rawValue), action: Int32(gamepad.buttonMenu.value == 0 ? 0 : 1))
        case gamepad.leftThumbstick:
            MainiOS.gamepadMoveEvent(forAxis: Int32(TCButtonType.STICK_MAIN.rawValue+1), value: CGFloat(-gamepad.leftThumbstick.up.value))
            MainiOS.gamepadMoveEvent(forAxis: Int32(TCButtonType.STICK_MAIN.rawValue+2), value: CGFloat(gamepad.leftThumbstick.down.value))
            MainiOS.gamepadMoveEvent(forAxis: Int32(TCButtonType.STICK_MAIN.rawValue+3), value: CGFloat(-gamepad.leftThumbstick.left.value))
            MainiOS.gamepadMoveEvent(forAxis: Int32(TCButtonType.STICK_MAIN.rawValue+4), value: CGFloat(gamepad.leftThumbstick.right.value))
        case gamepad.rightThumbstick:
            MainiOS.gamepadMoveEvent(forAxis: Int32(TCButtonType.STICK_C.rawValue+1), value: CGFloat(-gamepad.rightThumbstick.up.value))
            MainiOS.gamepadMoveEvent(forAxis: Int32(TCButtonType.STICK_C.rawValue+2), value: CGFloat(gamepad.rightThumbstick.down.value))
            MainiOS.gamepadMoveEvent(forAxis: Int32(TCButtonType.STICK_C.rawValue+3), value: CGFloat(-gamepad.rightThumbstick.left.value))
            MainiOS.gamepadMoveEvent(forAxis: Int32(TCButtonType.STICK_C.rawValue+4), value: CGFloat(gamepad.rightThumbstick.right.value))
        case gamepad.dpad:
            MainiOS.gamepadEvent(forButton: Int32(TCButtonType.BUTTON_UP.rawValue), action: gamepad.dpad.up.isPressed ? 1 : 0)
            MainiOS.gamepadEvent(forButton: Int32(TCButtonType.BUTTON_DOWN.rawValue), action: gamepad.dpad.down.isPressed ? 1 : 0)
            MainiOS.gamepadEvent(forButton: Int32(TCButtonType.BUTTON_LEFT.rawValue), action: gamepad.dpad.left.isPressed ? 1 : 0)
            MainiOS.gamepadEvent(forButton: Int32(TCButtonType.BUTTON_RIGHT.rawValue), action: gamepad.dpad.right.isPressed ? 1 : 0)
        case gamepad.leftTrigger:
            MainiOS.gamepadMoveEvent(forAxis: Int32(TCButtonType.TRIGGER_L.rawValue), value: CGFloat(gamepad.leftTrigger.value))
        case gamepad.rightTrigger:
            MainiOS.gamepadMoveEvent(forAxis: Int32(TCButtonType.TRIGGER_R.rawValue), value: CGFloat(gamepad.rightTrigger.value))
        default:
            NSLog("oeuf")
        }
        }
    } else {
        // Fallback on earlier versions
    }
    //MainiOS.gamepadEvent(forButton: Int32(controllerButton), action: 1)
  }
  
}
