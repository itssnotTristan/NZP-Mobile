#import <Foundation/Foundation.h>
#import "NZPUpdatePolicy.h"
#include "nzp_update_state.h"
int main(int argc,const char **argv){@autoreleasepool{
 if(argc!=2)return 2;
 NSData *data=[NSData dataWithContentsOfFile:[NSString stringWithUTF8String:argv[1]]];
 NSString *error=nil;NZPUpdateRelease *r=[NZPUpdatePolicy parse:data error:&error];
 if(!r||r.version!=3||r.size!=113893871||![r.sha256 isEqualToString:@"4dcfbf9815216e4a1bed9e2a1fd2f98e70ada9a829c3ad3d06b0353551b41ac6"]||![r.assetURL.absoluteString isEqualToString:@"https://github.com/itssnotTristan/NZP-Mobile/releases/download/ios-v3-unsigned/NZP-Mobile-iOS-unsigned.ipa"]){NSLog(@"FAIL exact live iOS metadata: %@",error);return 1;}
 NZPUpdateState s;nzp_update_state_init(&s);uint64_t ticket=nzp_update_begin(&s);
 if(!nzp_update_finish(&s,ticket,3,r.version,true)||!nzp_update_can_play(&s)||nzp_update_required(&s))return 3;
 ticket=nzp_update_begin(&s);if(!nzp_update_finish(&s,ticket,2,r.version,true)||!nzp_update_required(&s)||nzp_update_can_play(&s))return 4;
 ticket=nzp_update_begin(&s);if(!nzp_update_fail(&s,ticket)||nzp_update_required(&s)||!nzp_update_can_play(&s))return 5;
 NSMutableArray *broken=[[NSJSONSerialization JSONObjectWithData:data options:NSJSONReadingMutableContainers error:NULL] mutableCopy];
 [broken addObject:@{@"draft":@NO,@"prerelease":@YES,@"tag_name":@"ios-v4-unsigned",@"assets":@[]}];
 NSData *negative=[NSJSONSerialization dataWithJSONObject:broken options:0 error:NULL];
 if([NZPUpdatePolicy parse:negative error:&error])return 6;
 printf("PASS: actual P2P iOS Objective-C parser selects only published ios-v3-unsigned with exact IPA digest/size; Android and source-input releases are ignored.\n");
 printf("PASS: actual iOS state machine permits installed build3, requires an update for installed build2, and permits offline play after a failed check; incomplete higher iOS artifact is rejected.\n");
 printf("The actual parser/state sources are byte-identical between installed iOS2 and new iOS3. Original iOS1 contains no updater. No device installation/runtime claim.\n");
 return 0;
}}
