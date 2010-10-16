/*
 *  unlogo.cpp
 *  unlogo
 *
 *  Created by Jeffrey Crouse
 *  Copyright 2010 Eyebeam. All rights reserved.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <iostream>
#include "MatchableImage.h"

#define MATCHING_DELAY 10
#define MATCHING_PCT_THRESHOLD 0.1
#define GHOST_FRAMES_ALLOWED 50
#define RANSAC_PROJECTION_THRESH 2

using namespace fh;


typedef struct Logo
{
	const char* name;  // Kept for convenience and debugging
	MatchableImage logo;
	Image replacement;
	Point2f pos;
	int ghostFrames;
	Mat homography;
};


MatchableImage prev;			// The last frame -- for optical flow.
int framenum=0;					// Current frame number
vector<Logo> logos;
vector<Logo*> detected_logos;


extern "C" int init( const char* argstr )
{
	try {
		log(LOG_LEVEL_DEBUG, "Welcome to unlogo, using OpenCV version %s (%d.%d.%d)\n",
			CV_VERSION, CV_MAJOR_VERSION, CV_MINOR_VERSION, CV_SUBMINOR_VERSION);
		
		// Parse arguments.
		vector<string> argv = split(argstr, ":");
		int argc = argv.size();
		if(argc < 2)
		{
			log(LOG_LEVEL_ERROR, "You must supply at least 2 arguments.");
			exit(-1);
		}
		
		
		// Load in all of the logos from the arguments
		for(int i=0; i<argc; i+=2)
		{
			logos.push_back( Logo() );
			logos.back().name = argv[i].c_str();
			logos.back().logo.open( argv[i].c_str() );
			logos.back().logo.findFeatures("SURF");
			logos.back().logo.trainMatcher("SURF");
			
			logos.back().replacement.open( argv[i+1].c_str() );
			logos.back().replacement.convert( CV_RGBA2BGRA );
			logos.back().ghostFrames=0;
			logos.back().pos = Point2f(-1,-1);
			
			log(LOG_LEVEL_DEBUG, "Loaded logo %s", argv[i].c_str());
		}
		
		
#ifdef DEBUG		
		namedWindow("input");		cvMoveWindow("input", 0, 0);
		namedWindow("output");		cvMoveWindow("output", 650, 0);
#endif 
		
		return 0;
	}
	catch ( ... ) {
		return -1;
	}
}

extern "C" int uninit()
{
	return 0;
}



extern "C" int process( uint8_t* dst[4], int dst_stride[4],
					   uint8_t* src[4], int src_stride[4],
					   int width, int height)
{
	cout << "(frame " << framenum << ")  ";
	MatchableImage input( width, height, src[0], src_stride[0]);
	if(input.empty()) return 1;
	
	
#ifdef DEBUG
	input.show("input");
#endif
	

	bool doMatching = framenum==0 || framenum%MATCHING_DELAY==0 || detected_logos.size()==0;
	if( doMatching )
	{
		// Analize incoming images
		input.findFeatures("SURF");
		input.findDescriptors("SURF");
		
		detected_logos.clear();

		// Make a MatchSet for each frame/logo pair
		for(int i=0; i<(int)logos.size(); i++)
		{
			vector<int> matches;
			logos[i].logo.matchTo( input, matches );
			
			Mat img_corr;
			drawMatches(input.bw(), input.features, logos[i].logo.bw(), logos[i].logo.features, matches, img_corr);
			imshow(logos[i].name, img_corr);
			
		}
		
		if(detected_logos.size()==0)
		{
			log(LOG_LEVEL_DEBUG, "Casual warning: Matchers ran and found no logos.");
		}
	}
	else
	{
		
	}

	
	Image output(width, height, dst[0], dst_stride[0]);			// point the 'output' image to the FFMPEG data array	
	output.copyFromImage(input);								// copy input into the output memory
	output.text("unlogo", 20, 20);
	
	CV_Assert(&output.cvImage.data[0]==&dst[0][0]);				// Make sure output still points to dst
	
	
#ifdef DEBUG	
	output.show("output");
	waitKey(3);	// needed to update windows.
#endif
	
	framenum++;
	return 0;
}



#ifdef DEBUG
int main(int argc, char * const argv[])
{
	// Imitating the arguments that FFMPEG gives us through AVFilter.
	// see process() in unlogo.cpp
	int width, height;
	uint8_t* src[4];
	uint8_t* dst[4];
	int src_stride[4];
	int dst_stride[4];
	
	// Open the video
	cv::VideoCapture cap(argv[1]);
	cap.set(CV_CAP_PROP_CONVERT_RGB, 1);
	
    if(!cap.isOpened())  
	{
		std::cout << "Can not open video source" << std::endl;
        return -1;
	}
	width = cap.get(CV_CAP_PROP_FRAME_WIDTH);
	height = cap.get(CV_CAP_PROP_FRAME_HEIGHT);
	dst[0]= new uint8_t[ width* height * 3 ];
	dst_stride[0] = width * 3;
	
	init(argv[2]);													// from unlogo.cpp
	
	cv::Mat frame;
	for(;;)
    {
        cap >> frame; // get a new frame from camera
		if(frame.empty()) break;
		
		src[0] = frame.data;
		src_stride[0] = frame.step;
		
		process(dst, dst_stride, src, src_stride, width, height);  // from unlogo.cpp
	}
	
	uninit();														// from unlogo.cpp
	
	std::cout << "Exiting ..." << std::endl;
	return 0;
}
#endif