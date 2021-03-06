/*
 *  Mirror.cpp
 *  structure
 *
 *  Created by Jeffrey Crouse on 2/10/11.
 *  Copyright 2011 Eyebeam. All rights reserved.
 *
 */

#include "Mirror.h"


void Mirror::apply(Image* image)
{
	int code = 0;
	if( vertical && !horizontal ) code = 0;
	else if( !vertical && horizontal ) code = 1;
	else if( vertical && horizontal ) code = -1;
	else return;
	
	cv::flip( image->cvImg, image->cvImg, code);
	cv::flip( image->cvImg, image->cvImg, code);
}
