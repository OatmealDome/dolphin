// Copyright 2019 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#import "SoftwareTableViewController.h"

#import "DolphiniOS-Swift.h"
#import "SoftwareTableViewCell.h"

#import "MainiOS.h"

#import "UICommon/GameFile.h"

#import <MetalKit/MetalKit.h>
#import <SafariServices/SafariServices.h>

@interface SoftwareTableViewController () <UISearchResultsUpdating>

@property UISearchController *searchController;
@property UITableViewController *resultsController;

@end

@implementation SoftwareTableViewController

- (void)viewDidLoad
{
  [super viewDidLoad];
  // Load the GameFileCache
  self.m_cache = new UICommon::GameFileCache();
  self.m_cache->Load();
  self.navigationItem.searchController = [[UISearchController alloc] initWithSearchResultsController:[UITableViewController alloc]];
  self.searchController.searchResultsUpdater = self;
  self.navigationItem.searchController.searchBar.scopeButtonTitles = @[@"All", @"GameCube", @"Wii"];
  [self rescanGameFilesWithRefreshing:false];
  
  // Create a UIRefreshControl
  UIRefreshControl* refreshControl = [[UIRefreshControl alloc] init];
  [refreshControl addTarget:self action:@selector(refreshGameFileCache) forControlEvents:UIControlEventValueChanged];
  
  self.tableView.refreshControl = refreshControl;
}

- (void)refreshGameFileCache
{
  [self rescanGameFilesWithRefreshing:true];
}

- (void)rescanGameFilesWithRefreshing:(bool)refreshing
{
  dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
    // Get the software folder path
    NSString* userDirectory = [MainiOS getUserFolder];
    NSString* softwareDirectory = [userDirectory stringByAppendingPathComponent:@"Software"];

    // Create it if necessary
    NSFileManager* fileManager = [NSFileManager defaultManager];
    if (![fileManager fileExistsAtPath:softwareDirectory])
    {
      [fileManager createDirectoryAtPath:softwareDirectory withIntermediateDirectories:false
                   attributes:nil error:nil];
    }
    
    std::vector<std::string> folder_paths;
    folder_paths.push_back(std::string([softwareDirectory UTF8String]));
    
    // Update the cache
    bool cache_updated = self.m_cache->Update(UICommon::FindAllGamePaths(folder_paths, false));
    cache_updated |= self.m_cache->UpdateAdditionalMetadata();
    if (cache_updated)
    {
      self.m_cache->Save();
    }
    
    self.m_cache_loaded = true;
    
    dispatch_async(dispatch_get_main_queue(), ^{
      [self.tableView reloadData];
      
      if (refreshing)
      {
        [self.tableView.refreshControl endRefreshing];
      }
    });
  });
}

- (void)showAlertWithTitle:(NSString*)title text:(NSString*)text isFatal:(bool)isFatal
{
  UIAlertController* alert = [UIAlertController alertControllerWithTitle:title
                                 message:text
                                 preferredStyle:UIAlertControllerStyleAlert];
   
  UIAlertAction* okayAction = [UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault
     handler:^(UIAlertAction * action) {
    if (isFatal)
    {
      exit(0);
    }
  }];
   
  [alert addAction:okayAction];
  [self.navigationController presentViewController:alert animated:YES completion:nil];
}

- (void)viewDidAppear:(BOOL)animated
{
#ifndef SUPPRESS_UNSUPPORTED_DEVICE
  // Check if should skip this check
  NSString* bypass_flag_file = [[MainiOS getUserFolder] stringByAppendingPathComponent:@"bypass_unsupported_device"];
  NSFileManager* file_manager = [NSFileManager defaultManager];
  if (![file_manager fileExistsAtPath:bypass_flag_file])
  {
    // Check for GPU Family 3
    id<MTLDevice> metalDevice = MTLCreateSystemDefaultDevice();
    if (![metalDevice supportsFeatureSet:MTLFeatureSet_iOS_GPUFamily3_v2])
    {
      [self showAlertWithTitle:@"Unsupported Device"
            text:@"DolphiniOS can only run on devices with an A9 processor or newer.\n\nThis is because your device's GPU does not support a feature required by Dolphin for good performance. Your device would run Dolphin at an unplayable speed without this feature."
            isFatal:true];
    }
  }
#endif
  
  NSUserDefaults* user_defaults = [NSUserDefaults standardUserDefaults];
  
  // Check for jailbreakd (Chimera)
  if (![user_defaults boolForKey:@"seen_chimera_notice"])
  {
    NSFileManager* file_manager = [NSFileManager defaultManager];
    if ([file_manager fileExistsAtPath:@"/Library/LaunchDaemons/jailbreakd.plist"])
    {
      [self showAlertWithTitle:@"Unsupported Jailbreak"
            text:@"DolphiniOS is using an unstable method to enable the JIT recompiler because the stable method is not supported by the Chimera jailbreak.\n\nIf you quit DolphiniOS or if iOS quits DolphiniOS (for example, to free up RAM), you will need to reboot your device before starting DolphiniOS again.\n\nFor the best experience, switch to checkra1n (A9 to A11 processors only) or unc0ver."
            isFatal:false];
    }
    
    [user_defaults setBool:true forKey:@"seen_chimera_notice"];
  }
  
  // Get the number of launches
  NSInteger launch_times = [user_defaults integerForKey:@"launch_times"];
  if (launch_times == 0)
  {
    // Show the maintainer alert on first launch
    [self showAlertWithTitle:@"Note"
          text:@"DolphiniOS is NOT an official version of Dolphin. It is a separate version based on the original Dolphin's code.\n\nDO NOT ask for help on the official Dolphin forums or report bugs on the official Dolphin bug tracker.\n\nIf you need help, go to the Settings tab and tap \"Support\"."
          isFatal:false];
  }
  else if (launch_times % 10 == 0)
  {
    bool suppress_donation_message = [user_defaults boolForKey:@"suppress_donation_message"];
    
    if (!suppress_donation_message)
    {
      UIAlertController* alert = [UIAlertController alertControllerWithTitle:@"Donate"
                                     message:@"DolphiniOS is an unofficial version of Dolphin, maintained separately from the official Dolphin code.\n\nWhile DolphiniOS will forever remain free, it takes time and money to support its development and server costs. Your support is greatly appreciated. As a benefit for donating, you can get access to beta builds with new features."
                                     preferredStyle:UIAlertControllerStyleAlert];
       
      UIAlertAction* donate_action = [UIAlertAction actionWithTitle:@"Donate" style:UIAlertActionStyleDefault
         handler:^(UIAlertAction * action) {
        [[UIApplication sharedApplication] openURL:[NSURL URLWithString:@"https://www.patreon.com/oatmealdome"] options:@{} completionHandler:nil];
      }];
      
      UIAlertAction* no_thanks_action = [UIAlertAction actionWithTitle:@"Not Now" style:UIAlertActionStyleDefault
         handler:nil];
      
      [alert addAction:donate_action];
      [alert addAction:no_thanks_action];
      
      if (launch_times > 10)
      {
        UIAlertAction* do_not_show_action = [UIAlertAction actionWithTitle:@"Don't Show Again" style:UIAlertActionStyleDefault
           handler:^(UIAlertAction * action) {
          [user_defaults setBool:true forKey:@"suppress_donation_message"];
        }];
        
        [alert addAction:do_not_show_action];
      }
      
      [self presentViewController:alert animated:YES completion:nil];
    }
  }
  
  [user_defaults setInteger:launch_times + 1 forKey:@"launch_times"];
}

#pragma mark - Add Button

- (IBAction)addButtonPressed:(id)sender
{
  NSArray* types = @[
    @"org.dolphin-emu.ios.generic-software",
    @"org.dolphin-emu.ios.gamecube-software",
    @"org.dolphin-emu.ios.wii-software"
  ];
  
  UIDocumentPickerViewController* pickerController = [[UIDocumentPickerViewController alloc] initWithDocumentTypes:types inMode:UIDocumentPickerModeOpen];
  pickerController.delegate = self;
  pickerController.modalPresentationStyle = UIModalPresentationPageSheet;
  
  [self presentViewController:pickerController animated:true completion:nil];
}

#pragma mark - Table view data source

- (NSInteger)numberOfSectionsInTableView:(UITableView*)tableView
{
  return 1;
}

- (NSInteger)tableView:(UITableView*)tableView numberOfRowsInSection:(NSInteger)section
{
  if (!self.m_cache_loaded)
  {
    return 0;
  }
  
  return self.m_cache->GetSize();
}

- (UITableViewCell*)tableView:(UITableView*)tableView cellForRowAtIndexPath:(NSIndexPath*)indexPath
{
    SoftwareTableViewCell* cell = (SoftwareTableViewCell*)[tableView dequeueReusableCellWithIdentifier:@"softwareCell"
                                                                  forIndexPath:indexPath];
      
    NSString* game_system = @"";
      
    // Get the GameFile
    std::shared_ptr<const UICommon::GameFile> file = self.m_cache->Get(indexPath.item);
    DiscIO::Platform platform = file->GetPlatform();
    // Add the platform prefix
    if (platform == DiscIO::Platform::GameCubeDisc)
    {
        game_system = @"GameCube";
    }
    else if (platform == DiscIO::Platform::WiiDisc || platform == DiscIO::Platform::WiiWAD)
    {
        game_system = @"Wii";
    }
    else
    {
        game_system = @"Unknown";
    }
        
    // Append the game name
    NSString* game_name = [NSString stringWithUTF8String:file->GetLongName().c_str()];
    NSString* game_file_name = [NSString stringWithUTF8String:file->GetFileName().c_str()];
    NSString* game_id = [NSString stringWithUTF8String:file->GetGameTDBID().c_str()];
    NSString* game_region = [self getGameRegionCode:file];
      
    NSURL *game_cover = [NSURL URLWithString:[NSString stringWithFormat:@"https://art.gametdb.com/wii/cover/%@/%@.png", game_region, game_id]];
    NSLog(@"'%@'", game_file_name);
    // Set the cell label text
    cell.gameName.text = ([game_name length] > 0) ? game_name : game_file_name;
    cell.gameSystem.text = game_system;
    
    // Set the cell image
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_BACKGROUND, 0), ^{
        NSData *imageData = [NSData dataWithContentsOfURL:game_cover];

        dispatch_async(dispatch_get_main_queue(), ^{
            // Update the UI
            cell.gameCover.image = [UIImage imageWithData:imageData];
            [cell layoutSubviews];
        });
    });
    return cell;
}

- (void)tableView:(UITableView*)tableView didSelectRowAtIndexPath:(NSIndexPath*)indexPath
{
  [self performSegueWithIdentifier:@"toEmulation" sender:nil];
}

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath {
    return 68;
}

#pragma mark - Document picker delegate methods

- (void)documentPicker:(UIDocumentPickerViewController *)controller didPickDocumentsAtURLs:(NSArray<NSURL *> *)urls
{
  NSSet<NSURL*>* set = [NSSet setWithArray:urls];
  [MainiOS importFiles:set];
  
  [self rescanGameFilesWithRefreshing:false];
}

#pragma mark - Segues

- (void)prepareForSegue:(UIStoryboardSegue*)segue sender:(id)sender
{
  if ([segue.identifier isEqualToString:@"toEmulation"])
  {
    UINavigationController* navigationController = (UINavigationController*)segue.destinationViewController;
    EmulationViewController* viewController = (EmulationViewController*)([navigationController.viewControllers firstObject]);
    
    // Get the GameFile and set values
    std::shared_ptr<const UICommon::GameFile> file = self.m_cache->Get([self.tableView indexPathForSelectedRow].item);
    viewController.softwareFile = [NSString stringWithUTF8String:file->GetFilePath().c_str()];
    viewController.softwareName = [NSString stringWithUTF8String:file->GetLongName().c_str()];
    viewController.isWii = DiscIO::IsWii(file->GetPlatform());
  }
}

- (IBAction)unwindToSoftwareTable:(UIStoryboardSegue*)segue {}

// Override to support conditional editing of the table view.
- (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath {
    // Return NO if you do not want the specified item to be editable.
    return YES;
}


// Override to support editing the table view.
- (void)tableView:(UITableView *)tableView
commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath
*)indexPath { if (editingStyle == UITableViewCellEditingStyleDelete) {
        // Delete the row from the data source
        [tableView deleteRowsAtIndexPaths:@[indexPath]
withRowAnimation:UITableViewRowAnimationFade]; } else if (editingStyle ==
UITableViewCellEditingStyleInsert) {
        // Create a new instance of the appropriate class, insert it into the array, and add a new row to the table view
    }
}

#pragma mark - Handle Search

- (void)updateSearchResultsForSearchController:(nonnull UISearchController *)searchController {
    NSString* query = searchController.searchBar.text;
    NSLog(@"%@, %ld", query, (long)searchController.searchBar.selectedScopeButtonIndex);
}

# pragma mark - Convert Region to String
- (NSString *)getGameRegionCode:(std::shared_ptr<const UICommon::GameFile>)game {
    switch (game->GetRegion()) {
        case DiscIO::Region::NTSC_J:
            return @"JA";
            break;
        case DiscIO::Region::NTSC_U:
            return @"US";
            break;
        case DiscIO::Region::NTSC_K:
            return @"KO";
            break;
        default:
            return @"UN";
            break;
        case DiscIO::Region::PAL:
            const auto user_lang = DiscIO::Language::English;
            switch (user_lang) {
                case DiscIO::Language::German:
                    return @"DE";
                    break;
                case DiscIO::Language::French:
                    return @"FR";
                    break;
                case DiscIO::Language::Spanish:
                    return @"ES";
                    break;
                case DiscIO::Language::Italian:
                    return @"IT";
                    break;
                case DiscIO::Language::Dutch:
                    return @"NL";
                    break;
                case DiscIO::Language::English:
                default:
                    return @"EN";
                    break;
            }
            break;
    }
}

#pragma mark - Context Menu

- (UIContextMenuConfiguration *)tableView:(UITableView *)tableView contextMenuConfigurationForRowAtIndexPath:(NSIndexPath *)indexPath point:(CGPoint)point  API_AVAILABLE(ios(13.0)){
    return [UIContextMenuConfiguration configurationWithIdentifier:NULL previewProvider:NULL actionProvider:^(NSArray* suggestedAction){
        
        UITableViewCell *cell = [tableView cellForRowAtIndexPath:indexPath];
        std::shared_ptr<const UICommon::GameFile> file = self.m_cache->Get(indexPath.item);

        // Open Preview
        UIAction* wikiAction = [UIAction actionWithTitle:@"Wiki" image:[UIImage systemImageNamed:@"eyeglasses"] identifier:nil handler:^(UIAction *action){
            NSString * game_id = [NSString stringWithUTF8String:file->GetGameID().c_str()];
            NSURL *wiki_url = [NSURL URLWithString:[NSString stringWithFormat:@"https://wiki.dolphin-emu.org/index.php?title=%@", game_id]];
            SFSafariViewController *sf_controller = [[SFSafariViewController alloc] initWithURL:wiki_url];
            [self presentViewController:sf_controller animated:YES completion:nil];
            NSLog(@"%@", wiki_url);
        }];
        
        
        // Rename Action
        UIAction* infoAction = [UIAction actionWithTitle:@"Info" image:[UIImage systemImageNamed:@"info.circle"] identifier:nil handler:^(UIAction *action){

        }];
        
        // Copy Action
        UIAction* configAction = [UIAction actionWithTitle:@"Config" image:[UIImage systemImageNamed:@"dial"] identifier:nil handler:^(UIAction *action){
//            UIPasteboard *pb = [UIPasteboard generalPasteboard];
//            [pb setString:[NSString stringWithFormat:@"%@/%@", self.currentFolder.path, cell.textLabel.text]];
        }];
        
        // Move Action
        UIAction* cheatsAction = [UIAction actionWithTitle:@"Cheats" image:[UIImage systemImageNamed:@"cube"] identifier:nil handler:^(UIAction *action){
            
        }];
        
        // Share Action
        UIAction* shareAction = [UIAction actionWithTitle:@"Share" image:[UIImage systemImageNamed:@"square.and.arrow.up"] identifier:nil handler:^(UIAction *action){
            NSLog(@"BRUH");
        }];
        
        // Delete Action
        UIAction* deleteAction = [UIAction actionWithTitle:@"Delete" image:[UIImage systemImageNamed:@"trash"] identifier:nil  handler:^(UIAction *action){
            UITableViewCell *cell = [tableView cellForRowAtIndexPath:indexPath];
//            [self.currentFolder removeItemNamed:cell.textLabel.text];
//            [tableView deleteRowsAtIndexPaths:@[indexPath] withRowAnimation:UITableViewRowAnimationLeft];
        }];
        deleteAction.attributes = UIMenuElementAttributesDestructive;
        
        return [UIMenu menuWithTitle:@"" children:@[wikiAction, infoAction, configAction, cheatsAction, shareAction, deleteAction]];
    }];
}

/*
// Override to support rearranging the table view.
- (void)                        :(UITableView *)tableView moveRowAtIndexPath:(NSIndexPath
*)fromIndexPath toIndexPath:(NSIndexPath *)toIndexPath {
}
*/

/*
// Override to support conditional rearranging of the table view.
- (BOOL)tableView:(UITableView *)tableView canMoveRowAtIndexPath:(NSIndexPath *)indexPath {
    // Return NO if you do not want the item to be re-orderable.
    return YES;
}
*/

@end
