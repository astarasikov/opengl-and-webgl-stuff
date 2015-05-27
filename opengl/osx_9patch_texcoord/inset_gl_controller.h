#ifndef __FFMPEG_GL_CONTROLLER__H__
#define __FFMPEG_GL_CONTROLLER__H__

#import "inset_view.h"

@interface GLController : NSWindow
-(void)createGLView;

@property(nonatomic, readwrite, retain) InsetView *glView;
@end

#endif //__FFMPEG_GL_CONTROLLER__H__
