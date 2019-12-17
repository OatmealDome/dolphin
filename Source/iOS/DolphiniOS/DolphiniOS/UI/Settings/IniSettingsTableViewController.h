//
//  IniSettingsTableViewController.h
//  DolphiniOS
//
//  Created by Tucker Morley on 12/16/19.
//  Copyright © 2019 Dolphin Team. All rights reserved.
//

#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

@interface IniSettingsTableViewController : UITableViewController

@property(nonatomic) NSArray<NSDictionary*> *iniFile;
@property(nonatomic) NSString *iniPath;
@property(nonatomic) NSString *name;

@end

NS_ASSUME_NONNULL_END
