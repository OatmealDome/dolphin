// Copyright 2019 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#import "SettingsTableViewController.h"
#import "DolphiniOS-Swift.h"
#import "MainiOS.h"
#import "IniSettingsTableViewController.h"
@interface SettingsTableViewController ()

@end

@implementation SettingsTableViewController
- (IBAction)doneButton:(id)sender {
    [self dismissViewControllerAnimated:YES completion:nil];
}

- (void)viewDidLoad
{
  [super viewDidLoad];
  // Set the version from Info.plist

  NSDictionary* info = [[NSBundle mainBundle] infoDictionary];
  [self.m_version_label setText:[info objectForKey:@"CFBundleShortVersionString"]];
}

- (void)tableView:(UITableView*)tableView didSelectRowAtIndexPath:(NSIndexPath*)indexPath
{
  if (indexPath.section == 1)
  {
      NSString* userDirectory = [MainiOS getUserFolder];
    NSString* contents;
    NSString* path;
    NSString* name;
    if (indexPath.row == 0)
    {
      path = [userDirectory stringByAppendingPathComponent:@"Config/Dolphin.ini"];
      name = @"Config";
    }
    else if (indexPath.row == 1)
    {
      path = [userDirectory stringByAppendingPathComponent:@"Config/Config/GFX.ini"];
      name = @"Graphics";
    }
    else
    {
      path = [userDirectory stringByAppendingPathComponent:@"Config/Config/GFX.ini"];
      name = @"Controllers";
    }
    NSFileManager* fileManager = [NSFileManager defaultManager];
    if ([fileManager fileExistsAtPath:path])
    {
      NSData *data = [fileManager contentsAtPath:path];
      contents = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
    }
    DolphinINIParser* parser = [[DolphinINIParser alloc] initWithContents:contents];
    NSArray<NSDictionary*> *result = [parser convert];
    NSLog(@"%@", result);
    IniSettingsTableViewController *settingsVC = [[IniSettingsTableViewController alloc] initWithStyle:UITableViewStyleInsetGrouped];
    settingsVC.iniFile = result;
    settingsVC.iniPath = path;
    settingsVC.name = name;
    [self.navigationController pushViewController:settingsVC animated:YES];
  }
  if (indexPath.section == 2)
  {
    NSString* url;
    if (indexPath.row == 0)
    {
      url = @"https://oatmealdome.me/dolphinios/";
    }
    else if (indexPath.row == 1)
    {
      url = @"https://patreon.com/oatmealdome";
    }
    else
    {
      // Load the special thanks
      NSString* thanks_path = [[NSBundle mainBundle] pathForResource:@"SpecialThanks" ofType:@"txt"];
      NSString* thanks_str = [NSString stringWithContentsOfFile:thanks_path encoding:NSUTF8StringEncoding error:nil];
      
      UIAlertController* alert = [UIAlertController alertControllerWithTitle:@"Special Thanks"
                                     message:[NSString stringWithFormat:@"Thank you to the following generous Patrons!\n\n%@\nShoutouts to Shadów and Magnetar for their help.\n\nFinally, thanks to the Dolphin Team for making a great emulator!", thanks_str]
                                     preferredStyle:UIAlertControllerStyleAlert];
       
      UIAlertAction* ok_action = [UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault
         handler:nil];
      [alert addAction:ok_action];
      
      [self presentViewController:alert animated:true completion:nil];
      
      [self.tableView deselectRowAtIndexPath:indexPath animated:true];
      
      return;
    }
    
    [[UIApplication sharedApplication] openURL:[NSURL URLWithString:url] options:@{} completionHandler:nil];
  }
  
  [tableView deselectRowAtIndexPath:indexPath animated:YES];
}

@end
