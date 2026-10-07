#import <Foundation/Foundation.h>
#import "NZPUpdatePolicy.h"
#include "nzp_update_state.h"
int main(int argc,const char **argv){@autoreleasepool{
 if(argc!=2)return 2;
 NSData *data=[NSData dataWithContentsOfFile:[NSString stringWithUTF8String:argv[1]]];
 NSString *error=nil;NZPUpdateRelease *r=[NZPUpdatePolicy parse:data error:&error];
 if(!r||r.version!=2||r.size!=111678472||![r.sha256 isEqualToString:@"dac2b175ee88405ab4eb7a174247eb15d7594974681494f3741773b892ab0e1b"]||![r.assetURL.absoluteString isEqualToString:@"https://github.com/itssnotTristan/NZP-Mobile/releases/download/ios-v2-unsigned/NZP-Mobile-iOS-unsigned.ipa"]){NSLog(@"FAIL exact live iOS metadata: %@",error);return 1;}
 NZPUpdateState s;nzp_update_state_init(&s);uint64_t ticket=nzp_update_begin(&s);
 if(!nzp_update_finish(&s,ticket,2,r.version,true)||!nzp_update_can_play(&s)||nzp_update_required(&s))return 3;
 ticket=nzp_update_begin(&s);if(!nzp_update_finish(&s,ticket,1,r.version,true)||!nzp_update_required(&s)||nzp_update_can_play(&s))return 4;
 ticket=nzp_update_begin(&s);if(!nzp_update_fail(&s,ticket)||nzp_update_required(&s)||!nzp_update_can_play(&s))return 5;
 NSMutableArray *broken=[[NSJSONSerialization JSONObjectWithData:data options:NSJSONReadingMutableContainers error:NULL] mutableCopy];
 [broken addObject:@{@"draft":@NO,@"prerelease":@YES,@"tag_name":@"ios-v3-unsigned",@"assets":@[]}];
 NSData *negative=[NSJSONSerialization dataWithJSONObject:broken options:0 error:NULL];
 if([NZPUpdatePolicy parse:negative error:&error])return 6;
 printf("PASS: actual MP1 iOS Objective-C parser selects only published ios-v2-unsigned with exact IPA digest/size; Android and source-input releases are ignored.\n");
 printf("PASS: actual iOS state machine permits installed build2, requires an update for simulated build1, and permits offline play after a failed check; incomplete higher iOS artifact is rejected.\n");
 printf("Original iOS v1 contains no updater; simulated build1 is a state-machine test, not a claim that v1 can update in-app. No device installation/runtime claim.\n");
 return 0;
}}
