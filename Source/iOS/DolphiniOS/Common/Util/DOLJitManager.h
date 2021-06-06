// Copyright 2021 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#import <Foundation/Foundation.h>

#if TARGET_OS_TV
#import "DolphinATV-Swift.h"
#else
#import "DolphiniOS-Swift.h"
#endif

typedef NS_ENUM(NSUInteger, DOLJitType)
{
  DOLJitTypeNone,
  DOLJitTypeDebugger,
  DOLJitTypeAllowUnsigned,
  DOLJitTypePTrace,
  DOLJitTypeNotRestricted
};

typedef NS_ENUM(NSUInteger, DOLJitError)
{
  DOLJitErrorNone,
  DOLJitErrorNotArm64e, // on NJB iOS 14.2+, need arm64e
  DOLJitErrorImproperlySigned, // on NJB iOS 14.2+, need correct code directory version and flags set
  DOLJitErrorNeedUpdate, // iOS not supported
  DOLJitErrorWorkaroundRequired, // NJB iOS 14.4+ broke the JIT hack
  DOLJitErrorGestaltFailed, // an error occurred with loading MobileGestalt
  DOLJitErrorJailbreakdFailed, // an error occurred with contacting jailbreakd
  DOLJitErrorCsdbgdFailed // an error occurred with contacting csdbgd
};

NS_ASSUME_NONNULL_BEGIN

@interface DOLJitManager : NSObject

+ (DOLJitManager*)sharedManager;

- (void)setJitTypeToAcquire;
- (void)attemptToAcquireJitOnStartup;
- (void)attemptToAcquireJitByRemoteDebuggerUsingCancellationToken:(DOLCancellationToken*)token;
- (DOLJitType)jitType;
- (bool)appHasAcquiredJit;
- (DOLJitError)getJitErrorType;
- (void)setAuxillaryError:(NSString*)error;
- (NSString*)getAuxiliaryError;

@end

NS_ASSUME_NONNULL_END
