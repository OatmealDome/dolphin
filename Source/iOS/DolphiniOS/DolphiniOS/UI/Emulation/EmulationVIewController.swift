// Copyright 2019 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

import Foundation
import MetalKit
import UIKit
import CoreMotion

enum PadType {
  case wii
  case gameCube
}

class EmulationViewController: UIViewController {
  var softwareFile: String = ""
  var videoBackend: String = ""
  var padType: PadType = .wii
  var motion: CMMotionManager!
  
  @objc init(file: String, backend: String)
  {
    super.init(nibName: nil, bundle: nil)
    
    self.softwareFile = file
    self.videoBackend = backend
    
    self.modalPresentationStyle = .fullScreen
  }
  
  required init?(coder: NSCoder)
  {
    super.init(coder: coder)
  }
  
  override func loadView()
  {
    if (self.videoBackend == "OGL")
    {
      self.view = EAGLView(frame: UIScreen.main.bounds)
    }
    else if (self.videoBackend == "Vulkan")
    {
      self.view = MTKView(frame: UIScreen.main.bounds)
    }
    
    let padView = Bundle(for: type(of: self)).loadNibNamed(self.padType == .wii ? "TCWiiPad" : "TCGameCubePad", owner: self, options: nil)![0] as! UIView
    padView.frame = UIScreen.main.bounds
    self.view.addSubview(padView)
  }
  
  override func viewDidLoad()
  {
    if self.padType == .wii {
      self.motion = CMMotionManager()
      if self.motion.isDeviceMotionAvailable {
        self.motion.deviceMotionUpdateInterval = 1.0 / 200.0
        self.motion.startDeviceMotionUpdates(to: OperationQueue.main) { data, error in
          if let data = data {
            let accel = (
              x: data.userAcceleration.x - data.gravity.x,
              y: data.userAcceleration.y - data.gravity.y,
              z: data.userAcceleration.z - data.gravity.z
            )
            let x = (accel.x * 9.81, data.rotationRate.x)
            let y = (accel.y * 9.81, data.rotationRate.y)
            let accel_x, accel_y, rot_x, rot_y: Double
            let accel_z = accel.z * 9.81
            let rot_z = data.rotationRate.z
            let orientation = UIApplication.shared.statusBarOrientation
            switch orientation {
            case .portrait:
              accel_x = -x.0
              rot_x = -x.1
              accel_y = -y.0
              rot_y = -y.1
            case .landscapeLeft:
              accel_x = -y.0
              rot_x = -y.1
              accel_y = x.0
              rot_y = x.1
            case .landscapeRight:
              accel_x = y.0
              rot_x = y.1
              accel_y = -x.0
              rot_y = -x.1
            default:
              // Upside down or unknown
              accel_x = x.0
              rot_x = x.1
              accel_y = y.0
              rot_y = y.1
            }
            MainiOS.gamepadMoveEvent(forAxis: Int32(TCButtonType.WIIMOTE_ACCEL_LEFT.rawValue), value: CGFloat(accel_x))
            MainiOS.gamepadMoveEvent(forAxis: Int32(TCButtonType.WIIMOTE_ACCEL_RIGHT.rawValue), value: CGFloat(accel_x))
            MainiOS.gamepadMoveEvent(forAxis: Int32(TCButtonType.WIIMOTE_ACCEL_FORWARD.rawValue), value: CGFloat(accel_y))
            MainiOS.gamepadMoveEvent(forAxis: Int32(TCButtonType.WIIMOTE_ACCEL_BACKWARD.rawValue), value: CGFloat(accel_y))
            MainiOS.gamepadMoveEvent(forAxis: Int32(TCButtonType.WIIMOTE_ACCEL_UP.rawValue), value: CGFloat(accel_z))
            MainiOS.gamepadMoveEvent(forAxis: Int32(TCButtonType.WIIMOTE_ACCEL_DOWN.rawValue), value: CGFloat(accel_z))
            MainiOS.gamepadMoveEvent(forAxis: Int32(TCButtonType.WIIMOTE_GYRO_PITCH_UP.rawValue), value: CGFloat(rot_x))
            MainiOS.gamepadMoveEvent(forAxis: Int32(TCButtonType.WIIMOTE_GYRO_PITCH_DOWN.rawValue), value: CGFloat(rot_x))
            MainiOS.gamepadMoveEvent(forAxis: Int32(TCButtonType.WIIMOTE_GYRO_ROLL_LEFT.rawValue), value: CGFloat(rot_y))
            MainiOS.gamepadMoveEvent(forAxis: Int32(TCButtonType.WIIMOTE_GYRO_ROLL_RIGHT.rawValue), value: CGFloat(rot_y))
            MainiOS.gamepadMoveEvent(forAxis: Int32(TCButtonType.WIIMOTE_GYRO_YAW_LEFT.rawValue), value: CGFloat(rot_z))
            MainiOS.gamepadMoveEvent(forAxis: Int32(TCButtonType.WIIMOTE_GYRO_YAW_RIGHT.rawValue), value: CGFloat(rot_z))
          }
        }
      }
    }
    let queue = DispatchQueue(label: "org.dolphin-emu.ios.emulation-queue")
    let view = self.view
    
    queue.async {
      MainiOS.startEmulation(withFile: self.softwareFile, view: view)
    }
  }
  
  override func viewWillTransition(to size: CGSize, with coordinator: UIViewControllerTransitionCoordinator)
  {
    MainiOS.windowResized()
  }
  
}
