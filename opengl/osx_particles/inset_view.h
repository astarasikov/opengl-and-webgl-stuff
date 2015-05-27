#ifndef __FFMPEG_VIEW__H__
#define __FFMPEG_VIEW__H__

#import "opengl_view.h"

typedef struct RectInset {
	unsigned left;
	unsigned right;
	unsigned top;
	unsigned bottom;
} RectInset;

@interface InsetView : MyOpenGLViewBase
-(void)renderForTime:(CVTimeStamp)time;
-(void)setTexture:(char*)data andWidth:(unsigned)width
	andHeight:(unsigned)height withInsets:(RectInset)insets;
@end

#endif //__FFMPEG_VIEW__H__
